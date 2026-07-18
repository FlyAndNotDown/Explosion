//
// Created by johnk on 2026/7/7.
//

#include <algorithm>
#include <cstdint>
#include <format>
#include <ranges>
#include <vector>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <Editor/EditorLog.h>
#include <Editor/Frame/EditorFrame.h>
#include <Editor/System/Camera.h>
#include <Editor/Widget/InputWidgets.h>
#include <Mirror/Mirror.h>
#include <Runtime/Component/Name.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/ECS.h>

namespace Editor::Internal {
    static std::string ComponentDisplayName(const Mirror::Class& inClass)
    {
        const std::string& qualifiedName = inClass.GetName();
        const size_t namespaceSeparator = qualifiedName.rfind("::");
        return namespaceSeparator == std::string::npos
            ? qualifiedName
            : qualifiedName.substr(namespaceSeparator + 2);
    }

    static std::string EntityDisplayName(const Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity)
    {
        const auto* name = inRegistry.Find<Runtime::Name>(inEntity);
        return name != nullptr && !name->value.empty()
            ? name->value
            : std::format("Entity {}", inEntity);
    }

    static std::string LevelString(Core::LogLevel inLevel)
    {
        switch (inLevel) {
        case Core::LogLevel::verbose:
            return "verbose";
        case Core::LogLevel::info:
            return "info";
        case Core::LogLevel::warning:
            return "warning";
        case Core::LogLevel::error:
            return "error";
        default:
            return "unknown";
        }
    }
}

namespace Editor {
    EditorFrame::EditorFrame()
        : createEntityName("Entity")
        , selectedAddComponentIndex(0)
        , componentClasses(Mirror::Class::GetAll())
        , tabVisibility {
            .scene = true,
            .outliner = true,
            .inspector = true,
            .log = true
        }
    {
        std::erase_if(componentClasses, [](Runtime::CompClass clazz) -> bool { return !clazz->HasMeta("comp"); });
        std::ranges::sort(componentClasses, [](Runtime::CompClass lhs, Runtime::CompClass rhs) -> bool {
            return lhs->GetName() < rhs->GetName();
        });
    }

    EditorFrame::~EditorFrame() = default;

    void EditorFrame::Render(EditorContext& inContext, Runtime::ECRegistry& inRegistry, Runtime::Canvas& inSceneRenderCanvas, bool& outRequestQuit)
    {
        RenderMenuBar(inContext, outRequestQuit);
        if (tabVisibility.scene) {
            RenderSceneTab(inContext, inSceneRenderCanvas, tabVisibility.scene);
        } else {
            inContext.GetSceneClient().SetSceneHovered(false);
            inContext.GetSceneClient().SetSceneFocused(false);
        }
        if (tabVisibility.outliner) {
            RenderOutlinerTab(inContext, inRegistry, tabVisibility.outliner);
        }
        if (tabVisibility.inspector) {
            RenderInspectorTab(inContext, inRegistry, tabVisibility.inspector);
        }
        if (tabVisibility.log) {
            RenderLogTab(tabVisibility.log);
        }
    }

    void EditorFrame::RenderMenuBar(EditorContext& inContext, bool& outRequestQuit)
    {
        if (!ImGui::BeginMainMenuBar()) {
            return;
        }
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Level", "Ctrl+S")) {
                inContext.GetSceneClient().SaveLevel();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                outRequestQuit = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Scene", nullptr, &tabVisibility.scene);
            ImGui::MenuItem("Outliner", nullptr, &tabVisibility.outliner);
            ImGui::MenuItem("Inspector", nullptr, &tabVisibility.inspector);
            ImGui::MenuItem("Log", nullptr, &tabVisibility.log);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    void EditorFrame::RenderSceneTab(EditorContext& inContext, Runtime::Canvas& inSceneRenderCanvas, bool& inOutOpen)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        auto& sceneClient = inContext.GetSceneClient();
        if (ImGui::Begin("Scene", &inOutOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImVec2 sceneSize = ImGui::GetContentRegionAvail();
            sceneSize.x = std::max(sceneSize.x, 1.0f);
            sceneSize.y = std::max(sceneSize.y, 1.0f);

            const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
            sceneClient.ResizeRenderSurface(
                std::max(1u, static_cast<uint32_t>(sceneSize.x * framebufferScale.x)),
                std::max(1u, static_cast<uint32_t>(sceneSize.y * framebufferScale.y)));
            ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(inSceneRenderCanvas.GetTextureView())), sceneSize);
            sceneClient.SetSceneHovered(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem));
            sceneClient.SetSceneFocused(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
        } else {
            sceneClient.SetSceneHovered(false);
            sceneClient.SetSceneFocused(false);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorFrame::RenderOutlinerTab(EditorContext& inContext, Runtime::ECRegistry& inRegistry, bool& inOutOpen)
    {
        if (!ImGui::Begin("Outliner", &inOutOpen)) {
            ImGui::End();
            return;
        }
        ImGui::InputText("New Entity", &createEntityName);
        ImGui::SameLine();
        if (ImGui::Button("Create")) {
            const auto entity = inContext.CreateEntity(inRegistry, createEntityName);
            inContext.SetSelectedEntity(entity);
        }
        ImGui::Separator();

        inRegistry.Each([&](Runtime::Entity entity) -> void {
            if (inRegistry.Has<EditorCameraController>(entity)) {
                return;
            }
            const std::string label = Internal::EntityDisplayName(inRegistry, entity);
            const bool selected = inContext.GetSelectedEntity() == entity;
            ImGui::PushID(static_cast<int>(entity));
            if (ImGui::Selectable(label.c_str(), selected)) {
                inContext.SetSelectedEntity(entity);
            }
            ImGui::PopID();
        });
        ImGui::End();
    }

    void EditorFrame::RenderInspectorTab(EditorContext& inContext, Runtime::ECRegistry& inRegistry, bool& inOutOpen)
    {
        if (!ImGui::Begin("Inspector", &inOutOpen)) {
            ImGui::End();
            return;
        }
        const Runtime::Entity selectedEntity = inContext.GetSelectedEntity();
        if (selectedEntity == Runtime::entityNull || !inRegistry.Valid(selectedEntity)) {
            ImGui::TextDisabled("No entity selected");
            ImGui::End();
            return;
        }

        if (ImGui::Button("Destroy Entity")) {
            inContext.DestroyEntity(inRegistry, selectedEntity);
            ImGui::End();
            return;
        }
        ImGui::Separator();

        std::vector<const Mirror::Class*> addableComponents;
        for (const auto* clazz : componentClasses) {
            if (inContext.CanAddComponent(inRegistry, selectedEntity, clazz)) {
                addableComponents.emplace_back(clazz);
            }
        }

        if (!addableComponents.empty()) {
            selectedAddComponentIndex = std::clamp(selectedAddComponentIndex, 0, static_cast<int>(addableComponents.size() - 1));
            const std::string selectedComponentName = Internal::ComponentDisplayName(*addableComponents[selectedAddComponentIndex]);
            if (ImGui::BeginCombo("Add Component", selectedComponentName.c_str())) {
                for (int i = 0; i < static_cast<int>(addableComponents.size()); i++) {
                    const bool selected = i == selectedAddComponentIndex;
                    const std::string componentName = Internal::ComponentDisplayName(*addableComponents[i]);
                    ImGui::PushID(addableComponents[i]->GetName().c_str());
                    if (ImGui::Selectable(componentName.c_str(), selected)) {
                        selectedAddComponentIndex = i;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Add")) {
                const auto* selectedClass = addableComponents[selectedAddComponentIndex];
                inContext.AddComponent(inRegistry, selectedEntity, selectedClass);
            }
        }

        Runtime::CompClass componentToRemove = nullptr;
        inRegistry.CompEach(selectedEntity, [&](Runtime::CompClass compClass) -> void {
            ImGui::PushID(compClass->GetName().c_str());
            const std::string componentName = Internal::ComponentDisplayName(*compClass);
            const bool open = ImGui::CollapsingHeader(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (inContext.CanRemoveComponent(inRegistry, selectedEntity, compClass) && ImGui::BeginPopupContextItem("ComponentMenu")) {
                if (ImGui::MenuItem("Remove")) {
                    componentToRemove = compClass;
                }
                ImGui::EndPopup();
            }
            if (open && componentToRemove != compClass) {
                const Mirror::Any compRef = inRegistry.GetDyn(compClass, selectedEntity);
                for (const auto& memberVariable : compClass->GetMemberVariables() | std::views::values) {
                    const Mirror::Any memberRef = memberVariable.GetDyn(compRef);
                    Mirror::Any memberValue = memberRef.Value();
                    if (RenderInputWidget(memberVariable.GetName(), memberValue)) {
                        inContext.SetComponentMember(inRegistry, selectedEntity, compClass, memberVariable, memberValue);
                    }
                }
            }
            ImGui::PopID();
        });
        inRegistry.TagEach(selectedEntity, [&](Runtime::TagClass tagClass) -> void {
            ImGui::PushID(tagClass->GetName().c_str());
            const std::string tagName = Internal::ComponentDisplayName(*tagClass);
            ImGui::TextUnformatted(tagName.c_str());
            if (inContext.CanRemoveComponent(inRegistry, selectedEntity, tagClass) && ImGui::BeginPopupContextItem("ComponentMenu")) {
                if (ImGui::MenuItem("Remove")) {
                    componentToRemove = tagClass;
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        });
        if (componentToRemove != nullptr) {
            inContext.RemoveComponent(inRegistry, selectedEntity, componentToRemove);
        }
        ImGui::End();
    }

    void EditorFrame::RenderLogTab(bool& inOutOpen)
    {
        if (!ImGui::Begin("Log", &inOutOpen)) {
            ImGui::End();
            return;
        }
        const auto entries = EditorLogStream::Get().Snapshot();
        if (ImGui::Button("Copy")) {
            std::string text;
            for (const auto& entry : entries) {
                text += std::format("[{}][{}][{}] {}\n", entry.time, Internal::LevelString(entry.level), entry.tag, entry.content);
            }
            ImGui::SetClipboardText(text.c_str());
        }
        ImGui::Separator();
        ImGui::BeginChild("LogEntries", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& entry : entries) {
            ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            if (entry.level == Core::LogLevel::warning) {
                color = ImVec4(1.0f, 0.78f, 0.25f, 1.0f);
            } else if (entry.level == Core::LogLevel::error) {
                color = ImVec4(1.0f, 0.35f, 0.32f, 1.0f);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(std::format("[{}][{}][{}] {}", entry.time, Internal::LevelString(entry.level), entry.tag, entry.content).c_str());
            ImGui::PopStyleColor();
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
