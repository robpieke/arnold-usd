// Copyright 2019 Luma Pictures
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
//
// Modifications Copyright 2019 Autodesk, Inc.
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
/// @file render_delegate.h
///
/// Render Delegate class for Hydra.
#pragma once

#include <pxr/pxr.h>
#include "api.h"

#include <pxr/base/vt/array.h>

#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/imaging/hd/resourceRegistry.h>

#include <tbb/concurrent_queue.h>

#include "hdarnold.h"
#include "render_param.h"

#include <ai.h>

PXR_NAMESPACE_OPEN_SCOPE

struct HdArnoldRenderVar {
    /// Settings for the RenderVar.
    HdAovSettingsMap settings;
    /// Name of the render var.
    std::string name;
    /// Source name of the Render Var.
    std::string sourceName;
    /// Source type of the Render Var.
    TfToken sourceType;
    /// Data Type of the Render Var.
    TfToken dataType;
    /// Format of the AOV descriptor.
    HdFormat format = HdFormatFloat32Vec4;
    /// Clear Value, currently ignored.
    VtValue clearValue;
    /// Whether or not the render var is multisampled, currently ignored.
    bool multiSampled = true;
};

struct HdArnoldDelegateRenderProduct {
    /// List of RenderVars used by the RenderProduct.
    std::vector<HdArnoldRenderVar> renderVars;
    /// Map of settings for the RenderProduct.
    HdAovSettingsMap settings;
    /// Name of the product, this is equal to the output location.
    TfToken productName;
};

/// Render context for the render delegate.
enum class HdArnoldRenderContext {
    Hydra, ///< Generic Hydra renderer.
    Husk,  ///< Husk from Houdini.
};

/// Main class point for the Arnold Render Delegate.
class HdArnoldRenderDelegate final : public HdRenderDelegate {
public:
    HDARNOLD_API
    HdArnoldRenderDelegate(
        HdArnoldRenderContext context = HdArnoldRenderContext::Hydra); ///< Constructor for the Render Delegate.
    HDARNOLD_API
    ~HdArnoldRenderDelegate() override; ///< Destuctor for the Render Delegate.
    /// Returns an instance of HdArnoldRenderParam.
    ///
    /// @return Pointer to an instance of HdArnoldRenderParam.
    HDARNOLD_API
    HdRenderParam* GetRenderParam() const override;
    /// Returns the list of RPrim type names that the Render Delegate supports.
    ///
    /// This list contains renderable primitive types, like meshes, curves,
    /// volumes and so on.
    ///
    /// @return VtArray holding the name of the supported RPrim types.
    HDARNOLD_API
    const TfTokenVector& GetSupportedRprimTypes() const override;
    /// Returns the list of SPrim type names that the Render Delegate supports.
    ///
    /// This list contains state primitive types, like cameras, materials and
    /// lights.
    ///
    /// @return VtArray holding the name of the supported SPrim types.
    HDARNOLD_API
    const TfTokenVector& GetSupportedSprimTypes() const override;
    /// Returns the list of BPrim type names that the Render Delegate supports.
    ///
    /// This list contains buffer primitive types, like render buffers,
    /// openvdb assets and so on.
    ///
    /// @return VtArray holding the name of the supported BPrim types.
    HDARNOLD_API
    const TfTokenVector& GetSupportedBprimTypes() const override;
    /// Sets the Render Setting for the given key.
    ///
    /// @param key Name of the Render Setting to set.
    /// @param value Value of the Render Setting.
    HDARNOLD_API
    void SetRenderSetting(const TfToken& key, const VtValue& value) override;
    /// Gets the Render Setting for the given key.
    ///
    /// @param key Name of the Render Setting to get.
    /// @return Value of the Render Setting.
    HDARNOLD_API
    VtValue GetRenderSetting(const TfToken& _key) const override;
    /// Gets the list of Render Setting descriptors.
    ///
    /// @return std::vector holding HdRenderSettingDescriptor for all the
    ///  possible Render Settings.
    HDARNOLD_API
    HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override;
    /// Returns an open-format dictionary of render statistics
    ///
    /// @return VtDictionary holding the render stats.
    HDARNOLD_API
    VtDictionary GetRenderStats() const override;
    /// Gets the Resource Registry.
    ///
    /// @return Pointer to the shared HdResourceRegistry.
    HDARNOLD_API
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;
    /// Creates a new Render Pass.
    ///
    /// @param index Pointer to HdRenderIndex.
    /// @param collection RPrim collection to bind to the newly created Render
    ///  Pass.
    /// @return A shared pointer to the new Render Pass or nullptr on error.
    HDARNOLD_API
    HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex* index, HdRprimCollection const& collection) override;
#if PXR_VERSION >= 2102
    /// Request to create a new instancer.
    ///
    /// @param id The unique identifier of this instancer.
    /// @return A pointer to the new instancer or nullptr on error.
    HdInstancer* CreateInstancer(HdSceneDelegate* delegate, SdfPath const& id) override;
#else
    /// Creates a new Point Instancer.
    ///
    /// @param delegate Pointer to the Scene Delegate.
    /// @param id Path to the Point Instancer.
    /// @param instancerId Path to the parent Point Instancer.
    /// @return Pointer to a new Point Instancer or nullptr on error.
    HDARNOLD_API
    HdInstancer* CreateInstancer(HdSceneDelegate* delegate, SdfPath const& id, SdfPath const& instancerId) override;
#endif
    /// Destroys a Point Instancer.
    ///
    /// @param instancer Pointer to an instance of HdInstancer.
    HDARNOLD_API
    void DestroyInstancer(HdInstancer* instancer) override;
#if PXR_VERSION >= 2102
    /// Creates a new RPrim.
    ///
    /// @param typeId Type name of the primitive.
    /// @param rprimId Path to the primitive.
    /// @return Pointer to the newly created RPrim or nullptr on error.
    HDARNOLD_API
    HdRprim* CreateRprim(TfToken const& typeId, SdfPath const& rprimId) override;
#else
    /// Creates a new RPrim.
    ///
    /// @param typeId Type name of the primitive.
    /// @param rprimId Path to the primitive.
    /// @param instancerId Path to the parent Point Instancer.
    /// @return Pointer to the newly created RPrim or nullptr on error.
    HDARNOLD_API
    HdRprim* CreateRprim(TfToken const& typeId, SdfPath const& rprimId, SdfPath const& instancerId) override;
#endif
    /// Destroys an RPrim.
    ///
    /// @param rPrim Pointer to an RPrim.
    HDARNOLD_API
    void DestroyRprim(HdRprim* rPrim) override;
    /// Creates a new SPrim.
    ///
    /// @param typeId Type of the SPrim to create.
    /// @param sprimId Path to the primitive.
    /// @return Pointer to a new SPrim or nullptr on error.
    HDARNOLD_API
    HdSprim* CreateSprim(TfToken const& typeId, SdfPath const& sprimId) override;
    /// Creates a fallback SPrim.
    ///
    /// @param typeId Type of the fallback SPrim to create.
    /// @return Pointer to a fallback SPrim or nullptr on error.
    HDARNOLD_API
    HdSprim* CreateFallbackSprim(TfToken const& typeId) override;
    /// Destroys an SPrim.
    ///
    /// @param sPrim Pointer to an SPrim.
    HDARNOLD_API
    void DestroySprim(HdSprim* sPrim) override;
    /// Creates a new BPrim.
    ///
    /// @param typeId Type of the new BPrim to create.
    /// @param bprimId Path to the primitive.
    /// @return Pointer to the newly created BPrim or nullptr on error.
    HDARNOLD_API
    HdBprim* CreateBprim(TfToken const& typeId, SdfPath const& bprimId) override;
    /// Creates a fallback BPrim.
    ///
    /// @param typeId Type of the fallback Bprim to create.
    /// @return Pointer to the fallback BPrim or nullptr on error.
    HDARNOLD_API
    HdBprim* CreateFallbackBprim(TfToken const& typeId) override;
    /// Destroys a BPrim.
    ///
    /// @param bPrim Pointer to the BPrim.
    HDARNOLD_API
    void DestroyBprim(HdBprim* bPrim) override;
    /// Commits resources to the Render Delegate.
    ///
    /// This is a callback for a Render Delegate to move, update memory for
    /// resources.
    ///
    /// @param tracker Pointer to the Change Tracker.
    HDARNOLD_API
    void CommitResources(HdChangeTracker* tracker) override;
    /// Returns a token to indicate wich material binding should be used.
    ///
    /// The function currently returns "full", to indicate production renders,
    /// not the default "preview" value.
    ///
    /// @return Name of the preferred material binding.
    HDARNOLD_API
    TfToken GetMaterialBindingPurpose() const override;
    /// Returns a token to indicate which material network should be preferred.
    ///
    /// The function currently returns "arnold", to indicate using
    /// outputs:arnold:surface over outputs:surface (and displacement/volume)
    /// if avialable.
    ///
    /// @return Name of the preferred material network.
    HDARNOLD_API
    TfToken GetMaterialNetworkSelector() const override;
    /// Suffixes Node names with the Render Delegate's paths.
    ///
    /// @param name Name of the Node.
    /// @return The Node's name suffixed by the Render Delegate's path.
    HDARNOLD_API
    AtString GetLocalNodeName(const AtString& name) const;
    /// Gets the active Arnold Universe.
    ///
    /// @return Pointer to the Arnold Universe used by the Render Delegate.
    HDARNOLD_API
    AtUniverse* GetUniverse() const;
    /// Gets the Arnold Options node.
    ///
    /// @return Pointer to the Arnold Options Node.
    HDARNOLD_API
    AtNode* GetOptions() const;
    /// Gets the fallback Arnold Shader.
    ///
    /// The fallback shader is a "utility" shader, with "shade_mode" of "flat",
    /// "color_mode" of "color" and a "user_data_rgba" is connected to "color",
    /// which reads the "color" attribute with the default value of
    /// AtRGBA(1.0f, 1.0f, 1.0f, 1.0).
    ///
    /// @return Pointer to the fallback Arnold Shader.
    HDARNOLD_API
    AtNode* GetFallbackShader() const;
    /// Gets fallback Arnold Volume shader.
    ///
    /// The fallback shader is just an instances of standard_volume.
    ///
    /// @return Pointer to the fallback Arnold Volume Shader.
    HDARNOLD_API
    AtNode* GetFallbackVolumeShader() const;
    /// Gets the default settings for supported aovs.
    HDARNOLD_API
    HdAovDescriptor GetDefaultAovDescriptor(const TfToken& name) const override;

    /// Registers a light in a light linking collection.
    ///
    /// @param name Name of the collection.
    /// @param light Pointer to the Hydra Light object.
    /// @param isShadow If the clection is for shadow or light linking.
    HDARNOLD_API
    void RegisterLightLinking(const TfToken& name, HdLight* light, bool isShadow = false);

    /// Deregisters a light in a light linking collection.
    ///
    /// @param name Name of the collection.
    /// @param light Pointer to the Hydra Light object.
    /// @param isShadow If the clection is for shadow or light linking.
    HDARNOLD_API
    void DeregisterLightLinking(const TfToken& name, HdLight* light, bool isShadow = false);

    /// Apply light linking to a shape.
    ///
    /// @param shape Pointer to the Arnold Shape.
    /// @param categories List of categories the shape belongs to.
    HDARNOLD_API
    void ApplyLightLinking(AtNode* shape, const VtArray<TfToken>& categories);

    /// Tells whether or not the current convergence iteration should be skipped.
    ///
    /// This function can be used to skip calling the render function in HdRenderPass, so a sync step will be enforced
    /// before the next iteration.
    ///
    /// @param renderIndex Pointer to the Hydra Render Index.
    /// @param shutterOpen Shutter Open value of the active camera.
    /// @param shutterClose Shutter Close value of the active camera.
    /// @return True if the iteration should be skipped.
    HDARNOLD_API
    bool ShouldSkipIteration(HdRenderIndex* renderIndex, float shutterOpen, float shutterClose);

    using DelegateRenderProducts = std::vector<HdArnoldDelegateRenderProduct>;
    /// Returns the list of available Delegate Render Products.
    ///
    /// @return Const Reference to the list of Delegate Render Products.
    const DelegateRenderProducts& GetDelegateRenderProducts() const { return _delegateRenderProducts; }
    /// Advertise whether this delegate supports pausing and resuming of
    /// background render threads. Default implementation returns false.
    ///
    /// @return True if pause/restart is supported.
    HDARNOLD_API
    bool IsPauseSupported() const override;
    /// Pause all of this delegate's background rendering threads. Default
    /// implementation does nothing.
    ///
    /// @return True if successful.
    HDARNOLD_API
    bool Pause() override;
    /// Resume all of this delegate's background rendering threads previously
    /// paused by a call to Pause. Default implementation does nothing.
    ///
    /// @return True if successful.
    HDARNOLD_API
    bool Resume() override;

#if PXR_VERSION >= 2011
    using NativeRprimParamList = std::unordered_map<TfToken, const AtParamEntry*, TfToken::HashFunctor>;
#else
    using NativeRprimParamList = std::vector<std::pair<TfToken, const AtParamEntry*> >;
#endif

    /// Returns a list of parameters for each native rprim.
    ///
    /// @param arnoldNodeType Type of the arnold node.
    /// @return Constant Pointer to the list of parameters for each native rprim.
    HDARNOLD_API
    const NativeRprimParamList* GetNativeRprimParamList(const AtString& arnoldNodeType) const;

    /// Dirties a material when terminals change.
    /// Use this to trigger forced updates on shapes, in cases where it's uncertain if the shape has it's material id
    /// updated.
    ///
    /// @param id Path to the material.
    HDARNOLD_API
    void DirtyMaterial(const SdfPath& id);

    /// Remove material from the list tracking dependencies between shapes and materials.
    ///
    /// @param id Path to the material.
    HDARNOLD_API
    void RemoveMaterial(const SdfPath& id);

    /// Track materials assigned to a shape.
    ///
    /// @param shape Id of the shape to track.
    /// @param materials List of materials to track for each shape.
    HDARNOLD_API
    void TrackShapeMaterials(const SdfPath& shape, const VtArray<SdfPath>& materials);

    /// Untrack materials assigned to a shape.
    ///
    /// @param shape Id of the shape to track.
    /// @param materials List of materials to untrack for each shape.
    HDARNOLD_API
    void UntrackShapeMaterials(const SdfPath& shape, const VtArray<SdfPath>& materials);

private:
    HdArnoldRenderDelegate(const HdArnoldRenderDelegate&) = delete;
    HdArnoldRenderDelegate& operator=(const HdArnoldRenderDelegate&) = delete;

    void _SetRenderSetting(const TfToken& _key, const VtValue& value);

    void _ParseDelegateRenderProducts(const VtValue& value);

    /// Mutex for the shared Resource Registry.
    static std::mutex _mutexResourceRegistry;
    /// Atomic counter for the shared Resource Registry.
    static std::atomic_int _counterResourceRegistry;
    /// Pointer to the shared Resource Registry.
    static HdResourceRegistrySharedPtr _resourceRegistry;

    using LightLinkingMap = std::unordered_map<TfToken, std::vector<HdLight*>, TfToken::HashFunctor>;
    using NativeRprimTypeMap = std::unordered_map<TfToken, AtString, TfToken::HashFunctor>;
    using NativeRprimParams = std::unordered_map<AtString, NativeRprimParamList, AtStringHash>;
    // Should we use a std::vector here instead?
    using MaterialToShapeMap = std::unordered_map<SdfPath, std::unordered_set<SdfPath, SdfPath::Hash>, SdfPath::Hash>;
    using MaterialChangesQueue = tbb::concurrent_queue<SdfPath>;

    struct ShapeMaterialChange {
        SdfPath shape;
        VtArray<SdfPath> materials;

        ShapeMaterialChange() = default;

        ShapeMaterialChange(const SdfPath& _shape, const VtArray<SdfPath>& _materials)
            : shape(_shape), materials(_materials)
        {
        }
    };
    using ShapeMaterialChangesQueue = tbb::concurrent_queue<ShapeMaterialChange>;

    MaterialChangesQueue _materialDirtyQueue;             ///< Queue to track material terminal dirty events.
    MaterialChangesQueue _materialRemovalQueue;           ///< Queue to track material removal events.
    ShapeMaterialChangesQueue _shapeMaterialTrackQueue;   ///< Queue to track shape material assignment changes.
    ShapeMaterialChangesQueue _shapeMaterialUntrackQueue; ///< Queue to untrack shape material assignment changes.
    MaterialToShapeMap _materialToShapeMap;               ///< Map to track dependencies between materials and shapes.

    std::mutex _lightLinkingMutex;                  ///< Mutex to lock all light linking operations.
    LightLinkingMap _lightLinks;                    ///< Light Link categories.
    LightLinkingMap _shadowLinks;                   ///< Shadow Link categories.
    std::atomic<bool> _lightLinkingChanged;         ///< Whether or not Light Linking have changed.
    DelegateRenderProducts _delegateRenderProducts; ///< Delegate Render Products for batch renders via husk.
    TfTokenVector _supportedRprimTypes;             ///< List of supported rprim types.
    NativeRprimTypeMap _nativeRprimTypes;           ///< Remapping between the native rprim type names and arnold types.
    NativeRprimParams _nativeRprimParams;           ///< List of parameters for native rprims.
    /// Pointer to an instance of HdArnoldRenderParam.
    ///
    /// This is shared with all the primitives, so they can control the flow of
    /// rendering.
    std::unique_ptr<HdArnoldRenderParam> _renderParam;
    SdfPath _id;                   ///< Path of the Render Delegate.
    AtUniverse* _universe;         ///< Universe used by the Render Delegate.
    AtNode* _options;              ///< Pointer to the Arnold Options Node.
    AtNode* _fallbackShader;       ///< Pointer to the fallback Arnold Shader.
    AtNode* _fallbackVolumeShader; ///< Pointer to the fallback Arnold Volume Shader.
    std::string _logFile;
    /// Top level render context using Hydra. Ie. Hydra, Solaris, Husk.
    HdArnoldRenderContext _context = HdArnoldRenderContext::Hydra;
    int _verbosityLogFlags = AI_LOG_WARNINGS | AI_LOG_ERRORS;
    float _shutterOpen = 0.0f;  ///< Saved Shutter Open value of the active camera.
    float _shutterClose = 0.0f; ///< Saved Shutter Close value of the active camera.
    bool _ignoreVerbosityLogFlags = false;
};

PXR_NAMESPACE_CLOSE_SCOPE
