// The frame loop of the visionOS example: CompositorServices hands over each frame's
// drawables, ARKit gives the device's pose at the frame's presentation time, and the scene in
// ../xr_scene.esk draws each view into the drawable's color texture through wgpu
// (wgpuDeviceCreateTextureFromMetal, eskiu-lang/wgpu-native's eskiu branch).
#import <CompositorServices/CompositorServices.h>
#import <ARKit/ARKit.h>
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <stdio.h>

int xr_scene_open(void);
void xr_scene_step(float dt);
int xr_scene_render(uint64_t image, uint32_t width, uint32_t height, int format, float *pose,
                    float left, float right, float top, float bottom);
void xr_scene_wait(void);

void eskiu_xr_run(cp_layer_renderer_t renderer) {
    if (xr_scene_open() != 0) {
        fprintf(stderr, "xr: the scene did not open\n");
        return;
    }
    ar_session_t session = ar_session_create();
    ar_world_tracking_configuration_t wtc = ar_world_tracking_configuration_create();
    ar_world_tracking_provider_t world = ar_world_tracking_provider_create(wtc);
    ar_data_providers_t providers = ar_data_providers_create_with_data_providers(world, nil);
    ar_session_run(session, providers);

    id<MTLDevice> device = cp_layer_renderer_get_device(renderer);
    id<MTLCommandQueue> queue = [device newCommandQueue];
    cp_layer_renderer_configuration_t config = cp_layer_renderer_get_configuration(renderer);
    // WGPUTextureFormat_BGRA8UnormSrgb or WGPUTextureFormat_BGRA8Unorm
    int format = cp_layer_renderer_configuration_get_color_format(config) == MTLPixelFormatBGRA8Unorm_sRGB ? 0x1C : 0x1B;
    CFTimeInterval last = 0;
    unsigned long frames = 0;
    for (;;) {
        cp_layer_renderer_state state = cp_layer_renderer_get_state(renderer);
        if (state == cp_layer_renderer_state_invalidated) break;
        if (state == cp_layer_renderer_state_paused) {
            cp_layer_renderer_wait_until_running(renderer);
            continue;
        }
        @autoreleasepool {
            cp_frame_t frame = cp_layer_renderer_query_next_frame(renderer);
            if (frame == NULL) continue;
            cp_frame_timing_t timing = cp_frame_predict_timing(frame);
            if (timing == NULL) continue;
            cp_frame_start_update(frame);
            CFTimeInterval now = CACurrentMediaTime();
            xr_scene_step(last == 0 ? 0.0f : (float)(now - last));
            last = now;
            cp_frame_end_update(frame);
            cp_time_wait_until(cp_frame_timing_get_optimal_input_time(timing));

            cp_frame_start_submission(frame);
            cp_drawable_array_t drawables = cp_frame_query_drawables(frame);
            for (size_t k = 0; k < cp_drawable_array_get_count(drawables); k++) {
                cp_drawable_t drawable = cp_drawable_array_get_drawable(drawables, k);
                CFTimeInterval at = cp_time_to_cf_time_interval(
                    cp_frame_timing_get_presentation_time(cp_drawable_get_frame_timing(drawable)));
                ar_device_anchor_t anchor = ar_device_anchor_create();
                simd_float4x4 head = matrix_identity_float4x4;
                if (ar_world_tracking_provider_query_device_anchor_at_timestamp(world, at, anchor)
                        == ar_device_anchor_query_status_success) {
                    head = ar_anchor_get_origin_from_anchor_transform(anchor);
                    // the simulator's head sits at the origin, which is the floor on a device:
                    // lift it to standing height there
                    if (simd_length(head.columns[3].xyz) < 1e-4f) head.columns[3].y = 1.5f;
                    cp_drawable_set_device_anchor(drawable, anchor);
                } else {
                    // no pose yet: a head 1.5 m above the floor
                    head.columns[3] = simd_make_float4(0, 1.5f, 0, 1);
                }
                for (size_t v = 0; v < cp_drawable_get_view_count(drawable); v++) {
                    cp_view_t view = cp_drawable_get_view(drawable, v);
                    size_t ti = cp_view_texture_map_get_texture_index(cp_view_get_view_texture_map(view));
                    id<MTLTexture> color = cp_drawable_get_color_texture(drawable, ti);
                    simd_float4x4 pose = simd_mul(head, cp_view_get_transform(view));
                    simd_float4 tan = cp_view_get_tangents(view);
                    if (xr_scene_render((uint64_t)(__bridge void *)color, (uint32_t)color.width, (uint32_t)color.height,
                                        format, (float *)&pose, tan[0], tan[1], tan[2], tan[3]) != 0) {
                        fprintf(stderr, "xr: render failed\n");
                    }
                }
                xr_scene_wait();
                id<MTLCommandBuffer> cb = [queue commandBuffer];
                cp_drawable_encode_present(drawable, cb);
                [cb commit];
            }
            cp_frame_end_submission(frame);
            if (++frames == 120) fprintf(stderr, "xr: 120 frames\n");
        }
    }
}
