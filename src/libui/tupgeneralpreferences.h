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

#ifndef TUPGENERALPREFERENCES_H
#define TUPGENERALPREFERENCES_H

#include "tglobal.h"

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class TUPITUBE_EXPORT TupGeneralPreferences : public QWidget
{
    Q_OBJECT

    public:
        enum Group { Startup = 0, Confirm, Player };
        enum GeneralTab { General = 0, Cache };

        TupGeneralPreferences();
        ~TupGeneralPreferences();

        bool saveValues();
        bool showWarning();

    signals:
        void requestCloseCollaborativeProject();

    private slots:
        void updateAppLang(int index);
        void chooseDirectory();
        void restoreCachePath();
        void formatEmail();
        void sendRegisterRequest();
        void registerAnswer(QNetworkReply *reply);
        void slotError(QNetworkReply::NetworkError);
        void updateTimeFlag(int status);
        void resetSocialCredentials();
        void resetClassroomCredentials();

    private:
        QString patternText1;
        QString patternText2;
        QString patternText3;
        QLineEdit::EchoMode cacheMode;
        void setCachePatterns();

        int getLangIndex();
        bool getAutoSaveFlag();
        int getAutoSaveTime();
        QGridLayout * createForm(const QString &group, Group groupTag,
                                 QStringList keys, QStringList labels);

        QWidget * generalTab();
        QWidget * cacheTab();
        QWidget * socialTab();
        QWidget * classroomTab();

        QTabWidget *tabWidget;
        QString cacheID;
        QString cachePath;
        QLineEdit *cacheLine;
        QLineEdit *storageCacheEdit;
        QLineEdit *storageCacheBackupEdit;
        QLabel *storageCacheMismatchLabel;
        QLineEdit *storageLevelIDEdit;
        QLineEdit *storageLevelIDBackupEdit;
        QLabel *storageLevelIDMismatchLabel;

        QStringList interfaceOptions;
        QStringList confirmation;
        QStringList player;

        QComboBox *langCombo;
        QStringList langSupport;
        QString newLang;

        QCheckBox *saveCheck;
        QComboBox *saveCombo;
        QStringList saveTimeList;

        QList<QCheckBox *> interfaceList;
        QList<QCheckBox *> confirmList;
        QList<QCheckBox *> playerList;

        bool langChanged;
  
        // Multiuse variables for social network and classroom settings
        QString username;
        QString password;

        // Classroom settings
        QLineEdit *classroomServerEdit;
        QSpinBox *classroomPortSpin;
        QLineEdit *classroomUsernameEdit;                

        // Social Network settings
        QCheckBox *socialNetAnonymousCheckbox;
        QLineEdit *socialNetUsernameEdit;
        QLineEdit *socialNetEmailEdit;
        QPushButton *socialNetRegisterButton;

        QNetworkAccessManager *netAccessManager;
};

#endif
