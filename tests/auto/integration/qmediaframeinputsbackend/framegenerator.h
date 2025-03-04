// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FRAMEGENERATOR_H
#define FRAMEGENERATOR_H

#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <functional>
#include <chrono>

QT_BEGIN_NAMESPACE

using namespace std::chrono;

enum class ImagePattern
{
    SingleColor, // Image filled with a single color
    ColoredSquares // Colored squares, [red, green; blue, yellow]
};

class VideoGenerator : public QObject
{
    Q_OBJECT
public:
    void setPattern(ImagePattern pattern);
    void setFrameCount(int count);
    void setSize(QSize size);
    void setFrameRate(double rate);
    void setPeriod(milliseconds period);
    void emitEmptyFrameOnStop();
    QVideoFrame createFrame();

signals:
    void done();
    void frameCreated(const QVideoFrame &frame);

public slots:
    void nextFrame();

private:
    QList<QColor> colors = { Qt::red, Qt::green, Qt::blue, Qt::black, Qt::white };
    ImagePattern m_pattern = ImagePattern::SingleColor;
    QSize m_size{ 640, 480 };
    std::optional<int> m_maxFrameCount;
    int m_frameIndex = 0;
    std::optional<double> m_frameRate;
    std::optional<milliseconds> m_period;
    bool m_emitEmptyFrameOnStop = false;
};

class AudioGenerator : public QObject
{
    Q_OBJECT
public:
    AudioGenerator();
    void setFormat(const QAudioFormat &format);
    void setBufferCount(int count);
    void setDuration(microseconds duration);
    void setFrequency(qreal frequency);
    void emitEmptyBufferOnStop();
    QAudioBuffer createAudioBuffer();

signals:
    void done();
    void audioBufferCreated(const QAudioBuffer &buffer);

public slots:
    void nextBuffer();

private:
    int m_maxBufferCount = 1;
    microseconds m_duration = 1s;
    int m_bufferIndex = 0;
    QAudioFormat m_format;
    bool m_emitEmptyBufferOnStop = false;
    qreal m_frequency = 500.;
    qint32 m_sampleIndex = 0;
};

QT_END_NAMESPACE

#endif
