// Copyright 2019 Autodesk, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "reader.h"

#include <ai.h>

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdSkel/bakeSkinning.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <constant_strings.h>

#include "prim_reader.h"
#include "registry.h"

//-*************************************************************************

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

    // This is the class that is used to run a job from the WorkDispatcher
    struct _UsdArnoldPrimReaderJob {
        UsdPrim prim;
        UsdArnoldPrimReader *reader;
        UsdArnoldReaderContext *context;

        // function that gets executed when calling WorkDispatcher::Run
        void operator() () const {
            // use the primReader to read the input primitive, with the
            // provided context
            reader->Read(prim, *context);
            // delete the context that was created just for this job
            delete context;
        }
    };
    struct UsdThreadData {
        UsdThreadData() : threadId(0), threadCount(0), rootPrim(nullptr), 
                            context(nullptr), dispatcher(nullptr) {}

        unsigned int threadId;
        unsigned int threadCount;
        UsdPrim *rootPrim;
        UsdArnoldReaderThreadContext threadContext;
        UsdArnoldReaderContext *context;
        WorkDispatcher *dispatcher;
    };
};
// global reader registry, will be used in the default case
static UsdArnoldReaderRegistry *s_readerRegistry = nullptr;
static int s_anonymousOverrideCounter = 0;

static AtCritSec initializeGlobalReaderMutex()
{
    AtCritSec mutex;
    AiCritSecInit(&mutex);
    return mutex;
}
static AtCritSec s_globalReaderMutex = initializeGlobalReaderMutex();

UsdArnoldReader::~UsdArnoldReader()
{
    // What do we want to do at destruction here ?
    // Should we delete the created nodes in case there was no procParent ?

    if (_readerLock)
        AiCritSecClose((void **)&_readerLock);
}

void UsdArnoldReader::Read(const std::string &filename, AtArray *overrides, const std::string &path)
{
    // Nodes were already exported, should we skip here,
    // or should we just append the new nodes ?
    if (!_nodes.empty()) {
        return;
    }

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(filename);
    _filename = filename;   // Store the filename that is currently being read
    _overrides = overrides; // Store the overrides that are currently being applied

    if (overrides == nullptr || AiArrayGetNumElements(overrides) == 0) {
        // Only open the usd file as a root layer
        if (rootLayer == nullptr) {
            AiMsgError("[usd] Failed to open file (%s)", filename.c_str());
            return;
        }
        UsdStageRefPtr stage = UsdStage::Open(rootLayer, UsdStage::LoadAll);
        ReadStage(stage, path);
    } else {
        auto getLayerName = []() -> std::string {
            AiCritSecEnter(&s_globalReaderMutex);
            int counter = s_anonymousOverrideCounter++;
            AiCritSecLeave(&s_globalReaderMutex);
            std::stringstream ss;
            ss << "anonymous__override__" << counter << ".usda";
            return ss.str();
        };

        auto overrideLayer = SdfLayer::CreateAnonymous(getLayerName());
        const auto overrideCount = AiArrayGetNumElements(overrides);

        std::vector<std::string> layerNames;
        layerNames.reserve(overrideCount);
        // Make sure they kep around after the loop scope ends.
        std::vector<SdfLayerRefPtr> layers;
        layers.reserve(overrideCount);

        for (auto i = decltype(overrideCount){0}; i < overrideCount; ++i) {
            auto layer = SdfLayer::CreateAnonymous(getLayerName());
            if (layer->ImportFromString(AiArrayGetStr(overrides, i).c_str())) {
                layerNames.emplace_back(layer->GetIdentifier());
                layers.push_back(layer);
            }
        }

        overrideLayer->SetSubLayerPaths(layerNames);
        // If there is no rootLayer for a usd file, we only pass the overrideLayer to prevent
        // USD from crashing #235
        auto stage = rootLayer ? UsdStage::Open(rootLayer, overrideLayer, UsdStage::LoadAll)
                               : UsdStage::Open(overrideLayer, UsdStage::LoadAll);

        ReadStage(stage, path);
    }

    _filename = "";       // finished reading, let's clear the filename
    _overrides = nullptr; // clear the overrides pointer. Note that we don't own this array
}

void UsdArnoldReader::Read(int cacheId, const std::string &path)
{
    if (!_nodes.empty()) {
        return;
    }
    _cacheId = cacheId;
    // Load the USD stage in memory using a cache ID
    UsdStageCache &stageCache = UsdUtilsStageCache::Get();
    UsdStageCache::Id id = UsdStageCache::Id::FromLongInt(cacheId);
   
    UsdStageRefPtr stage = (id.IsValid()) ? stageCache.Find(id) : nullptr;
    if (!stage) {
        AiMsgError("[usd] Cache ID not valid %d", cacheId);
        return;
    }
    ReadStage(stage, path);
}

unsigned int UsdArnoldReader::ReaderThread(void *data)
{
    UsdThreadData *threadData = (UsdThreadData *)data;
    if (!threadData) 
        return 0;

    size_t index = 0;
    size_t threadId = threadData->threadId;
    size_t threadCount = threadData->threadCount;
    bool multithread = (threadCount > 1);
    UsdPrim *rootPrim = threadData->rootPrim;
    UsdArnoldReader *reader = threadData->threadContext.GetReader();
    TfToken visibility, purpose;
    UsdAttribute attr;
    const TimeSettings &time = reader->GetTimeSettings();
    float frame = time.frame;
    // Each thread context will have a stack of primvars vectors,
    // which represent the primvars at the current level of hierarchy.
    // Every time we find a Xform prim, we add an element to the stack 
    // with the updated primvars list. In every "post" visit, we pop the last
    // element. Thus, every time we'll read a prim, the last element of this 
    // stack will represent its input primvars that it inherits (see #282)
    std::vector<std::vector<UsdGeomPrimvar> > &primvarsStack = threadData->threadContext.GetPrimvarsStack();
    primvarsStack.clear(); 
    primvarsStack.reserve(64); // reserve first to avoid frequent memory allocations
    primvarsStack.push_back(std::vector<UsdGeomPrimvar>()); // add an empty element first
    
    // Traverse the stage, either the full one, or starting from a root primitive
    // (in case an object_path is set). We need to have "pre" and "post" visits in order
    // to keep track of the primvars list at every point in the hierarchy.
    UsdPrimRange range = UsdPrimRange::PreAndPostVisit((rootPrim) ? 
                    *rootPrim : reader->GetStage()->GetPseudoRoot());
    for (auto iter = range.begin(); iter != range.end(); ++iter) {
        const UsdPrim &prim(*iter);
        bool isInstanceable = prim.IsInstanceable();

        std::string objType = prim.GetTypeName().GetText();
        // skip untyped primitives (unless they're an instance)
        if (objType.empty() && !isInstanceable)
            continue;

        // We traverse every primitive twice : once from root to leaf, 
        // then back from leaf to root. We don't want to anything during "post" visits
        // apart from popping the last element in the primvars stack.
        // This way, the last element in the stack will always match the current 
        // set of primvars
        if (iter.IsPostVisit()) {
            primvarsStack.pop_back();
            continue; 
        }
   
        // Get the inheritable primvars for this xform, by giving its parent ones as input
        UsdGeomPrimvarsAPI primvarsAPI(prim);
        std::vector<UsdGeomPrimvar> primvars = 
            primvarsAPI.FindIncrementallyInheritablePrimvars(primvarsStack.back());
        
        // if the returned vector is empty, we want to keep using the same list as our parent
        if (primvars.empty())
            primvarsStack.push_back(primvarsStack.back());
        else
            primvarsStack.push_back(primvars); // primvars were modified for this xform

        // Check if that primitive is set as being invisible.
        // If so, skip it and prune its children to avoid useless conversions
        // Special case for arnold schemas, they don't inherit from UsdGeomImageable
        // but we author these attributes nevertheless
        if (prim.IsA<UsdGeomImageable>() || objType.substr(0, 6) == "Arnold") {
            UsdGeomImageable imageable(prim);
            bool pruneChildren = false;
            attr = imageable.GetVisibilityAttr();
            if (attr && attr.HasAuthoredValue())
                pruneChildren |= (attr.Get(&visibility, frame) && 
                        visibility == UsdGeomTokens->invisible);

            attr = imageable.GetPurposeAttr();
            if (attr && attr.HasAuthoredValue()) {
                pruneChildren |= ((attr.Get(&purpose, frame) && 
                        purpose != UsdGeomTokens->default_ && 
                        purpose != reader->GetPurpose()));
            }

            if (pruneChildren) {
                iter.PruneChildren();
                iter++; // to avoid post visit
                continue;
            }
        }
        

        // Each thread only considers one primitive for every amount of threads.
        // Note that this must happen after the above visibility test
        if (multithread && ((index++ + threadId) % threadCount))
            continue;

        reader->ReadPrimitive(prim, *threadData->context, isInstanceable);
        // Note: if the registry didn't find any primReader, we're not prunning
        // its children nodes, but just skipping this one.
    }

    // Wait until all the jobs we started finished the translation
    if (reader->GetDispatcher())
        reader->GetDispatcher()->Wait();

    return 0;
}
unsigned int UsdArnoldReader::ProcessConnectionsThread(void *data)
{
    UsdThreadData *threadData = (UsdThreadData *)data;
    if (threadData) {
        threadData->threadContext.ProcessConnections();
    }
    return 0;
}

void UsdArnoldReader::ReadStage(UsdStageRefPtr stage, const std::string &path)
{
    // set the stage while we're reading
    _stage = stage;
    if (stage == nullptr) {
        AiMsgError("[usd] Unable to create USD stage from %s", _filename.c_str());
        return;
    }

    if (_debug) {
        std::string txt("==== Initializing Usd Reader ");
        if (_procParent) {
            txt += " for procedural ";
            txt += AiNodeGetName(_procParent);
        }
        AiMsgWarning(txt.c_str());
    }
    // If this is read through a procedural, we don't want to read
    // options, drivers, filters, etc...
    int procMask = (_procParent) ? (AI_NODE_CAMERA | AI_NODE_LIGHT | AI_NODE_SHAPE | AI_NODE_SHADER | AI_NODE_OPERATOR)
                                 : AI_NODE_ALL;

    // We want to consider the intersection of the reader's mask,
    // and the eventual procedural mask set above
    _mask = _mask & procMask;

    // eventually use a dedicated registry
    if (_registry == nullptr) {
        // No registry was set (default), let's use the global one
        AiCritSecEnter(&s_globalReaderMutex);
        if (s_readerRegistry == nullptr) {
            s_readerRegistry = new UsdArnoldReaderRegistry(); // initialize the global registry
            s_readerRegistry->RegisterPrimitiveReaders();
        }
        AiCritSecLeave(&s_globalReaderMutex);
        _registry = s_readerRegistry;
    } else
        _registry->RegisterPrimitiveReaders();

    UsdPrim rootPrim;
    UsdPrim *rootPrimPtr = nullptr;

    if (!path.empty()) {
        SdfPath sdfPath(path);
        rootPrim = _stage->GetPrimAtPath(sdfPath);
        if (!rootPrim) {
            AiMsgError(
                "[usd] %s : Object Path %s is not valid", (_procParent) ? AiNodeGetName(_procParent) : "",
                path.c_str());
            return;
        }
        if (!rootPrim.IsActive()) {
            AiMsgWarning(
                "[usd] %s : Object Path primitive %s is not active", (_procParent) ? AiNodeGetName(_procParent) : "",
                path.c_str());
            return;   
        }
        rootPrimPtr = &rootPrim;
    }

    // If there is not parent procedural, and we need to lookup the options, then we first need to find the
    // render camera and check its shutter, in order to know if we need to read motion data or not (#346)
    if (_procParent == nullptr) {
        UsdPrim options = _stage->GetPrimAtPath(SdfPath("/options"));
        bool hasCameraToken = options && options.HasAttribute(str::t_arnold_camera);
        bool hasOldCameraToken = !hasCameraToken && options && options.HasAttribute(str::t_camera);

        if (hasCameraToken || hasOldCameraToken) {
            UsdAttribute cameraAttr = options.GetAttribute((hasCameraToken) ? str::t_arnold_camera : str::t_camera);
            if (cameraAttr) {
                std::string cameraName;
                cameraAttr.Get(&cameraName, _time.frame);
                if (!cameraName.empty()) {
                    UsdPrim cameraPrim = _stage->GetPrimAtPath(SdfPath(cameraName.c_str()));
                    if (cameraPrim) {
                        UsdGeomCamera cam(cameraPrim);

                        bool motionBlur = false;
                        float shutterStart = 0.f;
                        float shutterEnd = 0.f;

                        if (cam) {
                            VtValue shutterOpenValue;
                            if (cam.GetShutterOpenAttr().Get(&shutterOpenValue, _time.frame)) {
                                shutterStart = VtValueGetFloat(shutterOpenValue);
                            }
                            VtValue shutterCloseValue;
                            if (cam.GetShutterCloseAttr().Get(&shutterCloseValue, _time.frame)) {
                                shutterEnd = VtValueGetFloat(shutterCloseValue);
                            }
                        }

                        _time.motionBlur = (shutterEnd > shutterStart);
                        _time.motionStart = shutterStart;
                        _time.motionEnd = shutterEnd;
                    }
                }
            }
        }
    }

    // Apply eventual skinning in the scene, for the desired time interval
    UsdPrimRange range = (rootPrimPtr) ? UsdPrimRange(*rootPrimPtr) : _stage->Traverse();
    GfInterval interval(_time.start(), _time.end());

    // Apply the skinning to the whole scene. Note that we don't want to do this
    // with a cache id since the usd stage is owned by someone else and we 
    // shouldn't modify it
    if (_cacheId == 0)
        UsdSkelBakeSkinning(range, interval);

    size_t threadCount = _threadCount; // do we want to do something
                                       // automatic when threadCount = 0 ?

    // If threads = 0, we'll start a single thread to traverse the stage,
    // and every time it finds a primitive to translate it will run a 
    // WorkDispatcher job. 
    if (threadCount == 0) {
        threadCount = 1;
        _dispatcher = new WorkDispatcher();
    }

    // Multi-thread inspection where each thread has its own "context".
    // We'll be looping over the stage primitives,
    // but won't process any connection between nodes, since we need to wait for
    // the target nodes to be created first. We stack the connections, and process them when finished
    std::vector<UsdThreadData> threadData(threadCount);
    std::vector<void *> threads(threadCount, nullptr);

    // First step, we traverse the stage in order to create all nodes
    _readStep = READ_TRAVERSE;
    for (size_t i = 0; i < threadCount; ++i) {
        threadData[i].threadId = i;
        threadData[i].threadCount = threadCount;
        threadData[i].threadContext.SetReader(this);
        threadData[i].rootPrim = rootPrimPtr;
        threadData[i].threadContext.SetDispatcher(_dispatcher);
        threadData[i].context = new UsdArnoldReaderContext(&threadData[i].threadContext);
        threads[i] = AiThreadCreate(UsdArnoldReader::ReaderThread, &threadData[i], AI_PRIORITY_HIGH);
    }

    // Wait until all threads are finished and merge all the nodes that
    // they have created to our list
    for (size_t i = 0; i < threadCount; ++i) {
        AiThreadWait(threads[i]);
        AiThreadClose(threads[i]);
        UsdArnoldReaderThreadContext &context = threadData[i].threadContext;
        _nodes.insert(_nodes.end(), context.GetNodes().begin(), context.GetNodes().end());
        _nodeNames.insert(context.GetNodeNames().begin(), context.GetNodeNames().end());
        context.GetNodes().clear();
        context.GetNodeNames().clear();
        threads[i] = nullptr;
    }

    // Clear the dispatcher here as we no longer need it.
    if (_dispatcher) {
        delete _dispatcher;
        _dispatcher = nullptr;
    }

    // In a second step, each thread goes through the connections it stacked
    // and processes them given that now all the nodes were supposed to be created.
    _readStep = READ_PROCESS_CONNECTIONS;
    for (size_t i = 0; i < threadCount; ++i) {
        // now I just want to append the links from each thread context
        threads[i] = AiThreadCreate(UsdArnoldReader::ProcessConnectionsThread, &threadData[i], AI_PRIORITY_HIGH);
    }
    std::vector<UsdArnoldReaderThreadContext::Connection> danglingConnections;
    // There is an exception though, some connections could be pointing
    // to primitives that were skipped because they weren't visible.
    // In that case the arnold nodes still don't exist yet, and we
    // need to force their export. Here, all the connections pointing
    // to nodes that don't exist yet are kept in each context connections list.
    // We append them in a list of "dangling connections".
    for (size_t i = 0; i < threadCount; ++i) {
        AiThreadWait(threads[i]);
        AiThreadClose(threads[i]);
        threads[i] = nullptr;
        danglingConnections.insert(
            danglingConnections.end(), threadData[i].threadContext.GetConnections().begin(),
            threadData[i].threadContext.GetConnections().end());
        threadData[i].threadContext.GetConnections().clear();
    }
    
    // 3rd step, in case some links were pointing to nodes that didn't exist.
    // If they were skipped because of their visibility, we need to force
    // their export now. We handle this in a single thread to avoid costly
    // synchronizations between the threads.
    _readStep = READ_DANGLING_CONNECTIONS;
    if (!danglingConnections.empty()) {
        // We only use the first thread context
        UsdArnoldReaderThreadContext &context = threadData[0].threadContext;
        // loop over the dangling connections, ensure the node still doesn't exist
        // (as it might be referenced multiple times in our list),
        // and if not we try to read it
        for (auto &&conn : danglingConnections) {
            const char *name = conn.target.c_str();
            AtNode *target = LookupNode(name, true);
            if (target == nullptr) {
                SdfPath sdfPath(name);
                UsdPrim prim = _stage->GetPrimAtPath(sdfPath);
                if (prim)
                    ReadPrimitive(prim, *threadData[0].context);
            }
            // we can now process the connection
            context.ProcessConnection(conn);
        }
        // Some nodes were possibly created in the above loop,
        // we need to append them to our reader
        _nodes.insert(_nodes.end(), context.GetNodes().begin(), context.GetNodes().end());
        _nodeNames.insert(context.GetNodeNames().begin(), context.GetNodeNames().end());
        context.GetNodeNames().clear();
        context.GetNodes().clear();
    }

    for (size_t i = 0; i < threadCount; ++i) {
        delete threadData[i].context;
    }
    _stage = UsdStageRefPtr(); // clear the shared pointer, delete the stage
    _readStep = READ_FINISHED; // We're done
}

void UsdArnoldReader::ReadPrimitive(const UsdPrim &prim, UsdArnoldReaderContext &context, bool isInstance)
{
    std::string objName = prim.GetPath().GetText();

    if (isInstance) {
#if PXR_VERSION >= 2011
        auto proto = prim.GetPrototype();
#else
        auto proto = prim.GetMaster();
#endif
        if (!proto)
            return;
        const TimeSettings &time = context.GetTimeSettings();
        
        AtNode *ginstance = context.CreateArnoldNode("ginstance", objName.c_str());
        if (prim.IsA<UsdGeomXformable>())
            ReadMatrix(prim, ginstance, time, context);
        AiNodeSetFlt(ginstance, str::motion_start, time.motionStart);
        AiNodeSetFlt(ginstance, str::motion_end, time.motionEnd);
        AiNodeSetByte(ginstance, str::visibility, AI_RAY_ALL);
        AiNodeSetBool(ginstance, str::inherit_xform, false);

        // Add a connection from this instance to the prototype. It's likely not going to be
        // Arnold, and will therefore appear as a "dangling" connection. The prototype will
        // therefore be created by a single thread in ProcessConnection. Given that this prim
        // is a prototype, it will be created as a nested usd procedural with object path set 
        // to the protoype prim's name. This will support instances of hierarchies.
        context.AddConnection(
                    ginstance, "node", proto.GetPath().GetText(), CONNECTION_PTR);
        return;
    }        

    std::string objType = prim.GetTypeName().GetText();
    UsdArnoldPrimReader *primReader = _registry->GetPrimReader(objType);
    if (primReader && (_mask & primReader->GetType())) {
        if (_debug) {
            std::string txt;

            txt += "Object ";
            txt += objName;
            txt += " (type: ";
            txt += objType;
            txt += ")";

            AiMsgInfo(txt.c_str());
        }

        if (_dispatcher) {
            AtArray *matrix = ReadMatrix(prim, context.GetTimeSettings(), context, prim.IsA<UsdGeomXformable>());
            // Read the matrix
            UsdArnoldReaderContext *jobContext = new UsdArnoldReaderContext(context, matrix, context.GetThreadContext()->GetPrimvarsStack().back());

            _UsdArnoldPrimReaderJob job = 
                {prim, primReader, jobContext };
                
            _dispatcher->Run(job);
        } else 
            primReader->Read(prim, context); // read this primitive
    }
}
void UsdArnoldReader::SetThreadCount(unsigned int t)
{
    _threadCount = t;

    // if we are in multi-thread, we need to initialize a mutex now
    if (_threadCount != 1 && !_readerLock)
        AiCritSecInit((void **)&_readerLock);
}
void UsdArnoldReader::SetFrame(float frame)
{
    ClearNodes(); // FIXME do we need to clear here ? We should rather re-export
                  // the data
    _time.frame = frame;
}

void UsdArnoldReader::SetMotionBlur(bool motionBlur, float motionStart, float motionEnd)
{
    ClearNodes(); // FIXME do we need to clear here ? We should rather re-export
                  // the data
    _time.motionBlur = motionBlur;
    _time.motionStart = motionStart;
    _time.motionEnd = motionEnd;
}

void UsdArnoldReader::SetDebug(bool b)
{
    // We obviously don't need to clear the data here, but it will make it
    // simpler since the data will be re-generated
    ClearNodes();
    _debug = b;
}
void UsdArnoldReader::SetConvertPrimitives(bool b)
{
    ClearNodes();
    _convert = b;
}
void UsdArnoldReader::ClearNodes()
{
    // FIXME should we also delete the nodes if there is a proc parent ?
    if (_procParent == nullptr) {
        // No parent proc, this means we should delete all nodes ourselves
        for (size_t i = 0; i < _nodes.size(); ++i) {
            AiNodeDestroy(_nodes[i]);
        }
    }
    _nodes.clear();
    _defaultShader = nullptr; // reset defaultShader
}

void UsdArnoldReader::SetProceduralParent(const AtNode *node)
{
    // should we clear the nodes when a new procedural parent is set ?
    ClearNodes();
    _procParent = node;
    _universe = (node) ? AiNodeGetUniverse(node) : nullptr;
}

void UsdArnoldReader::SetRegistry(UsdArnoldReaderRegistry *registry) { _registry = registry; }
void UsdArnoldReader::SetUniverse(AtUniverse *universe)
{
    if (_procParent) {
        if (universe != _universe) {
            AiMsgError(
                "UsdArnoldReader: we cannot set a universe that is different "
                "from the procedural parent");
        }
        return;
    }
    // should we clear the nodes when a new universe is set ?
    ClearNodes();
    _universe = universe;
}

AtNode *UsdArnoldReader::GetDefaultShader()
{
    // Eventually lock the mutex
    LockReader();

    if (_defaultShader == nullptr) {
        // The default shader doesn't exist yet, let's create a standard_surface,
        // which base_color is linked to a user_data_rgb that looks up the user data
        // called "displayColor". This way, by default geometries that don't have any
        // shader assigned will appear as in hydra.
        _defaultShader = AiNode(_universe, "standard_surface", "_default_arnold_shader", _procParent);
        AtNode *userData = AiNode(_universe, "user_data_rgb", "_default_arnold_shader_color", _procParent);
        _nodes.push_back(_defaultShader);
        _nodes.push_back(userData);
        AiNodeSetStr(userData, str::attribute, "displayColor");
        AiNodeSetRGB(userData, str::_default, 1.f, 1.f, 1.f); // neutral white shader if no user data is found
        AiNodeLink(userData, str::base_color, _defaultShader);
    }

    UnlockReader();

    return _defaultShader;
}

UsdArnoldReaderThreadContext::~UsdArnoldReaderThreadContext()
{
    if (_xformCache)
        delete _xformCache;

    for (std::unordered_map<float, UsdGeomXformCache *>::iterator it = _xformCacheMap.begin();
         it != _xformCacheMap.end(); ++it)
        delete it->second;

    _xformCacheMap.clear();
    if (_createNodeLock)
        AiCritSecClose((void **)&_createNodeLock);
    if (_addConnectionLock)
        AiCritSecClose((void **)&_addConnectionLock);
    if (_addNodeNameLock)
        AiCritSecClose((void **)&_addNodeNameLock);

    _createNodeLock = _addConnectionLock = _addNodeNameLock = nullptr;
    
}
void UsdArnoldReaderThreadContext::SetReader(UsdArnoldReader *r)
{
    if (r == nullptr)
        return; // shouldn't happen
    _reader = r;
    // UsdGeomXformCache will be used to trigger world transformation matrices
    // by caching the already computed nodes xforms in the hierarchy.
    if (_xformCache == nullptr)
        _xformCache = new UsdGeomXformCache(UsdTimeCode(r->GetTimeSettings().frame));
}
void UsdArnoldReaderThreadContext::AddNodeName(const std::string &name, AtNode *node)
{
    if (_addNodeNameLock)
        AiCritSecEnter(&_addNodeNameLock);
    _nodeNames[name] = node;
    if (_addNodeNameLock)
        AiCritSecLeave(&_addNodeNameLock);
}

void UsdArnoldReaderThreadContext::SetDispatcher(WorkDispatcher *dispatcher)
{

    _dispatcher = dispatcher;
    if (_dispatcher) {
        if (!_createNodeLock) 
            AiCritSecInit((void **)&_createNodeLock);
        if (!_addConnectionLock)
            AiCritSecInit((void **)&_addConnectionLock);
        if (!_addNodeNameLock)
            AiCritSecInit((void **)&_addNodeNameLock);
    }
}

AtNode *UsdArnoldReaderThreadContext::CreateArnoldNode(const char *type, const char *name)
{    
    AtNode *node = AiNode(_reader->GetUniverse(), type, name, _reader->GetProceduralParent());
    // All shape nodes should have an id parameter if we're coming from a parent procedural
    if (_reader->GetProceduralParent() && AiNodeEntryGetType(AiNodeGetNodeEntry(node)) == AI_NODE_SHAPE) {
        AiNodeSetUInt(node, str::id, _reader->GetId());
    }
    
    if (_createNodeLock)
        AiCritSecEnter(&_createNodeLock);
    _nodes.push_back(node);

    if (_createNodeLock) {
        AiCritSecLeave(&_createNodeLock);
    }
    return node;
}
void UsdArnoldReaderThreadContext::AddConnection(
    AtNode *source, const std::string &attr, const std::string &target, UsdArnoldReader::ConnectionType type, 
    const std::string &outputElement)
{
    if (_reader->GetReadStep() == UsdArnoldReader::READ_TRAVERSE) {
        // store a link between attributes/nodes to process it later
        // If we have a dispatcher, we want to lock here
        if (_addConnectionLock) 
            AiCritSecEnter(&_addConnectionLock);

        _connections.push_back(Connection());
        Connection &conn = _connections.back();
        conn.sourceNode = source;
        conn.sourceAttr = attr;
        conn.target = target;
        conn.type = type;
        conn.outputElement = outputElement;
        if (_addConnectionLock) 
            AiCritSecLeave(&_addConnectionLock);

    } else if (_reader->GetReadStep() == UsdArnoldReader::READ_DANGLING_CONNECTIONS) {
        // we're in the main thread, processing the dangling connections. We want to
        // apply the connection right away
        Connection conn;
        conn.sourceNode = source;
        conn.sourceAttr = attr;
        conn.target = target;
        conn.type = type;
        conn.outputElement = outputElement;
        ProcessConnection(conn);
    }
}
void UsdArnoldReaderThreadContext::ProcessConnections()
{
    _primvarsStack.clear();
    _primvarsStack.push_back(std::vector<UsdGeomPrimvar>());

    std::vector<Connection> danglingConnections;
    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
        // if ProcessConnections returns false, it means that the target
        // wasn't found. We want to stack those dangling connections
        // and keep them in our list
        if (!ProcessConnection(*it))
            danglingConnections.push_back(*it);
    }
    // our connections list is now cleared by contains all the ones
    // that couldn't be resolved
    _connections = danglingConnections;
}

bool UsdArnoldReaderThreadContext::ProcessConnection(const Connection &connection)
{
    UsdArnoldReader::ReadStep step = _reader->GetReadStep();
    if (connection.type == UsdArnoldReader::CONNECTION_ARRAY) {
        std::vector<AtNode *> vecNodes;
        std::stringstream ss(connection.target);
        std::string token;
        while (std::getline(ss, token, ' ')) {
            AtNode *target = _reader->LookupNode(token.c_str(), true);
            if (target == nullptr) {
                if (step == UsdArnoldReader::READ_DANGLING_CONNECTIONS) {
                    // generate the missing node right away
                    SdfPath sdfPath(token.c_str());
                    UsdPrim prim = _reader->GetStage()->GetPrimAtPath(sdfPath);
                    if (prim) {
                        // We need to compute the full list of primvars, including 
                        // inherited ones. 
                        UsdGeomPrimvarsAPI primvarsAPI(prim);
                        _primvarsStack.back() = primvarsAPI.FindPrimvarsWithInheritance();
                        UsdArnoldReaderContext context(this);
                        _reader->ReadPrimitive(prim, context);
                        target = _reader->LookupNode(token.c_str(), true);
                    }
                }
                if (target == nullptr)
                    return false; // node is missing, we don't process the connection
            }
            vecNodes.push_back(target);
        }
        AiNodeSetArray(
            connection.sourceNode, connection.sourceAttr.c_str(),
            AiArrayConvert(vecNodes.size(), 1, AI_TYPE_NODE, &vecNodes[0]));

    } else {
        AtNode *target = _reader->LookupNode(connection.target.c_str(), true);
        if (target == nullptr) {
            if (step == UsdArnoldReader::READ_DANGLING_CONNECTIONS) {
                // generate the missing node right away
                SdfPath sdfPath(connection.target.c_str());
                UsdPrim prim = _reader->GetStage()->GetPrimAtPath(sdfPath);
                if (prim) {
                    UsdGeomPrimvarsAPI primvarsAPI(prim);
                    // We need to compute the full list of primvars, including 
                    // inherited ones. 
                    _primvarsStack.back() = primvarsAPI.FindPrimvarsWithInheritance();
                    UsdArnoldReaderContext context(this);
                    _reader->ReadPrimitive(prim, context);
                    target = _reader->LookupNode(connection.target.c_str(), true);

                    if (target == nullptr && connection.type == UsdArnoldReader::CONNECTION_PTR &&
#if PXR_VERSION >= 2011
                        prim.IsPrototype()
#else
                        prim.IsMaster()
#endif
                        ) {
                        // Since the instance can represent any point in the hierarchy, including
                        // xforms that aren't translated to arnold, we need to create a nested
                        // usd procedural that will only read this specific prim. Note that this 
                        // is similar to what is done by the point instancer reader
                        target = CreateArnoldNode("usd", connection.target.c_str());
                        AiNodeSetStr(target, str::filename, _reader->GetFilename().c_str());
                        AiNodeSetStr(target, str::object_path, connection.target.c_str());
                        const TimeSettings &time = _reader->GetTimeSettings();
                        AiNodeSetFlt(target, str::frame, time.frame); // give it the desired frame
                        AiNodeSetFlt(target, str::motion_start, time.motionStart);
                        AiNodeSetFlt(target, str::motion_end, time.motionEnd);
                        const AtArray *overrides = _reader->GetOverrides();
                        if (overrides)
                            AiNodeSetArray(target, str::overrides, AiArrayCopy(overrides));
                        // Hide the prototype, we'll only want the instance to be visible
                        AiNodeSetByte(target, str::visibility, 0);
                    }
                }
            }
            if (target == nullptr) {
                return false; // node is missing, we don't process the connection
            }
        }
        if (connection.type == UsdArnoldReader::CONNECTION_PTR)
            AiNodeSetPtr(connection.sourceNode, connection.sourceAttr.c_str(), (void *)target);
        else if (connection.type == UsdArnoldReader::CONNECTION_LINK) {

            if (target == nullptr) {
                AiNodeUnlink(connection.sourceNode, connection.sourceAttr.c_str());
            } else {

                static const std::string supportedElems ("xyzrgba");
                const std::string &elem = connection.outputElement;
                // Connection to an output component
                if (elem.length() > 1 && elem[elem.length() - 2] == ':' && supportedElems.find(elem.back()) != std::string::npos) {
                     AiNodeLinkOutput(target, std::string(1,elem.back()).c_str(), connection.sourceNode, connection.sourceAttr.c_str());
                } else {
                    AiNodeLink(target, connection.sourceAttr.c_str(), connection.sourceNode);
                }
            }            
        }
    }
    return true;
}

UsdGeomXformCache *UsdArnoldReaderThreadContext::GetXformCache(float frame)
{
    const TimeSettings &time = _reader->GetTimeSettings();

    if ((time.motionBlur == false || frame == time.frame) && _xformCache)
        return _xformCache; // fastest path : return the main xform cache for the current frame

    UsdGeomXformCache *xformCache = nullptr;

    // Look for a xform cache for the requested frame
    std::unordered_map<float, UsdGeomXformCache *>::iterator it = _xformCacheMap.find(frame);
    if (it == _xformCacheMap.end()) {
        // Need to create a new one.
        // Should we set a hard limit for the amount of xform caches we create ?
        xformCache = new UsdGeomXformCache(UsdTimeCode(frame));
        _xformCacheMap[frame] = xformCache;
    } else {
        xformCache = it->second;
    }

    return xformCache;
}

/// Checks the visibility of the usdPrim
///
/// @param prim The usdPrim we are checking the visibility of
/// @param frame At what frame we are checking the visibility
/// @return  Whether or not the prim is visible
bool UsdArnoldReaderContext::GetPrimVisibility(const UsdPrim &prim, float frame)
{
    // Only compute the visibility when processing the dangling connections,
    // otherwise we return true to avoid costly computation.
    if (_threadContext->GetReader()->GetReadStep() == UsdArnoldReader::READ_DANGLING_CONNECTIONS) {
        UsdGeomImageable imageable = UsdGeomImageable(prim);
        if (imageable)
            return imageable.ComputeVisibility(frame) != UsdGeomTokens->invisible;
    }

    return true;
}
