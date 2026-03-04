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

#include "tupcollaboratorslist.h"

#include <QPainter>

TupCollaboratorsList::TupCollaboratorsList(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    
    QLabel *titleLabel = new QLabel(tr("Collaborators"));
    titleLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    layout->addWidget(titleLabel);
    
    userListWidget = new QListWidget;
    userListWidget->setMaximumWidth(150);
    userListWidget->setMinimumWidth(120);
    userListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    userListWidget->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(userListWidget);
    
    setMaximumWidth(160);
}

TupCollaboratorsList::~TupCollaboratorsList()
{
}

void TupCollaboratorsList::setCurrentUser(const QString &username)
{
    currentUsername = username;
}

void TupCollaboratorsList::setInitialUsers(const QStringList &users)
{
    clear();
    for (const QString &user : users) {
        // Skip the current user - only show partners
        if (user != currentUsername)
            updateUserItem(user, false); // Start as offline, will update when they connect
    }
}

void TupCollaboratorsList::updateUserStatus(const QString &login, int state)
{
    // Skip the current user - only show partners
    if (login == currentUsername)
        return;
        
    bool online = (state == 1);
    updateUserItem(login, online);
}

void TupCollaboratorsList::updateUserItem(const QString &login, bool online)
{
    if (userItems.contains(login)) {
        // Update existing user
        QListWidgetItem *item = userItems[login];
        item->setIcon(createStatusIcon(online));
    } else {
        // Add new user
        QListWidgetItem *item = new QListWidgetItem(createStatusIcon(online), login);
        userListWidget->addItem(item);
        userItems[login] = item;
    }
}

QIcon TupCollaboratorsList::createStatusIcon(bool online)
{
    QPixmap pixmap(12, 12);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (online) {
        // Green circle for online
        painter.setBrush(QColor(0, 180, 0));
        painter.setPen(QColor(0, 140, 0));
    } else {
        // Gray circle for offline
        painter.setBrush(QColor(128, 128, 128));
        painter.setPen(QColor(100, 100, 100));
    }
    
    painter.drawEllipse(1, 1, 10, 10);
    painter.end();
    
    return QIcon(pixmap);
}

void TupCollaboratorsList::clear()
{
    userListWidget->clear();
    userItems.clear();
}
