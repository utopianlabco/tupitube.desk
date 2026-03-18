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

#include "tuppreferencesdialog.h"
#include "tapplicationproperties.h"
#include "talgorithm.h"
#include "tosd.h"
#include "tapptheme.h"

#include <QMessageBox>

TupPreferencesDialog::TupPreferencesDialog(QWidget *parent) : TConfigurationDialog(parent)
{
    setWindowTitle(tr("TupiTube Preferences"));

    general = new TupGeneralPreferences;
    addPage(general, tr("General"), QPixmap(THEME_DIR + "icons/tupi_general_preferences.png"));

    // Connect the requestCloseCollaborativeProject signal to the main window slot if possible
    if (parent) {
        // Try to connect if parent is a TupMainWindow
        QObject *mainWindow = parent;
        QObject::connect(general, SIGNAL(requestCloseCollaborativeProject()), mainWindow, SLOT(closeCollaborativeProjectIfOpen()));
    }

    theme = new TupThemePreferences;
    connect(theme, SIGNAL(colorPicked(int, const QColor&)),
            this, SLOT(testThemeColor(int, const QColor&)));

    addPage(theme, tr("Theme"), QPixmap(THEME_DIR + "icons/tupi_theme_preferences.png"));

    workspace = new TupPaintAreaPreferences;
    addPage(workspace, tr("Workspace"), QIcon(THEME_DIR + "icons/tupi_workspace_preferences.png"));

    setCurrentItem(General);
}

TupPreferencesDialog::~TupPreferencesDialog()
{
}

void TupPreferencesDialog::apply()
{
    if (general->saveValues()) {
        theme->saveValues();
        workspace->saveValues();
        if (general->showWarning() || theme->showWarning()) {
            QMessageBox msgBox;

            // Use the currently selected theme parameters for styling
            QString themeName = "default";
            if (theme->getAppTheme() == DARK_THEME)
                themeName = "dark";
            QString uiStyleSheet = TAppTheme::themeStyles(themeName, theme->getCurrentColor());
            msgBox.setStyleSheet(uiStyleSheet);

            msgBox.setWindowTitle(tr("Preferences Saved"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("Please restart TupiTube to apply the changes."));
            msgBox.addButton(QString(tr("OK")), QMessageBox::AcceptRole);
            msgBox.show();

            QPair<int, int> dimension = TAlgorithm::screenDimension();
            int screenWidth = dimension.first;
            int screenHeight = dimension.second;
            msgBox.move(static_cast<int>((screenWidth - msgBox.width()) / 2),
                        static_cast<int>((screenHeight - msgBox.height()) / 2));

            msgBox.exec();
        } else {
            TOsd::self()->display(TOsd::Info, tr("Preferences saved successfully"));
        }
        accept();
    }
}

QSize TupPreferencesDialog::sizeHint() const
{
    return QSize(600, 430);
}

void TupPreferencesDialog::testThemeColor(int appTheme, const QColor &color)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupPreferencesDialog::testThemeColor()] - appTheme ->" << appTheme;
        qDebug() << "[TupPreferencesDialog::testThemeColor()] - color ->" << color.name();
    #endif

    QString theme = "default";
    if (appTheme == 0)
        theme = "dark";

    QString uiStyleSheet = TAppTheme::themeStyles(theme, color);
    setStyleSheet(uiStyleSheet);
}
