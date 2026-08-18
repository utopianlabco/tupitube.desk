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

#ifndef TUPPROJECTMANAGER_H
#define TUPPROJECTMANAGER_H

#include "tglobal.h"

#include <QUndoStack>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QSize>
#include <QPointer>

class TupProject;
class TupProjectRequest;
class TupProjectCommand;
class TupProjectManagerParams;
class TupAbstractProjectHandler;
class QUndoStack;
class TupCommandExecutor;
class TupProjectResponse;

class TUPITUBE_EXPORT TupProjectManager : public QObject
{
    Q_OBJECT

    public:
        TupProjectManager(QObject *parent = nullptr);
        virtual ~TupProjectManager();

        void setParams(TupProjectManagerParams *getParams);
        TupProjectManagerParams *getParams() const;

        virtual void setupNewProject();
        virtual void closeProject();

        bool isOpen() const;
        bool projectWasModified() const;
        TupProject *getProject();
        void setHandler(TupAbstractProjectHandler *getHandler, bool isNetworked);
        TupAbstractProjectHandler *getHandler() const;

        void createCommand(TupProjectCommand *command);
        void clearUndoStack();

        virtual bool saveProject(const QString &fileName);
        virtual bool loadProject(const QString &fileName);

        bool isValid() const;
        void setOpen(bool isOpen);

        void updateProjectDimension(const QSize size);

        int framesCount(int sceneIndex);
        void setSceneBgColor(int sceneIndex, const QColor &bgColor);
        QColor getSceneBgColor(int sceneIndex);

    public slots:
        void setModificationStatus(bool changed);
        void beginUndoMacro(const QString &text);
        void endUndoMacro();

    protected slots:
        virtual void handleProjectRequest(const TupProjectRequest *request);
        virtual void handleLocalRequest(const TupProjectRequest *request);
        virtual void createCommand(const TupProjectRequest *request, bool addToStack);

    private slots:
        void emitResponse(TupProjectResponse *response);
        void undo();
        void redo();
        void advanceAuthoritativeConvertRestore(const QString &commandId, bool undoRestore);
        void finishAuthoritativeConvertRestore(const QString &commandId);
        void advanceAuthoritativeEditNodesRestore(const QString &commandId, bool undoRestore);
        void finishAuthoritativeEditNodesRestore(const QString &commandId);
        void advanceAuthoritativeTransformRestore(const QString &commandId, bool undoRestore);
        void finishAuthoritativeTransformRestore(const QString &commandId);
        void markAuthoritativeRestoreConflict(const QString &commandId, bool undoRestore);
        void reconcileAuthoritativeCreatedObjectId(const QString &commandId, const QString &objectId);

    signals:
        void responsed(TupProjectResponse *reponse);
        void requestOpenProject(const QString &filename);
        void projectPathChanged();
        void soundPathsChanged();

    private:
        bool modified;
        int sceneIndex;
        int layerIndex;
        int frameIndex;
        bool isNetworked;
        bool macroInProgress;
        QString pendingConvertRestoreCommandId;
        QString pendingEditNodesRestoreCommandId;
        QString pendingTransformRestoreCommandId;

        TupProject *project;
        QUndoStack *undoStack;
        QPointer<TupAbstractProjectHandler> handler; 
        TupProjectManagerParams *params;
        TupCommandExecutor *commandExecutor;
};

#endif
