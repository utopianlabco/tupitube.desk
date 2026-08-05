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

#ifndef TUPPROJECTREQUEST_H
#define TUPPROJECTREQUEST_H

#include "tglobal.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QUuid>

class TupProjectRequest;
class TupProjectResponse;

class TUPITUBE_EXPORT TupProjectRequestArgument
{
    public:
        TupProjectRequestArgument();
        TupProjectRequestArgument(const QString &value);
        ~TupProjectRequestArgument();
        
        void operator = (const QString &value);
        void setValue(const QString &value);
        
        bool toBool();
        int toInt();
        double toReal();
        QString toString();

    private:
        QString m_value;
};

class TUPITUBE_EXPORT TupProjectRequest
{
    public:
        enum Action
        {
            None = 0,
            Add = 1,
            Duplicate = 2,
            Remove = -Add,
            RemoveSelection = 3,
            Reset = 4,
            Exchange = 5,
            Move = 6,
            Lock = 7,
            Rename = 8,
            Select = 9,
            View = 10,
            
            Group = 11,
            Ungroup = -Group,
            
            // Items
            Transform = 12,
            Convert = 13,
            EditNodes = 14,
            Pen = 15,
            Brush = 16,
            TextColor = 17,
            InsertSymbolIntoFrame = 18,
            RemoveSymbolFromFrame = -InsertSymbolIntoFrame,

            SetTween = 19,
            UpdateTween = 20,
            RemoveTween = 21,
            UpdateTweenPath = 22,
            
            // frames
            Update = 23,
            Extend = 24,
            Copy = 25,
            CopySelection = 26,
            Paste = 27,
            PasteSelection = 28,
            ReverseSelection = -PasteSelection,

            // scenes
            GetInfo = 29,
            BgColor = 30,

            // layer
            AddLipSync = 31,
            RemoveLipSync = -AddLipSync,
            UpdateLipSync = 32,
            UpdateOpacity = 33,

            // Raster
            AddRasterItem = 34,
            RemoveRasterItem = -AddRasterItem,
            ClearRasterCanvas = 35,

            // Scene properties
            SetFps = 36
        };

        enum Part
        {
            Project = 1000,
            Scene,
            Layer,
            Frame,
            Item,
            Library
        };
        
        TupProjectRequest(const QString &data = nullptr);
        TupProjectRequest(const TupProjectRequest &request);
        virtual ~TupProjectRequest();
        
        void setActionId(int actionId);
        virtual int getActionId() const;
        virtual bool isValid() const;
        
        QString getXml() const;
        
        void setExternal(bool b);
        bool isRequestExternal() const;
        
        void setCommandId(const QString &commandId);
        QString getCommandId() const;

        void setDependencyCommandId(const QString &commandId);
        QString getDependencyCommandId() const;
        bool hasDependency() const;

        TupProjectRequest &operator = (const TupProjectRequest &other);

    private:
        QString xml;
        int actionId;
        bool isExternal;
        QString commandId;
        QString dependencyCommandId;
};

#endif
