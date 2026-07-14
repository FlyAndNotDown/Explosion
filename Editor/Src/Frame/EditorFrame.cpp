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
    {
    }

    EditorFrame::~EditorFrame() = default;

    void EditorFrame::Render(EditorContext& inContext, Runtime::Canvas& inSceneRenderCanvas, bool& outRequestQuit)
    {
        RenderMenuBar(inContext, outRequestQuit);
        RenderSceneTab(inContext, inSceneRenderCanvas);
        RenderOutlinerTab(inContext);
        RenderInspectorTab(inContext);
        RenderLogTab();
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
        ImGui::EndMainMenuBar();
    }

    void EditorFrame::RenderSceneTab(EditorContext& inContext, Runtime::Canvas& inSceneRenderCanvas)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImVec2 sceneSize = ImGui::GetContentRegionAvail();
        sceneSize.x = std::max(sceneSize.x, 1.0f);
        sceneSize.y = std::max(sceneSize.y, 1.0f);

        const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
        auto& sceneClient = inContext.GetSceneClient();
        sceneClient.ResizeRenderSurface(
            std::max(1u, static_cast<uint32_t>(sceneSize.x * framebufferScale.x)),
            std::max(1u, static_cast<uint32_t>(sceneSize.y * framebufferScale.y)));
        ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(inSceneRenderCanvas.GetTextureView())), sceneSize);
        sceneClient.SetSceneHovered(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem));
        sceneClient.SetSceneFocused(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorFrame::RenderOutlinerTab(EditorContext& inContext)
    {
        ImGui::Begin("Outliner");
        ImGui::InputText("New Entity", &createEntityName);
        ImGui::SameLine();
        if (ImGui::Button("Create")) {
            const auto entity = inContext.CreateEntity(createEntityName);
            inContext.SetSelectedEntity(entity);
        }
        ImGui::Separator();

        auto& registry = inContext.GetSceneClient().GetWorld().GetRegistry();
        registry.Each([&](Runtime::Entity entity) -> void {
            if (registry.HasTag<Runtime::TransientTag>(entity)) {
                return;
            }
            const std::string label = Internal::EntityDisplayName(registry, entity);
            const bool selected = inContext.GetSelectedEntity() == entity;
            ImGui::PushID(static_cast<int>(entity));
            if (ImGui::Selectable(label.c_str(), selected)) {
                inContext.SetSelectedEntity(entity);
            }
            ImGui::PopID();
        });
        ImGui::End();
    }

    void EditorFrame::RenderInspectorTab(EditorContext& inContext)
    {
        ImGui::Begin("Inspector");
        const Runtime::Entity selectedEntity = inContext.GetSelectedEntity();
        auto& registry = inContext.GetSceneClient().GetWorld().GetRegistry();
        if (selectedEntity == Runtime::entityNull || !registry.Valid(selectedEntity)) {
            ImGui::TextDisabled("No entity selected");
            ImGui::End();
            return;
        }

        std::string entityName = Internal::EntityDisplayName(registry, selectedEntity);
        if (InputWidget<std::string>::Render("Name", entityName)) {
            inContext.RenameEntity(selectedEntity, entityName);
        }
        if (ImGui::Button("Destroy Entity")) {
            inContext.DestroyEntity(selectedEntity);
            ImGui::End();
            return;
        }
        ImGui::Separator();

        std::vector<const Mirror::Class*> addableComponents;
        for (const auto* clazz : Mirror::Class::GetAll()) {
            const bool isTag = clazz->HasMeta(Runtime::MetaPresets::tag);
            const bool hasType = isTag ? registry.HasTagDyn(clazz, selectedEntity) : registry.HasDyn(clazz, selectedEntity);
            if (clazz->HasMeta("comp") && !hasType && (isTag || clazz->HasDefaultConstructor())) {
                addableComponents.emplace_back(clazz);
            }
        }
        std::ranges::sort(addableComponents, [](const Mirror::Class* lhs, const Mirror::Class* rhs) -> bool {
            return lhs->GetName() < rhs->GetName();
        });

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
                if (selectedClass->HasMeta(Runtime::MetaPresets::tag)) {
                    registry.AddTagDyn(selectedClass, selectedEntity);
                } else {
                    registry.EmplaceDyn(selectedClass, selectedEntity, {});
                }
                inContext.NotifyComponentsChanged(selectedEntity);
            }
        }

        Runtime::CompClass componentToRemove = nullptr;
        bool tagToRemove = false;
        registry.CompEach(selectedEntity, [&](Runtime::CompClass compClass) -> void {
            if (compClass->HasMeta("transient")) {
                return;
            }
            ImGui::PushID(compClass->GetName().c_str());
            const std::string componentName = Internal::ComponentDisplayName(*compClass);
            const bool open = ImGui::CollapsingHeader(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (ImGui::BeginPopupContextItem("ComponentMenu")) {
                if (ImGui::MenuItem("Remove")) {
                    componentToRemove = compClass;
                    tagToRemove = false;
                }
                ImGui::EndPopup();
            }
            if (open && componentToRemove != compClass) {
                const Mirror::Any compRef = registry.GetDyn(compClass, selectedEntity);
                bool componentEdited = false;
                for (const auto& memberVariable : compClass->GetMemberVariables() | std::views::values) {
                    if (memberVariable.IsTransient()) {
                        continue;
                    }
                    Mirror::Any memberRef = memberVariable.GetDyn(compRef);
                    componentEdited |= RenderInputWidget(memberVariable.GetName(), memberRef);
                }
                if (componentEdited) {
                    inContext.NotifyComponentEdited(selectedEntity, compClass);
                }
            }
            ImGui::PopID();
        });
        registry.TagEach(selectedEntity, [&](Runtime::TagClass tagClass) -> void {
            if (tagClass->HasMeta("transient")) {
                return;
            }
            ImGui::PushID(tagClass->GetName().c_str());
            const std::string tagName = Internal::ComponentDisplayName(*tagClass);
            ImGui::TextUnformatted(tagName.c_str());
            if (ImGui::BeginPopupContextItem("ComponentMenu")) {
                if (ImGui::MenuItem("Remove")) {
                    componentToRemove = tagClass;
                    tagToRemove = true;
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        });
        if (componentToRemove != nullptr) {
            if (tagToRemove) {
                registry.RemoveTagDyn(componentToRemove, selectedEntity);
            } else {
                registry.RemoveDyn(componentToRemove, selectedEntity);
            }
            inContext.NotifyComponentsChanged(selectedEntity);
        }
        ImGui::End();
    }

    void EditorFrame::RenderLogTab()
    {
        ImGui::Begin("Log");
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
