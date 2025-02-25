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
/// @file renderer_plugin.h
///
/// Renderer plugin for the Render Delegate.
#pragma once

#include <pxr/pxr.h>
#include "api.h"

#include "hdarnold.h"

#include <pxr/imaging/hd/rendererPlugin.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdArnoldRendererPlugin final : public HdRendererPlugin {
public:
    /// Default constructor for HdArnoldRendererPlugin.
    HDARNOLD_API
    HdArnoldRendererPlugin() = default;
    /// Default destructor for HdArnoldRendererPlugin.
    HDARNOLD_API
    ~HdArnoldRendererPlugin() override = default;

    /// This class does not support copying.
    HdArnoldRendererPlugin(const HdArnoldRendererPlugin&) = delete;
    /// This class does not support copying.
    HdArnoldRendererPlugin& operator=(const HdArnoldRendererPlugin&) = delete;

    /// Creates a new Arnold Render Delegate.
    ///
    /// @return Pointer to the newly created Arnold Render Delegate.
    HDARNOLD_API
    HdRenderDelegate* CreateRenderDelegate() override;

    /// Factory a Render Delegate object, that Hydra can use to
    /// factory prims and communicate with a renderer.  Pass in initial
    /// settings.
    ///
    /// @param settingsMap Initial render settings for the Render Delegate.
    /// @return Pointer to the newly created Arnold Render Delegate.
    HD_API
    HdRenderDelegate* CreateRenderDelegate(const HdRenderSettingsMap& settingsMap) override;

    /// Deletes an Arnold Render Delegate.
    ///
    /// @param renderDelegate Pointer to the Arnold Render Delegate to delete.
    HDARNOLD_API
    void DeleteRenderDelegate(HdRenderDelegate* renderDelegate) override;

    /// Returns true if the Render Delegate is supported.
    ///
    /// @return Value indicating if the Render Delegate is supported.
    HDARNOLD_API
    bool IsSupported() const override;
};

PXR_NAMESPACE_CLOSE_SCOPE
