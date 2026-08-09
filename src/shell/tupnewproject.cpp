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

#include "tupnewproject.h"
#include "tupnetprojectmanagerparams.h"

#include "tformfactory.h"
#include "tconfig.h"
#include "tapplication.h"
#include "tosd.h"
#include "tapptheme.h"
#include "tupsecurity.h"
#include "talgorithm.h"

#include <QFormLayout>

//SQA: Add a field to define the project description 

TupNewProject::TupNewProject(QWidget *parent) : TabDialog(parent)
{
    enableUseNetwork = false;

    setWindowIcon(QPixmap(THEME_DIR + "icons/new.png"));
    setWindowTitle(tr("Create New Project"));
    setModal(true);

    setStyleSheet(TAppTheme::themeStyles());

    QFrame *infoContainer = new QFrame();
    QGridLayout *layout = new QGridLayout(infoContainer);

    QLabel *nameLabel = new QLabel(tr("Project Name"), infoContainer);
    layout->addWidget(nameLabel, 0, 0);

    projectName = new QLineEdit(infoContainer);
    projectName->setMaxLength(30);
    projectName->setText(tr("my_project"));
    layout->addWidget(projectName, 0, 1);

    QLabel *authorLabel = new QLabel(tr("Author"), infoContainer);
    layout->addWidget(authorLabel, 1, 0);

    authorName = new QLineEdit(infoContainer);
    authorName->setMaxLength(30);
    authorName->setText(tr("Your name"));
    layout->addWidget(authorName, 1, 1);

    QLabel *descLabel = new QLabel(tr("Description"), infoContainer);
    layout->addWidget(descLabel, 3, 0);

    description = new QLineEdit(infoContainer);
    description->setMaxLength(50);
    description->setText(tr("Just for fun!"));
    layout->addWidget(description, 3, 1);

    connect(projectName, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(authorName, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(description, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));

    QBoxLayout *presetsLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    QLabel *presetsLabel = new QLabel(tr("Presets") + " ");

    TCONFIG->beginGroup("PaintArea");
    int presetIndex = TCONFIG->value("DefaultFormat", FORMAT_1080).toInt();

    presets = new QComboBox();
    presets->addItem(tr("Free Format"));
    presets->addItem(tr("520x380 - 24"));
    presets->addItem(tr("640x480 - 24"));
    presets->addItem(tr("480 (PAL DV/DVD) - 25"));
    presets->addItem(tr("576 (PAL DV/DVD) - 25"));
    presets->addItem(tr("720 (HD) - 24"));
    presets->addItem(tr("1080 (Mobile) - 24"));
    presets->addItem(tr("1080 (Full HD Vertical) - 24"));
    presets->addItem(tr("1080 (Full HD) - 24"));

    connect(presets, SIGNAL(currentIndexChanged(int)), this, SLOT(setPresets(int)));

    presetsLayout->addWidget(presetsLabel);
    presetsLayout->addWidget(presets);
    layout->addLayout(presetsLayout, 4, 0, 1, 2, Qt::AlignCenter);

    QGroupBox *renderAndFps= new QGroupBox(tr("Options"));
	
    QBoxLayout *subLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    renderAndFps->setLayout(subLayout);

    TCONFIG->beginGroup("PaintArea");
    QString colorName = TCONFIG->value("BackgroundDefaultColor", "#ffffff").toString();

    color = QColor(colorName);

    colorButton = new QPushButton();
    colorButton->setText(tr("Background"));
    colorButton->setToolTip(tr("Click here to change background color"));
    colorButton->setStyleSheet("QPushButton { background-color: " + color.name()
                                + "; color: " + labelColor() + "; }");
    connect(colorButton, SIGNAL(clicked()), this, SLOT(setBgColor()));

    QBoxLayout *fpsLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    QLabel *label = new QLabel(tr("FPS"));
    fps = new QSpinBox();
    fps->setValue(24);
    connect(fps, SIGNAL(valueChanged(int)), this, SLOT(updateOkButtonState()));

    fpsLayout->addWidget(label);
    fpsLayout->addWidget(fps);
    subLayout->addWidget(colorButton);
    subLayout->addLayout(fpsLayout);

    size = new TXYSpinBox(tr("Dimension"), tr("X:"), tr("Y:"), infoContainer);
    size->setMinimum(50);
    size->setMaximum(15000);
    size->setX(520);
    size->setY(380);

    connect(size, SIGNAL(valuesHaveChanged()), this, SLOT(updateFormatCombo()));
    connect(size, SIGNAL(valuesHaveChanged()), this, SLOT(updateOkButtonState()));

    QWidget *panel = new QWidget;
    QVBoxLayout *sizeLayout = new QVBoxLayout(panel);
    sizeLayout->addWidget(size);

    layout->addWidget(panel, 5, 0);
    layout->addWidget(renderAndFps, 5, 1);

    QCheckBox *activeNetOptions = new QCheckBox(tr("Collaborative Project"));
    connect(activeNetOptions, SIGNAL(toggled(bool)), this, SLOT(enableNetOptions(bool)));

    layout->addWidget(activeNetOptions, 6, 0, 1, 2, Qt::AlignLeft);

    addTab(infoContainer, tr("Project Info"));

    QFrame *netContainer = new QFrame();
    netLayout = new QBoxLayout(QBoxLayout::TopToBottom, netContainer);
    netLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    setupNetOptions();

    addTab(netContainer, tr("Classroom"));
    enableNetOptions(false);

    if (presetIndex >= 0)
        presets->setCurrentIndex(presetIndex);

    updateOkButtonState();
}

TupNewProject::~TupNewProject()
{
    if (enableUseNetwork) {
        TConfig *config = kApp->config("CollabServer");
        config->setValue("Server", server->text());
        config->setValue("Port", port->value());
        config->setValue("Login", username->text());
        if (storePassword->isChecked()) {
            config->setValue("Password", TupSecurity::encryptPassword(SECRET_KEY));
            config->setValue("StorePassword", "true");
        } else {
            config->setValue("Password", "");
            config->setValue("StorePassword", "false");
        }

        TAlgorithm::storeRecord(cacheData->text());
    }

    delete projectName;
    delete authorName;
    delete description;
    delete colorButton;
    delete fps;
    delete presets;
    delete size;
    delete netOptions;
    delete netLayout;
}

void TupNewProject::setupNetOptions()
{
    username = new QLineEdit;
    server = new QLineEdit;
    port = new QSpinBox;
    port->setMinimum(80);
    port->setMaximum(65000); 
    cacheData = new QLineEdit;

    connect(username, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(server, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(cacheData, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(port, SIGNAL(valueChanged(int)), this, SLOT(updateOkButtonState()));

    TConfig *config = kApp->config("CollabServer");

    server->setText(config->value("Server", "").toString());
    int portValue = config->value("Port", 8080).toInt();
    int storePasswordFlag = TCONFIG->value("StorePassword").toInt();
    // qDebug() << "TupNewProject::setupNetOptions() - portValue: " << portValue;
    if (portValue == 0)
        portValue = 8080;
    port->setValue(portValue);

    username->setText(config->value("Login", "").toString());
    cacheData->setEchoMode(QLineEdit::Password);
    // cacheData->setText(config->value("Password", "").toString());   

    QLabel *infoLabel = new QLabel(tr("This feature allows you to work with other students in your class using the TupiTube server app."));
    infoLabel->setWordWrap(true);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("QLabel { padding: 10px; }");
    netLayout->addWidget(infoLabel);

    netOptions = new QGroupBox(tr("Connection Settings"));
    QVBoxLayout *groupLayout = new QVBoxLayout(netOptions);
    groupLayout->setSpacing(15);
    groupLayout->setContentsMargins(20, 20, 20, 20);
    if (storePasswordFlag)
        cacheData->setText(TAlgorithm::windowRecordID());

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->addRow(tr("Server:"), server);
    formLayout->addRow(tr("Port:"), port);
    formLayout->addRow(tr("Username:"), username);
    formLayout->addRow(tr("Password:"), cacheData);

    groupLayout->addLayout(formLayout);

    storePassword = new QCheckBox(tr("Store password"));
    storePassword->setChecked(storePasswordFlag);
    groupLayout->addWidget(storePassword);

    groupLayout->addStretch();

    netLayout->addWidget(netOptions, 1);
}

TupProjectManagerParams *TupNewProject::parameters()
{
    int w = size->x();
    if (w % 2)
        w++;
    int h = size->y();
    if (h % 2)
        h++;

    if (enableUseNetwork) {
        TupNetProjectManagerParams *params = new TupNetProjectManagerParams;
        params->setProjectName(projectName->text());
        params->setAuthor(authorName->text());
        params->setDescription(description->text());
        params->setBgColor(color);
        const QSize projectSize(w, h);
        params->setDimension(projectSize);
        params->setFPS(fps->value());

        // Network settings
        params->setServer(server->text());
        params->setPort(port->value());
        params->setLogin(username->text());
        params->setWindowRecordID(cacheData->text());

        return params;
    }

    TupProjectManagerParams *params = new TupProjectManagerParams;
    params->setProjectName(projectName->text());
    params->setAuthor(authorName->text());
    params->setDescription(description->text());
    params->setBgColor(color);
    const QSize projectSize(w, h);
    params->setDimension(projectSize);
    params->setFPS(fps->value());

    return params;
}

bool TupNewProject::useNetwork() const
{
    return enableUseNetwork;
}

void TupNewProject::ok()
{
    if (projectName->text().trimmed().isEmpty()) {
        TOsd::self()->display(TOsd::Error, tr("Please, set a name for the project"));
        return;
    }

    if (authorName->text().trimmed().isEmpty()) {
        TOsd::self()->display(TOsd::Error, tr("Please, fill in the author name"));
        return;
    }

    if (description->text().trimmed().isEmpty()) {
        TOsd::self()->display(TOsd::Error, tr("Please, fill in the project description"));
        return;
    }

    if (size->x() <= 0 || size->y() <= 0) {
        TOsd::self()->display(TOsd::Error, tr("Please, set the project dimensions"));
        return;
    }

    if (fps->value() <= 0) {
        TOsd::self()->display(TOsd::Error, tr("Please, set the project FPS"));
        return;
    }

    if (enableUseNetwork) {
        if (server->text().trimmed().isEmpty()) {
            TOsd::self()->display(TOsd::Error, tr("Please, fill in the server name or IP"));
            return;
        }

        if (port->value() <= 0) {
            TOsd::self()->display(TOsd::Error, tr("Please, fill in the server port"));
            return;
        }

        if (username->text().trimmed().isEmpty()) {
            TOsd::self()->display(TOsd::Error, tr("Please, fill in your username"));
            return;
        }

        if (cacheData->text().isEmpty()) {
            TOsd::self()->display(TOsd::Error, tr("Please, fill in your password"));
            return;
        }
    }

    TCONFIG->beginGroup("PaintArea");
    TCONFIG->setValue("BackgroundDefaultColor", color.name());
    TCONFIG->setValue("DefaultFormat", presets->currentIndex());
    TCONFIG->sync();

    TabDialog::ok();
}

void TupNewProject::enableNetOptions(bool isEnabled)
{
    enableUseNetwork = isEnabled;
    tabWidget()->setTabVisible(1, isEnabled);
    updateOkButtonState();
}

void TupNewProject::focusProjectLabel() 
{
    projectName->setFocus();
    projectName->selectAll();
}

void TupNewProject::setBgColor()
{
    color = QColorDialog::getColor(color, this);

    // SQA: what is this?
    // QString labelColorStr = "black";

     if (color.isValid()) {
         colorButton->setText(color.name());
         colorButton->setStyleSheet("QPushButton { background-color: " + color.name()
                                     + "; color: " + labelColor() + "; }");
     } else {
         color = QColor(Qt::white);
         colorButton->setText(tr("White"));
         colorButton->setStyleSheet("QPushButton { background-color: #fff }; color: black;");
     }
}

void TupNewProject::setPresets(int format)
{
    size->blockSignals(true);

    switch(format) {
           case FREE:
           break;
           case FORMAT_520:
           {
               size->setX(520);
               size->setY(380);
               fps->setValue(24);
           }
           break;
           case FORMAT_640:
           {
               size->setX(640);
               size->setY(480);
               fps->setValue(24);
           }
           break;
           case FORMAT_480:
           {
               size->setX(720);
               size->setY(480);
               fps->setValue(25);
           }
           break;
           case FORMAT_576:
           {
               size->setX(720);
               size->setY(576);
               fps->setValue(25);
           }
           break;
           case FORMAT_720:
           {
               size->setX(1280);
               size->setY(720);
               fps->setValue(24);
           }
           break;
           case FORMAT_MOBILE:
           {
               size->setX(1080);
               size->setY(1080);
               fps->setValue(24);
           }
           break;
           case FORMAT_1080_VERTICAL:
           {
               size->setX(1080);
               size->setY(1920);
               fps->setValue(24);
           }
           break;
           case FORMAT_1080:
           {
               size->setX(1920);
               size->setY(1080);
               fps->setValue(24);
           }
           break;
    }

    if (format != FORMAT_MOBILE) {
        if (size->buttonIsChecked())
            size->toggleModify();
    } else {
        if (!size->buttonIsChecked())
            size->toggleModify();
    }

    size->blockSignals(false);
}

QString TupNewProject::login() const
{
    return username->text();
}

void TupNewProject::updateFormatCombo()
{
    presets->blockSignals(true);
    presets->setCurrentIndex(0);
    presets->blockSignals(false);
}

void TupNewProject::updateOkButtonState()
{
    bool valid = !projectName->text().trimmed().isEmpty()
                 && !authorName->text().trimmed().isEmpty()
                 && !description->text().trimmed().isEmpty()
                 && size->x() > 0
                 && size->y() > 0
                 && fps->value() > 0;

    if (valid && enableUseNetwork) {
        valid = !server->text().trimmed().isEmpty()
                && port->value() > 0
                && !username->text().trimmed().isEmpty()
                && !cacheData->text().isEmpty();
    }

    if (QPushButton *okButton = button(TabDialog::Ok))
        okButton->setEnabled(valid);
}

QString TupNewProject::labelColor() const
{
    QString text = "white";
    if (color.red() > 50 && color.green() > 50 && color.blue() > 50)
        text = "black";
    return text;
}
