#ifndef TITLEBAR_H
#define TITLEBAR_H

// Custom neon title bar for the frameless mixer window.
// Header-only and without Q_OBJECT on purpose (no moc step needed).

#include <QWidget>
#include <QLabel>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QPixmap>
#include <QFont>
#include <QEvent>
#include <QMouseEvent>
#include <QWindow>
#include <QGraphicsDropShadowEffect>
#include <QVariantAnimation>

class TitleButton : public QAbstractButton
{
public:
    enum Kind { Minimize, Close };

    explicit TitleButton(Kind k, QWidget *parent = nullptr) : QAbstractButton(parent), kind(k)
    {
        setFixedSize(40, 26);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
    }

protected:
    void enterEvent(QEvent *) override { hover = true; update(); }
    void leaveEvent(QEvent *) override { hover = false; update(); }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor fg = palette().color(QPalette::WindowText);
        QRectF r = QRectF(rect()).adjusted(2, 2, -2, -2);

        if (hover || isDown()) {
            QColor bg = (kind == Close) ? QColor("#e5484d") : palette().color(QPalette::Link);
            bg.setAlpha(kind == Close ? (isDown() ? 255 : 220) : 70);
            p.setPen(Qt::NoPen);
            p.setBrush(bg);
            p.drawRoundedRect(r, 6, 6);
            if (kind == Close)
                fg = QColor("#ffffff");
        }

        p.setPen(QPen(fg, 1.6, Qt::SolidLine, Qt::RoundCap));
        QPointF c = rect().center();
        if (kind == Minimize) {
            p.drawLine(QPointF(c.x() - 5, c.y() + 3), QPointF(c.x() + 5, c.y() + 3));
        } else {
            p.drawLine(QPointF(c.x() - 4.5, c.y() - 4.5), QPointF(c.x() + 4.5, c.y() + 4.5));
            p.drawLine(QPointF(c.x() - 4.5, c.y() + 4.5), QPointF(c.x() + 4.5, c.y() - 4.5));
        }
    }

private:
    Kind kind;
    bool hover = false;
};


class TitleBar : public QWidget
{
public:
    TitleButton *minimizeButton;
    TitleButton *closeButton;

    explicit TitleBar(const QString &title, QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(36);
        setAutoFillBackground(false);

        QHBoxLayout *lay = new QHBoxLayout(this);
        lay->setContentsMargins(12, 0, 6, 0);
        lay->setSpacing(8);

        iconLabel = new QLabel(this);
        iconLabel->setFixedSize(22, 22);
        iconLabel->setScaledContents(true);
        iconLabel->setPixmap(QPixmap(":/Icons/App/icon.png"));
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        lay->addWidget(iconLabel);

        titleLabel = new QLabel(title, this);
        QFont f = titleLabel->font();
        f.setPointSize(13);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.6);
        titleLabel->setFont(f);
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        lay->addWidget(titleLabel);

        lay->addStretch(1);

        minimizeButton = new TitleButton(TitleButton::Minimize, this);
        closeButton = new TitleButton(TitleButton::Close, this);
        lay->addWidget(minimizeButton);
        lay->addWidget(closeButton);

        // neon glow around the title text, gently pulsing
        glow = new QGraphicsDropShadowEffect(titleLabel);
        glow->setOffset(0, 0);
        glow->setBlurRadius(16);
        titleLabel->setGraphicsEffect(glow);

        pulse = new QVariantAnimation(this);
        pulse->setDuration(2200);
        pulse->setLoopCount(-1);
        pulse->setStartValue(9.0);
        pulse->setKeyValueAt(0.5, 22.0);
        pulse->setEndValue(9.0);
        QObject::connect(pulse, &QVariantAnimation::valueChanged, glow,
                         [this](const QVariant &v) { glow->setBlurRadius(v.toReal()); });
        pulse->start();

        refresh();
    }

    void setTitle(const QString &t) { titleLabel->setText(t); }

protected:
    bool isDarkTheme() const { return palette().color(QPalette::Window).lightness() < 128; }
    QColor neon() const { return palette().color(QPalette::Link); }

    void refresh()
    {
        QColor n = neon();
        QColor core = isDarkTheme() ? n.lighter(150) : n;
        titleLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(core.name()));

        QColor g = n;
        g.setAlpha(isDarkTheme() ? 255 : 200);
        glow->setColor(g);
        update();
    }

    void changeEvent(QEvent *e) override
    {
        if (e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange)
            refresh();
        QWidget::changeEvent(e);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && window() && window()->windowHandle()) {
            window()->windowHandle()->startSystemMove();
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool dark = isDarkTheme();
        QRectF r = QRectF(rect());

        QLinearGradient bg(0, 0, 0, r.height());
        if (dark) {
            bg.setColorAt(0, QColor("#1d2027"));
            bg.setColorAt(1, QColor("#13151a"));
        } else {
            bg.setColorAt(0, QColor("#ffffff"));
            bg.setColorAt(1, QColor("#e8eaf2"));
        }
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r, 8, 8);

        // neon underline fading out at both ends
        QColor n = neon();
        QLinearGradient ul(0, 0, r.width(), 0);
        QColor c0 = n, c1 = n, c2 = n;
        c0.setAlpha(0); c1.setAlpha(dark ? 230 : 190); c2.setAlpha(0);
        ul.setColorAt(0.0, c0);
        ul.setColorAt(0.5, c1);
        ul.setColorAt(1.0, c2);
        p.setBrush(ul);
        p.drawRect(QRectF(0, r.height() - 2, r.width(), 2));

        QColor halo = n;
        halo.setAlpha(dark ? 40 : 30);
        QLinearGradient hl(0, 0, r.width(), 0);
        QColor h0 = halo, h2 = halo;
        h0.setAlpha(0); h2.setAlpha(0);
        hl.setColorAt(0.0, h0);
        hl.setColorAt(0.5, halo);
        hl.setColorAt(1.0, h2);
        p.setBrush(hl);
        p.drawRect(QRectF(0, r.height() - 6, r.width(), 4));
    }

private:
    QLabel *iconLabel;
    QLabel *titleLabel;
    QGraphicsDropShadowEffect *glow;
    QVariantAnimation *pulse;
};

// Transparent overlay that draws a thin neon outline around a frameless window.
class FrameOverlay : public QWidget
{
public:
    explicit FrameOverlay(QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        QColor c = palette().color(QPalette::Link);
        c.setAlpha(130);
        p.setPen(QPen(c, 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));
    }
};

#endif // TITLEBAR_H
