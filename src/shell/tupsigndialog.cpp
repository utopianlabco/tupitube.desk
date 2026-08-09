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

#include "tupsigndialog.h"
#include "tconfig.h"
#include "tformfactory.h"
#include "talgorithm.h"
#include "tapplication.h"
#include "tosd.h"
#include "tupsecurity.h"
#include "tseparator.h"

#include <QDesktopServices>

TupSignDialog::TupSignDialog(QWidget *parent) : QDialog(parent)
{
    setModal(true);
    setWindowIcon(QPixmap(THEME_DIR + "icons/social_network.png"));
    setWindowTitle(tr("Sign In"));

    setForm();
}

TupSignDialog::~TupSignDialog()
{
}

void TupSignDialog::setForm()
{
    layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    username = new QLineEdit;
    username->setMinimumWidth(200);
    TCONFIG->beginGroup("Website");
    username->setText(TCONFIG->value("Username", "").toString());

    metadata = new QLineEdit;
    metadata->setMinimumWidth(200);
    metadata->setEchoMode(QLineEdit::Password);

    QWidget *form = new QWidget;
    QVBoxLayout *formLayout = new QVBoxLayout(form);
    formLayout->addLayout(TFormFactory::makeGrid(QStringList() << tr("Username") << tr("Password"),
                          QWidgetList() << username << metadata), Qt::AlignHCenter);

    storeMetadata = new QCheckBox(tr("Store password"));
    storeMetadata->setChecked(TCONFIG->value("StorePassword").toBool());
    formLayout->addWidget(storeMetadata);

    // Add 'Post as anonymous' checkbox
    anonymousBox = new QCheckBox(tr("Post as anonymous"));
    anonymousBox->setChecked(TCONFIG->value("Anonymous", false).toBool());
    formLayout->addWidget(anonymousBox);

    // Connect to slot to enable/disable fields and refresh form validity
    connect(anonymousBox, &QCheckBox::toggled, this, [=](bool checked) {
        username->setDisabled(checked);
        metadata->setDisabled(checked);
        storeMetadata->setDisabled(checked);
        updateOkButtonState();
    });

    connect(username, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));
    connect(metadata, SIGNAL(textChanged(QString)), this, SLOT(updateOkButtonState()));

    // Set initial state
    bool anonChecked = anonymousBox->isChecked();
    username->setDisabled(anonChecked);
    metadata->setDisabled(anonChecked);
    storeMetadata->setDisabled(anonChecked);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    QPushButton *signUpButton = new QPushButton(tr("Sign Up"));
    connect(signUpButton, SIGNAL(clicked()), this, SLOT(signUp()));
    buttonLayout->addWidget(signUpButton);

    acceptButton = new QPushButton;
    acceptButton->setMinimumWidth(60);
    acceptButton->setIcon(QIcon(THEME_DIR + "icons/apply.png"));
    acceptButton->setToolTip(tr("Accept"));
    connect(acceptButton, SIGNAL(clicked()), this, SLOT(apply()));
    buttonLayout->addWidget(acceptButton);

    QPushButton *cancelButton = new QPushButton;
    cancelButton->setMinimumWidth(60);
    cancelButton->setIcon(QIcon(THEME_DIR + "icons/close.png"));
    cancelButton->setToolTip(tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(form, Qt::AlignHCenter);
    layout->addWidget(new TSeparator(Qt::Horizontal));
    layout->addLayout(buttonLayout);

    updateOkButtonState();
}

void TupSignDialog::updateOkButtonState()
{
    const bool valid = anonymousBox->isChecked()
                       || (!username->text().trimmed().isEmpty()
                           && !metadata->text().isEmpty());

    acceptButton->setEnabled(valid);
}

void TupSignDialog::signUp()
{
    QUrl url(TUPITUBE_URL);
    QDesktopServices::openUrl(url);
}

void TupSignDialog::apply()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupSignDialog::apply()]";
    #endif

    bool isAnonymous = anonymousBox->isChecked();
    if (!isAnonymous) {
        if (username->text().isEmpty()) {
            TOsd::self()->display(TOsd::Error, tr("Please, fill in your username"));
            return;
        }
        if (metadata->text().isEmpty()) {
            TOsd::self()->display(TOsd::Error, tr("Please, fill in your password"));
            return;
        }

        // Saving credentials
        TCONFIG->beginGroup("Website");
        TCONFIG->setValue("Username", username->text());
        if (storeMetadata->isChecked()) {
            TCONFIG->setValue("Password", TupSecurity::encryptPassword(SECRET_KEY));
            TCONFIG->setValue("StorePassword", "true");
        } else {
            TCONFIG->setValue("Password", "");
            TCONFIG->setValue("StorePassword", "false");
        }

        TCONFIG->setValue("Anonymous", "false");
        TCONFIG->endGroup();
        TCONFIG->sync();

        // Storing cache settings
        QString data = metadata->text();
        TAlgorithm::storeData(data);
    } else {
        TCONFIG->beginGroup("Website");
        TCONFIG->setValue("Username", "");
        TCONFIG->setValue("Password", "");
        TCONFIG->setValue("Anonymous", "true");
        TCONFIG->setValue("StorePassword", "true");
        TCONFIG->endGroup();
        TCONFIG->sync();
    }

    accept();
}

bool TupSignDialog::isAnonymous() const
{
    return anonymousBox ? anonymousBox->isChecked() : false;
}

QString TupSignDialog::getUsername() const
{
    return username->text();
}

QString TupSignDialog::getMetadata() const
{
    return metadata->text();
}
