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
    QString m_errorString;
};

#endif
