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

#ifndef TUPCAMERASTATUS_H
#define TUPCAMERASTATUS_H

#include "tconfig.h"
#include "timagebutton.h"
#include "tupproject.h"

#include <QFrame>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>

class TUPITUBE_EXPORT TupCameraStatus : public QFrame
{
    Q_OBJECT

    public:
        TupCameraStatus(int scenesTotal, QWidget *parent = nullptr);
        ~TupCameraStatus();

        void setScenes(QStringList scenes);

        void setFPS(int frames);
        int getFPS();
        void setCurrentScene(int index);
        bool isLooping();
        void enableButtons(bool flag);
        void enableAudioControls(bool enabled);

    public slots:
        void setVolume(int volume);

    signals:
        void playModeChanged(PlayMode mode, int sceneIndex);
        void sceneIndexChanged(int index);
        void muteEnabled(bool muteAction);
        void volumeChanged(int volume);
        void fpsChanged(int fps);
        void loopChanged();
        void exportClicked();
        void postClicked();

    private slots:
        void muteAction();
        void updateScenesComboStatus();
        void updateVolumeLabel(int value);

    private:
        QWidget *spaceWidget;
        QCheckBox *playAllBox;
        QSpinBox *fpsBox;
        QComboBox *scenesCombo;
        QCheckBox *loopBox;

        bool loop;
        bool mute;
        TImageButton *soundButton;
        QSlider *volumeSlider;
        QLabel *volumeLabel;
        QPushButton *exportButton;
        QPushButton *postButton;
        int scenesCount;
};

#endif
