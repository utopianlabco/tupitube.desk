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

#include "toptionaldialog.h"
#include "tapptheme.h"
#include "tapplicationproperties.h"

#include <QDesktopServices>
#include <QUrl>

TOptionalDialog::TOptionalDialog(const QString &text,const QString &title,
                                 bool showAgainBox, bool showDiscardButton,
                                 bool showPolicyButton, QWidget *parent) : QDialog(parent),
    mainLayout(nullptr),
    buttonLayout(nullptr),
    checkBox(nullptr),
    cancelButton(nullptr),
    okButton(nullptr),
    result(Cancelled),
    buttonMode(IconButtons),
    acceptText(tr("Accept")),
    cancelText(tr("Cancel"))
{
    setStyleSheet(TAppTheme::themeStyles());

    setWindowTitle(title);
    mainLayout = new QVBoxLayout;
    mainLayout->addStretch(10);
    QLabel *label = new QLabel(text, this);
    mainLayout->addWidget(label);
    mainLayout->addStretch(10);
    mainLayout->addWidget(new TSeparator);

    setButtonsPanel(showAgainBox, showDiscardButton, showPolicyButton);
    setLayout(mainLayout);
}

TOptionalDialog::~TOptionalDialog()
{
}

void TOptionalDialog::setButtonsPanel(bool showAgainBox, bool showDiscardButton,
                                      bool showPolicyButton)
{
    buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);

    if (showPolicyButton) {
        QPushButton *policyButton = new QPushButton(this);
        policyButton->setText(" " + tr("View Privacy Policy") + " ");
        connect(policyButton, SIGNAL(clicked()), this, SLOT(openPrivacyPolicyLink()));
        buttonLayout->addWidget(policyButton);
    }

    if (showAgainBox) {
        checkBox = new QCheckBox(tr("Don't show again"));
        buttonLayout->addWidget(checkBox);
    }

    cancelButton = new QPushButton(this);
    cancelButton->setToolTip(cancelText);
    cancelButton->setMinimumWidth(60);
    cancelButton->setIcon(QIcon(THEME_DIR + "icons/close.png"));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(callCancelAction()));
    buttonLayout->addWidget(cancelButton);

    if (showDiscardButton) {
        QPushButton *discardButton = new QPushButton(this);
        discardButton->setToolTip(tr("Discard"));
        discardButton->setMinimumWidth(60);
        discardButton->setIcon(QIcon(THEME_DIR + "icons/delete.png"));
        connect(discardButton, SIGNAL(clicked()), this, SLOT(callDiscardAction()));
        buttonLayout->addWidget(discardButton);
    }

    okButton = new QPushButton(this);
    okButton->setToolTip(acceptText);
    okButton->setMinimumWidth(60);
    okButton->setIcon(QIcon(THEME_DIR + "icons/apply.png"));
    connect(okButton, SIGNAL(clicked()), this, SLOT(callAcceptAction()));
    buttonLayout->addWidget(okButton);

    mainLayout->addLayout(buttonLayout);
}

bool TOptionalDialog::shownAgain()
{
    if (!checkBox)
        return true;

    return !checkBox->isChecked();
}

void TOptionalDialog::callAcceptAction()
{
    result = Accepted;
    accept();
}

void TOptionalDialog::callDiscardAction()
{
    result = Discarded;
    reject();
}

void TOptionalDialog::callCancelAction()
{
    result = Cancelled;
    reject();
}

TOptionalDialog::Result TOptionalDialog::getResult()
{
    return result;
}

void TOptionalDialog::setButtonMode(ButtonMode mode)
{
    buttonMode = mode;
    updateButtonPresentation();
}

void TOptionalDialog::setAcceptText(const QString &text)
{
    acceptText = text;
    updateButtonPresentation();
}

void TOptionalDialog::setCancelText(const QString &text)
{
    cancelText = text;
    updateButtonPresentation();
}

void TOptionalDialog::updateButtonPresentation()
{
    if (cancelButton) {
        cancelButton->setToolTip(cancelText);
        if (buttonMode == TextButtons) {
            cancelButton->setIcon(QIcon());
            cancelButton->setText(cancelText);
        } else {
            cancelButton->setText(QString());
            cancelButton->setIcon(QIcon(THEME_DIR + "icons/close.png"));
        }
    }

    if (okButton) {
        okButton->setToolTip(acceptText);
        if (buttonMode == TextButtons) {
            okButton->setIcon(QIcon());
            okButton->setText(acceptText);
        } else {
            okButton->setText(QString());
            okButton->setIcon(QIcon(THEME_DIR + "icons/apply.png"));
        }
    }
}

void TOptionalDialog::openPrivacyPolicyLink()
{
    QUrl url(QString(PRIVACY_POLICY_URL));
    QDesktopServices::openUrl(url);
}
