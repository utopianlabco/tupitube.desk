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

#ifndef TWAVEFORMWIDGET_H
#define TWAVEFORMWIDGET_H

#include "tglobal.h"
#include "taudiosampler.h"

#include <QWidget>
#include <QScrollArea>
#include <QMouseEvent>

/**
 * @brief Generic audio waveform visualization widget.
 * 
 * This widget displays an audio waveform with zoom controls,
 * playhead tracking, and basic seeking via mouse clicks.
 * Can be extended for application-specific features.
 */
class TUPITUBE_EXPORT TWaveformWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit TWaveformWidget(QWidget *parent = nullptr);
        virtual ~TWaveformWidget();

        QSize sizeHint() const override;
        void setScrollArea(QScrollArea *scrollArea);
        
        void setAudioSampler(TAudioSampler *sampler);
        TAudioSampler *audioSampler() const;

        void setFps(int fps);
        int fps() const;

        int currentFrame() const;
        int totalFrames() const;

        // Visual customization
        void setWaveformColors(const QColor &fill, const QColor &outline);
        void setPlayheadColor(const QColor &color);
        void setBackgroundColor(const QColor &color);
        void setFrameLineColor(const QColor &color);

    signals:
        void frameChanged(int frame);
        void clicked(int frame);
        void seekRequested(int frame);

    public slots:
        void zoomIn();
        void zoomOut();
        void autoZoom();
        void setCurrentFrame(int frame);
        void positionChanged(qint64 milliseconds);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

        virtual void drawWaveform(QPainter &painter);
        virtual void drawPlayhead(QPainter &painter);
        virtual void drawFrameLines(QPainter &painter);

        int frameAtPosition(int x) const;
        int positionAtFrame(int frame) const;

    protected:
        QScrollArea *scrollArea;
        TAudioSampler *sampler;
        
        int sampleWidth;
        int samplesPerFrame;
        int samplesPerSec;
        int frameWidth;
        int framerate;
        
        int numDisplaySamples;
        double *amplitudes;
        
        int currentFrameIndex;
        bool isDragging;
        bool isSilent;

        // Colors
        QColor waveformFillColor;
        QColor waveformOutlineColor;
        QColor playheadColor;
        QColor playheadFillColor;
        QColor backgroundColor;
        QColor frameLineColor;

    private:
        void computeAmplitudes();
        void updateScrollPosition();
};

#endif // TWAVEFORMWIDGET_H
