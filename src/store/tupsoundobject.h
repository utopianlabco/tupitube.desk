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

#ifndef TUPSOUNDOBJECT_H
#define TUPSOUNDOBJECT_H

#include "tglobal.h"
#include "tupabstractserializable.h"

class TUPITUBE_EXPORT TupSoundObject : public QObject, public TupAbstractSerializable
{
    public:
        TupSoundObject(QObject *parent = nullptr);
        ~TupSoundObject();

        void setDefaultValues();

        void setMute(bool flag);
        bool isMuted();

        void setBackgroundTrack(bool flag);
        bool isBackgroundTrack();

        void setDuration(const QString &time);
        QString getDuration() const;

        void setVolume(int level);
        int getVolume() const;

        void setAudioScenes(QList<SoundScene> scenes);
        QList<SoundScene> getAudioScenes();

        SoundScene getAudioSceneAt(int sceneIndex);
        QList<int> getFramesToPlayAt(int sceneIndex);
        void updateFramesToPlay(int sceneIndex, QList<int> frames);

        void addSceneToPlay(SoundScene scene);
        void updateSoundScene(int sceneIndex, SoundScene scene);
        void removeSceneToPlay(int sceneIndex);
        void swapSoundScenes(int sceneIndex, int newSceneIndex);

        SoundType getSoundType();
        void setSoundType(SoundType type);

    public:
        virtual void fromXml(const QString &xml);
        virtual QDomElement toXml(QDomDocument &doc) const;

    private:
        SoundType soundType;
        bool mute;
        bool backgroundTrack;
        QList<SoundScene> audioScenes;
        QString duration;
        int volume;
};

#endif
