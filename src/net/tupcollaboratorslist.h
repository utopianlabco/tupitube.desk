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

#ifndef TUPCOLLABORATORSLIST_H
#define TUPCOLLABORATORSLIST_H

#include "tglobal.h"

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QMap>

class TUPITUBE_EXPORT TupCollaboratorsList : public QWidget
{
    Q_OBJECT
    
    public:
        TupCollaboratorsList(QWidget *parent = nullptr);
        ~TupCollaboratorsList();
        
        void setCurrentUser(const QString &username);
        void updateUserStatus(const QString &login, int state);
        void setInitialUsers(const QStringList &users);
        void clear();
        
    private:
        void updateUserItem(const QString &login, bool online);
        QIcon createStatusIcon(bool online);
        
        QListWidget *userListWidget;
        QMap<QString, QListWidgetItem*> userItems;
        QString currentUsername;
};

#endif
