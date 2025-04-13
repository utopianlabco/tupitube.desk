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

#ifndef TUPMICMANAGER_H
#define TUPMICMANAGER_H

#include "tglobal.h"
#include "tapplicationproperties.h"
#include "tupmiclevel.h"
#include "tinputfield.h"

#include <QWidget>
#include <QMediaRecorder>
#include <QAudioRecorder>
#include <QAudioBuffer>
#include <QAudioProbe>
#include <QComboBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QMediaPlayer>
#include <QTimer>

class TUPITUBE_EXPORT TupMicManager : public QWidget
{
    Q_OBJECT

    public:
        TupMicManager();
        ~TupMicManager();

        QString getRecordPath();
        bool isRecording();
        void cancelRecording();

    signals:
        void soundReady(bool enabled);

    public slots:
        void handleBuffer(const QAudioBuffer &buffer);

    private slots:
        void discardRecording();
        void togglePause();
        void toggleRecord();

        void updateStatus(QMediaRecorder::Status status);
        void onStateChanged(QMediaRecorder::State state);
        void updateProgress(qint64 pos);
        void showErrorMessage();
        void enableRecordButton(bool enabled);

        void playRecording();
        void trackPlayerStatus();
        void enablePlayButton();

    private:
        void initRecorder();
        void setupUI();
        void setConnections();
        void clearMicLevels();
        void resetMediaPlayer();

        QAudioRecorder *micRecorder;
        QAudioProbe *micProbe;
        QList<TupMicLevel*> micLevels;

        QWidget *centralWidget;
        QWidget *controlsWidget;
        QWidget *playerWidget;
        QWidget *bottomWidget;
        QVBoxLayout *levelsScreenLayout;
        TupMicLevel *audioLevel1;
        TupMicLevel *audioLevel2;
        bool audioLevelsIncluded;

        TInputField *nameInput;
        TInputField *durationInput;
        QComboBox *audioDevDropList;
        QLabel *statusLabel;

        QPushButton *recordButton;
        QPushButton *pauseButton;

        QPushButton *playButton;
        QPushButton *discardButton;

        QList<QMediaPlayer *> player;
        QAudioProbe *playerProbe;
        QTimer *timer;
        qreal secCounter;
        qreal audioDuration;
        bool recording;
        QString extension;
        QString recordTime;
};

#endif // TUPMICMANAGER_H
