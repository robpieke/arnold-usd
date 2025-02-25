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
#include "write_camera.h"

#include <ai.h>

#include <pxr/base/gf/camera.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usdGeom/camera.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "registry.h"

//-*************************************************************************

PXR_NAMESPACE_USING_DIRECTIVE

void UsdArnoldWriteCamera::Write(const AtNode *node, UsdArnoldWriter &writer)
{
    std::string nodeName = GetArnoldNodeName(node, writer); // what is the USD name for this primitive
    UsdStageRefPtr stage = writer.GetUsdStage();    // Get the USD stage defined in the writer
    SdfPath objPath(nodeName);
    writer.CreateHierarchy(objPath);
    UsdGeomCamera cam = UsdGeomCamera::Define(stage, objPath);
    UsdPrim prim = cam.GetPrim();

    TfToken projection;
    bool persp = false;
    if (_type == CAMERA_PERSPECTIVE) {
        persp = true;
        projection = TfToken("perspective");
    } else if (_type == CAMERA_ORTHOGRAPHIC) {
        projection = TfToken("orthographic");
    } else {
        AiMsgError("[usd] Invalid camera type %s", nodeName.c_str());
        return; // invalid camera type
    }
    writer.SetAttribute(cam.CreateProjectionAttr(), projection);
    
    if (persp) {
        AtNode *options = AiUniverseGetOptions(writer.GetUniverse());

        float fov = AiNodeGetFlt(node, "fov");
        float horizontalAperature = tan(fov * AI_DTOR * 0.5f);
        horizontalAperature *= (2.f * 50.f * GfCamera::FOCAL_LENGTH_UNIT);
        horizontalAperature /= GfCamera::APERTURE_UNIT;

        writer.SetAttribute(cam.CreateHorizontalApertureAttr(), horizontalAperature);
        float verticalAperture = horizontalAperature;

        // Use the options image resolution to determine the vertical aperture
        if (options) {
            verticalAperture *= (float)AiNodeGetInt(options, "yres") / (float)AiNodeGetInt(options, "xres");
        }
        
        writer.SetAttribute(cam.CreateVerticalApertureAttr(), verticalAperture);
        // Note that we're not adding "fov" to the list of exported attrs, because we still
        // want it to be set as an arnold-specific attribute. This way, when it's read from usd,
        // we can get the exact same value without any difference caused by the back and forth conversions
        writer.SetAttribute(cam.CreateFocusDistanceAttr(), AiNodeGetFlt(node, "focus_distance"));
        _exportedAttrs.insert("focus_distance");
    }

    // To be written in both perspective and orthographic cameras:
    GfVec2f clippingRange = GfVec2f(AiNodeGetFlt(node, "near_clip"), AiNodeGetFlt(node, "far_clip"));
    writer.SetAttribute(cam.CreateClippingRangeAttr(), clippingRange);

    _exportedAttrs.insert("near_clip");
    _exportedAttrs.insert("far_clip");

    writer.SetAttribute(cam.CreateShutterOpenAttr(), double(AiNodeGetFlt(node, "shutter_start")));
    writer.SetAttribute(cam.CreateShutterCloseAttr(), double(AiNodeGetFlt(node, "shutter_end")));

    _exportedAttrs.insert("shutter_start");
    _exportedAttrs.insert("shutter_end");

    _WriteMatrix(cam, node, writer);
    _WriteArnoldParameters(node, writer, prim, "primvars:arnold");
}
