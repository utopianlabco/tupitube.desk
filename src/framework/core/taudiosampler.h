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
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef TAUDIOSAMPLER_H
#define TAUDIOSAMPLER_H

#include "tglobal.h"
#include "sndfile.h"

#include <QObject>
#include <QString>
#include <cmath>

#define MAX_AUDIO_FRAMES 53000000

/**
 * @brief Generic audio sample extractor using libsndfile.
 * 
 * This class loads audio files (WAV, AIFF, etc.) and provides
 * amplitude data for waveform visualization.
 */
class TUPITUBE_EXPORT TAudioSampler : public QObject
{
    Q_OBJECT

    public:
        explicit TAudioSampler(QObject *parent = nullptr);
        TAudioSampler(const QString &path, bool reverse = false, QObject *parent = nullptr);
        ~TAudioSampler();

        bool loadSoundFile(const QString &path);
        bool isLoaded() const;
        bool isValid() const; // Alias for isLoaded()
        
        double duration() const;
        int sampleRate() const;
        int numChannels() const;
        int numSamples() const;

        double getAmplitude(double startTime, double endTime) const;
        double getRMSAmplitude(double startTime, double endTime) const;
        double getMaxAmplitude(double startTime, double endTime) const;

        float *buffer() const;

    private:
        int timeToSample(double time, bool clamped) const;

        SF_INFO soundInfo;
        int sampleCount;
        sf_count_t frameCount;
        float *samples;
        QString sourcePath;
        bool loaded;
};

#endif // TAUDIOSAMPLER_H
