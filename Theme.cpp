#include "Theme.h"

#include <QApplication>
#include <QColor>
#include <QList>
#include <QPair>
#include <QPalette>
#include <QSettings>
#include <QString>
#include <QStyleFactory>
#include <QTimer>

#include "Config.h"

namespace Theme {

namespace {

struct Colors {
    QString window, base, altBase, button, buttonHover, border, border2;
    QString text, dim, dimTab, accent, accentText, title, handle;
    QString light, midlight, mid, dark, shadow;
};

Mode g_mode = System;
bool g_dark = true;
QTimer *g_watch = nullptr;

Colors darkColors()
{
    Colors c;
    c.window = "#101114";  c.base = "#17181c";  c.altBase = "#1d1f24";
    c.button = "#23252c";  c.buttonHover = "#2e3139";
    c.border = "#3a3d46";  c.border2 = "#2a2d35";
    c.text = "#e8eaed";    c.dim = "#6b7080";   c.dimTab = "#9aa0ae";
    c.accent = "#8b5cf6";  c.accentText = "#ffffff";
    c.title = "#22d3ee";   c.handle = "#3a3d46";
    c.light = "#30333b";   c.midlight = "#2a2d35"; c.mid = "#4a4e5a";
    c.dark = "#0b0b0d";    c.shadow = "#6b7080";
    return c;
}

Colors lightColors()
{
    Colors c;
    c.window = "#f2f3f7";  c.base = "#ffffff";  c.altBase = "#f6f7fa";
    c.button = "#e9ebf1";  c.buttonHover = "#dfe2ea";
    c.border = "#c3c7d2";  c.border2 = "#d6d9e2";
    c.text = "#1b1c20";    c.dim = "#8a8f9c";   c.dimTab = "#555b69";
    c.accent = "#6d3df0";  c.accentText = "#ffffff";
    c.title = "#6d3df0";   c.handle = "#c3c7d2";
    c.light = "#ffffff";   c.midlight = "#eceef3"; c.mid = "#b8bcc6";
    c.dark = "#a6abb8";    c.shadow = "#7d8290";
    return c;
}

bool systemIsDark()
{
#ifdef Q_OS_WIN
    QSettings s("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                QSettings::NativeFormat);
    return s.value("AppsUseLightTheme", 1).toInt() == 0;
#else
    return false;
#endif
}

QString buildStyleSheet(const Colors &c)
{
    QString css =
        "QToolTip { background-color: @btn; color: @text; border: 1px solid @border; padding: 4px; }"
        "QMenu { background-color: @base; color: @text; border: 1px solid @border2; padding: 6px; }"
        "QMenu::item { padding: 7px 28px 7px 22px; border-radius: 5px; }"
        "QMenu::item:selected { background-color: @accent; color: @accentText; }"
        "QMenu::item:disabled { color: @dim; }"
        "QMenu::separator { height: 1px; background: @border2; margin: 5px 8px; }"
        "QPushButton { background-color: @btn; color: @text; border: 1px solid @border; border-radius: 6px; }"
        "QPushButton:hover { background-color: @btnHover; border-color: @accent; }"
        "QPushButton:pressed { background-color: @accent; color: @accentText; }"
        "QPushButton:checked { background-color: @accent; color: @accentText; border-color: @accent; }"
        "QPushButton:disabled { color: @dim; }"
        "QToolButton { background-color: @btn; color: @dimtab; border: 1px solid @border; border-radius: 5px; padding: 2px 4px; }"
        "QToolButton:hover { background-color: @btnHover; color: @text; border-color: @accent; }"
        "QToolButton:checked { background-color: @accent; color: @accentText; font-weight: bold; border: 2px solid @title; }"
        "QToolButton:checked:hover { background-color: @accent; color: @accentText; }"
        "QToolButton:disabled { color: @dim; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background-color: @base; color: @text;"
        "  border: 1px solid @border; border-radius: 6px; padding: 3px 6px; selection-background-color: @accent; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: @accent; }"
        "QComboBox QAbstractItemView { background-color: @base; color: @text;"
        "  selection-background-color: @accent; selection-color: @accentText; border: 1px solid @border; }"
        "QGroupBox { border: 1px solid @border2; border-radius: 8px; margin-top: 10px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: @title; }"
        "QTabWidget::pane { border: 1px solid @border2; border-radius: 6px; }"
        "QTabBar::tab { background: @base; color: @dimtab; padding: 7px 16px; border: 1px solid @border2;"
        "  border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
        "QTabBar::tab:selected { background: @btn; color: @text; border-color: @accent; }"
        "QTabBar::tab:hover { color: @text; }"
        "QListWidget, QTreeWidget, QTableWidget, QListView, QTableView { background-color: @base; color: @text;"
        "  border: 1px solid @border2; border-radius: 6px; alternate-background-color: @altbase; }"
        "QHeaderView::section { background-color: @btn; color: @text; border: none; padding: 5px; }"
        "QSplitter::handle { background-color: @border2; }"
        "QScrollBar:horizontal { background: @base; height: 12px; margin: 0; border: none; }"
        "QScrollBar::handle:horizontal { background: @handle; border-radius: 5px; min-width: 30px; }"
        "QScrollBar::handle:horizontal:hover { background: @accent; }"
        "QScrollBar:vertical { background: @base; width: 12px; margin: 0; border: none; }"
        "QScrollBar::handle:vertical { background: @handle; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: @accent; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; background: none; border: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: none; }"
        "QSlider::groove:horizontal { height: 4px; background: @border2; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: @accent; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: @text; width: 14px; margin: -6px 0; border-radius: 7px; }"
        "QCheckBox, QRadioButton { spacing: 8px; }";

    // longer tokens first so that e.g. @accentText is not eaten by @accent
    QList<QPair<QString, QString> > tokens;
    tokens << qMakePair(QString("@accentText"), c.accentText)
           << qMakePair(QString("@accent"), c.accent)
           << qMakePair(QString("@altbase"), c.altBase)
           << qMakePair(QString("@base"), c.base)
           << qMakePair(QString("@btnHover"), c.buttonHover)
           << qMakePair(QString("@btn"), c.button)
           << qMakePair(QString("@border2"), c.border2)
           << qMakePair(QString("@border"), c.border)
           << qMakePair(QString("@dimtab"), c.dimTab)
           << qMakePair(QString("@dim"), c.dim)
           << qMakePair(QString("@text"), c.text)
           << qMakePair(QString("@title"), c.title)
           << qMakePair(QString("@handle"), c.handle);
    for (int i = 0; i < tokens.count(); i++)
        css.replace(tokens[i].first, tokens[i].second);
    return css;
}

void applyColors(bool dark)
{
    const Colors c = dark ? darkColors() : lightColors();

    QPalette p;
    p.setColor(QPalette::Window, QColor(c.window));
    p.setColor(QPalette::WindowText, QColor(c.text));
    p.setColor(QPalette::Base, QColor(c.base));
    p.setColor(QPalette::AlternateBase, QColor(c.altBase));
    p.setColor(QPalette::ToolTipBase, QColor(c.button));
    p.setColor(QPalette::ToolTipText, QColor(c.text));
    p.setColor(QPalette::Text, QColor(c.text));
    p.setColor(QPalette::Button, QColor(c.button));
    p.setColor(QPalette::ButtonText, QColor(c.text));
    p.setColor(QPalette::BrightText, QColor("#ffffff"));
    p.setColor(QPalette::Link, QColor(c.title));
    p.setColor(QPalette::Highlight, QColor(c.accent));
    p.setColor(QPalette::HighlightedText, QColor(c.accentText));
    p.setColor(QPalette::Light, QColor(c.light));
    p.setColor(QPalette::Midlight, QColor(c.midlight));
    p.setColor(QPalette::Mid, QColor(c.mid));
    p.setColor(QPalette::Dark, QColor(c.dark));
    p.setColor(QPalette::Shadow, QColor(c.shadow));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(c.dim));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(c.dim));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(c.dim));

    g_dark = dark;
    qApp->setPalette(p);
    qApp->setStyleSheet(buildStyleSheet(c));
}

void applyCurrent()
{
    bool dark = (g_mode == Dark) || (g_mode == System && systemIsDark());
    applyColors(dark);
}

}

void init(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QSettings st(Config::CONFIG_APP_FILE_PATH, QSettings::IniFormat);
    int m = st.value("Theme", static_cast<int>(System)).toInt();
    g_mode = (m == Dark) ? Dark : (m == Light) ? Light : System;

    applyCurrent();

    // follow the Windows setting while the program is running
    g_watch = new QTimer(&app);
    g_watch->setInterval(2000);
    QObject::connect(g_watch, &QTimer::timeout, []() {
        if (g_mode == System && systemIsDark() != g_dark)
            applyCurrent();
    });
    g_watch->start();
}

Mode mode()
{
    return g_mode;
}

void setMode(Mode m)
{
    g_mode = m;

    QSettings st(Config::CONFIG_APP_FILE_PATH, QSettings::IniFormat);
    st.setValue("Theme", static_cast<int>(m));

    applyCurrent();
}

bool isDark()
{
    return g_dark;
}

}
