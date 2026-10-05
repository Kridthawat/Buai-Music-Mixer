#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "version.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->lbVersion->setText(VER_FILEVERSION_STR);

    QString version = QT_VERSION_STR;
    ui->lbQtVer->setText("Using Qt version " + version);
    // GPLv3 requires keeping the original copyright / license notice.
    ui->lbCopyright->setText(QString(VER_LEGALCOPYRIGHT_STR) +
        "\nDerived from Handy Karaoke, Copyright (C) 2017-2019 pie62 (GPLv3)");
    ui->lbCopyright->setWordWrap(true);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
