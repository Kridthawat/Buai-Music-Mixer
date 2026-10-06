#include "VSTLabel.h"
#include "ui_VSTLabel.h"

#include <QMouseEvent>
#include <QEvent>
#include <QGraphicsDropShadowEffect>


VSTLabel::VSTLabel(QWidget *parent, const QString &label, int fxIndex, bool bypass) :
    QWidget(parent),
    ui(new Ui::VSTLabel)
{
    ui->setupUi(this);

    this->fxIndex = fxIndex;
    this->fxBypass = bypass;

    ui->label->setText(label);
    ui->label->setToolTip(label);

    // neon glow (colors follow the theme)
    btnGlow = new QGraphicsDropShadowEffect(ui->btn);
    btnGlow->setOffset(0, 0);
    btnGlow->setBlurRadius(14);
    ui->btn->setGraphicsEffect(btnGlow);

    frameGlow = new QGraphicsDropShadowEffect(ui->frame);
    frameGlow->setOffset(0, 0);
    frameGlow->setBlurRadius(10);
    ui->frame->setGraphicsEffect(frameGlow);

    applyNeon();

    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequested(QPoint)));
}

VSTLabel::~VSTLabel()
{
    delete ui;
}

void VSTLabel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)
        applyNeon();
    QWidget::changeEvent(event);
}

void VSTLabel::applyNeon()
{
    const QColor neon = palette().color(QPalette::Link);
    const bool dark = palette().color(QPalette::Window).lightness() < 128;

    // setStyleSheet can itself trigger palette events: only act when the colors really changed
    const QString key = neon.name() + (dark ? "d" : "l");
    if (key == neonKey)
        return;
    neonKey = key;

    const QColor core = dark ? neon.lighter(150) : neon;

    setStyleSheet(QString("#frame { border: 1px solid %1; border-radius: 4px; background: rgba(%2, %3, %4, 30); }"
                          "#label { color: %5; background: transparent; }")
                  .arg(neon.name()).arg(neon.red()).arg(neon.green()).arg(neon.blue()).arg(core.name()));

    QColor g = neon;
    g.setAlpha(dark ? 150 : 110);
    frameGlow->setColor(g);

    updateBtnStyle();
}

void VSTLabel::updateBtnStyle()
{
    const QColor neon = palette().color(QPalette::Link);
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor core = dark ? neon.lighter(150) : neon;

    if (fxBypass)
        ui->btn->setStyleSheet(QString("background: transparent; border: 1px solid %1; border-radius: 6px;")
                               .arg(neon.name()));
    else
        ui->btn->setStyleSheet(QString("background: %1; border: 1px solid %2; border-radius: 6px;")
                               .arg(core.name()).arg(neon.name()));

    btnGlow->setColor(neon);
    btnGlow->setEnabled(!fxBypass);
}

void VSTLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    emit doubleClicked(fxIndex);
}

void VSTLabel::on_btn_clicked()
{
    fxBypass = !fxBypass;

    updateBtnStyle();

    emit byPassChanged(fxIndex, fxBypass);
}

void VSTLabel::contextMenuRequested(const QPoint &pos)
{
    emit menuRequested(fxIndex, pos);
}
