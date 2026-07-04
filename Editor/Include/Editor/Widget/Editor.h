//
// Created by Kindem on 2025/3/22.
//

#pragma once

#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

#include <Editor/EditorContext.h>

namespace Editor {
    class EditorViewport;

    class ExplosionEditor final : public QMainWindow {
        Q_OBJECT

    public:
        ExplosionEditor();
        ~ExplosionEditor() override;

    private:
        void SetupWindow();
        void StartEngineTick();
        void TickEngine();

        EditorContext* context;
        EditorViewport* viewport;
        QTimer* tickTimer;
        QElapsedTimer frameTimer;
    };
}
