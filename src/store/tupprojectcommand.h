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

#ifndef TUPPROJECTCOMMAND_H
#define TUPPROJECTCOMMAND_H

#include "tglobal.h"

#include <QUndoCommand>
#include <type_traits>
#include <utility>

class TupProject;
class TupProjectRequest;
class TupPaintAreaEvent;
class TupCommandExecutor;
class TupProjectResponse;

class TUPITUBE_EXPORT TupProjectCommand : public QUndoCommand
{
    public:
        TupProjectCommand(TupCommandExecutor *executor, const TupProjectRequest *event);
        TupProjectCommand(TupCommandExecutor *executor, TupProjectResponse *response);
        ~TupProjectCommand() override;
        
        void redo() override;
        void undo() override;

        bool succeeded() const;
        QString errorCode() const;
        QString commandId() const;
        bool isItemConvert() const;
        bool isItemEditNodes() const;
        bool isUndoBlocked() const;
        bool isRedoBlocked() const;
        void setUndoBlocked(bool blocked);
        void setRedoBlocked(bool blocked);
        void skipNextStackExecution();
        QString eventType() const;
        bool hasAuthoritativeEventPayload() const;
        QString authoritativeEventPayload() const;
        
    private:
        bool executeResponse();
        bool frameCommand();
        bool layerCommand();
        bool sceneCommand();
        bool itemCommand();
        bool libraryCommand();
        bool paintAreaCommand();

        QString actionString(int action) const;
        void initText();
        void resetExecutionResult();
        bool fail(const QString &code);

        template<typename Operation>
        typename std::enable_if<
            std::is_same<decltype(std::declval<Operation>()()), bool>::value,
            bool>::type executeOperation(Operation operation)
        {
            return operation();
        }

        template<typename Operation>
        typename std::enable_if<
            std::is_void<decltype(std::declval<Operation>()())>::value,
            bool>::type executeOperation(Operation operation)
        {
            operation();
            return true;
        }
        
    private:
        TupCommandExecutor *executor;
        TupProjectResponse *response;
        bool executed;
        bool executionSucceeded;
        bool skipStackExecution;
        bool undoBlocked;
        bool redoBlocked;
        QString executionErrorCode;
};

#endif
