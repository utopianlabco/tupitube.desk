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

#ifndef TUPTHEMEPREFERENCES_H
#define TUPTHEMEPREFERENCES_H

#include "tglobal.h"
#include "tcolorbutton.h"
#include "tconfig.h"
#include "tradiobutton.h"
#include "tupcolorbutton.h"
#include "tslider.h"

#include <QHBoxLayout>

class TUPITUBE_EXPORT TupThemePreferences : public QWidget
{
    Q_OBJECT

    public:
        TupThemePreferences(QWidget *parent = nullptr);
        ~TupThemePreferences();

        void saveValues();
        bool showWarning();
        int getAppTheme() const;
        QColor getCurrentColor() const;

    signals:
        void colorPicked(int appTheme, const QColor&);

    private slots:
        void updateCurrentRow(int row);
        void updateCurrentColor(const QColor &color);
        void restoreDefaultTheme();

    private:
        enum ThemeColors {
            Gray=0, Brown, Chocolate, Blue, Honey, Green, Violet, Orange, Black
        };

        void setupPage();
        // void addColorEntry(int theme, int id, const QString &label, const QColor &initColor,
        //                    const QColor &endColor);
        void addColorEntry(int id, const QString &label, const QColor &initColor,
                           const QColor &endColor);
        void updateUITheme(int row);

        QGridLayout *lightFormLayout;
        QGridLayout *darkFormLayout;
        QList<TRadioButton *> radioList;
        QList<TupColorButton *> cellList;
        QList<TSlider *> sliderList;

        int currentColorRow;
        int oldColorRow;
        int colorPos;
        QColor currentColor;
        QColor oldColor;
        int appTheme;
        bool themeChanged;        
};

#endif
