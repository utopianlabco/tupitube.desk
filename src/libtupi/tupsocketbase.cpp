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

#include "tupsocketbase.h"

TupSocketBase::TupSocketBase(QObject *parent) : QTcpSocket(parent)
{
    connect(this, SIGNAL(readyRead ()), this, SLOT(readFromServer()));
    connect(this, SIGNAL(connected()), this, SLOT(sendQueue()));
    connect(this, SIGNAL(disconnected()), this, SLOT(clearQueue()));

    // Keep the TCP connection alive through router/firewall idle timeouts
    setSocketOption(QAbstractSocket::KeepAliveOption, 1);
}

TupSocketBase::~TupSocketBase()
{
}

void TupSocketBase::sendQueue()
{
    while (queue.count() > 0) {
        if (state() == QAbstractSocket::ConnectedState)
            send(queue.dequeue());
        else 
            break;
    }
}

void TupSocketBase::clearQueue()
{
    queue.clear();
}

void TupSocketBase::send(const QString &message)
{
    if (state() == QAbstractSocket::ConnectedState) {
        QTextStream stream(this);
        stream.setCodec("UTF-8");
        stream << message.toUtf8().toBase64() << "%%" << Qt::endl;
    } else {
        queue.enqueue(message);
    }
}

void TupSocketBase::send(const QDomDocument &doc)
{
    send(doc.toString(0));
}

void TupSocketBase::readFromServer()
{    
    // #ifdef TUP_DEBUG
    //     qDebug() << "[TupSocketBase::readFromServer] this=" << this;
    //     qDebug() << "  Socket state:" << this->state();
    // #endif

    QString readed = "";

    int lineCount = 0;
    while (this->canReadLine()) {
        // #ifdef TUP_DEBUG
        //     qDebug() << "  Reading line" << lineCount;
        // #endif
        
        QByteArray line = this->readLine();
        
        // #ifdef TUP_DEBUG
        //     qDebug() << "    Raw line:" << line;
        // #endif

        readed += QString::fromUtf8(line);
        if (readed.endsWith("%%\n"))
            break;
        lineCount++;
    }

    // #ifdef TUP_DEBUG
    //     qDebug() << "  Accumulated readed:" << readed;
    // #endif
    if (!readed.isEmpty()) {
        int idx = readed.lastIndexOf("%%");
        // #ifdef TUP_DEBUG
        //     qDebug() << "  Last index of %%:" << idx;
        // #endif
        if (idx != -1)
            readed.remove(idx, 2);
        // readed = QString::fromUtf8(QByteArray::fromBase64(readed.toLocal8Bit()));
        readed = QString::fromUtf8(QByteArray::fromBase64(readed.toUtf8()));
        // #ifdef TUP_DEBUG
        //     qDebug() << "  Decoded readed:" << readed;
        // #endif
        this->readed(readed);
    }

    if (this->canReadLine()) {
        // #ifdef TUP_DEBUG
        //     qDebug() << "  More lines to read, recursing.";
        // #endif
        readFromServer();
    }
}
