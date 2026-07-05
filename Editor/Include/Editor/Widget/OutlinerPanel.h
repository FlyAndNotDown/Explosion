//
// Created by johnk on 2026/7/5.
//

#pragma once

#include <Editor/EditorContext.h>
#include <Editor/Widget/WebWidget.h>
#include <Mirror/Meta.h>

namespace Editor {
    struct EClass() OutlinerEntityInfo {
        EClassBody(OutlinerEntityInfo)

        EProperty() uint32_t id;
        EProperty() std::string name;
    };

    class OutlinerBackend final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QJsonValue entities READ GetEntities NOTIFY EntitiesChanged)
        Q_PROPERTY(uint selectedEntity READ GetSelectedEntity NOTIFY SelectionChanged)

    public:
        explicit OutlinerBackend(EditorContext& inContext, QObject* inParent = nullptr);

        QJsonValue GetEntities() const;
        uint GetSelectedEntity() const;

    public Q_SLOTS:
        void SelectEntity(uint inEntity);
        void CreateEntity(const QString& inName);
        void DestroyEntity(uint inEntity);
        void RenameEntity(uint inEntity, const QString& inName);

    Q_SIGNALS:
        void EntitiesChanged();
        void SelectionChanged();

    private:
        EditorContext& context;
    };

    class OutlinerPanel final : public WebWidget {
        Q_OBJECT

    public:
        explicit OutlinerPanel(EditorContext& inContext, QWidget* inParent = nullptr);

    private:
        OutlinerBackend* backend;
    };
}
