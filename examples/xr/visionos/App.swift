// The visionOS example's app: a small window that opens a full immersive space at launch and closes,
// whose CompositorLayer runs host.m's frame loop on a thread of its own.
import SwiftUI
import CompositorServices

struct XRConfiguration: CompositorLayerConfiguration {
    func makeConfiguration(capabilities: LayerRenderer.Capabilities, configuration: inout LayerRenderer.Configuration) {
        configuration.colorFormat = .bgra8Unorm_srgb
        configuration.depthFormat = .depth32Float
        configuration.layout = .dedicated
        // the views are drawn at an even resolution, without the drawable's rasterization rate
        // maps; with foveation the compositor would read them as foveated and warp them
        configuration.isFoveationEnabled = false
    }
}

@main
struct XRApp: App {
    var body: some SwiftUI.Scene {
        WindowGroup(id: "launcher") {
            Launcher()
        }
        ImmersiveSpace(id: "scene") {
            CompositorLayer(configuration: XRConfiguration()) { renderer in
                // a pinch is a click along its selection ray, down while held. The ray stays where
                // the pinch began (where the eyes were); the hand's motion since then moves the
                // point it aims at, scaled up with distance, so a pinch can drag
                var starts: [SpatialEventCollection.Event.ID: (ray: Ray3D, hand: Point3D?)] = [:]
                renderer.onSpatialEvent = { events in
                    for event in events {
                        guard let ray = event.selectionRay else { continue }
                        let held = event.phase == .active
                        let hand = event.inputDevicePose?.pose3D.position
                        if starts[event.id] == nil { starts[event.id] = (ray, hand) }
                        let start = starts[event.id]!
                        var o = start.ray.origin
                        var d = start.ray.direction
                        if let h = hand, let h0 = start.hand {
                            let reach = 2.0
                            let gain = 4.0
                            let target = Point3D(x: o.x + d.x * reach + (h.x - h0.x) * gain,
                                                 y: o.y + d.y * reach + (h.y - h0.y) * gain,
                                                 z: o.z + d.z * reach + (h.z - h0.z) * gain)
                            let v = Vector3D(x: target.x - o.x, y: target.y - o.y, z: target.z - o.z)
                            d = v.normalized
                        } else {
                            o = ray.origin
                            d = ray.direction
                        }
                        eskiu_xr_pointer(Float(o.x), Float(o.y), Float(o.z), Float(d.x), Float(d.y), Float(d.z), held, true)
                        if !held {
                            eskiu_xr_pointer(Float(o.x), Float(o.y), Float(o.z), Float(d.x), Float(d.y), Float(d.z), false, false)
                            starts[event.id] = nil
                        }
                    }
                }
                let thread = Thread { eskiu_xr_run(renderer) }
                thread.name = "eskiu render"
                thread.qualityOfService = .userInteractive
                thread.start()
            }
        }
        .immersionStyle(selection: .constant(.full), in: .full)
    }
}

struct Launcher: View {
    @Environment(\.openImmersiveSpace) private var openImmersiveSpace
    @Environment(\.dismissWindow) private var dismissWindow

    var body: some View {
        Text("Eskiu scene")
            .padding()
            .task {
                if case .opened = await openImmersiveSpace(id: "scene") {
                    dismissWindow(id: "launcher")
                }
            }
    }
}
