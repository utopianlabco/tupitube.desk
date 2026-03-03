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

#include "tviewbutton.h"
#include "toolview.h"

TViewButton::TViewButton(ToolView *toolView, QWidget *parent): QToolButton(parent), m_area(Qt::LeftToolBarArea), m_toolView(toolView)
{
    setText(m_toolView->windowTitle());
    setIcon(m_toolView->windowIcon());

    m_blinkTimer = new QTimer(this);
    m_blinkState = false;
    connect(m_blinkTimer, &QTimer::timeout, this, &TViewButton::toggleBlinkState);
}

TViewButton::~TViewButton()
{
}

void TViewButton::setArea(Qt::ToolBarArea area)
{
    m_area = area;
    update();
}

Qt::ToolBarArea TViewButton::area() const
{
    return m_area;
}

void TViewButton::mousePressEvent(QMouseEvent *event)
{
    m_toolView->setExpandingFlag();
    QToolButton::mousePressEvent(event);
}

void TViewButton::toggleView()
{
    #ifdef TUP_DEBUG
       qDebug() << "[ToolView::toggleView()]";
    #endif

    m_toolView->setUpdatesEnabled(false);
    m_toolView->toggleViewAction()->trigger();
    m_toolView->setUpdatesEnabled(true);
}

ToolView *TViewButton::toolView() const
{
    return m_toolView;
}

void TViewButton::startBlinking()
{
    if (!m_blinkTimer->isActive()) {
        m_originalStyleSheet = styleSheet();
        m_blinkTimer->start(500); // Blink every 500ms
    }
}

void TViewButton::stopBlinking()
{
    if (m_blinkTimer->isActive()) {
        m_blinkTimer->stop();
        m_blinkState = false;
        setStyleSheet(m_originalStyleSheet);
    }
}

bool TViewButton::isBlinking() const
{
    return m_blinkTimer->isActive();
}

void TViewButton::toggleBlinkState()
{
    m_blinkState = !m_blinkState;
    if (m_blinkState) {
        setStyleSheet("background-color: #308cc6; color: white;"); // Blue highlight
    } else {
        setStyleSheet(m_originalStyleSheet);
    }
}
