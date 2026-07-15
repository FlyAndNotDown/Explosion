#include <Editor/System/Camera.h>
#include <Editor/SystemGraphPresets.h>
#include <Runtime/System/Render.h>
#include <Runtime/System/Scene.h>
#include <Runtime/System/Transform.h>

namespace Editor {
    const Runtime::SystemGraph& EditorSystemGraphPresets::DefaultEditorWorld()
    {
        static Runtime::SystemGraph graph = []() -> Runtime::SystemGraph {
            Runtime::SystemGraph systemGraph;

            auto& interactionGroup = systemGraph.AddGroup("EditorInteraction", Runtime::SystemExecuteStrategy::sequential);
            interactionGroup.EmplaceSystem<EditorCameraSystem>();

            auto& transformGroup = systemGraph.AddGroup("Transform", Runtime::SystemExecuteStrategy::sequential);
            transformGroup.EmplaceSystem<Runtime::TransformSystem>();

            auto& sceneRenderingGroup = systemGraph.AddGroup("SceneRendering", Runtime::SystemExecuteStrategy::sequential);
            sceneRenderingGroup.EmplaceSystem<Runtime::SceneSystem>();
            sceneRenderingGroup.EmplaceSystem<Runtime::RenderSystem>();

            return systemGraph;
        }();
        return graph;
    }
}
