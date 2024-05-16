// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMAUDIOBUFFERINPUT_P_H
#define QPLATFORMAUDIOBUFFERINPUT_P_H

#include "qaudioformat.h"
#include "qaudiobuffer.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QPlatformAudioBufferInputBase : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void newAudioBuffer(const QAudioBuffer &buffer);
};

class Q_MULTIMEDIA_EXPORT QPlatformAudioBufferInput : public QPlatformAudioBufferInputBase
{
    Q_OBJECT
public:
    QPlatformAudioBufferInput(QAudioFormat format = {}) : m_format(std::move(format)) { }

    const QAudioFormat &audioFormat() const { return m_format; }

    using EncoderReadinessGetter = std::function<bool()>;
    const EncoderReadinessGetter &encoderReadinessGetter() const
    {
        return m_encoderReadinessGetter;
    }
    void setEncoderReadinessGetter(std::function<bool()> encoderReadinessGetter);

Q_SIGNALS:
    void encoderUpdated();

private:
    std::function<bool()> m_encoderReadinessGetter;
    QAudioFormat m_format;
};

QT_END_NAMESPACE

#endif // QPLATFORMAUDIOBUFFERINPUT_P_H
