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

#include "tupgeneralpreferences.h"
#include "tconfig.h"
#include "tformfactory.h"
#include "talgorithm.h"
#include "tosd.h"
#include "tseparator.h"
#include "tupsecurity.h"

#include <QToolButton>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <QFormLayout>

TupGeneralPreferences::TupGeneralPreferences()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    tabWidget = new QTabWidget;
    tabWidget->addTab(generalTab(), tr("General"));
    setCachePatterns();
    tabWidget->addTab(cacheTab(), tr("Cache"));
    tabWidget->addTab(classroomTab(), tr("Classroom"));
    tabWidget->addTab(socialTab(), tr("Social Network"));

    layout->addWidget(tabWidget, Qt::AlignLeft);
    layout->addStretch(3);
}

TupGeneralPreferences::~TupGeneralPreferences()
{
}

QGridLayout * TupGeneralPreferences::createForm(const QString &groupName, Group group,
                                                QStringList keys, QStringList labels)
{
    langChanged = false;
    QGridLayout *form = new QGridLayout;
    int total = labels.count();

    TCONFIG->beginGroup(groupName);
    QList<QCheckBox *> list;
    for (int i=0; i<total; i++) {
         bool flag = TCONFIG->value(keys.at(i), true).toBool();
         QCheckBox *check = new QCheckBox(labels.at(i));
         check->setChecked(flag);
         list << check;
         form->addWidget(check, i, 0, Qt::AlignLeft);
    }

    if (group == Startup)
        interfaceList = list;
    else if (group == Confirm)
        confirmList = list;
    else if (group == Player)
        playerList = list;

    return form;
}

QWidget * TupGeneralPreferences::generalTab()
{
    newLang = "";
    interfaceOptions << "OpenLastProject" << "EnableStatistics";

    QStringList labels;
    labels << tr("Always open last project")
           << tr("Allow TupiTube to collect app usage statistics (Anonymous data)");

    QGridLayout *interfaceForm = createForm("General", Startup, interfaceOptions, labels);

    confirmation << "ConfirmRemoveFrame" << "ConfirmRemoveLayer"
                 << "ConfirmRemoveScene" << "ConfirmRemoveObject";

    labels.clear();
    labels << tr("Confirm \"Remove frame\" action") << tr("Confirm \"Remove layer\" action")
           << tr("Confirm \"Remove scene\" action") << tr("Confirm \"Remove object\" action from library");

    QGridLayout *confirmForm = createForm("General", Confirm, confirmation, labels);

    player << "AutoPlay";

    labels.clear();
    labels << tr("Render and play project automatically");

    QGridLayout *playerForm = createForm("AnimationParameters", Player, player, labels);

    QLabel *generalLabel = new QLabel(tr("General Preferences"));
    QFont labelFont = font();
    labelFont.setBold(true);
    labelFont.setPointSize(labelFont.pointSize() + 3);
    generalLabel->setFont(labelFont);

    QLabel *interfaceLabel = new QLabel(tr("Interface"));
    labelFont = font();
    labelFont.setBold(true);
    interfaceLabel->setFont(labelFont);

    saveCheck = new QCheckBox(tr("Enable autosave feature every"));
    saveCheck->setChecked(getAutoSaveFlag());
    connect(saveCheck, SIGNAL(stateChanged(int)), this, SLOT(updateTimeFlag(int)));

    saveCombo = new QComboBox();
    saveTimeList = TCONFIG->timeRanges();
    saveCombo->addItems(saveTimeList);
    if (!saveCheck->isChecked())
        saveCombo->setEnabled(false);
    saveCombo->setCurrentIndex(getAutoSaveTime());
    QLabel *minLabel = new QLabel(tr("minutes"));

    QHBoxLayout *saveLayout = new QHBoxLayout;
    saveLayout->addWidget(saveCheck);
    saveLayout->addWidget(saveCombo);
    saveLayout->addWidget(minLabel);
    saveLayout->addStretch();

    langSupport = TCONFIG->languages();
    QLabel *langLabel = new QLabel(tr("Language:"));
    langCombo = new QComboBox();
    langCombo->addItem("English");
    langCombo->addItem("Español");
    langCombo->addItem("Français");
    langCombo->addItem("Polski");
    langCombo->addItem("Português");
    langCombo->addItem("русский");
    langCombo->addItem("українська");
    langCombo->addItem("简体中文"); // Simplified Chinese
    langCombo->addItem("繁體中文"); // Traditional Chinese

    langCombo->setCurrentIndex(getLangIndex());
    connect(langCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateAppLang(int)));

    QHBoxLayout *langLayout = new QHBoxLayout;
    langLayout->addWidget(langLabel);
    langLayout->addWidget(langCombo);
    langLayout->addStretch();

    QLabel *confirmLabel = new QLabel(tr("Confirmation Dialogs"));
    confirmLabel->setFont(labelFont);

    QLabel *playerLabel = new QLabel(tr("Player"));
    playerLabel->setFont(labelFont);    

    QWidget *widget = new QWidget;
    QVBoxLayout *widgetLayout = new QVBoxLayout(widget);
    widgetLayout->addWidget(generalLabel);
    widgetLayout->addSpacing(15);
    widgetLayout->addWidget(interfaceLabel);
    widgetLayout->addLayout(langLayout);
    widgetLayout->addLayout(saveLayout);
    widgetLayout->addLayout(interfaceForm);
    widgetLayout->addSpacing(15);
    widgetLayout->addWidget(confirmLabel);
    widgetLayout->addLayout(confirmForm);
    widgetLayout->addSpacing(15);
    widgetLayout->addWidget(playerLabel);
    widgetLayout->addLayout(playerForm);

    return widget;
}

QWidget * TupGeneralPreferences::cacheTab()
{
    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(widget);

    QLabel *cacheLabel = new QLabel(tr("Cache Settings"));
    QFont font = this->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 3);
    cacheLabel->setFont(font);

    QString msg = tr("The CACHE path is the folder where TupiTube creates temporary files while you work "
                     "on your animation projects.<br/>"
                     "<b>Note:</b> Don't change this parameter unless you know what you are doing.");
    QLabel *descLabel = new QLabel(msg);

    TCONFIG->beginGroup("General");
    cachePath = TCONFIG->value("Cache").toString();
    cacheLine = new QLineEdit(cachePath);

    QToolButton *openButton = new QToolButton;
    openButton->setIcon(QIcon(THEME_DIR + "icons/open.png"));
    openButton->setToolTip(tr("Choose another path"));
    connect(openButton, SIGNAL(clicked()), this, SLOT(chooseDirectory()));

    QHBoxLayout *filePathLayout = new QHBoxLayout;
    filePathLayout->addWidget(new QLabel(tr("CACHE Path: ")));
    filePathLayout->addWidget(cacheLine);
    filePathLayout->addWidget(openButton);

    QPushButton *restoreButton = new QPushButton(tr("Restore default value"));
    connect(restoreButton, SIGNAL(clicked()), this, SLOT(restoreCachePath()));
    QWidget *restoreWidget = new QWidget;
    QHBoxLayout *restoreLayout = new QHBoxLayout(restoreWidget);
    restoreLayout->addWidget(restoreButton);
    restoreLayout->addStretch();

    layout->addWidget(cacheLabel);
    layout->addSpacing(15);
    layout->addWidget(descLabel);
    layout->addLayout(filePathLayout);
    layout->addWidget(new TSeparator);
    layout->addWidget(restoreWidget);
    layout->addStretch();

    return widget;
}

QWidget * TupGeneralPreferences::classroomTab()
{
    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(widget);

    TCONFIG->beginGroup("CollabServer");
    QString server = TCONFIG->value("Server", "").toString();
    int port = TCONFIG->value("Port", 8080).toInt();
    QString login = TCONFIG->value("Login", "").toString();
    QString password = TCONFIG->value("Password", "").toString();

    storageCacheEdit = new QLineEdit;
    storageCacheEdit->setEchoMode(cacheMode);
    storageCacheEdit->setPlaceholderText(patternText1);

    storageCacheBackupEdit = new QLineEdit;
    storageCacheBackupEdit->setEchoMode(cacheMode);
    storageCacheBackupEdit->setPlaceholderText(patternText2);

    storageCacheMismatchLabel = new QLabel(patternText3);
    QPalette palette = storageCacheMismatchLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::red);
    storageCacheMismatchLabel->setPalette(palette);
    storageCacheMismatchLabel->setVisible(false);

    auto compareCacheValues = [this]() {
        bool mismatch = storageCacheEdit->text() != storageCacheBackupEdit->text();
        bool show = !storageCacheEdit->text().isEmpty() || !storageCacheBackupEdit->text().isEmpty();
        storageCacheMismatchLabel->setVisible(mismatch && show);
    };
    connect(storageCacheEdit, &QLineEdit::textChanged, this, compareCacheValues);
    connect(storageCacheBackupEdit, &QLineEdit::textChanged, this, compareCacheValues);

    QLabel *classroomLabel = new QLabel(tr("Collaborative Credentials"));
    QFont font = this->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 3);
    classroomLabel->setFont(font);

    QLabel *serverLabel = new QLabel(tr("Server Address:"));
    classroomServerEdit = new QLineEdit;
    classroomServerEdit->setText(server);

    QLabel *portLabel = new QLabel(tr("Port:"));
    classroomPortSpin = new QSpinBox;
    classroomPortSpin->setRange(1, 65535);
    classroomPortSpin->setValue(port);

    QLabel *usernameLabel = new QLabel(tr("Username:"));
    classroomUsernameEdit = new QLineEdit;
    classroomUsernameEdit->setText(login);

    QLabel *passwordLabel = new QLabel(tr("Password:"));
    QLabel *confirmPasswordLabel = new QLabel(tr("Confirm Password:"));

    QFormLayout *form = new QFormLayout;
    form->addRow(serverLabel, classroomServerEdit);
    form->addRow(portLabel, classroomPortSpin);
    form->addRow(usernameLabel, classroomUsernameEdit);
    form->addRow(passwordLabel, storageCacheEdit);
    form->addRow(confirmPasswordLabel, storageCacheBackupEdit);
    form->addRow(new QLabel(""), storageCacheMismatchLabel); // empty label for alignment

    QPushButton *resetClassroomButton = new QPushButton(tr("Reset Credentials"));
    connect(resetClassroomButton, SIGNAL(clicked()), this, SLOT(resetClassroomCredentials()));
    QWidget *buttonContainer = new QWidget;
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->addWidget(resetClassroomButton, 0, Qt::AlignLeft);
    buttonLayout->addStretch();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(0);
    buttonContainer->setLayout(buttonLayout);
    form->addRow(buttonContainer, new QLabel(""));

    layout->addWidget(classroomLabel);
    layout->addSpacing(15);
    layout->addLayout(form);
    layout->addStretch();

    return widget;
}

QWidget * TupGeneralPreferences::socialTab()
{
    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(widget);

    storageLevelIDEdit = new QLineEdit();
    storageLevelIDEdit->setEchoMode(cacheMode);
    storageLevelIDEdit->setPlaceholderText(patternText1);

    storageLevelIDBackupEdit = new QLineEdit();
    storageLevelIDBackupEdit->setEchoMode(cacheMode);
    storageLevelIDBackupEdit->setPlaceholderText(patternText2);

    storageLevelIDMismatchLabel = new QLabel(patternText3);
    QPalette palette = storageLevelIDMismatchLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::red);
    storageLevelIDMismatchLabel->setPalette(palette);
    storageLevelIDMismatchLabel->setVisible(false);

    auto compareCacheValuesSocial = [this]() {
        bool mismatch = storageLevelIDEdit->text() != storageLevelIDBackupEdit->text();
        bool show = !storageLevelIDEdit->text().isEmpty() || !storageLevelIDBackupEdit->text().isEmpty();
        storageLevelIDMismatchLabel->setVisible(mismatch && show);
    };
    connect(storageLevelIDEdit, &QLineEdit::textChanged, this, compareCacheValuesSocial);
    connect(storageLevelIDBackupEdit, &QLineEdit::textChanged, this, compareCacheValuesSocial);

    TCONFIG->beginGroup("Website");
    username = TCONFIG->value("Username").toString();
    password = TCONFIG->value("Password").toString();
    bool socialNetAnonymous = TCONFIG->value("Anonymous", false).toBool();

    QLabel *socialLabel = new QLabel(tr("TupiTube Credentials"));

    QFont font = this->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 3);
    socialLabel->setFont(font);

    QLabel *usernameLabel = new QLabel(tr("Username / Email:"));
    socialNetUsernameEdit = new QLineEdit();
    QLabel *passwdLabel = new QLabel(tr("Password:"));
    QLabel *confirmPasswordLabel = new QLabel(tr("Confirm Password:"));

    socialNetUsernameEdit->setText(username);

    QFormLayout *form = new QFormLayout;
    form->addRow(usernameLabel, socialNetUsernameEdit);
    form->addRow(passwdLabel, storageLevelIDEdit);
    form->addRow(confirmPasswordLabel, storageLevelIDBackupEdit);
    form->addRow(new QLabel(""), storageLevelIDMismatchLabel); // empty label for alignment

    QPushButton *resetSocialButton = new QPushButton(tr("Reset Credentials"));
    connect(resetSocialButton, SIGNAL(clicked()), this, SLOT(resetSocialCredentials()));
    QWidget *resetSocialWidget = new QWidget;
    QHBoxLayout *resetSocialLayout = new QHBoxLayout(resetSocialWidget);
    resetSocialLayout->addWidget(resetSocialButton);
    resetSocialLayout->addStretch();

    QWidget *buttonContainer = new QWidget;
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->addWidget(resetSocialButton, 0, Qt::AlignLeft);
    buttonLayout->addStretch();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(0);
    buttonContainer->setLayout(buttonLayout);

    form->addRow(buttonContainer, new QLabel(""));
    socialNetAnonymousCheckbox = new QCheckBox(tr("Enable anonymous mode"));
    socialNetAnonymousCheckbox->setChecked(socialNetAnonymous);

    font.setPointSize(font.pointSize() - 3);
    font.setBold(true);

    QLabel *registerLabel = new QLabel(tr("Don't have a TupiTube account?"));
    registerLabel->setFont(font);

    font.setBold(false);

    QLabel *emailLabel = new QLabel(tr("Email: "));
    emailLabel->setFont(font);
    socialNetEmailEdit = new QLineEdit();
    connect(socialNetEmailEdit, SIGNAL(returnPressed()), this, SLOT(formatEmail()));
    socialNetEmailEdit->setFont(font);

    QHBoxLayout *emailLayout = new QHBoxLayout;
    emailLayout->addWidget(emailLabel);
    emailLayout->addWidget(socialNetEmailEdit);

    socialNetRegisterButton = new QPushButton(tr("Register"));
    connect(socialNetRegisterButton, SIGNAL(clicked()), this, SLOT(sendRegisterRequest()));

    QWidget *registerWidget = new QWidget;
    QHBoxLayout *registerLayout = new QHBoxLayout(registerWidget);
    registerLayout->addWidget(socialNetRegisterButton, 0, Qt::AlignLeft);
    registerLayout->addStretch();
    registerLayout->setContentsMargins(0, 0, 0, 0);
    registerLayout->setSpacing(0);
    registerWidget->setLayout(registerLayout);

    layout->addWidget(socialLabel);
    layout->addSpacing(15);
    layout->addLayout(form);
    layout->addWidget(socialNetAnonymousCheckbox);
    layout->addSpacing(10);
    layout->addWidget(new TSeparator);
    layout->addWidget(registerLabel);
    layout->addLayout(emailLayout);

    QFormLayout *registerForm = new QFormLayout;
    registerForm->addRow(registerWidget, new QLabel(""));
    layout->addLayout(registerForm);
    layout->addStretch();

    return widget;
}

void TupGeneralPreferences::resetSocialCredentials()
{
    socialNetUsernameEdit->clear();
    storageLevelIDEdit->clear();
    storageLevelIDBackupEdit->clear();

    TCONFIG->beginGroup("Website");
    TCONFIG->setValue("Username", "");
    TCONFIG->setValue("Password", "");
    TCONFIG->sync();

    TAlgorithm::resetCacheID();
}

void TupGeneralPreferences::resetClassroomCredentials()
{
    classroomUsernameEdit->clear();
    storageCacheEdit->clear();
    storageCacheBackupEdit->clear();

    TCONFIG->beginGroup("CollabServer");
    TCONFIG->setValue("Login", "");
    TCONFIG->setValue("Password", "");
    TCONFIG->setValue("StorePassword", false);
    TCONFIG->sync();

    TAlgorithm::resetCacheRecord();

    emit requestCloseCollaborativeProject();
}

void TupGeneralPreferences::formatEmail()
{
    QString input = socialNetEmailEdit->text();
    socialNetEmailEdit->setText(input.toLower());
}

void TupGeneralPreferences::sendRegisterRequest()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupGeneralPreferences::sendRegisterRequest()]";
    #endif

    QString email = socialNetEmailEdit->text().toLower();
    if (!email.isEmpty()) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QRegExp mailREX("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
        mailREX.setCaseSensitivity(Qt::CaseInsensitive);
        mailREX.setPatternSyntax(QRegExp::RegExp);
        if (mailREX.exactMatch(email)) {
            socialNetRegisterButton->setEnabled(false);
            socialNetEmailEdit->setText(email);
            QString url = TUPITUBE_URL + QString("/api/?a=register&e=" + email);
            netAccessManager = new QNetworkAccessManager(this);
            connect(netAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(registerAnswer(QNetworkReply*)));
            #ifdef TUP_DEBUG
                qDebug() << "GET request -> " << url;
            #endif
            QNetworkRequest request;
            request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
            request.setUrl(QUrl(url));
            request.setRawHeader("User-Agent", BROWSER_FINGERPRINT);

            QNetworkReply *reply = netAccessManager->get(request);
            connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[TupGeneralPreferences::sendRegisterRequest()] - Error: Invalid email syntax! -> " << email;
            #endif
            socialNetEmailEdit->setText(" " + tr("Email is invalid. Please, fix it!"));
            QTimer::singleShot(2000, this, SLOT(cleanMessage()));
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupGeneralPreferences::sendRegisterRequest()] - Invalid email: field is empty!";
        #endif
        socialNetEmailEdit->setText(" " + tr("Email field is empty. Type one!"));
        QTimer::singleShot(2000, this, SLOT(cleanMessage()));
    }
}

void TupGeneralPreferences::registerAnswer(QNetworkReply *reply)
{
    #ifdef TUP_DEBUG
       qDebug() << "[TupGeneralPreferences::registerAnswer()]";
    #endif

    QByteArray array = reply->readAll();
    QString answer(array);
    if (!answer.isEmpty()) {
        if (answer.compare("FALSE") == 0) {
            #ifdef TUP_DEBUG
                qDebug() << "[TupGeneralPreferences::registerAnswer()] - Error: e-mail already registered! :(";
            #endif
            socialNetEmailEdit->setText(" " + tr("Error: Email already registered!"));
            QTimer::singleShot(2000, this, SLOT(cleanMessage()));
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "[TupGeneralPreferences::registerAnswer()] - URL: " << answer;
            #endif
            if (answer.startsWith("http")) {
                QDesktopServices::openUrl(answer);
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "[TupGeneralPreferences::registerAnswer()] - Error: Invalid register URL! :(";
                #endif
                socialNetEmailEdit->setText(" " + tr("Please contact us at info@tupitube.com"));
                QTimer::singleShot(3000, this, SLOT(cleanMessage()));
            }
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "[TupGeneralPreferences::registerAnswer()] - Error: No data from server! :(";
        #endif
        socialNetEmailEdit->setText(" " + tr("Please contact us at info@tupitube.com"));
        QTimer::singleShot(3000, this, SLOT(cleanMessage()));
    }

    netAccessManager->deleteLater();
    socialNetRegisterButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void TupGeneralPreferences::slotError(QNetworkReply::NetworkError error)
{
    switch (error) {
        case QNetworkReply::HostNotFoundError:
             {
             #ifdef TUP_DEBUG
                 qDebug() << "[TupGeneralPreferences::slotError()] - Network Error: Host not found";
             #endif
             }
        break;
        case QNetworkReply::TimeoutError:
             {
             #ifdef TUP_DEBUG
                 qDebug() << "[TupGeneralPreferences::slotError()] - Network Error: Time out!";
             #endif
             }
        break;
        case QNetworkReply::ConnectionRefusedError:
             {
             #ifdef TUP_DEBUG
                 qDebug() << "[TupGeneralPreferences::slotError()] - Network Error: Connection Refused!";
             #endif
             }
        break;
        case QNetworkReply::ContentNotFoundError:
             {
             #ifdef TUP_DEBUG
                 qDebug() << "[TupGeneralPreferences::slotError()] - Network Error: Content not found!";
             #endif
             }
        break;
        case QNetworkReply::UnknownNetworkError:
        default:
             {
             #ifdef TUP_DEBUG
                 qDebug() << "[TupGeneralPreferences::slotError()] - Network Error: Unknown Network error!";
             #endif
             }
        break;
    }
}

void TupGeneralPreferences::chooseDirectory()
{
    cachePath = QFileDialog::getExistingDirectory(this, tr("Choose a directory..."), QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    if (!cachePath.isEmpty())
        cacheLine->setText(cachePath);
}

void TupGeneralPreferences::restoreCachePath()
{
    cachePath = QDir::tempPath();
    cacheLine->setText(cachePath);

    TCONFIG->beginGroup("General");
    TCONFIG->setValue("Cache", cachePath);
    TCONFIG->sync();
}

bool TupGeneralPreferences::saveValues()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupGeneralPreferences::saveValues()]";
    #endif

    TCONFIG->beginGroup("General");

    int total = interfaceOptions.count();
    for (int i=0; i<total; i++)
         TCONFIG->setValue(interfaceOptions.at(i), interfaceList.at(i)->isChecked());

    total = confirmation.count();
    for (int i=0; i<total; i++)
         TCONFIG->setValue(confirmation.at(i), confirmList.at(i)->isChecked());

    if (newLang.length() > 0)
        TCONFIG->setValue("Language", newLang);

    TCONFIG->setValue("AutoSave", saveCheck->isChecked());
    TCONFIG->setValue("AutoSaveTime", saveCombo->currentText());

    bool changed = false;
    QString data = storageLevelIDEdit->text();
    if (!data.isEmpty()) {
        if (TAlgorithm::cacheIDChanged(data)) {
            changed = true;
            TAlgorithm::storeData(data);
        }
    }

    cachePath = cacheLine->text();
    if (cachePath.isEmpty()) {
        tabWidget->setCurrentIndex(Cache);
        cacheLine->setFocus();
        TOsd::self()->display(TOsd::Error, tr("Cache path is empty. Set a value!"));
        return false;
    } else {
        QDir dir(cachePath);
        if (!dir.exists()) {
            tabWidget->setCurrentIndex(Cache);
            cacheLine->setFocus();
            TOsd::self()->display(TOsd::Error, tr("Cache path doesn't exist. Create it!"));
            return false;
        } else {
            TCONFIG->setValue("Cache", cachePath);
        }
    }

    TCONFIG->beginGroup("Website");
    QString login = socialNetUsernameEdit->text();
    if (!login.isEmpty()) {
        if (login.compare(username) != 0)
            TCONFIG->setValue("Username", login);
    }

    if (changed) {
        if (password.isEmpty())
            TCONFIG->setValue("Password", TupSecurity::encryptPassword(SECRET_KEY));
        TCONFIG->setValue("StorePassword", true);
    }

    bool socialNetAnonymous = false;
    if (socialNetAnonymousCheckbox->isChecked())
        socialNetAnonymous = true;
    TCONFIG->setValue("Anonymous", socialNetAnonymous);

    TCONFIG->beginGroup("AnimationParameters");
    total = player.count();
    for (int i=0; i<total; i++)
         TCONFIG->setValue(player.at(i), playerList.at(i)->isChecked());

    QString recordId = storageCacheEdit->text();
    TCONFIG->beginGroup("CollabServer");
    TCONFIG->setValue("Server", classroomServerEdit->text());
    TCONFIG->setValue("Port", classroomPortSpin->value());
    TCONFIG->setValue("Login", classroomUsernameEdit->text());
    
    if (!recordId.isEmpty()) { 
        TCONFIG->setValue("Password", TupSecurity::encryptPassword(SECRET_KEY));        
        TCONFIG->setValue("StorePassword", true);
        TAlgorithm::storeRecord(recordId);
    }

    TCONFIG->endGroup();
    TCONFIG->sync();

    return true;
}

int TupGeneralPreferences::getLangIndex()
{
    TCONFIG->beginGroup("General");
    QString locale = TCONFIG->value("Language", "en").toString();
    int index = langSupport.indexOf(locale);
    if (index == -1)
        index = langSupport.indexOf("en");

    return index;
}

bool TupGeneralPreferences::getAutoSaveFlag()
{
    TCONFIG->beginGroup("General");
    return TCONFIG->value("AutoSave", "true").toBool();
}

int TupGeneralPreferences::getAutoSaveTime()
{
    TCONFIG->beginGroup("General");
    QString time = TCONFIG->value("AutoSaveTime", "5").toString();
    int index = saveTimeList.indexOf(time);
    if (index == -1)
        index = 5;

    return index;
}

void TupGeneralPreferences::updateAppLang(int index)
{
    langChanged = true;
    newLang = langSupport.at(index);
}

bool TupGeneralPreferences::showWarning()
{
    return langChanged;
}

void TupGeneralPreferences::updateTimeFlag(int status)
{
    bool flag = false;
    if (status == Qt::Checked)
        flag = true;

    saveCombo->setEnabled(flag);
}

void TupGeneralPreferences::setCachePatterns()
{
    patternText1 = tr("Leave empty to keep current");
    patternText2 = tr("Confirm new password");
    patternText3 = tr("Passwords do not match");
    cacheMode = QLineEdit::Password;
}