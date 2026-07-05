//
// Created by Kindem on 2025/3/22.
//

#include <format>

#include <QCloseEvent>
#include <QMenuBar>

#include <Core/Paths.h>
#include <Editor/Widget/Editor.h>
#include <Editor/Widget/EditorViewport.h>
#include <Editor/Widget/InspectorPanel.h>
#include <Editor/Widget/LogPanel.h>
#include <Editor/Widget/OutlinerPanel.h>
#include <Editor/Widget/moc_Editor.cpp> // NOLINT
#include <Runtime/Engine.h>

namespace Editor::Internal {
    constexpr int tickIntervalMs = 16;

    static QString MakeWindowTitle()
    {
        const std::string projectName = Core::Paths::HasSetGameRoot()
            ? Core::Paths::GameRootDir().DirName()
            : "Untitled";
        return QString::fromStdString(std::format("{} - Explosion Editor", projectName));
    }
}

namespace Editor {
    ExplosionEditor::ExplosionEditor()
        : context(new EditorContext(this))
        , dockManager(new DockManager(*this))
        , viewport(nullptr)
        , tickTimer(new QTimer(this))
    {
        SetupWindow();
        SetupDocks();
        SetupMenus();
        dockManager->RestoreLayout();
        context->GetClient().OpenProjectLevel();
        StartEngineTick();
    }

    ExplosionEditor::~ExplosionEditor()
    {
        tickTimer->stop();
        // the viewport must unregister from the client (and drain in-flight gpu work) before the context
        // (world/client) goes away with the remaining children
        delete viewport;
        viewport = nullptr;
    }

    void ExplosionEditor::closeEvent(QCloseEvent* inEvent)
    {
        dockManager->SaveLayout();
        QMainWindow::closeEvent(inEvent);
    }

    void ExplosionEditor::SetupWindow()
    {
        setWindowTitle(Internal::MakeWindowTitle());
        resize(1600, 900);
    }

    void ExplosionEditor::SetupDocks()
    {
        viewport = new EditorViewport(context->GetClient(), this);

        auto* viewportDock = dockManager->Register("viewport", "Viewport", Qt::LeftDockWidgetArea, viewport);
        dockManager->Register("outliner", "Outliner", Qt::RightDockWidgetArea, new OutlinerPanel(*context));
        dockManager->Register("inspector", "Inspector", Qt::RightDockWidgetArea, new InspectorPanel(*context));
        dockManager->Register("log", "Log", Qt::BottomDockWidgetArea, new LogPanel());
        resizeDocks({ viewportDock }, { 1100 }, Qt::Horizontal);
    }

    void ExplosionEditor::SetupMenus()
    {
        QMenu* fileMenu = menuBar()->addMenu("&File");
        QAction* saveLevelAction = fileMenu->addAction("&Save Level");
        saveLevelAction->setShortcut(QKeySequence::Save);
        connect(saveLevelAction, &QAction::triggered, this, [this]() -> void { context->GetClient().SaveLevel(); });

        QMenu* viewMenu = menuBar()->addMenu("&View");
        dockManager->BuildViewMenu(*viewMenu);
    }

    void ExplosionEditor::StartEngineTick()
    {
        frameTimer.start();
        connect(tickTimer, &QTimer::timeout, this, [this]() -> void { TickEngine(); });
        tickTimer->start(Internal::tickIntervalMs);
    }

    void ExplosionEditor::TickEngine()
    {
        const float deltaSeconds = static_cast<float>(frameTimer.restart()) / 1000.0f;
        viewport->TickEditorCamera(deltaSeconds);
        Runtime::EngineHolder::Get().Tick(deltaSeconds);
    }
} // namespace Editor
