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

#ifndef TUPPROJECTLISTDIALOG_H
#define TUPPROJECTLISTDIALOG_H

#include "tglobal.h"
#include "tapplicationproperties.h"
#include "treelistwidget.h"
#include "treewidgetsearchline.h"

#include <QDialog>
#include <QTreeWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QTreeWidgetItem>
#include <QIcon>

class TUPITUBE_EXPORT TupProjectListDialog : public QDialog
{
    Q_OBJECT

    public:
        TupProjectListDialog(int works, int contributions, const QString &serverName);
        ~TupProjectListDialog();

        void addWork(const QString &filename, const QString &name, const QString &desc, const QString &date);
        void addContribution(const QString &project, const QString &name, const QString &author, const QString &desc,
                             const QString &date);
        QString projectID() const;
        QString owner() const;
        bool workIsMine();
        
    private slots:
        void execAccept(QTreeWidgetItem *item, int index);
        void updateWorkTree();
        void updateContribTree();
    
    private:
        QTreeWidget *tree(bool myWorks);

        QTreeWidget *works;
        QTreeWidget *contributions;
        QPushButton *okButton; // Store OK button for enabling/disabling
        QList<QString> workList;
        QList<QString> contribList;
        QList<QString> authors;
        int index;
        QString filename;
        QString user;
        bool isMine;
};

#endif
