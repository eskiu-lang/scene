// The frame loop of the Quest example: an OpenXR session on the Vulkan device the scene
// draws with (XR_KHR_vulkan_enable, the device made by wgpu, whose objects xr_scene_vulkan
// gives), one swapchain per eye, and ../xr_scene.esk drawing each eye into its swapchain
// image through wgpu (wgpuDeviceCreateTextureFromVulkan, eskiu-lang/wgpu-native's eskiu
// branch). A NativeActivity: android_main is the app's thread.
#include <android/log.h>
#include <android_native_app_glue.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <vulkan/vulkan.h>
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_VULKAN
#include <jni.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "eskiu-xr", __VA_ARGS__)
#define CHECK(x) do { XrResult r_ = (x); if (XR_FAILED(r_)) { LOG("%s failed: %d", #x, (int)r_); return false; } } while (0)

int xr_scene_open(void);
void xr_scene_step(float dt);
int xr_scene_render(uint64_t image, uint32_t width, uint32_t height, int format, float *pose,
                    float left, float right, float top, float bottom,
                    uint64_t depth_image, int depth_format, bool reverse);
void xr_scene_wait(void);
int xr_scene_vulkan(void **instance, void **physical_device, void **device, uint32_t *queue_family, uint32_t *queue_index);
void xr_scene_pointer(float ox, float oy, float oz, float dx, float dy, float dz, bool down, bool active);

#define WGPU_RGBA8_SRGB 0x17
#define WGPU_DEPTH32FLOAT 0x30
// xr_scene.esk's XR_NEAR and XR_FAR
#define XR_NEAR 0.1f
#define XR_FAR 50.0f

typedef struct {
    XrSwapchain swapchain;
    uint32_t width, height, count;
    XrSwapchainImageVulkanKHR images[8];
    XrSwapchain depth_swapchain;
    uint32_t depth_count;
    XrSwapchainImageVulkanKHR depth_images[8];
} Eye;

typedef struct {
    XrInstance instance;
    XrSystemId system;
    XrSession session;
    XrSpace space;
    Eye eyes[2];
    XrSessionState state;
    bool running;
    bool depth_layer;
    XrActionSet actions;
    XrAction aim, trigger;
    XrPath hands[2];
    XrSpace aim_spaces[2];
    int hand;
} Xr;

// A column-major world transform from an OpenXR pose.
static void pose_matrix(XrPosef p, float *m) {
    float x = p.orientation.x, y = p.orientation.y, z = p.orientation.z, w = p.orientation.w;
    m[0] = 1 - 2 * (y * y + z * z); m[1] = 2 * (x * y + z * w);     m[2] = 2 * (x * z - y * w);     m[3] = 0;
    m[4] = 2 * (x * y - z * w);     m[5] = 1 - 2 * (x * x + z * z); m[6] = 2 * (y * z + x * w);     m[7] = 0;
    m[8] = 2 * (x * z + y * w);     m[9] = 2 * (y * z - x * w);     m[10] = 1 - 2 * (x * x + y * y); m[11] = 0;
    m[12] = p.position.x; m[13] = p.position.y; m[14] = p.position.z; m[15] = 1;
}

static XrPath path(Xr *xr, const char *s) {
    XrPath p = XR_NULL_PATH;
    xrStringToPath(xr->instance, s, &p);
    return p;
}

// Both hands' aim pose and trigger, bound for the Touch controllers (Quest) and the simple
// controller every runtime has; the hand whose trigger was pulled last is the pointer.
static bool xr_actions(Xr *xr) {
    XrActionSetCreateInfo si = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(si.actionSetName, "scene");
    strcpy(si.localizedActionSetName, "Scene");
    CHECK(xrCreateActionSet(xr->instance, &si, &xr->actions));
    xr->hands[0] = path(xr, "/user/hand/left");
    xr->hands[1] = path(xr, "/user/hand/right");
    XrActionCreateInfo ai = {XR_TYPE_ACTION_CREATE_INFO};
    ai.countSubactionPaths = 2;
    ai.subactionPaths = xr->hands;
    ai.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy(ai.actionName, "aim");
    strcpy(ai.localizedActionName, "Aim");
    CHECK(xrCreateAction(xr->actions, &ai, &xr->aim));
    ai.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy(ai.actionName, "select");
    strcpy(ai.localizedActionName, "Select");
    CHECK(xrCreateAction(xr->actions, &ai, &xr->trigger));
    XrActionSuggestedBinding touch[4] = {
        {xr->aim, path(xr, "/user/hand/left/input/aim/pose")},
        {xr->aim, path(xr, "/user/hand/right/input/aim/pose")},
        {xr->trigger, path(xr, "/user/hand/left/input/trigger/value")},
        {xr->trigger, path(xr, "/user/hand/right/input/trigger/value")},
    };
    XrInteractionProfileSuggestedBinding sb = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    sb.interactionProfile = path(xr, "/interaction_profiles/oculus/touch_controller");
    sb.suggestedBindings = touch;
    sb.countSuggestedBindings = 4;
    xrSuggestInteractionProfileBindings(xr->instance, &sb);
    XrActionSuggestedBinding simple[4] = {
        {xr->aim, path(xr, "/user/hand/left/input/aim/pose")},
        {xr->aim, path(xr, "/user/hand/right/input/aim/pose")},
        {xr->trigger, path(xr, "/user/hand/left/input/select/click")},
        {xr->trigger, path(xr, "/user/hand/right/input/select/click")},
    };
    sb.interactionProfile = path(xr, "/interaction_profiles/khr/simple_controller");
    sb.suggestedBindings = simple;
    xrSuggestInteractionProfileBindings(xr->instance, &sb);
    for (int h = 0; h < 2; h++) {
        XrActionSpaceCreateInfo asi = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        asi.action = xr->aim;
        asi.subactionPath = xr->hands[h];
        asi.poseInActionSpace.orientation.w = 1;
        CHECK(xrCreateActionSpace(xr->session, &asi, &xr->aim_spaces[h]));
    }
    XrSessionActionSetsAttachInfo at = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    at.countActionSets = 1;
    at.actionSets = &xr->actions;
    CHECK(xrAttachSessionActionSets(xr->session, &at));
    xr->hand = 1;
    return true;
}

// Syncs the actions and hands the pointing hand's ray and trigger to the scene.
static void xr_input(Xr *xr, XrTime time) {
    if (xr->actions == XR_NULL_HANDLE) return;
    XrActiveActionSet active = {xr->actions, XR_NULL_PATH};
    XrActionsSyncInfo sync = {XR_TYPE_ACTIONS_SYNC_INFO};
    sync.countActiveActionSets = 1;
    sync.activeActionSets = &active;
    if (XR_FAILED(xrSyncActions(xr->session, &sync))) return;
    bool pressed[2] = {false, false};
    for (int h = 0; h < 2; h++) {
        XrActionStateGetInfo gi = {XR_TYPE_ACTION_STATE_GET_INFO};
        gi.action = xr->trigger;
        gi.subactionPath = xr->hands[h];
        XrActionStateBoolean b = {XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(xr->session, &gi, &b)) && b.isActive) {
            pressed[h] = b.currentState;
            if (b.changedSinceLastSync && b.currentState) xr->hand = h;
        }
    }
    int h = xr->hand;
    XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
    bool tracked = XR_SUCCEEDED(xrLocateSpace(xr->aim_spaces[h], xr->space, time, &loc))
        && (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
        && (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT);
    if (!tracked) {
        xr_scene_pointer(0, 0, 0, 0, 0, -1, false, false);
        return;
    }
    float m[16];
    pose_matrix(loc.pose, m);
    // the aim pose points along its -Z
    xr_scene_pointer(m[12], m[13], m[14], -m[8], -m[9], -m[10], pressed[h], true);
}

static bool xr_start(Xr *xr, struct android_app *app) {
    PFN_xrInitializeLoaderKHR init_loader = NULL;
    xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction *)&init_loader);
    if (init_loader) {
        XrLoaderInitInfoAndroidKHR li = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        li.applicationVM = app->activity->vm;
        li.applicationContext = app->activity->clazz;
        init_loader((XrLoaderInitInfoBaseHeaderKHR *)&li);
    }
    const char *exts[3] = {XR_KHR_VULKAN_ENABLE_EXTENSION_NAME, XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
    uint32_t ext_count = 2;
    // the depth layer lets the runtime reproject by depth, when it has the extension
    XrExtensionProperties props[128];
    uint32_t nprops = 0;
    for (int i = 0; i < 128; i++) props[i] = (XrExtensionProperties){XR_TYPE_EXTENSION_PROPERTIES};
    xrEnumerateInstanceExtensionProperties(NULL, 128, &nprops, props);
    for (uint32_t i = 0; i < nprops; i++) {
        if (strcmp(props[i].extensionName, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME) == 0) {
            exts[ext_count++] = XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME;
            xr->depth_layer = true;
        }
    }
    XrInstanceCreateInfoAndroidKHR ai = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    ai.applicationVM = app->activity->vm;
    ai.applicationActivity = app->activity->clazz;
    XrInstanceCreateInfo ci = {XR_TYPE_INSTANCE_CREATE_INFO};
    ci.next = &ai;
    strcpy(ci.applicationInfo.applicationName, "Eskiu scene");
    ci.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
    ci.enabledExtensionCount = ext_count;
    ci.enabledExtensionNames = exts;
    CHECK(xrCreateInstance(&ci, &xr->instance));
    XrSystemGetInfo si = {XR_TYPE_SYSTEM_GET_INFO};
    si.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    CHECK(xrGetSystem(xr->instance, &si, &xr->system));

    // XR_KHR_vulkan_enable wants the requirements queried before the session
    PFN_xrGetVulkanGraphicsRequirementsKHR reqs_fn = NULL;
    xrGetInstanceProcAddr(xr->instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&reqs_fn);
    XrGraphicsRequirementsVulkanKHR reqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
    CHECK(reqs_fn(xr->instance, xr->system, &reqs));
    // the device extensions the runtime needs; wgpu made the device, so a missing one shows
    // here first when the session fails
    PFN_xrGetVulkanDeviceExtensionsKHR exts_fn = NULL;
    xrGetInstanceProcAddr(xr->instance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction *)&exts_fn);
    char dev_exts[2048];
    uint32_t dev_len = 0;
    if (exts_fn && XR_SUCCEEDED(exts_fn(xr->instance, xr->system, sizeof dev_exts, &dev_len, dev_exts))) {
        LOG("the runtime asks for Vulkan device extensions: %s", dev_exts);
    }

    if (xr_scene_open() != 0) {
        LOG("the scene did not open");
        return false;
    }
    void *vk_instance, *vk_physical, *vk_device;
    uint32_t family, index;
    if (xr_scene_vulkan(&vk_instance, &vk_physical, &vk_device, &family, &index) != 0) {
        LOG("the GPU is not on Vulkan");
        return false;
    }
    PFN_xrGetVulkanGraphicsDeviceKHR dev_fn = NULL;
    xrGetInstanceProcAddr(xr->instance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction *)&dev_fn);
    VkPhysicalDevice want = VK_NULL_HANDLE;
    CHECK(dev_fn(xr->instance, xr->system, (VkInstance)vk_instance, &want));
    if (want != (VkPhysicalDevice)vk_physical) LOG("OpenXR asks for another physical device than wgpu's");

    XrGraphicsBindingVulkanKHR binding = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
    binding.instance = (VkInstance)vk_instance;
    binding.physicalDevice = (VkPhysicalDevice)vk_physical;
    binding.device = (VkDevice)vk_device;
    binding.queueFamilyIndex = family;
    binding.queueIndex = index;
    XrSessionCreateInfo sci = {XR_TYPE_SESSION_CREATE_INFO};
    sci.next = &binding;
    sci.systemId = xr->system;
    CHECK(xrCreateSession(xr->instance, &sci, &xr->session));

    XrReferenceSpaceCreateInfo rs = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    rs.poseInReferenceSpace.orientation.w = 1;
    if (XR_FAILED(xrCreateReferenceSpace(xr->session, &rs, &xr->space))) {
        rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        CHECK(xrCreateReferenceSpace(xr->session, &rs, &xr->space));
    }

    uint32_t n = 0;
    XrViewConfigurationView views[2] = {{XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW}};
    CHECK(xrEnumerateViewConfigurationViews(xr->instance, xr->system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &n, views));
    for (int e = 0; e < 2; e++) {
        Eye *eye = &xr->eyes[e];
        XrSwapchainCreateInfo sc = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        sc.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        sc.format = VK_FORMAT_R8G8B8A8_SRGB;
        sc.sampleCount = 1;
        sc.width = eye->width = views[e].recommendedImageRectWidth;
        sc.height = eye->height = views[e].recommendedImageRectHeight;
        sc.faceCount = 1;
        sc.arraySize = 1;
        sc.mipCount = 1;
        CHECK(xrCreateSwapchain(xr->session, &sc, &eye->swapchain));
        for (int i = 0; i < 8; i++) eye->images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
        CHECK(xrEnumerateSwapchainImages(eye->swapchain, 8, &eye->count, (XrSwapchainImageBaseHeader *)eye->images));
        if (xr->depth_layer) {
            sc.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            sc.format = VK_FORMAT_D32_SFLOAT;
            if (XR_FAILED(xrCreateSwapchain(xr->session, &sc, &eye->depth_swapchain))) {
                xr->depth_layer = false;
                continue;
            }
            for (int i = 0; i < 8; i++) eye->depth_images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
            CHECK(xrEnumerateSwapchainImages(eye->depth_swapchain, 8, &eye->depth_count, (XrSwapchainImageBaseHeader *)eye->depth_images));
        }
    }
    if (!xr_actions(xr)) LOG("no controller actions");
    LOG("session ready: %ux%u per eye", xr->eyes[0].width, xr->eyes[0].height);
    return true;
}

static void xr_events(Xr *xr) {
    XrEventDataBuffer ev = {XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(xr->instance, &ev) == XR_SUCCESS) {
        if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            XrEventDataSessionStateChanged *s = (XrEventDataSessionStateChanged *)&ev;
            xr->state = s->state;
            if (s->state == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo bi = {XR_TYPE_SESSION_BEGIN_INFO};
                bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                xr->running = XR_SUCCEEDED(xrBeginSession(xr->session, &bi));
            } else if (s->state == XR_SESSION_STATE_STOPPING) {
                xrEndSession(xr->session);
                xr->running = false;
            }
        }
        ev.type = XR_TYPE_EVENT_DATA_BUFFER;
    }
}

static void xr_frame(Xr *xr, float dt) {
    XrFrameState fs = {XR_TYPE_FRAME_STATE};
    if (XR_FAILED(xrWaitFrame(xr->session, NULL, &fs))) return;
    xrBeginFrame(xr->session, NULL);
    xr_input(xr, fs.predictedDisplayTime);
    xr_scene_step(dt);
    XrCompositionLayerProjectionView pv[2];
    XrCompositionLayerDepthInfoKHR depth_info[2];
    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    const XrCompositionLayerBaseHeader *layers[1] = {(XrCompositionLayerBaseHeader *)&layer};
    uint32_t layer_count = 0;
    if (fs.shouldRender) {
        XrView views[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};
        XrViewState vs = {XR_TYPE_VIEW_STATE};
        XrViewLocateInfo li = {XR_TYPE_VIEW_LOCATE_INFO};
        li.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        li.displayTime = fs.predictedDisplayTime;
        li.space = xr->space;
        uint32_t n = 0;
        xrLocateViews(xr->session, &li, &vs, 2, &n, views);
        for (int e = 0; e < 2; e++) {
            Eye *eye = &xr->eyes[e];
            uint32_t i = 0;
            xrAcquireSwapchainImage(eye->swapchain, NULL, &i);
            XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            wi.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(eye->swapchain, &wi);
            uint64_t depth_image = 0;
            if (xr->depth_layer) {
                uint32_t di = 0;
                xrAcquireSwapchainImage(eye->depth_swapchain, NULL, &di);
                xrWaitSwapchainImage(eye->depth_swapchain, &wi);
                depth_image = (uint64_t)eye->depth_images[di].image;
            }
            float m[16];
            pose_matrix(views[e].pose, m);
            XrFovf f = views[e].fov;
            xr_scene_render((uint64_t)eye->images[i].image, eye->width, eye->height, WGPU_RGBA8_SRGB, m,
                            tanf(f.angleLeft), tanf(f.angleRight), tanf(f.angleUp), tanf(f.angleDown),
                            depth_image, WGPU_DEPTH32FLOAT, false);
            xr_scene_wait();
            xrReleaseSwapchainImage(eye->swapchain, NULL);
            if (xr->depth_layer) xrReleaseSwapchainImage(eye->depth_swapchain, NULL);
            pv[e] = (XrCompositionLayerProjectionView){XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            pv[e].pose = views[e].pose;
            pv[e].fov = views[e].fov;
            pv[e].subImage.swapchain = eye->swapchain;
            pv[e].subImage.imageRect.extent.width = (int32_t)eye->width;
            pv[e].subImage.imageRect.extent.height = (int32_t)eye->height;
            if (xr->depth_layer) {
                depth_info[e] = (XrCompositionLayerDepthInfoKHR){XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
                depth_info[e].subImage.swapchain = eye->depth_swapchain;
                depth_info[e].subImage.imageRect = pv[e].subImage.imageRect;
                depth_info[e].minDepth = 0;
                depth_info[e].maxDepth = 1;
                depth_info[e].nearZ = XR_NEAR;
                depth_info[e].farZ = XR_FAR;
                pv[e].next = &depth_info[e];
            }
        }
        layer.space = xr->space;
        layer.viewCount = 2;
        layer.views = pv;
        layer_count = 1;
    }
    XrFrameEndInfo ei = {XR_TYPE_FRAME_END_INFO};
    ei.displayTime = fs.predictedDisplayTime;
    ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    ei.layerCount = layer_count;
    ei.layers = layers;
    xrEndFrame(xr->session, &ei);
}

static double now_seconds(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

void android_main(struct android_app *app) {
    Xr xr;
    memset(&xr, 0, sizeof xr);
    if (!xr_start(&xr, app)) {
        ANativeActivity_finish(app->activity);
    }
    double last = now_seconds();
    while (!app->destroyRequested) {
        int events;
        struct android_poll_source *source;
        while (ALooper_pollOnce(xr.running ? 0 : 100, NULL, &events, (void **)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) break;
        }
        if (xr.session == XR_NULL_HANDLE) continue;
        xr_events(&xr);
        if (!xr.running) continue;
        double t = now_seconds();
        xr_frame(&xr, (float)(t - last));
        last = t;
    }
    if (xr.session) xrDestroySession(xr.session);
    if (xr.instance) xrDestroyInstance(xr.instance);
}
