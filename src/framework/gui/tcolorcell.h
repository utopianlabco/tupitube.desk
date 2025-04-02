/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              * 
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Utopian Lab Development Team                                             *
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

#ifndef TCOLORCELL_H
#define TCOLORCELL_H

#include "tglobal.h"

#include <QBrush>
#include <QPainter>
#include <QPaintEvent>
#include <QSize>
#include <QWidget>

class TUPITUBE_EXPORT TColorCell : public QWidget
{
    Q_OBJECT

    public:
        enum FillType {None, Contour = 0, Inner, Background, Basic, PreviousFrames, NextFrames, Layers};
        TColorCell(FillType typeIndex, const QBrush &b, const QSize &dimension);
        ~TColorCell();

        QSize sizeHint() const;
        QBrush brush();
        QColor color();
        void setEnabled(bool isEnabled);
        void setChecked(bool isChecked);
        bool isChecked();
        void setBrush(const QBrush &b);
        void click();

    protected:
        void paintEvent(QPaintEvent *painter);
        void mousePressEvent(QMouseEvent *event);

    signals:
        void clicked(TColorCell::FillType index);

    private:
        bool checked;
        bool enabled;
        FillType index;
        QBrush cellBrush;
        QSize size;
        int uiTheme;
};

#endif
