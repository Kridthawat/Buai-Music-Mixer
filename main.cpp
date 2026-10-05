#include "MainWindow.h"

#include <QApplication>
#include <QSplashScreen>
#include <QDirIterator>
#include <QMetaType>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

#include "BASSFX/VSTFX.h"
#include "version.h"
#include "Config.h"
#include "Utils.h"

#ifdef __linux__
#include <QFontDatabase>
#endif

void registerMetaType();
void applyDarkTheme(QApplication &app);

void checkDatabase(QSplashScreen *splash, SongDatabase *db);
void loadSoundfonts(QSplashScreen *splash, MidiSynthesizer *synth);

#ifndef __linux__
void loadVSTi(QSplashScreen *splash, MidiSynthesizer *synth);
void makeVSTList(QSplashScreen *splash, MidiSynthesizer *synth);
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
    QCoreApplication::setApplicationVersion(VER_FILEVERSION_STR);
    QCoreApplication::setOrganizationName(VER_COMPANYNAME_STR);
    QCoreApplication::setOrganizationDomain(VER_COMPANYDOMAIN_STR);

    registerMetaType();

    QPixmap *pixmap = new QPixmap(":/Icons/App/splash.png");
    QSplashScreen *splash = new QSplashScreen(*pixmap);
    splash->show();
    qApp->processEvents();

    splash->showMessage("กำลังเริ่มโปรแกรม...", Qt::AlignBottom|Qt::AlignRight, QColor("#cfd3dc"));
    qApp->processEvents();

    { // Config Dir
        Config::initConfigDataPath();

        QDir dir(TEMP_DIR_PATH);
        if (!dir.exists())
            dir.mkpath(TEMP_DIR_PATH);

        dir.setPath(ALL_DATA_DIR_PATH);
        if (!dir.exists())
            dir.mkpath(ALL_DATA_DIR_PATH);

        dir.setPath(Config::CONFIG_DIR_PATH);
        if (!dir.exists())
            dir.mkpath(Config::CONFIG_DIR_PATH);
    }

    // Modern dark theme
    applyDarkTheme(a);


    // Add font for linux
    #ifdef __linux__
    QFontDatabase::addApplicationFont(":/Fonts/THSarabunNew/THSarabunNew Bold.ttf");
    #endif
    //----------------------------------

    // Buai Music Mixer edition: MainWindow only acts as the hidden audio/MIDI
    // back-end. The visible window of the program is the synth mixer.
    MainWindow w;
    qApp->setWindowIcon(QIcon(":/Icons/App/icon.png"));

    loadSoundfonts(splash, w.midiPlayer()->midiSynthesizer());

    #ifndef __linux__
    loadVSTi(splash, w.midiPlayer()->midiSynthesizer());
    makeVSTList(splash, w.midiPlayer()->midiSynthesizer());
    w.synthMixerDialog()->setVSTVendorMenu();
    #endif
    w.synthMixerDialog()->setFXToSynth();

    SynthMixerDialog *mixer = w.synthMixerDialog();
    mixer->setWindowTitle("Buai Music Mixer");
    w.hide();
    mixer->show();

    splash->finish(mixer);

    delete splash;
    delete pixmap;

    return a.exec();
}


void applyDarkTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    const QColor window("#101114"), base("#17181c"), alt("#1d1f24"), button("#23252c");
    const QColor text("#e8eaed"), dim("#6b7080"), accent("#8b5cf6"), link("#22d3ee");

    QPalette p;
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, alt);
    p.setColor(QPalette::ToolTipBase, button);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor("#ffffff"));
    p.setColor(QPalette::Link, link);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    p.setColor(QPalette::Light, QColor("#30333b"));
    p.setColor(QPalette::Midlight, QColor("#2a2d35"));
    p.setColor(QPalette::Mid, QColor("#4a4e5a"));
    p.setColor(QPalette::Dark, QColor("#0b0b0d"));
    p.setColor(QPalette::Shadow, QColor("#6b7080"));
    p.setColor(QPalette::Disabled, QPalette::Text, dim);
    p.setColor(QPalette::Disabled, QPalette::WindowText, dim);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, dim);
    app.setPalette(p);

    app.setStyleSheet(
        "QToolTip { background-color: #23252c; color: #e8eaed; border: 1px solid #3a3d46; padding: 4px; }"
        "QMenu { background-color: #17181c; color: #e8eaed; border: 1px solid #2a2d35; padding: 6px; }"
        "QMenu::item { padding: 7px 28px 7px 22px; border-radius: 5px; }"
        "QMenu::item:selected { background-color: #8b5cf6; color: #ffffff; }"
        "QMenu::item:disabled { color: #6b7080; }"
        "QMenu::separator { height: 1px; background: #2a2d35; margin: 5px 8px; }"
        "QPushButton { background-color: #23252c; color: #e8eaed; border: 1px solid #3a3d46; border-radius: 6px; }"
        "QPushButton:hover { background-color: #2e3139; border-color: #8b5cf6; }"
        "QPushButton:pressed { background-color: #8b5cf6; }"
        "QPushButton:checked { background-color: #8b5cf6; color: #ffffff; border-color: #a78bfa; }"
        "QPushButton:disabled { color: #6b7080; background-color: #1a1b20; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background-color: #17181c; color: #e8eaed;"
        "  border: 1px solid #3a3d46; border-radius: 6px; padding: 3px 6px; selection-background-color: #8b5cf6; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #8b5cf6; }"
        "QComboBox QAbstractItemView { background-color: #17181c; color: #e8eaed; selection-background-color: #8b5cf6; border: 1px solid #3a3d46; }"
        "QGroupBox { border: 1px solid #2a2d35; border-radius: 8px; margin-top: 10px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #22d3ee; }"
        "QTabWidget::pane { border: 1px solid #2a2d35; border-radius: 6px; }"
        "QTabBar::tab { background: #17181c; color: #9aa0ae; padding: 7px 16px; border: 1px solid #2a2d35; border-bottom: none;"
        "  border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
        "QTabBar::tab:selected { background: #23252c; color: #ffffff; border-color: #8b5cf6; }"
        "QTabBar::tab:hover { color: #ffffff; }"
        "QListWidget, QTreeWidget, QTableWidget, QListView, QTableView { background-color: #17181c; color: #e8eaed;"
        "  border: 1px solid #2a2d35; border-radius: 6px; alternate-background-color: #1d1f24; }"
        "QHeaderView::section { background-color: #23252c; color: #e8eaed; border: none; padding: 5px; }"
        "QSplitter::handle { background-color: #2a2d35; }"
        "QScrollBar:horizontal { background: #17181c; height: 12px; margin: 0; border: none; }"
        "QScrollBar::handle:horizontal { background: #3a3d46; border-radius: 5px; min-width: 30px; }"
        "QScrollBar::handle:horizontal:hover { background: #8b5cf6; }"
        "QScrollBar:vertical { background: #17181c; width: 12px; margin: 0; border: none; }"
        "QScrollBar::handle:vertical { background: #3a3d46; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #8b5cf6; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; background: none; border: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: none; }"
        "QSlider::groove:horizontal { height: 4px; background: #2a2d35; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #8b5cf6; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #f4f5fa; width: 14px; margin: -6px 0; border-radius: 7px; }"
        "QCheckBox, QRadioButton { spacing: 8px; }"
    );
}

void registerMetaType()
{
    qRegisterMetaType<MidiEvent>("MidiEvent");
    qRegisterMetaType<InstrumentType>("InstrumentType");

    //<QList<int>>("QList<int>");
    qRegisterMetaTypeStreamOperators<QList<int>>("QList<int>");

    //qRegisterMetaType<QList<uint>>("QList<uint>");
    qRegisterMetaTypeStreamOperators<QList<uint>>("QList<uint>");

    //qRegisterMetaType<QList<float>>("QList<float>");
    qRegisterMetaTypeStreamOperators<QList<float>>("QList<float>");

    //qRegisterMetaType<QList<bool>>("QList<bool>");
    qRegisterMetaTypeStreamOperators<QList<bool>>("QList<bool>");

    //qRegisterMetaType<QList<QList<float>>>("QList<QList<float>>");
    qRegisterMetaTypeStreamOperators<QList<QList<float>>>("QList<QList<float>>");

    qRegisterMetaTypeStreamOperators<QList<QByteArray>>("QList<QByteArray>>");
}

void checkDatabase(QSplashScreen *splash, SongDatabase *db)
{
    if (db->isNewVersion())
        return;

    splash->showMessage("กำลังปรับปรุงฐานข้อมูลเพลง", Qt::AlignBottom|Qt::AlignRight, QColor("#cfd3dc"));
    qApp->processEvents();

    db->updateToNewVersion();
}

void loadSoundfonts(QSplashScreen *splash, MidiSynthesizer *synth)
{
    QSettings settings(Config::CONFIG_APP_FILE_PATH, QSettings::IniFormat);

    synth->setLoadAllSoundfont(settings.value("SynthSoundfontsLoadAll", false).toBool());

    // set soundfont to synth
    QStringList sfList = settings.value("SynthSoundfonts", QStringList()).toStringList();
    settings.beginReadArray("SynthSoundfontsVolume");
    for (int i=0; i<sfList.count(); i++)
    {
        settings.setArrayIndex(i);
        int volume = settings.value("SoundfontVolume", 100).toInt();

        splash->showMessage("กำลังโหลด : " + QFileInfo(sfList.at(i)).fileName(), Qt::AlignBottom|Qt::AlignRight, QColor("#cfd3dc"));
        qApp->processEvents();

        if (synth->addSoundfont(sfList.at(i)))
            synth->setSoundfontVolume(i, volume / 100.0f);
    }
    settings.endArray();


    // set Map soundfont
    for (int i = 0; i < SF_PRESET_COUNT; i++) {
        QString sfKey = "SynthSoundfontsMap";
        QString drKey = "SynthSoundfontsDrumMap";
        if (i > 0) {
            sfKey = sfKey + QString::number(i);
            drKey = drKey + QString::number(i);
        }
        QList<int> sfMap     = settings.value(sfKey).value<QList<int>>();
        QList<int> sfDrumMap = settings.value(drKey).value<QList<int>>();

        if (sfMap.count() == 0)
            sfMap = synth->getMapSoundfontIndex(i);
        if (sfDrumMap.count() == 0)
            sfDrumMap = synth->getDrumMapSfIndex(i);

        synth->setMapSoundfontIndex(i, sfMap, sfDrumMap);
    }
}

#ifndef __linux__

void loadVSTi(QSplashScreen *splash, MidiSynthesizer *synth)
{
    QSettings st(Config::CONFIG_SYNTH_FILE_PATH, QSettings::IniFormat);

    st.beginReadArray("VSTiGroup");

    for (int i=0; i<4; i++)
    {
        st.setArrayIndex(i);

        QString      filePath   = st.value("VstiFilePath", "").toString();
        int          program    = st.value("VstiPrograms", 0).toInt();
        QList<float> params     = st.value("VstiParams").value<QList<float>>();
        QByteArray   chunk      = st.value("VstiChunk", QByteArray()).toByteArray();

        if (filePath == "")
            continue;

        splash->showMessage("กำลังโหลด : " + QFileInfo(filePath).fileName(), Qt::AlignBottom|Qt::AlignRight, QColor("#cfd3dc"));
        qApp->processEvents();

        DWORD vsti = synth->setVSTiFile(i, filePath);

        if (vsti != 0)
        {
            if (chunk.length() > 0)
                BASS_VST_SetChunk(vsti, false, chunk.constData(), chunk.length());

            BASS_VST_SetProgram(vsti, program);
            FX::setVSTParams(vsti, params);
        }
    }

    st.endArray();
}

void makeVSTList(QSplashScreen *splash, MidiSynthesizer *synth)
{
    QSettings st(Config::CONFIG_SYNTH_FILE_PATH, QSettings::IniFormat);
    QStringList dirs = st.value("VSTDirs", QStringList()).toStringList();

    QStringList vstDirs;
    vstDirs << QDir::currentPath() + "/VST";
    vstDirs += dirs;

    QMap<uint, VSTNamePath> vstList;

    for (const QString &dir : vstDirs)
    {
        QDirIterator it(dir, QStringList() << "*.DLL" << "*.dll",
                        QDir::Files|QDir::NoSymLinks, QDirIterator::Subdirectories);

        while(it.hasNext()) {

            it.next();

            splash->showMessage("กำลังตรวจสอบ : " + it.fileName(), Qt::AlignBottom|Qt::AlignRight, QColor("#cfd3dc"));
            qApp->processEvents();

            VSTNamePath info;

            if (!Utils::vstInfo(it.filePath(), &info))
                continue;

            vstList[info.uniqueID] = info;
        }
    }

    synth->setVSTList(vstList);
}

#endif
