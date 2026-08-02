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

#ifndef TUPCOMMANDCOORDINATOR_H
#define TUPCOMMANDCOORDINATOR_H

#include "tglobal.h"
#include "tupprojectrequest.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class TUPITUBE_EXPORT TupCommandCoordinator : public QObject
{
    Q_OBJECT

public:
    enum ResultStatus
    {
        InvalidStatus = 0,
        Committed,
        Rejected,
        Failed
    };

    explicit TupCommandCoordinator(QObject *parent = nullptr);
    ~TupCommandCoordinator() override;

    bool registerDependency(
        const QString &prerequisiteCommandId,
        const TupProjectRequest &dependentRequest);

    bool hasPendingCommands(const QString &prerequisiteCommandId) const;
    int pendingCommandCount(const QString &prerequisiteCommandId) const;
    int totalPendingCommandCount() const;
    bool isEmpty() const;

public slots:
    void handleCommandResult(
        const QString &commandId,
        const QString &status,
        const QString &errorCode = QString(),
        const QString &message = QString());

    void clear();

signals:
    // This signal is intended to be connected directly to
    // TupProjectManager::handleProjectRequest(const TupProjectRequest *).
    // The request pointer is valid only for the duration of signal delivery.
    void commandReady(const TupProjectRequest *request);

    void commandCancelled(
        const QString &dependentCommandId,
        const QString &prerequisiteCommandId,
        const QString &errorCode,
        const QString &message);

    void dependencyResolved(
        const QString &prerequisiteCommandId,
        int releasedCommandCount);

    void dependencyRejected(
        const QString &prerequisiteCommandId,
        int cancelledCommandCount,
        const QString &errorCode,
        const QString &message);

private:
    static ResultStatus statusFromString(const QString &status);

    void releaseDependentCommands(const QString &prerequisiteCommandId);
    void cancelDependentCommands(
        const QString &prerequisiteCommandId,
        const QString &errorCode,
        const QString &message);

private:
    QHash<QString, QList<TupProjectRequest>> m_pendingCommands;
};

#endif
