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

#include "taudiosampler.h"

#include <QFile>
#include <QDebug>

TAudioSampler::TAudioSampler(QObject *parent)
    : QObject(parent)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TAudioSampler::TAudioSampler()] - Default constructor";
    #endif

    frameCount = 0;
    sampleCount = 0;
    samples = nullptr;
    loaded = false;
    memset(&soundInfo, 0, sizeof(soundInfo));
}

TAudioSampler::TAudioSampler(const QString &path, bool reverse, QObject *parent)
    : QObject(parent)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TAudioSampler::TAudioSampler()] - path:" << path << "reverse:" << reverse;
    #endif

    frameCount = 0;
    sampleCount = 0;
    samples = nullptr;
    loaded = false;
    sourcePath = path;
    memset(&soundInfo, 0, sizeof(soundInfo));

    if (path.isEmpty()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TAudioSampler::TAudioSampler()] - Fatal Error: path is empty!";
        #endif
        return;
    }

    if (!QFile::exists(path)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TAudioSampler::TAudioSampler()] - Fatal Error: file doesn't exist ->" << path;
        #endif
        return;
    }

    loaded = loadSoundFile(path);
    if (loaded && reverse && samples) {
        // Reverse the audio samples
        for (int i = 0; i < soundInfo.frames / 2; i++) {
            int revI = static_cast<int>(soundInfo.frames) - 1 - i;
            for (int j = 0; j < soundInfo.channels; j++) {
                float temp = samples[i * soundInfo.channels + j];
                samples[i * soundInfo.channels + j] = samples[revI * soundInfo.channels + j];
                samples[revI * soundInfo.channels + j] = temp;
            }
        }
    }
}

TAudioSampler::~TAudioSampler()
{
    if (samples) {
        delete[] samples;
        samples = nullptr;
    }
}

bool TAudioSampler::isLoaded() const
{
    return loaded && samples != nullptr;
}

bool TAudioSampler::isValid() const
{
    return isLoaded();
}

double TAudioSampler::duration() const
{
    if (samples == nullptr || soundInfo.samplerate == 0)
        return 0.0;

    return static_cast<double>(soundInfo.frames) / static_cast<double>(soundInfo.samplerate);
}

int TAudioSampler::sampleRate() const
{
    return soundInfo.samplerate;
}

int TAudioSampler::numChannels() const
{
    return soundInfo.channels;
}

int TAudioSampler::numSamples() const
{
    return sampleCount;
}

double TAudioSampler::getAmplitude(double startTime, double endTime) const
{
    if (samples == nullptr || endTime <= startTime)
        return 0.0;

    int start = timeToSample(startTime, true);
    int end = timeToSample(endTime, true);
    if (end == start)
        return 0.0;

    double total = 0.0;
    for (int i = start; i < end; i++) {
        double sample = fabs(samples[i]);
        if (sample > 1.001)
            continue;
        total += sample;
    }

    return total / static_cast<double>(end - start);
}

double TAudioSampler::getRMSAmplitude(double startTime, double endTime) const
{
    if (samples == nullptr || endTime <= startTime)
        return 0.0;

    int start = timeToSample(startTime, true);
    int end = timeToSample(endTime, true);
    if (end == start)
        return 0.0;

    double total = 0.0;
    for (int i = start; i < end; i++) {
        double sample = fabs(samples[i]);
        if (sample > 1.001)
            continue;
        total += sample * sample;
    }

    return sqrt(total / static_cast<double>(end - start));
}

double TAudioSampler::getMaxAmplitude(double startTime, double endTime) const
{
    if (samples == nullptr || endTime <= startTime)
        return 0.0;

    int start = timeToSample(startTime, true);
    int end = timeToSample(endTime, true);
    if (end == start)
        return 0.0;

    double maxAmp = 0.0;
    for (int i = start; i < end; i++) {
        double sample = fabs(samples[i]);
        if (sample > 1.001)
            continue;
        if (sample > maxAmp)
            maxAmp = sample;
    }

    return maxAmp;
}

float *TAudioSampler::buffer() const
{
    return samples;
}

int TAudioSampler::timeToSample(double time, bool clamped) const
{
    if (samples == nullptr)
        return 0;

    double sampleTime = time * static_cast<double>(soundInfo.samplerate * soundInfo.channels);
    int sample = static_cast<int>(round(sampleTime));

    // Align to channel boundary
    if (soundInfo.channels > 0) {
        while (sample % soundInfo.channels)
            sample--;
    }

    if (clamped) {
        if (sample < 0)
            return 0;
        if (sample > sampleCount - 1)
            return sampleCount - 1;
    }

    return sample;
}

bool TAudioSampler::loadSoundFile(const QString &path)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TAudioSampler::loadSoundFile()] - path:" << path;
    #endif

    if (path.isEmpty()) {
        #ifdef TUP_DEBUG
            qDebug() << "[TAudioSampler::loadSoundFile()] - Fatal Error: path is empty!";
        #endif
        return false;
    }

    if (!QFile::exists(path)) {
        #ifdef TUP_DEBUG
            qDebug() << "[TAudioSampler::loadSoundFile()] - Fatal Error: file doesn't exist ->" << path;
        #endif
        return false;
    }

    sourcePath = path;
    SNDFILE *sndFile = sf_open(path.toUtf8().constData(), SFM_READ, &soundInfo);
    if (!sndFile) {
        #ifdef TUP_DEBUG
            qDebug() << "[TAudioSampler::loadSoundFile()] - Failed to open file!";
        #endif
        return false;
    }

    if (soundInfo.frames > MAX_AUDIO_FRAMES)
        soundInfo.frames = MAX_AUDIO_FRAMES;

    sampleCount = static_cast<int>(soundInfo.frames * soundInfo.channels);
    samples = new float[sampleCount];

    frameCount = sf_readf_float(sndFile, samples, soundInfo.frames);
    sf_close(sndFile);

    loaded = true;

    #ifdef TUP_DEBUG
        qDebug() << "[TAudioSampler::loadSoundFile()] - Loaded" << sampleCount << "samples";
    #endif

    return true;
}
