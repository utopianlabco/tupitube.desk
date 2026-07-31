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

#include "tuplocalprojectmanagerhandler.h"

#include "tupfilemanager.h"
#include "tupprojectrequest.h"

#include <QDebug>

TupLocalProjectManagerHandler::TupLocalProjectManagerHandler(QObject *parent)
    : TupAbstractProjectHandler(parent)
{
}

TupLocalProjectManagerHandler::~TupLocalProjectManagerHandler()
{
}

void TupLocalProjectManagerHandler::handleProjectRequest(
    const TupProjectRequest *request)
{
#ifdef TUP_DEBUG
    qDebug() << "[TupLocalProjectManagerHandler::handleProjectRequest()]";
#endif

    if (!request) {
        qWarning()
            << "[TupLocalProjectManagerHandler::handleProjectRequest()]"
            << "Null request.";
        return;
    }

    if (!request->isValid()) {
#ifdef TUP_DEBUG
        qWarning()
            << "[TupLocalProjectManagerHandler::handleProjectRequest()]"
            << "Invalid request."
            << "Action:" << request->getActionId();
#endif
        return;
    }

    if (request->getCommandId().isEmpty()) {
        qWarning()
            << "[TupLocalProjectManagerHandler::handleProjectRequest()]"
            << "Request has no command ID."
            << "Action:" << request->getActionId();
        return;
    }

#ifdef TUP_DEBUG
    qDebug()
        << "[TupLocalProjectManagerHandler::handleProjectRequest()]"
        << "Executing local command:" << request->getCommandId()
        << "Action:" << request->getActionId();
#endif

    emit sendCommand(request, true);
}

bool TupLocalProjectManagerHandler::saveProject(
    const QString &fileName,
    TupProject *project)
{
#ifdef TUP_DEBUG
    qDebug()
        << "[TupLocalProjectManagerHandler::saveProject()]"
        << "fileName ->" << fileName;
#endif

    QString file = fileName;

    if (!file.endsWith(QStringLiteral(".tup")))
        file += QStringLiteral(".tup");

    TupFileManager *manager = new TupFileManager;

    connect(
        manager,
        SIGNAL(projectPathChanged()),
        this,
        SIGNAL(projectPathChanged()));

    connect(
        manager,
        SIGNAL(soundPathsChanged()),
        this,
        SIGNAL(soundPathsChanged()));

    const bool result = manager->save(file, project);

    disconnect(
        manager,
        SIGNAL(projectPathChanged()),
        this,
        SIGNAL(projectPathChanged()));

    disconnect(
        manager,
        SIGNAL(soundPathsChanged()),
        this,
        SIGNAL(soundPathsChanged()));

    delete manager;

    return result;
}

bool TupLocalProjectManagerHandler::loadProject(
    const QString &fileName,
    TupProject *project)
{
#ifdef TUP_DEBUG
    qDebug()
        << "[TupLocalProjectManagerHandler::loadProject()]"
        << "fileName ->" << fileName;
#endif

    TupFileManager *manager = new TupFileManager;
    const bool result = manager->load(fileName, project);
    delete manager;

    return result;
}

void TupLocalProjectManagerHandler::setProject(TupProject *project)
{
    Q_UNUSED(project)
}
