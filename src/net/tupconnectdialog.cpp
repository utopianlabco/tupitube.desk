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

#include "tupconnectdialog.h"

TupConnectDialog::TupConnectDialog(QWidget *parent): QDialog(parent)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupConnectDialog::TupConnectDialog()]";
    #endif

    setWindowTitle(tr("Connection Settings"));
    setMinimumWidth(320);

    // Credentials section
    loginLine = new QLineEdit;
    loginLine->setMinimumWidth(200);

    cacheLine = new QLineEdit;
    cacheLine->setEchoMode(QLineEdit::Password);
    cacheLine->setMinimumWidth(200);

    storePasswdBox = new QCheckBox(tr("Store password"));

    QFormLayout *credentialsLayout = new QFormLayout;
    credentialsLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    credentialsLayout->setLabelAlignment(Qt::AlignRight);
    credentialsLayout->setSpacing(10);
    credentialsLayout->addRow(tr("Username:"), loginLine);
    credentialsLayout->addRow(tr("Password:"), cacheLine);
    credentialsLayout->addRow("", storePasswdBox);

    QGroupBox *credentialsGroup = new QGroupBox(tr("Credentials"));
    credentialsGroup->setLayout(credentialsLayout);

    // Server section
    serverLine = new QLineEdit;
    serverLine->setMinimumWidth(200);

    portBox = new QSpinBox;
    portBox->setMinimum(1);
    portBox->setMaximum(65000);
    portBox->setFixedWidth(80);

    QFormLayout *serverLayout = new QFormLayout;
    serverLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    serverLayout->setLabelAlignment(Qt::AlignRight);
    serverLayout->setSpacing(10);
    serverLayout->addRow(tr("Server:"), serverLine);
    serverLayout->addRow(tr("Port:"), portBox);

    QGroupBox *serverGroup = new QGroupBox(tr("Collaboration Server"));
    serverGroup->setLayout(serverLayout);

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    okButton = new QPushButton;
    okButton->setMinimumWidth(60);
    okButton->setIcon(QIcon(THEME_DIR + "icons/apply.png"));
    okButton->setToolTip(tr("Accept"));
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    buttonBox->addButton(okButton, QDialogButtonBox::AcceptRole);

    connect(loginLine, SIGNAL(textChanged(QString)),
            this, SLOT(updateOkButtonState()));
    connect(cacheLine, SIGNAL(textChanged(QString)),
            this, SLOT(updateOkButtonState()));
    connect(serverLine, SIGNAL(textChanged(QString)),
            this, SLOT(updateOkButtonState()));

    QPushButton *cancelButton = new QPushButton;
    cancelButton->setMinimumWidth(60);
    cancelButton->setIcon(QIcon(THEME_DIR + "icons/close.png"));
    cancelButton->setToolTip(tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->addWidget(credentialsGroup);
    mainLayout->addWidget(serverGroup);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    loadSettings();
    updateOkButtonState();
    loginLine->setFocus();
}

TupConnectDialog::~TupConnectDialog()
{
    saveSettings();
}

void TupConnectDialog::setServer(const QString &server)
{
    serverLine->setText(server);
}

void TupConnectDialog::setPort(int port)
{
    portBox->setValue(port);
}

QString TupConnectDialog::login() const
{
    return loginLine->text();
}

QString TupConnectDialog::password() const
{   
    return TupSecurity::encryptPassword(SECRET_KEY);
}

QString TupConnectDialog::server() const
{
    return serverLine->text();
}

int TupConnectDialog::port() const
{
    return portBox->value();
}

QString TupConnectDialog::windowRecordID() const
{
    return cacheLine->text();
}

void TupConnectDialog::loadSettings()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupConnectDialog::loadSettings()]";
    #endif

    TCONFIG->beginGroup("CollabServer");
    QString username = TCONFIG->value("Login", "").toString();  
    serverLine->setText(TCONFIG->value("Server", "").toString());
    portBox->setValue(TCONFIG->value("Port", 8080).toInt());
    loginLine->setText(username);
    storePasswdBox->setChecked(TCONFIG->value("StorePassword", "false").toBool());
    TCONFIG->endGroup();

    if (username.isEmpty() || !storePasswdBox->isChecked()) {
        cacheLine->setText("");
        TAlgorithm::resetCacheRecord();
    } else {
        cacheLine->setText(TAlgorithm::windowRecordID());    
    }
}

void TupConnectDialog::saveSettings()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupConnectDialog::saveSettings()]";
    #endif

    TCONFIG->beginGroup("CollabServer");
    TCONFIG->setValue("Server", serverLine->text());
    TCONFIG->setValue("Port", portBox->value());
    TCONFIG->setValue("Login", loginLine->text());
    
    if (storePasswdBox->isChecked())
        TCONFIG->setValue("Password", TupSecurity::encryptPassword(SECRET_KEY));
    else 
        TCONFIG->setValue("Password", "");
    
    TCONFIG->setValue("StorePassword", storePasswdBox->isChecked());
    TCONFIG->endGroup();
    TCONFIG->sync();

    TAlgorithm::storeRecord(cacheData());
}

void TupConnectDialog::updateOkButtonState()
{
    const bool valid = !loginLine->text().trimmed().isEmpty()
                       && !cacheLine->text().isEmpty()
                       && !serverLine->text().trimmed().isEmpty();

    okButton->setEnabled(valid);
}

void TupConnectDialog::accept()
{
    if (loginLine->text().trimmed().isEmpty()) {
        TOsd::self()->display(TOsd::Error, tr("Please, fill in your username"));
        return;
    }

    if (serverLine->text().trimmed().isEmpty()) {
        TOsd::self()->display(TOsd::Error, tr("Please, fill in the server address"));
        return;
    }

    if (cacheLine->text().isEmpty()) {
        TOsd::self()->display(TOsd::Error, tr("Please, fill in your password"));
        return;
    }

    QDialog::accept();    
}

QString TupConnectDialog::cacheData() const   
{
    return cacheLine->text();
}