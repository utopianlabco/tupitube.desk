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

#include "tupprojectlistdialog.h"
#include "tapptheme.h"

TupProjectListDialog::TupProjectListDialog(int projects, int collabs, const QString &serverName) : QDialog()
{
    setWindowIcon(QIcon(QPixmap(THEME_DIR + "icons/open.png")));
    setWindowTitle(tr("Project List from Server") + " - [ " + serverName  + " ]");
    setStyleSheet(TAppTheme::themeStyles());
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    if (projects > 0) {
        works = tree(true);
        connect(works, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(updateWorkTree()));
        connect(works, SIGNAL(itemSelectionChanged()), this, SLOT(updateWorkTree()));
        connect(works, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(execAccept(QTreeWidgetItem *, int)));
    }

    if (collabs > 0) {
        contributions = tree(false);
        connect(contributions, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(updateContribTree()));
        connect(contributions, SIGNAL(itemSelectionChanged()), this, SLOT(updateContribTree()));
        connect(contributions, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(execAccept(QTreeWidgetItem *, int)));
    }

    QHBoxLayout *search = new QHBoxLayout;
    TreeWidgetSearchLine *searchLine = 0;
    QToolButton *button = new QToolButton;
    button->setIcon(QIcon(THEME_DIR + "icons/zoom.png"));

    QLabel *worksLabel = new QLabel(tr("My works:"));
    QLabel *contribLabel = new QLabel(tr("My contributions:"));

    if (projects > 0 && collabs > 0) {
        QList<QTreeWidget *> trees;
        trees << works << contributions;
        searchLine = new TreeWidgetSearchLine(this, trees);
        search->addWidget(searchLine);
        search->addWidget(button);

        layout->addLayout(search);
        layout->addWidget(worksLabel);
        layout->addWidget(works);
        layout->addWidget(contribLabel);
        layout->addWidget(contributions);
    } else if (projects > 0) {
               searchLine = new TreeWidgetSearchLine(this, works);
               search->addWidget(searchLine);
               search->addWidget(button);

               layout->addLayout(search);
               layout->addWidget(worksLabel);
               layout->addWidget(works);
    } else if (collabs > 0) {
               searchLine = new TreeWidgetSearchLine(this, contributions);
               search->addWidget(searchLine);
               search->addWidget(button);

               layout->addLayout(search);
               layout->addWidget(contribLabel);
               layout->addWidget(contributions);
    }

    connect(button, SIGNAL(clicked()), searchLine, SLOT(clear()));

    //----
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch(1);

    QPushButton *cancelButton = new QPushButton;
    cancelButton->setIcon(QIcon(THEME_DIR + "icons/close.png"));
    cancelButton->setToolTip(tr("Cancel"));
    cancelButton->setMinimumWidth(60);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    okButton = new QPushButton;
    okButton->setIcon(QIcon(THEME_DIR + "icons/apply.png"));
    okButton->setToolTip(tr("OK"));
    okButton->setMinimumWidth(60);
    okButton->setDefault(true);
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    okButton->setEnabled(false); // Initially disabled

    buttons->addWidget(cancelButton);
    buttons->addWidget(okButton);
    layout->addLayout(buttons);

    setMinimumWidth(615); 
    index = 0;
}

TupProjectListDialog::~TupProjectListDialog()
{
}

QTreeWidget *TupProjectListDialog::tree(bool myWorks)
{
    QTreeWidget *tree = new QTreeWidget;
    tree->setFixedHeight(120);
    if (myWorks)
        tree->setHeaderLabels(QStringList() << tr("Name") << tr("Description") << tr("Date"));
    else
        tree->setHeaderLabels(QStringList() << tr("Name") << tr("Author") << tr("Description") << tr("Date"));

    tree->header()->show();

    if (myWorks) {
        tree->setColumnWidth(0, 200);
        tree->setColumnWidth(1, 250);
        tree->setColumnWidth(2, 55);
    } else {
        tree->setColumnWidth(0, 150);
        tree->setColumnWidth(1, 100);
        tree->setColumnWidth(2, 200);
        tree->setColumnWidth(3, 55);
    }

    return tree;
}

void TupProjectListDialog::addWork(const QString &project, const QString &name, const QString &description, const QString &date)
{
    workList.append(project);

    QTreeWidgetItem *item = new QTreeWidgetItem(works);
    item->setText(0, name);
    item->setText(1, description);
    item->setText(2, date);

    if (index == 0) {
        isMine = true;
        works->setCurrentItem(item);
        filename = project;
        // Ensure OK button is enabled when the first item is added
        if (okButton) okButton->setEnabled(true);
    }

    index++;
}

void TupProjectListDialog::addContribution(const QString &filename, const QString &name, const QString &author, const QString &description, const QString &date)
{
    contribList.append(filename);
    authors.append(author);

    QTreeWidgetItem *item = new QTreeWidgetItem(contributions);
    item->setText(0, name);
    item->setText(1, author);
    item->setText(2, description);
    item->setText(3, date);

    // If this is the first contribution and there are no works, select it and enable OK
    if (contribList.size() == 1 && (!works || works->topLevelItemCount() == 0)) {
        isMine = false;
        contributions->setCurrentItem(item);
        this->filename = filename;
        this->user = author;
        if (okButton) okButton->setEnabled(true);
    }
}

QString TupProjectListDialog::projectID() const
{
    return filename;
}

QString TupProjectListDialog::owner() const
{
    return user;
}

void TupProjectListDialog::execAccept(QTreeWidgetItem *item, int index)
{
    Q_UNUSED(item);

    if (index >= 0)
        accept();
}

void TupProjectListDialog::updateWorkTree()
{
    if (works->hasFocus()) {
        if (contribList.size() > 0)
            contributions->clearSelection();
        int index = works->currentIndex().row();
        if (index >= 0 && index < workList.size()) {
            filename = workList.at(index);
            isMine = true;
            okButton->setEnabled(true);
        } else {
            okButton->setEnabled(false);
        }
    }
}

void TupProjectListDialog::updateContribTree()
{
    if (contributions->hasFocus()) {
        if (workList.size() > 0)
            works->clearSelection();
        int index = contributions->currentIndex().row();
        if (index >= 0 && index < contribList.size()) {
            isMine = false;
            filename = contribList.at(index);
            user = authors.at(index);
            okButton->setEnabled(true);
        } else {
            okButton->setEnabled(false);
        }
    }
}

bool TupProjectListDialog::workIsMine()
{
    return isMine;
}

