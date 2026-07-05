//
// Created by johnk on 2026/7/5.
//

#pragma once

#include <vector>

#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>

namespace Editor {
    // thin layer over QMainWindow docking: registers panels as dock widgets, builds the view menu and persists the
    // layout, so the backing dock implementation can be swapped without touching the panels
    class DockManager final : public QObject {
        Q_OBJECT

    public:
        explicit DockManager(QMainWindow& inWindow);
        ~DockManager() override;

        QDockWidget* Register(const QString& inId, const QString& inTitle, Qt::DockWidgetArea inArea, QWidget* inContent);
        void BuildViewMenu(QMenu& outMenu) const;
        void SaveLayout() const;
        void RestoreLayout() const;

    private:
        QMainWindow& window;
        std::vector<QDockWidget*> docks;
    };
}
