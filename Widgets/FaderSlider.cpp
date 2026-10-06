#include "FaderSlider.h"

#include <QResizeEvent>
#include <QPainter>
#include <QEvent>
#include <QLinearGradient>
#include <QPixmap>

FaderSlider::FaderSlider(QWidget *parent) : QFrame(parent)
{
    sHandle = new QLabel(this);
    buildHandle();
}

// Fader knob drawn in the current theme colors, with a neon glowing center line.
void FaderSlider::buildHandle()
{
    const qreal dpr = devicePixelRatioF();
    const int w = 19, h = 28;

    QPixmap pm(int(w * dpr), int(h * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor neon = palette().color(QPalette::Link);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient g(0, 0, 0, h);
    if (dark) {
        g.setColorAt(0, QColor("#4b4f5c"));
        g.setColorAt(1, QColor("#25272e"));
    } else {
        g.setColorAt(0, QColor("#ffffff"));
        g.setColorAt(1, QColor("#d3d7e0"));
    }
    p.setBrush(g);
    p.setPen(QPen(dark ? QColor("#6b7080") : QColor("#9aa0ae"), 1));
    p.drawRoundedRect(QRectF(1.5, 1.5, w - 3, h - 3), 3, 3);

    // grip ridges
    p.setPen(QPen(dark ? QColor(255, 255, 255, 40) : QColor(0, 0, 0, 35), 1));
    const int ridges[4] = { 6, 8, 20, 22 };
    for (int i = 0; i < 4; i++)
        p.drawLine(QPointF(4, ridges[i]), QPointF(w - 4, ridges[i]));

    // neon line in the middle
    const qreal cy = h / 2.0;
    QColor glow = neon;
    glow.setAlpha(55);
    p.setPen(QPen(glow, 7, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(4, cy), QPointF(w - 4, cy));
    glow.setAlpha(120);
    p.setPen(QPen(glow, 4, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(4, cy), QPointF(w - 4, cy));
    QColor core = dark ? neon.lighter(165) : neon;
    p.setPen(QPen(core, 1.6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(4, cy), QPointF(w - 4, cy));
    p.end();

    sHandle->setScaledContents(false);
    sHandle->setFixedSize(w, h);
    sHandle->setPixmap(pm);
}

void FaderSlider::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        buildHandle();
        update();
    }
    QFrame::changeEvent(event);
}

FaderSlider::~FaderSlider()
{
    delete sHandle;
}

void FaderSlider::setLevel(int level)
{
    if (level == sLv)
        return;

    if (level > sMaxLv)
        sLv = sMaxLv;
    else if (level < sMinLv)
        sLv = sMinLv;
    else
        sLv = level;

    moveHandle();

    emit levelChanged(sLv);
    emit levelChanged(QString::number(sLv));
}

void FaderSlider::setMinimumLevel(int minLevel)
{
    if (minLevel == sMinLv || minLevel >= sMaxLv)
        return;

    sMinLv = minLevel;
    if (sLv < sMinLv) {
        sLv = sMinLv;
        emit levelChanged(sLv);
        emit levelChanged(QString::number(sLv));
    }

    moveHandle();
}

void FaderSlider::setMaximumLevel(int maxLevel)
{
    if (maxLevel == sMaxLv || maxLevel <= sMinLv)
        return;

    sMaxLv = maxLevel;
    if (sLv > sMaxLv) {
        sLv = sMaxLv;
        emit levelChanged(sLv);
        emit levelChanged(QString::number(sLv));
    }

    moveHandle();
}

void FaderSlider::setChangeStep(int cstp)
{
    if (cstp < 1)
        return;

    sChangStep = cstp;
}

void FaderSlider::setTickCount(int tc)
{
    if (tc == sTickCount || tc < 0)
        return;

    sTickCount = tc;
    update();
}

void FaderSlider::setEnableMousePress(bool mp)
{
    sMousePress = mp;
}

void FaderSlider::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    emit mouseDoubleClicked();
}

void FaderSlider::wheelEvent(QWheelEvent *event)
{
    int lv = sLv;
    if (event->delta() > 0) { // up
        lv += sChangStep;
    }
    else {
        lv -= sChangStep;
    }

    if (lv > sMaxLv)
        lv = sMaxLv;
    else if (lv < sMinLv)
        lv = sMinLv;

    if (lv == sLv)
        return;

    setLevel(lv);

    emit userLevelChanged(level());
    emit userLevelChanged(QString::number(level()));

    event->accept();
}

void FaderSlider::mousePressEvent(QMouseEvent *event)
{
    if (!sMousePress)
        return;

    if (event->button() != Qt::LeftButton)
        return;

    int abslv = abs(sMaxLv-sMinLv);

    int lv = abslv - (abslv * event->y() / height());
    lv = lv + sMinLv;

    if (lv > sMaxLv)
        lv = sMaxLv;
    else if (lv < sMinLv)
        lv = sMinLv;

    if (lv == sLv)
        return;

    setLevel(lv);

    emit userLevelChanged(level());
    emit userLevelChanged(QString::number(level()));
}

void FaderSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() != Qt::LeftButton)
        return;

    int abslv = abs(sMaxLv-sMinLv);

    int lv = abslv - (abslv * event->y() / height());
    lv = lv + sMinLv;

    if (lv > sMaxLv)
        lv = sMaxLv;
    else if (lv < sMinLv)
        lv = sMinLv;

    if (lv == sLv)
        return;

    setLevel(lv);

    emit userLevelChanged(level());
    emit userLevelChanged(QString::number(level()));
}

void FaderSlider::resizeEvent(QResizeEvent *event)
{
    moveHandle();
}

void FaderSlider::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor neon = palette().color(QPalette::Link);
    const qreal x = width() / 2.0;
    const qreal hy = sHandle->y() + sHandle->height() / 2.0;

    // whole track: faint neon
    QColor dim = neon;
    dim.setAlpha(dark ? 70 : 90);
    p.setPen(QPen(dim, 2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(x, 0), QPointF(x, height()));

    // part below the knob (the level): bright neon with glow
    QColor glow = neon;
    glow.setAlpha(45);
    p.setPen(QPen(glow, 8, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(x, hy), QPointF(x, height()));
    glow.setAlpha(100);
    p.setPen(QPen(glow, 5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(x, hy), QPointF(x, height()));
    p.setPen(QPen(dark ? neon.lighter(160) : neon, 2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(x, hy), QPointF(x, height()));

    p.end();
}

void FaderSlider::moveHandle()
{
    int abslv = abs(sMaxLv-sMinLv);
    int maxHeight = height() - sHandle->height();

    int absCurrentLv =  abs(sMinLv-sLv);

    sHandle->move((this->width() - sHandle->width()) / 2,
                  maxHeight * (abslv - absCurrentLv) / abslv);
    update();
}
