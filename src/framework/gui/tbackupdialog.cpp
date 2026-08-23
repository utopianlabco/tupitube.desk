/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              * 
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Utopian Lab Development Team                                         *
 *   2010:                                                                 *
 *    Gustav Gonzalez                                                      *
 *   ---                                                                   *
 *   KTooN's versions:                                                     *
 *   2006:                                                                 *
 *    David Cuadrado                                                       *
 *    Jorge Cuadrado                                                       *
 *   2003:                                                                 *
 *    Fernado Roldan                                                       *
 *    Simena Dinas                                                         *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "tbackupdialog.h"
#include "tseparator.h"
#include "tosd.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>

TBackupDialog::TBackupDialog(const QString &project, QWidget *parent) : QDialog(parent),
                                                                       pathLine(nullptr),
                                                                       projectName(project)
{
    setModal(true);
    setupGUI();
}

TBackupDialog::~TBackupDialog()
{
}

void TBackupDialog::setupGUI()
{
    setWindowTitle(tr("Recovery Mode"));
    setWindowIcon(QPixmap(THEME_DIR + "icons/alert.png"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    QString msg = tr("TupiTube could not create the normal project package or its automatic recovery copy for <b>%1</b>.<br/>"
                     "Choose another folder where TupiTube can preserve an unpacked recovery snapshot.")
                  .arg(projectName);
    QLabel *label = new QLabel(msg);
    label->setWordWrap(true);

    destPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (destPath.isEmpty())
        destPath = QDir::homePath();

    pathLine = new QLineEdit(destPath);

    QToolButton *openButton = new QToolButton;
    openButton->setIcon(QIcon(THEME_DIR + "icons/open.png"));
    openButton->setToolTip(tr("Choose another path"));
    connect(openButton, SIGNAL(clicked()), this, SLOT(chooseDirectory()));

    QHBoxLayout *filePathLayout = new QHBoxLayout;
    filePathLayout->addWidget(new QLabel(tr("Folder: ")));
    filePathLayout->addWidget(pathLine);
    filePathLayout->addWidget(openButton);

    QPushButton *backupButton = new QPushButton(tr("Preserve Recovery Copy"));
    connect(backupButton, SIGNAL(clicked()), this, SLOT(acceptDirectory()));

    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(backupButton, 1, Qt::AlignHCenter);
    buttonLayout->addWidget(closeButton, 1, Qt::AlignHCenter);

    layout->addWidget(label);
    layout->addLayout(filePathLayout);
    layout->addWidget(new TSeparator);
    layout->addLayout(buttonLayout);
}

void TBackupDialog::chooseDirectory()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose a directory..."), pathLine->text(),
                                                      QFileDialog::ShowDirsOnly
                                                      | QFileDialog::DontResolveSymlinks);
    if (!path.isEmpty()) {
        destPath = path;
        pathLine->setText(destPath);
    }
}

void TBackupDialog::acceptDirectory()
{
    destPath = pathLine->text().trimmed();
    if (destPath.isEmpty()) {
        TOsd::self()->display(TOsd::Error, tr("Please, choose a recovery folder."));
        return;
    }

    QDir dir(destPath);
    if (!dir.exists() && !dir.mkpath(".")) {
        TOsd::self()->display(TOsd::Error, tr("The recovery folder cannot be created."));
        return;
    }

    accept();
}

QString TBackupDialog::selectedDirectory() const
{
    return destPath;
}
