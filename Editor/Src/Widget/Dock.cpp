//
// Created by johnk on 2026/7/5.
//

#include <QSettings>

#include <Core/Paths.h>
#include <Editor/Widget/Dock.h>
#include <Editor/Widget/moc_Dock.cpp> // NOLINT

namespace Editor::Internal {
    static QSettings MakeLayoutSettings()
    {
        const auto settingsFile = (Core::Paths::HasSetGameRoot() ? Core::Paths::GameCacheDir() : Core::Paths::EngineCacheDir()) / "Editor" / "Layout.ini";
        return { QString::fromStdString(settingsFile.String()), QSettings::IniFormat };
    }
}

namespace Editor {
    DockManager::DockManager(QMainWindow& inWindow)
        : QObject(&inWindow)
        , window(inWindow)
    {
        window.setDockNestingEnabled(true);
    }

    DockManager::~DockManager() = default;

    QDockWidget* DockManager::Register(const QString& inId, const QString& inTitle, Qt::DockWidgetArea inArea, QWidget* inContent)
    {
        auto* dock = new QDockWidget(inTitle, &window);
        dock->setObjectName(inId);
        dock->setWidget(inContent);
        window.addDockWidget(inArea, dock);
        docks.emplace_back(dock);
        return dock;
    }

    void DockManager::BuildViewMenu(QMenu& outMenu) const
    {
        for (auto* dock : docks) {
            outMenu.addAction(dock->toggleViewAction());
        }
    }

    void DockManager::SaveLayout() const
    {
        QSettings settings = Internal::MakeLayoutSettings();
        settings.setValue("mainWindowGeometry", window.saveGeometry());
        settings.setValue("mainWindowState", window.saveState());
    }

    void DockManager::RestoreLayout() const
    {
        QSettings settings = Internal::MakeLayoutSettings();
        if (const auto geometry = settings.value("mainWindowGeometry"); geometry.isValid()) {
            window.restoreGeometry(geometry.toByteArray());
        }
        if (const auto state = settings.value("mainWindowState"); state.isValid()) {
            window.restoreState(state.toByteArray());
        }
    }
}
