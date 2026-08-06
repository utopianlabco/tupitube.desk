/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef TUPCOMMANDTRACKER_H
#define TUPCOMMANDTRACKER_H

#include "tglobal.h"
#include "tupprojectrequest.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class TUPITUBE_EXPORT TupCommandTracker : public QObject
{
    public:
        explicit TupCommandTracker(QObject *parent = nullptr);
        ~TupCommandTracker() override;

        bool track(const TupProjectRequest &request);
        bool contains(const QString &commandId) const;
        int pendingCount() const;

        QString commandXml(const QString &commandId) const;
        int retryCount(const QString &commandId) const;
        qint64 lastSentAt(const QString &commandId) const;

        QList<QString> expiredCommandIds(qint64 timeoutMs) const;
        bool markRetried(const QString &commandId);

        bool complete(const QString &commandId);
        void clear();

    private:
        struct PendingCommand
        {
            QString commandId;
            QString xml;
            qint64 lastSentAt = 0;
            int retryCount = 0;
        };

        QHash<QString, PendingCommand> m_pendingCommands;
};

#endif // TUPCOMMANDTRACKER_H
