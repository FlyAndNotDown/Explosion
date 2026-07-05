//
// Created by johnk on 2026/7/5.
//

#include <format>

#include <Editor/Qt/JsonSerialization.h>
#include <Editor/Widget/OutlinerPanel.h>
#include <Editor/Widget/moc_OutlinerPanel.cpp> // NOLINT
#include <Runtime/Component/Name.h>

namespace Editor {
    OutlinerBackend::OutlinerBackend(EditorContext& inContext, QObject* inParent)
        : QObject(inParent)
        , context(inContext)
    {
        connect(&context, &EditorContext::WorldStructureChanged, this, [this]() -> void { emit EntitiesChanged(); });
        connect(&context, &EditorContext::SelectionChanged, this, [this](Runtime::Entity) -> void { emit SelectionChanged(); });
    }

    QJsonValue OutlinerBackend::GetEntities() const
    {
        const auto& registry = context.GetClient().GetWorld().GetRegistry();

        std::vector<OutlinerEntityInfo> entities;
        registry.Each([&](Runtime::Entity entity) -> void {
            if (registry.Has<Runtime::TransientTag>(entity)) {
                return;
            }
            OutlinerEntityInfo info;
            info.id = entity;
            const auto* name = registry.Find<Runtime::Name>(entity);
            info.name = name != nullptr && !name->value.empty() ? name->value : std::format("Entity {}", entity);
            entities.emplace_back(std::move(info));
        });

        QJsonValue result;
        QtJsonSerialize(result, entities);
        return result;
    }

    uint OutlinerBackend::GetSelectedEntity() const
    {
        return context.GetSelectedEntity();
    }

    void OutlinerBackend::SelectEntity(uint inEntity)
    {
        context.SetSelectedEntity(inEntity);
    }

    void OutlinerBackend::CreateEntity(const QString& inName)
    {
        const auto entity = context.CreateEntity(inName.isEmpty() ? "Entity" : inName.toStdString());
        context.SetSelectedEntity(entity);
    }

    void OutlinerBackend::DestroyEntity(uint inEntity)
    {
        context.DestroyEntity(inEntity);
    }

    void OutlinerBackend::RenameEntity(uint inEntity, const QString& inName)
    {
        context.RenameEntity(inEntity, inName.toStdString());
    }

    OutlinerPanel::OutlinerPanel(EditorContext& inContext, QWidget* inParent)
        : WebWidget(inParent)
    {
        Load("/editor/outliner");
        backend = new OutlinerBackend(inContext, this);
        GetWebChannel()->registerObject("backend", backend);
    }
}
