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

#include "twaveformwidget.h"

#include <QPainter>
#include <QScrollBar>

#define SCROLL_MARGIN 30

TWaveformWidget::TWaveformWidget(QWidget *parent) : QWidget(parent)
{
    scrollArea = nullptr;
    sampler = nullptr;
    amplitudes = nullptr;
    
    sampleWidth = 2;
    samplesPerFrame = 2;
    samplesPerSec = 0;
    frameWidth = 4;
    framerate = 24;
    
    numDisplaySamples = 0;
    currentFrameIndex = 0;
    isDragging = false;
    isSilent = true;

    // Default colors
    waveformFillColor = QColor(50, 50, 100, 128);
    waveformOutlineColor = QColor(40, 40, 85);
    playheadColor = QColor(225, 50, 50);
    playheadFillColor = QColor(246, 216, 1);
    backgroundColor = QColor(40, 40, 60);
    frameLineColor = QColor(200, 200, 200, 100);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

TWaveformWidget::~TWaveformWidget()
{
    if (amplitudes) {
        delete[] amplitudes;
        amplitudes = nullptr;
    }
}

QSize TWaveformWidget::sizeHint() const
{
    return QSize(numDisplaySamples * sampleWidth, 100);
}

void TWaveformWidget::setScrollArea(QScrollArea *area)
{
    scrollArea = area;
}

void TWaveformWidget::setAudioSampler(TAudioSampler *audioSampler)
{
    sampler = audioSampler;
    if (sampler && sampler->isLoaded()) {
        samplesPerSec = sampler->sampleRate() / 100;
        isSilent = false;
        autoZoom();
    }
}

TAudioSampler *TWaveformWidget::audioSampler() const
{
    return sampler;
}

void TWaveformWidget::setFps(int fps)
{
    if (fps > 0) {
        framerate = fps;
        computeAmplitudes();
    }
}

int TWaveformWidget::fps() const
{
    return framerate;
}

int TWaveformWidget::currentFrame() const
{
    return currentFrameIndex;
}

int TWaveformWidget::totalFrames() const
{
    if (!sampler || !sampler->isLoaded())
        return 0;
    
    return static_cast<int>(sampler->duration() * framerate);
}

void TWaveformWidget::setWaveformColors(const QColor &fill, const QColor &outline)
{
    waveformFillColor = fill;
    waveformOutlineColor = outline;
    update();
}

void TWaveformWidget::setPlayheadColor(const QColor &color)
{
    playheadColor = color;
    update();
}

void TWaveformWidget::setBackgroundColor(const QColor &color)
{
    backgroundColor = color;
    update();
}

void TWaveformWidget::setFrameLineColor(const QColor &color)
{
    frameLineColor = color;
    update();
}

void TWaveformWidget::zoomIn()
{
    sampleWidth += 1;
    frameWidth = sampleWidth * samplesPerFrame;
    computeAmplitudes();
}

void TWaveformWidget::zoomOut()
{
    if (sampleWidth > 1) {
        sampleWidth -= 1;
        frameWidth = sampleWidth * samplesPerFrame;
        computeAmplitudes();
    }
}

void TWaveformWidget::autoZoom()
{
    if (!scrollArea || !sampler || !sampler->isLoaded())
        return;

    int viewportWidth = scrollArea->viewport()->width();
    int frames = totalFrames();
    
    if (frames <= 0)
        return;

    // Calculate optimal sample width to fit entire audio in viewport
    int targetWidth = viewportWidth - SCROLL_MARGIN;
    int requiredSamplesPerFrame = 2;  // minimum
    
    int calculatedWidth = targetWidth / (frames * requiredSamplesPerFrame);
    sampleWidth = qMax(1, calculatedWidth);
    samplesPerFrame = requiredSamplesPerFrame;
    frameWidth = sampleWidth * samplesPerFrame;
    
    computeAmplitudes();
}

void TWaveformWidget::setCurrentFrame(int frame)
{
    if (frame >= 0 && frame < totalFrames()) {
        currentFrameIndex = frame;
        updateScrollPosition();
        update();
        emit frameChanged(currentFrameIndex);
    }
}

void TWaveformWidget::positionChanged(qint64 milliseconds)
{
    if (!sampler || !sampler->isLoaded())
        return;

    double seconds = milliseconds / 1000.0;
    int frame = static_cast<int>(seconds * framerate);
    
    if (frame != currentFrameIndex) {
        currentFrameIndex = frame;
        updateScrollPosition();
        update();
    }
}

void TWaveformWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        int frame = frameAtPosition(event->pos().x());
        if (frame >= 0 && frame < totalFrames()) {
            currentFrameIndex = frame;
            update();
            emit clicked(currentFrameIndex);
            emit seekRequested(currentFrameIndex);
        }
    }
    
    QWidget::mousePressEvent(event);
}

void TWaveformWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        int frame = frameAtPosition(event->pos().x());
        frame = qBound(0, frame, totalFrames() - 1);
        
        if (frame != currentFrameIndex) {
            currentFrameIndex = frame;
            update();
            emit frameChanged(currentFrameIndex);
        }
    }
    
    QWidget::mouseMoveEvent(event);
}

void TWaveformWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        emit seekRequested(currentFrameIndex);
    }
    
    QWidget::mouseReleaseEvent(event);
}

void TWaveformWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Background
    painter.fillRect(rect(), backgroundColor);

    if (!sampler || !sampler->isLoaded() || isSilent) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    drawFrameLines(painter);
    drawWaveform(painter);
    drawPlayhead(painter);
}

void TWaveformWidget::drawWaveform(QPainter &painter)
{
    if (!amplitudes || numDisplaySamples == 0)
        return;

    int halfHeight = height() / 2;
    
    QPolygon polygon;
    polygon.append(QPoint(0, halfHeight));
    
    for (int i = 0; i < numDisplaySamples; i++) {
        int amplitude = static_cast<int>(amplitudes[i] * halfHeight * 0.95);
        int x = i * sampleWidth;
        polygon.append(QPoint(x, halfHeight - amplitude));
    }
    
    // Return to baseline on the way back down
    for (int i = numDisplaySamples - 1; i >= 0; i--) {
        int amplitude = static_cast<int>(amplitudes[i] * halfHeight * 0.95);
        int x = i * sampleWidth;
        polygon.append(QPoint(x, halfHeight + amplitude));
    }
    
    polygon.append(QPoint(0, halfHeight));
    
    painter.setBrush(waveformFillColor);
    painter.setPen(waveformOutlineColor);
    painter.drawPolygon(polygon);
}

void TWaveformWidget::drawPlayhead(QPainter &painter)
{
    int xPos = positionAtFrame(currentFrameIndex);
    
    // Draw playhead line
    painter.setPen(QPen(playheadColor, 2));
    painter.drawLine(xPos, 0, xPos, height());
    
    // Draw small triangle marker at top
    QPolygon triangle;
    triangle << QPoint(xPos - 5, 0)
             << QPoint(xPos + 5, 0)
             << QPoint(xPos, 8);
    painter.setBrush(playheadFillColor);
    painter.setPen(playheadColor);
    painter.drawPolygon(triangle);
}

void TWaveformWidget::drawFrameLines(QPainter &painter)
{
    int frames = totalFrames();
    painter.setPen(QPen(frameLineColor, 1, Qt::DotLine));
    
    // Draw frame boundaries every 10 frames
    for (int f = 0; f <= frames; f += 10) {
        int x = positionAtFrame(f);
        painter.drawLine(x, 0, x, height());
    }
}

int TWaveformWidget::frameAtPosition(int x) const
{
    if (frameWidth <= 0)
        return 0;
    return x / frameWidth;
}

int TWaveformWidget::positionAtFrame(int frame) const
{
    return frame * frameWidth;
}

void TWaveformWidget::computeAmplitudes()
{
    if (!sampler || !sampler->isLoaded())
        return;

    // Clean up previous data
    if (amplitudes) {
        delete[] amplitudes;
        amplitudes = nullptr;
    }

    double duration = sampler->duration();
    if (duration <= 0.0)
        return;

    // Calculate number of display samples based on frame rate and samples per frame
    int totalFramesCount = static_cast<int>(duration * framerate);
    numDisplaySamples = totalFramesCount * samplesPerFrame;

    if (numDisplaySamples <= 0)
        return;

    amplitudes = new double[numDisplaySamples];
    
    double sampleDuration = duration / numDisplaySamples;
    
    for (int i = 0; i < numDisplaySamples; i++) {
        double startTime = i * sampleDuration;
        double endTime = (i + 1) * sampleDuration;
        amplitudes[i] = sampler->getRMSAmplitude(startTime, endTime);
    }

    // Update widget size
    int newWidth = numDisplaySamples * sampleWidth;
    setMinimumWidth(newWidth);
    setFixedWidth(newWidth);
    
    update();
}

void TWaveformWidget::updateScrollPosition()
{
    if (!scrollArea)
        return;

    int playheadX = positionAtFrame(currentFrameIndex);
    int viewportWidth = scrollArea->viewport()->width();
    int scrollPos = scrollArea->horizontalScrollBar()->value();
    
    // Keep playhead visible with margin
    if (playheadX < scrollPos + SCROLL_MARGIN) {
        scrollArea->horizontalScrollBar()->setValue(qMax(0, playheadX - SCROLL_MARGIN));
    } else if (playheadX > scrollPos + viewportWidth - SCROLL_MARGIN) {
        scrollArea->horizontalScrollBar()->setValue(playheadX - viewportWidth + SCROLL_MARGIN);
    }
}
