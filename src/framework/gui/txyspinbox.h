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

#ifndef TXYSPINBOX_H
#define TXYSPINBOX_H

#include "tglobal.h"
#include "tapplicationproperties.h"
#include "tapplication.h"

#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QSizePolicy>

class T_GUI_EXPORT TXYSpinBox : public QGroupBox
{
    Q_OBJECT

    public:
        TXYSpinBox(const QString &title, const QString &xLabel, const QString &yLabel,
                   QWidget *parent = nullptr);
        ~TXYSpinBox();

        void setSingleStep(int step);
        void setMinimum(int min);
        void setMaximum(int max);
        void setX(int x);
        void setY(int y);
        int x();
        int y();
        bool buttonIsChecked();

    signals:
        void valuesHaveChanged();

    public slots:
        void toggleModify();

    private slots:
        void updateXValue();
        void updateYValue();

    private:
        QLabel *m_textX;
        QLabel *m_textY;
        QSpinBox *m_x;
        QSpinBox *m_y;
        QPushButton *button;
        bool isChecked;
};

#endif
