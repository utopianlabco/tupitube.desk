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

#include "tupwatermark.h"
#include <QFont>
#include <QGraphicsTextItem>
#include <QRectF>

TupWaterMark::TupWaterMark(QObject *parent) : QObject(parent)
{
}

TupWaterMark::~TupWaterMark()
{
}

QGraphicsTextItem * TupWaterMark::generateWaterMark(const QColor &bgColor, const QSize &size, int zLevel)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupWaterMark::generateWaterMark()]";
    #endif

    int imgW = size.width();
    int imgH = size.height();

    // Use 20% of width for landscape, 30% for portrait
    double wLimit = (imgW > imgH) ? (imgW * 0.20) : (imgW * 0.30);

    QColor fgColor = waterMarkColor(bgColor);
    QGraphicsTextItem *watermark = new QGraphicsTextItem("@tupitube");
    watermark->setDefaultTextColor(fgColor);

    // Robust font fallback
    QFont font("Paytone One, Arial, Helvetica, sans-serif");
    font.setBold(true);

    // Estimate font size efficiently (e.g., ~5% of image height)
    int fontSize = qMax(10, imgH / 20);
    font.setPointSize(fontSize);
    watermark->setFont(font);

    // Refine size if it exceeds the limit
    QRectF rect = watermark->boundingRect();
    while (rect.width() > wLimit && fontSize > 10) {
        fontSize--;
        font.setPointSize(fontSize);
        watermark->setFont(font);
        rect = watermark->boundingRect();
    }

    int x = (imgW - rect.width()) / 2;
    watermark->setPos(x, 0); // Safe Y position
    watermark->setZValue(zLevel);

    return watermark;
}

QColor TupWaterMark::waterMarkColor(const QColor &bgColor)
{
    // Calculate perceived brightness (luminance)
    int luminance = (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) / 1000;

    // Return light gray for dark backgrounds, dark gray for light backgrounds
    return (luminance < 128) ? QColor(255, 255, 255) : QColor(120, 120, 120);
}
