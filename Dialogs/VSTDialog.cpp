#include "VSTDialog.h"

#include <QShowEvent>
#include <QCloseEvent>
#include <QHideEvent>

VSTDialog::VSTDialog(QWidget *parent, DWORD fxHandle, const QString &instName) : QDialog(parent)
{
    this->fxHandle = fxHandle;

    // The plug-in editor is embedded over the whole client area, so the app's custom
    // title bar would be hidden under it (no close button). Keep the native frame.
    setProperty("buaiNoSkin", true);

    BASS_VST_INFO info;
    if (BASS_VST_GetInfo(fxHandle, &info) && info.hasEditor)
    {
        QString name = info.effectName;
        name += " - ";
        name += info.vendorName;

        setWindowTitle(name + "  [" + instName + "]");
        setFixedSize(info.editorWidth, info.editorHeight);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

        canOpen = true;
    }
}

void VSTDialog::showEvent(QShowEvent *event)
{
    if (!attached) {
        BASS_VST_EmbedEditor(fxHandle, NULL);   // make sure no stale embed remains
        BASS_VST_EmbedEditor(fxHandle, (HWND)this->winId());
        attached = true;
    }
    event->accept();
}

void VSTDialog::detachEditor()
{
    if (attached && fxHandle != 0)
        BASS_VST_EmbedEditor(fxHandle, NULL);
    attached = false;
}

void VSTDialog::hideEvent(QHideEvent *event)
{
    // reject()/hide() (used by the skinned title-bar close) sends no closeEvent,
    // so the editor must be released here or it can never be embedded again.
    detachEditor();
    QDialog::hideEvent(event);
}

void VSTDialog::closeEvent(QCloseEvent *event)
{
    detachEditor();
    event->accept();
}

VSTDialog::~VSTDialog()
{
    detachEditor();
}
