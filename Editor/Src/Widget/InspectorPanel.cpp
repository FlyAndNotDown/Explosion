//
// Created by johnk on 2026/7/5.
//

#include <QJsonArray>
#include <QJsonObject>

#include <Editor/Qt/JsonSerialization.h>
#include <Editor/Widget/InspectorPanel.h>
#include <Editor/Widget/moc_InspectorPanel.cpp> // NOLINT

namespace Editor::Internal {
    // the dynamic (de)serializers of QtJsonSerializer only depend on the runtime Mirror::Class, any meta class works
    // as the template instantiation carrier
    using DynJsonSerializer = QtJsonSerializer<Runtime::TransientTag>;

    static const Mirror::Class* FindCompClass(const QString& inClassName)
    {
        return Mirror::Class::Find(Mirror::Id(inClassName.toStdString()));
    }
}

namespace Editor {
    InspectorBackend::InspectorBackend(EditorContext& inContext, QObject* inParent)
        : QObject(inParent)
        , context(inContext)
    {
        connect(&context, &EditorContext::SelectionChanged, this, [this](Runtime::Entity) -> void { emit SelectedEntityDataChanged(); });
        connect(&context, &EditorContext::ComponentsChanged, this, [this](Runtime::Entity inEntity) -> void {
            if (inEntity == context.GetSelectedEntity()) {
                emit SelectedEntityDataChanged();
            }
        });
        connect(&context, &EditorContext::WorldStructureChanged, this, [this]() -> void { emit SelectedEntityDataChanged(); });
    }

    QJsonValue InspectorBackend::GetSelectedEntityData() const
    {
        const auto entity = context.GetSelectedEntity();
        auto& registry = context.GetClient().GetWorld().GetRegistry();
        if (entity == Runtime::entityNull || !registry.Valid(entity)) {
            return QJsonValue::Null;
        }

        // the component payloads are inherently dynamic, only the envelope is built by hand and every component
        // content goes through the reflection-driven serializer
        QJsonArray comps;
        registry.CompEach(entity, [&](Runtime::CompClass compClass) -> void {
            const Mirror::Any compRef = registry.GetDyn(compClass, entity);

            QJsonObject content;
            Internal::DynJsonSerializer::QtJsonSerializeDyn(content, *compClass, compRef);

            QJsonObject comp;
            comp["className"] = QString::fromStdString(compClass->GetName());
            comp["content"] = content;
            comps.append(comp);
        });

        QJsonObject result;
        result["entity"] = static_cast<qint64>(entity);
        result["comps"] = comps;
        return result;
    }

    QJsonValue InspectorBackend::GetAddableComponents() const
    {
        std::vector<std::string> classNames;
        for (const auto* clazz : Mirror::Class::GetAll()) {
            if (clazz->HasMeta("comp")) {
                classNames.emplace_back(clazz->GetName());
            }
        }
        std::ranges::sort(classNames);

        QJsonValue result;
        QtJsonSerialize(result, classNames);
        return result;
    }

    void InspectorBackend::UpdateComponent(uint inEntity, const QString& inClassName, const QJsonValue& inContent)
    {
        auto& registry = context.GetClient().GetWorld().GetRegistry();
        const auto* compClass = Internal::FindCompClass(inClassName);
        if (compClass == nullptr || !registry.Valid(inEntity) || !registry.HasDyn(compClass, inEntity)) {
            return;
        }

        registry.UpdateDyn(compClass, inEntity, [&](const Mirror::Any& compRef) -> void {
            Internal::DynJsonSerializer::QtJsonDeserializeDyn(inContent.toObject(), *compClass, compRef);
        });
        context.NotifyComponentsChanged(inEntity);
    }

    void InspectorBackend::AddComponent(uint inEntity, const QString& inClassName)
    {
        auto& registry = context.GetClient().GetWorld().GetRegistry();
        const auto* compClass = Internal::FindCompClass(inClassName);
        if (compClass == nullptr || !registry.Valid(inEntity) || registry.HasDyn(compClass, inEntity) || !compClass->HasDefaultConstructor()) {
            return;
        }

        registry.EmplaceDyn(compClass, inEntity, {});
        context.NotifyComponentsChanged(inEntity);
    }

    void InspectorBackend::RemoveComponent(uint inEntity, const QString& inClassName)
    {
        auto& registry = context.GetClient().GetWorld().GetRegistry();
        const auto* compClass = Internal::FindCompClass(inClassName);
        if (compClass == nullptr || !registry.Valid(inEntity) || !registry.HasDyn(compClass, inEntity)) {
            return;
        }

        registry.RemoveDyn(compClass, inEntity);
        context.NotifyComponentsChanged(inEntity);
    }

    InspectorPanel::InspectorPanel(EditorContext& inContext, QWidget* inParent)
        : WebWidget(inParent)
    {
        Load("/editor/inspector");
        backend = new InspectorBackend(inContext, this);
        GetWebChannel()->registerObject("backend", backend);
    }
}
