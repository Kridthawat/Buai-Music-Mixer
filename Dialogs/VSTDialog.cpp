#include "VSTDialog.h"

#include <QShowEvent>
#include <QCloseEvent>
#include <QHideEvent>

VSTDialog::VSTDialog(QWidget *parent, DWORD fxHandle, const QString &instName) : QDialog(parent)
{
    this->fxHandle = fxHandle;

    // The plug-in editor lives in its own native child widget (host) below the
    // themed title bar; a 1px margin keeps the neon frame outline visible around it.
    setContentsMargins(2, 0, 2, 6);   // bottom room keeps the rounded corners clear of the plug-in
    host = new QWidget(this);
    host->setAttribute(Qt::WA_NativeWindow);
    host->setAttribute(Qt::WA_DontCreateNativeAncestors);

    BASS_VST_INFO info;
    if (bv::GetInfo(fxHandle, &info) && info.hasEditor)
    {
        QString name = info.effectName;
        name += " - ";
        name += info.vendorName;

        setWindowTitle(name + "  [" + instName + "]");
        edW = info.editorWidth;
        edH = info.editorHeight;
        host->resize(edW, edH);
        setFixedSize(edW + 4, edH + 6);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

        canOpen = true;
    }
}

void VSTDialog::showEvent(QShowEvent *event)
{
    if (!attached) {
        QMargins m = contentsMargins();      // the skin adds the title bar height on top
        host->setGeometry(m.left(), m.top(), edW, edH);
        bv::EmbedEditor(fxHandle, NULL);   // make sure no stale embed remains
        bv::EmbedEditor(fxHandle, (HWND)host->winId());
        attached = true;
    }
    event->accept();
}

void VSTDialog::detachEditor()
{
    if (attached && fxHandle != 0)
        bv::EmbedEditor(fxHandle, NULL);
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
