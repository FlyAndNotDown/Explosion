//
// Created by johnk on 2026/7/5.
//

#pragma once

#include <Editor/EditorContext.h>
#include <Editor/Widget/WebWidget.h>

namespace Editor {
    class InspectorBackend final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QJsonValue selectedEntityData READ GetSelectedEntityData NOTIFY SelectedEntityDataChanged)
        Q_PROPERTY(QJsonValue addableComponents READ GetAddableComponents CONSTANT)

    public:
        explicit InspectorBackend(EditorContext& inContext, QObject* inParent = nullptr);

        QJsonValue GetSelectedEntityData() const;
        QJsonValue GetAddableComponents() const;

    public Q_SLOTS:
        void UpdateComponent(uint inEntity, const QString& inClassName, const QJsonValue& inContent);
        void AddComponent(uint inEntity, const QString& inClassName);
        void RemoveComponent(uint inEntity, const QString& inClassName);

    Q_SIGNALS:
        void SelectedEntityDataChanged();

    private:
        EditorContext& context;
    };

    class InspectorPanel final : public WebWidget {
        Q_OBJECT

    public:
        explicit InspectorPanel(EditorContext& inContext, QWidget* inParent = nullptr);

    private:
        InspectorBackend* backend;
    };
}
