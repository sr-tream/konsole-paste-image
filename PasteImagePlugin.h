#pragma once

#include <pluginsystem/IKonsolePlugin.h>

#include <QAction>
#include <QHash>
#include <QPointer>

namespace Konsole {
class MainWindow;
class SessionController;
}

class PasteImagePlugin : public Konsole::IKonsolePlugin
{
    Q_OBJECT
public:
    PasteImagePlugin(QObject *parent, const QVariantList &args);
    ~PasteImagePlugin() override;

    void createWidgetsForMainWindow(Konsole::MainWindow *mainWindow) override;
    void activeViewChanged(Konsole::SessionController *controller, Konsole::MainWindow *mainWindow) override;
    QList<QAction *> menuBarActions(Konsole::MainWindow *mainWindow) const override;

private:
    struct PerWindow {
        QPointer<QAction> pasteAction;
        QPointer<QAction> sendCtrlVAction;
        QPointer<Konsole::SessionController> controller;
    };

    void ensureActions(Konsole::MainWindow *mainWindow) const;
    void doPaste(Konsole::MainWindow *mainWindow);
    void sendCtrlV(Konsole::MainWindow *mainWindow);

    mutable QHash<Konsole::MainWindow *, PerWindow> m_windows;
};
