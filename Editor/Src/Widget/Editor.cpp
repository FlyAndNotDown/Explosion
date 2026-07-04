//
// Created by Kindem on 2025/3/22.
//

#include <format>

#include <Core/Paths.h>
#include <Editor/Widget/Editor.h>
#include <Editor/Widget/EditorViewport.h>
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
        , viewport(nullptr)
        , tickTimer(new QTimer(this))
    {
        SetupWindow();
        StartEngineTick();
    }

    ExplosionEditor::~ExplosionEditor()
    {
        tickTimer->stop();
        // the viewport must unregister from the client before the context (world/client) goes away with the
        // remaining children
        delete takeCentralWidget();
        viewport = nullptr;
    }

    void ExplosionEditor::SetupWindow()
    {
        setWindowTitle(Internal::MakeWindowTitle());
        resize(1600, 900);

        viewport = new EditorViewport(context->GetClient(), this);
        setCentralWidget(viewport);
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
        Runtime::EngineHolder::Get().Tick(deltaSeconds);
    }
} // namespace Editor
