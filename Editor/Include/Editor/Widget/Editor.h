//
// Created by Kindem on 2025/3/22.
//

#pragma once

#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

#include <Editor/EditorContext.h>
#include <Editor/Widget/Dock.h>

namespace Editor {
    class EditorViewport;

    class ExplosionEditor final : public QMainWindow {
        Q_OBJECT

    public:
        ExplosionEditor();
        ~ExplosionEditor() override;

    protected:
        void closeEvent(QCloseEvent* inEvent) override;

    private:
        void SetupWindow();
        void SetupDocks();
        void SetupMenus();
        void StartEngineTick();
        void TickEngine();

        EditorContext* context;
        DockManager* dockManager;
        EditorViewport* viewport;
        QTimer* tickTimer;
        QElapsedTimer frameTimer;
    };
}
