// The visionOS example's app: a small window that opens a full immersive space at launch and closes,
// whose CompositorLayer runs host.m's frame loop on a thread of its own.
import SwiftUI
import CompositorServices

struct XRConfiguration: CompositorLayerConfiguration {
    func makeConfiguration(capabilities: LayerRenderer.Capabilities, configuration: inout LayerRenderer.Configuration) {
        configuration.colorFormat = .bgra8Unorm_srgb
        configuration.depthFormat = .depth32Float
        configuration.layout = .dedicated
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
