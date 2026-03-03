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

#ifndef TOOLVIEW_H
#define TOOLVIEW_H

#include "tglobal.h"
#include "tviewbutton.h"

#include <QDockWidget>
#include <QIcon>
#include <QKeySequence>

class T_GUI_EXPORT ToolView: public QDockWidget
{
    Q_OBJECT

    public:
        ToolView(const QString &title, const QIcon &icon = QIcon(), const QString &code = QString(), QWidget *parent = nullptr);
        virtual ~ToolView();

        void setShortcut(QKeySequence shortcut);

        TViewButton *button() const;
        void enableButton(bool flag);

        void expandDock(bool flag);
        bool isExpanded();
        void setExpandingFlag();

        void setPerspective(UIView wsp);
        UIView perspective() const;

        void startBlinking();
        void stopBlinking();
        bool isBlinking() const;

    public slots:
        void expandDock();

    private:        
        TViewButton *currentButton;
        UIView currentPerspective;
        bool expanded;
};

#endif
