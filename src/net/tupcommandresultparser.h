/***************************************************************************
 *   Project TupiTube Desk                                                 *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 ***************************************************************************/

#ifndef TUPCOMMANDRESULTPARSER_H
#define TUPCOMMANDRESULTPARSER_H

#include "tglobal.h"

#include <QString>

class TUPITUBE_EXPORT TupCommandResultParser
{
public:
    enum Status
    {
        Invalid = 0,
        Committed,
        Rejected,
        Failed
    };

    TupCommandResultParser();
    explicit TupCommandResultParser(const QString &xml);
    ~TupCommandResultParser();

    bool parse(const QString &xml);

    int version() const;
    QString commandId() const;
    Status status() const;
    QString errorCode() const;
    QString message() const;
    qint64 committedRevision() const;
    int eventIndex() const;
    QString eventType() const;
    QString authoritativePayload() const;
    QString errorString() const;

private:
    void reset();
    static Status statusFromString(const QString &value);

private:
    int m_version;
    QString m_commandId;
    Status m_status;
    QString m_errorCode;
    QString m_message;
    qint64 m_committedRevision;
    int m_eventIndex;
    QString m_eventType;
    QString m_authoritativePayload;
    QString m_errorString;
};

#endif
