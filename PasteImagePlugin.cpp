#include "PasteImagePlugin.h"

#include <MainWindow.h>
#include <session/Session.h>
#include <session/SessionController.h>
#include <terminalDisplay/TerminalDisplay.h>

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QCryptographicHash>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QKeySequence>
#include <QMimeData>

K_PLUGIN_CLASS_WITH_JSON(PasteImagePlugin, "konsole_pasteimageplugin.json")

PasteImagePlugin::PasteImagePlugin(QObject *parent, const QVariantList &args)
    : Konsole::IKonsolePlugin(parent, args)
{
    setName(QStringLiteral("PasteImage"));
}

PasteImagePlugin::~PasteImagePlugin() = default;

void PasteImagePlugin::ensureActions(Konsole::MainWindow *mainWindow) const
{
    auto &pw = m_windows[mainWindow];
    auto *self = const_cast<PasteImagePlugin *>(this);

    if (!pw.pasteAction) {
        auto *action = new QAction(i18n("Paste Image as Path"), mainWindow);
        action->setObjectName(QStringLiteral("paste-image-as-path"));
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        mainWindow->actionCollection()->addAction(action->objectName(), action);
        mainWindow->actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_V));
        QObject::connect(action, &QAction::triggered, mainWindow, [self, mainWindow]() {
            self->doPaste(mainWindow);
        });
        pw.pasteAction = action;
    }

    if (!pw.sendCtrlVAction) {
        auto *action = new QAction(i18n("Send Ctrl+V to Terminal"), mainWindow);
        action->setObjectName(QStringLiteral("paste-image-send-ctrl-v"));
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        mainWindow->actionCollection()->addAction(action->objectName(), action);
        mainWindow->actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
        QObject::connect(action, &QAction::triggered, mainWindow, [self, mainWindow]() {
            self->sendCtrlV(mainWindow);
        });
        pw.sendCtrlVAction = action;
    }
}

void PasteImagePlugin::createWidgetsForMainWindow(Konsole::MainWindow *mainWindow)
{
    ensureActions(mainWindow);
}

void PasteImagePlugin::activeViewChanged(Konsole::SessionController *controller, Konsole::MainWindow *mainWindow)
{
    auto &pw = m_windows[mainWindow];
    pw.controller = controller;
    if (!controller) {
        return;
    }

    // Strip Ctrl+V and Ctrl+Shift+V from Konsole's built-in paste so our actions
    // win. Shift+Insert remains as the standard alternate paste shortcut, and
    // our fallback re-invokes edit_paste for non-image clipboards.
    if (auto *konsolePaste = controller->actionCollection()->action(QStringLiteral("edit_paste"))) {
        controller->actionCollection()->setDefaultShortcut(konsolePaste,
                                                           QKeySequence(Qt::SHIFT | Qt::Key_Insert));
    }

    ensureActions(mainWindow);
    if (auto view = controller->view()) {
        if (pw.pasteAction) {
            view->addAction(pw.pasteAction);
        }
        if (pw.sendCtrlVAction) {
            view->addAction(pw.sendCtrlVAction);
        }
    }
}

QList<QAction *> PasteImagePlugin::menuBarActions(Konsole::MainWindow *mainWindow) const
{
    ensureActions(mainWindow);
    const auto &pw = m_windows[mainWindow];
    return {pw.pasteAction.data(), pw.sendCtrlVAction.data()};
}

void PasteImagePlugin::doPaste(Konsole::MainWindow *mainWindow)
{
    const auto &pw = m_windows.value(mainWindow);
    const QMimeData *mime = QGuiApplication::clipboard()->mimeData();

    auto sendTextToTerminal = [&pw](QString text) -> bool {
        if (!pw.controller || text.isEmpty()) {
            return false;
        }
        if (auto session = pw.controller->session()) {
            if (auto view = pw.controller->view()) {
                if (view->bracketedPasteMode()) {
                    text.prepend(QStringLiteral("\033[200~"));
                    text.append(QStringLiteral("\033[201~"));
                }
            }
            session->sendTextToTerminal(text, QChar());
            return true;
        }
        return false;
    };

    auto triggerKonsolePaste = [&pw]() -> bool {
        if (!pw.controller) {
            return false;
        }
        if (auto *konsolePaste = pw.controller->actionCollection()->action(QStringLiteral("edit_paste"))) {
            if (konsolePaste->isEnabled()) {
                konsolePaste->trigger();
                return true;
            }
        }
        return false;
    };

    if (pw.controller && mime && mime->hasImage()) {
        const QImage img = qvariant_cast<QImage>(mime->imageData());
        if (!img.isNull()) {
            QByteArray pngBytes;
            QBuffer buf(&pngBytes);
            buf.open(QIODevice::WriteOnly);
            if (img.save(&buf, "PNG")) {
                const QByteArray digest =
                    QCryptographicHash::hash(pngBytes, QCryptographicHash::Sha256).toHex().left(16);
                const QString path = QStringLiteral("/tmp/konsole-clip-%1.png")
                                         .arg(QString::fromLatin1(digest));

                bool ok = QFile::exists(path);
                if (!ok) {
                    QFile out(path);
                    if (out.open(QIODevice::WriteOnly)) {
                        ok = (out.write(pngBytes) == pngBytes.size());
                    }
                }
                if (ok) {
                    // Wrap with bracketed-paste markers when the running
                    // program enabled DEC mode 2004. Otherwise apps like
                    // Claude Code's REPL treat the chars as typed input
                    // rather than a paste event (and miss path-as-image
                    // auto-attachment).
                    if (sendTextToTerminal(path)) {
                        return;
                    }
                }
            }
        }
    }

    if (mime && !mime->hasImage()) {
        if (triggerKonsolePaste()) {
            return;
        }
        if (mime->hasText()) {
            sendTextToTerminal(mime->text());
            return;
        }
    }

    triggerKonsolePaste();
}

void PasteImagePlugin::sendCtrlV(Konsole::MainWindow *mainWindow)
{
    const auto &pw = m_windows.value(mainWindow);
    if (!pw.controller) {
        return;
    }
    if (auto session = pw.controller->session()) {
        // ASCII SYN (0x16) is what the kernel TTY layer delivers for Ctrl+V.
        session->sendTextToTerminal(QStringLiteral("\x16"), QChar());
    }
}

#include "PasteImagePlugin.moc"
