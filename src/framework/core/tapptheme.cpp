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

#include "tapptheme.h"
#include "tconfig.h"
#include "tapplicationproperties.h"

QString TAppTheme::themeStyles()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TAppTheme::themeStyles()] - Loading ui.qss ->" << THEME_DIR + "config/ui.qss";
    #endif

    QString themeStyles = "";
    TCONFIG->beginGroup("Theme");
    QString bgColor = TCONFIG->value("BgColor", "#a0a0a0").toString();

    QFile file(THEME_DIR + "config/ui.qss");
    if (file.exists()) {
        file.open(QFile::ReadOnly);
        themeStyles = QLatin1String(file.readAll());
        if (themeStyles.length() == 0) {
        #ifdef TUP_DEBUG
            qWarning() << "[TAppTheme::themeStyles()] - Fatal Error: Theme settings input is empty!";
        #endif
        }
        file.close();
    } else {
        #ifdef TUP_DEBUG 
            qWarning() << "[TAppTheme::themeStyles()] - "
                          "Fatal Error: Theme file doesn't exist ->" << QString(THEME_DIR + "config/ui.qss");
        #endif
    }

    return themeStyles.replace("BG_PARAM", bgColor);
}

QString TAppTheme::themeStyles(const QString &theme, const QString &bgColor)
{
    QString themePath = kAppProp->shareDir() + "themes/" + theme + "/config/ui.qss";
    #ifdef TUP_DEBUG
        qDebug() << "[TAppTheme::themeStyles()] - Testing ui.qss ->" << themePath;
    #endif

    QString themeStyles = "";
    QFile file(themePath);
    if (file.exists()) {
        file.open(QFile::ReadOnly);
        themeStyles = QLatin1String(file.readAll());
        if (themeStyles.length() == 0) {
            #ifdef TUP_DEBUG
                qWarning() << "[TAppTheme::themeStyles()] - Fatal Error: Theme settings input is empty!";
            #endif
        }
        file.close();
    } else {
        #ifdef TUP_DEBUG
            qWarning() << "[TAppTheme::themeStyles()] - "
                          "Fatal Error: Theme file doesn't exist -> " << themePath;
        #endif
    }

    return themeStyles.replace("BG_PARAM", bgColor);
}
