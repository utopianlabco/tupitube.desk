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
#include "tosd.h"
#include "tapptheme.h"

TupPreferencesDialog::TupPreferencesDialog(QWidget *parent) : TConfigurationDialog(parent)
{
    setWindowTitle(tr("TupiTube Preferences"));

    general = new TupGeneralPreferences;
    addPage(general, tr("General"), QPixmap(THEME_DIR + "icons/tupi_general_preferences.png"));

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
        if (general->showWarning() || theme->showWarning())
            TOsd::self()->display(TOsd::Warning, tr("Please restart TupiTube"));
        else
            TOsd::self()->display(TOsd::Info, tr("Preferences saved successfully"));
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
        qDebug() << "[TupPreferencesDialog::testThemeColor()] - color ->" << color;
    #endif

    QString r = QString::number(color.red());
    QString g = QString::number(color.green());
    QString b = QString::number(color.blue());
    QString bgColor = "rgb(" + r + "," + g + "," + b + ")";

    QString theme = "default";
    if (appTheme == 0)
        theme = "dark";

    QString uiStyleSheet = TAppTheme::themeStyles(theme, bgColor);
    setStyleSheet(uiStyleSheet);

    /*
    QString listWidget = "QListWidget { background-color: rgb(220,220,220) }";

    QString uiStyleSheet = "";
    if (appTheme == LIGHT_THEME) {
        uiStyleSheet = "QWidget { background-color: " + bgColor + "; }" +
                       listWidget;
    } else {
        uiStyleSheet = TAppTheme::themeSettings();


        uiStyleSheet = "QWidget { background-color: " + bgColor + "; color: #ffffff; }" +
                       listWidget;
    }
    setStyleSheet(uiStyleSheet);
    */
}
