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

#ifndef INFOPANEL_H
#define INFOPANEL_H

#include "tglobal.h"
#include "tuppenthicknesswidget.h"

#include <QLabel>
#include <QBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>

/**
 * @author Gustav Gonzalez 
*/

class TUPITUBE_PLUGIN GeometricSettings : public QWidget
{
    Q_OBJECT

    public:
        enum ToolType { Rectangle = 1, Ellipse, Line, Triangle, Hexagon };
        enum LineType { Bendable = 0, Straight };
        enum TriangleType { Top = 0, Bottom, Left, Right, TopLeft, TopRight, BottomLeft, BottomRight };
        enum HexagonType { Horizontal = 0, Vertical };

        GeometricSettings(GeometricSettings::ToolType type, QWidget *parent = nullptr);
        ~GeometricSettings();

        void updateLineType(int type);

    signals:
        void toolEnabled(ToolMode tool);
        void eraserSizeChanged(int size);
        void lineTypeChanged(GeometricSettings::LineType type);
        void triangleTypeChanged(GeometricSettings::TriangleType type);
        void hexagonTypeChanged(GeometricSettings::HexagonType type);

    public slots:
        void enableLineMode();
        void enableEraserMode();
        void switchMode();

    private slots:
        void sendLineState(bool state);
        void setTriangleDirection();
        void setHexagonDirection();
        void updateEraserSizeFromSlider(int value);
        void updateEraserSizeFromBox(int size);

    private:
        QRadioButton *option1;
        QRadioButton *option2;

        QButtonGroup* buttonsGroup;
        TriangleType triangleType;
        HexagonType hexagonType;

        QWidget *lineWidget;
        QWidget *eraserWidget;

        QPushButton *lineButton;
        QPushButton *eraserButton;

        TupPenThicknessWidget *eraserPreview;
        QSlider *eraserSlider;
        QSpinBox *eraserSizeBox;
};

#endif
