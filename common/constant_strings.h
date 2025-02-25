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
/// @file constant_strings.h
///
/// File holding shared, constant definitions of AtString instances.
///
/// Defining EXPAND_ARNOLD_USD_STRINGS before including constant_strings.h will not only
/// declare but also define the AtString instances.
#pragma once

#include <ai_string.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace str {

#ifdef EXPAND_ARNOLD_USD_STRINGS
#define ASTR(x)                 \
    extern const AtString x;    \
    const AtString x(#x);       \
    extern const TfToken t_##x; \
    const TfToken t_##x(#x)
#define ASTR2(x, y)             \
    extern const AtString x;    \
    const AtString x(y);        \
    extern const TfToken t_##x; \
    const TfToken t_##x(y)
#else
#define ASTR(x)              \
    extern const AtString x; \
    extern const TfToken t_##x
#define ASTR2(x, y)          \
    extern const AtString x; \
    extern const TfToken t_##x
#endif

ASTR2(_default, "default");
ASTR2(_max, "max");
ASTR2(_min, "min");
ASTR2(arnold__attributes, "arnold::attributes");
ASTR2(arnold_camera, "arnold:camera");
ASTR2(arnold_filename, "arnold:filename");
ASTR2(arnold_node_entry, "arnold:node_entry");
ASTR2(arnold_prefix, "arnold:");
ASTR2(b_spline, "b-spline");
ASTR2(catmull_rom, "catmull-rom");
ASTR2(inputs_code, "inputs:code");
ASTR2(primvars_arnold_subdiv_type, "primvars:arnold:subdiv_type");
ASTR2(render_context, "RENDER_CONTEXT");
ASTR2(renderPassAOVDriver, "HdArnoldRenderPass_aov_driver");
ASTR2(renderPassCamera, "HdArnoldRenderPass_camera");
ASTR2(renderPassClosestFilter, "HdArnoldRenderPass_closestFilter");
ASTR2(renderPassFilter, "HdArnoldRenderPass_beautyFilter");
ASTR2(renderPassMainDriver, "HdArnoldRenderPass_main_driver");
ASTR2(constantArrayFloat, "CONSTANT ARRAY FLOAT");

ASTR(AA_sample_clamp);
ASTR(AA_sample_clamp_affects_aovs);
ASTR(AA_samples);
ASTR(AA_samples_max);
ASTR(AA_seed);
ASTR(CPU);
ASTR(GI_diffuse_depth);
ASTR(GI_diffuse_samples);
ASTR(GI_specular_depth);
ASTR(GI_specular_samples);
ASTR(GI_sss_samples);
ASTR(GI_total_depth);
ASTR(GI_transmission_depth);
ASTR(GI_transmission_samples);
ASTR(GI_volume_depth);
ASTR(GI_volume_samples);
ASTR(GPU);
ASTR(HdArnoldDriverAOV);
ASTR(HdArnoldDriverMain);
ASTR(UsdPreviewSurface);
ASTR(UsdPrimvarReader_float);
ASTR(UsdPrimvarReader_float2);
ASTR(UsdPrimvarReader_float3);
ASTR(UsdPrimvarReader_float4);
ASTR(UsdPrimvarReader_int);
ASTR(UsdPrimvarReader_normal);
ASTR(UsdPrimvarReader_point);
ASTR(UsdPrimvarReader_string);
ASTR(UsdPrimvarReader_vector);
ASTR(UsdTransform2d);
ASTR(UsdUVTexture);
ASTR(abort_on_error);
ASTR(abort_on_license_fail);
ASTR(accelerations);
ASTR(all_attributes);
ASTR(ambocc);
ASTR(angle);
ASTR(angular);
ASTR(aov_input);
ASTR(aov_name);
ASTR(aov_pointer);
ASTR(aov_shaders);
ASTR(aov_write_float);
ASTR(aov_write_int);
ASTR(aov_write_rgb);
ASTR(aov_write_rgba);
ASTR(aov_write_vector);
ASTR(aperture_size);
ASTR(append);
ASTR(arnold);
ASTR(attribute);
ASTR(auto_transparency_depth);
ASTR(autobump_visibility);
ASTR(barndoor);
ASTR(barndoor_bottom_edge);
ASTR(barndoor_bottom_left);
ASTR(barndoor_bottom_right);
ASTR(barndoor_left_bottom);
ASTR(barndoor_left_edge);
ASTR(barndoor_left_top);
ASTR(barndoor_right_bottom);
ASTR(barndoor_right_edge);
ASTR(barndoor_right_top);
ASTR(barndoor_top_edge);
ASTR(barndoor_top_left);
ASTR(barndoor_top_right);
ASTR(base);
ASTR(base_color);
ASTR(basis);
ASTR(bezier);
ASTR(bias);
ASTR(black);
ASTR(blend_opacity);
ASTR(bottom);
ASTR(box_filter);
ASTR(bucket_scanning);
ASTR(bucket_size);
ASTR(camera);
ASTR(catclark);
ASTR(clamp);
ASTR(clearcoat);
ASTR(clearcoatRoughness);
ASTR(closest_filter);
ASTR(coat);
ASTR(coat_roughness);
ASTR(code);
ASTR(color);
ASTR(color_mode);
ASTR(color_pointer);
ASTR(color_to_signed);
ASTR(cone_angle);
ASTR(cosine_power);
ASTR(crease_idxs);
ASTR(crease_sharpness);
ASTR(curves);
ASTR(cylinder_light);
ASTR(depth_pointer);
ASTR(diffuse);
ASTR(diffuseColor);
ASTR(disk_light);
ASTR(disp_map);
ASTR(displacement);
ASTR(displayColor);
ASTR(distant_light);
ASTR(driver_deepexr);
ASTR(emission);
ASTR(emission_color);
ASTR(emissiveColor);
ASTR(enable_adaptive_sampling);
ASTR(enable_dependency_graph);
ASTR(enable_dithered_sampling);
ASTR(enable_gpu_rendering);
ASTR(enable_new_point_light_sampler);
ASTR(enable_new_quad_light_sampler);
ASTR(enable_procedural_cache);
ASTR(enable_progressive_pattern);
ASTR(enable_progressive_render);
ASTR(exposure);
ASTR(fallback);
ASTR(far_clip);
ASTR(file);
ASTR(filename);
ASTR(filters);
ASTR(filterwidth);
ASTR(flat);
ASTR(focus_distance);
ASTR(format);
ASTR(fov);
ASTR(fps);
ASTR(frame);
ASTR(gaussian_filter);
ASTR(ginstance);
ASTR(grids);
ASTR(husk);
ASTR(hydra);
ASTR(id);
ASTR(id_pointer);
ASTR(ignore_atmosphere);
ASTR(ignore_bump);
ASTR(ignore_displacement);
ASTR(ignore_dof);
ASTR(ignore_lights);
ASTR(ignore_motion);
ASTR(ignore_motion_blur);
ASTR(ignore_operators);
ASTR(ignore_shaders);
ASTR(ignore_shadows);
ASTR(ignore_smoothing);
ASTR(ignore_sss);
ASTR(ignore_subdivision);
ASTR(ignore_textures);
ASTR(image);
ASTR(in);
ASTR(indirect_sample_clamp);
ASTR(inherit_xform);
ASTR(input);
ASTR(instance_inherit_xform);
ASTR(instance_matrix);
ASTR(instance_motion_end);
ASTR(instance_motion_start);
ASTR(instance_shader);
ASTR(instance_visibility);
ASTR(instancer);
ASTR(intensity);
ASTR(interactive);
ASTR(interactive_fps_min);
ASTR(interactive_target_fps);
ASTR(interactive_target_fps_min);
ASTR(ior);
ASTR(latlong);
ASTR(layer_enable_filtering);
ASTR(layer_half_precision);
ASTR(layer_tolerance);
ASTR(light_group);
ASTR(light_path_expressions);
ASTR(linear);
ASTR(log_file);
ASTR(log_flags_console);
ASTR(log_flags_file);
ASTR(log_verbosity);
ASTR(mask);
ASTR(matrix);
ASTR(matrix_multiply_vector);
ASTR(metallic);
ASTR(metalness);
ASTR(mirror);
ASTR(mirrored_ball);
ASTR(missing);
ASTR(missing_texture_color);
ASTR(motion_end);
ASTR(motion_start);
ASTR(multiply);
ASTR(name);
ASTR(near_clip);
ASTR(nidxs);
ASTR(nlist);
ASTR(node);
ASTR(node_entry);
ASTR(node_idxs);
ASTR(nodes);
ASTR(none);
ASTR(normal);
ASTR(normal_nonexistant_rename);
ASTR(normalize);
ASTR(nsides);
ASTR(num_points);
ASTR(object_path);
ASTR(offset);
ASTR(opacity);
ASTR(opaque);
ASTR(options);
ASTR(orientations);
ASTR(ortho_camera);
ASTR(osl);
ASTR(osl_includepath);
ASTR(outputs);
ASTR(overrides);
ASTR(parallel_node_init);
ASTR(penumbra_angle);
ASTR(periodic);
ASTR(persp_camera);
ASTR(photometric_light);
ASTR(pin_threads);
ASTR(pixel_aspect_ratio);
ASTR(plugin_searchpath);
ASTR(point_light);
ASTR(points);
ASTR(polymesh);
ASTR(procedural_searchpath);
ASTR(profile_file);
ASTR(progressive);
ASTR(progressive_min_AA_samples);
ASTR(progressive_show_all_outputs);
ASTR(projMtx);
ASTR(quad_light);
ASTR(radius);
ASTR(reference_time);
ASTR(region_max_x);
ASTR(region_max_y);
ASTR(region_min_x);
ASTR(region_min_y);
ASTR(render_device);
ASTR(repeat);
ASTR(rotation);
ASTR(roughness);
ASTR(scale);
ASTR(scope);
ASTR(shade_mode);
ASTR(shader);
ASTR(shadow_group);
ASTR(shidxs);
ASTR(shutter_end);
ASTR(shutter_start);
ASTR(sidedness);
ASTR(skydome_light);
ASTR(smoothing);
ASTR(specular);
ASTR(specularColor);
ASTR(specular_IOR);
ASTR(specular_color);
ASTR(specular_roughness);
ASTR(spot_light);
ASTR(st);
ASTR(standard_surface);
ASTR(standard_volume);
ASTR(step_size);
ASTR(subdiv_iterations);
ASTR(subdiv_type);
ASTR(swrap);
ASTR(texture_accept_unmipped);
ASTR(texture_accept_untiled);
ASTR(texture_automip);
ASTR(texture_autotile);
ASTR(texture_conservative_lookups);
ASTR(texture_failure_retries);
ASTR(texture_max_open_files);
ASTR(texture_per_file_stats);
ASTR(texture_searchpath);
ASTR(tflip);
ASTR(thread_priority);
ASTR(threads);
ASTR(top);
ASTR(total_progress);
ASTR(translation);
ASTR(twrap);
ASTR(useMetadata);
ASTR(useSpecularWorkflow);
ASTR(use_light_group);
ASTR(use_shadow_group);
ASTR(user_data_float);
ASTR(user_data_int);
ASTR(user_data_rgb);
ASTR(user_data_rgba);
ASTR(user_data_string);
ASTR(utility);
ASTR(uv);
ASTR(uvcoords);
ASTR(uvidxs);
ASTR(uvlist);
ASTR(uvs);
ASTR(uvset);
ASTR(varname);
ASTR(velocities);
ASTR(vertices);
ASTR(vidxs);
ASTR(viewMtx);
ASTR(visibility);
ASTR(vlist);
ASTR(volume);
ASTR(width);
ASTR(wrapS);
ASTR(wrapT);
ASTR(xres);
ASTR(yres);

#undef ASTR
#undef ASTR2

} // namespace str

PXR_NAMESPACE_CLOSE_SCOPE
