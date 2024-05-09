// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERMEDIAPLAYER_P_H
#define QGSTREAMERMEDIAPLAYER_P_H

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

#include <QtCore/qstack.h>
#include <private/qplatformmediaplayer_p.h>
#include <private/qtmultimediaglobal_p.h>
#include <private/qmultimediautils_p.h>
#include <qurl.h>
#include <common/qgst_p.h>
#include <common/qgstpipeline_p.h>

#include <QtCore/qtimer.h>

#include <array>

QT_BEGIN_NAMESPACE

class QNetworkAccessManager;
class QGstreamerMessage;
class QGstAppSource;
class QGstreamerAudioOutput;
class QGstreamerVideoOutput;

class QGstreamerMediaPlayer : public QObject,
                              public QPlatformMediaPlayer,
                              public QGstreamerBusMessageFilter,
                              public QGstreamerSyncMessageFilter
{
public:
    static QMaybe<QPlatformMediaPlayer *> create(QMediaPlayer *parent = nullptr);
    ~QGstreamerMediaPlayer();

    qint64 position() const override;
    qint64 duration() const override;

    float bufferProgress() const override;

    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &, QIODevice *) override;

    bool streamPlaybackSupported() const override { return true; }

    void setAudioOutput(QPlatformAudioOutput *output) override;

    QMediaMetaData metaData() const override;

    void setVideoSink(QVideoSink *sink) override;

    int trackCount(TrackType) override;
    QMediaMetaData trackMetaData(TrackType /*type*/, int /*streamNumber*/) override;
    int activeTrack(TrackType) override;
    void setActiveTrack(TrackType, int /*streamNumber*/) override;

    void setPosition(qint64 pos) override;

    void play() override;
    void pause() override;
    void stop() override;

    void *nativePipeline() override;

    bool processBusMessage(const QGstreamerMessage& message) override;
    bool processSyncMessage(const QGstreamerMessage& message) override;

    void updatePosition() { positionChanged(position()); }

private:
    QGstreamerMediaPlayer(QGstreamerVideoOutput *videoOutput, QGstElement videoInputSelector,
                          QGstElement audioInputSelector, QGstElement subTitleInputSelector,
                          QMediaPlayer *parent);

    struct TrackSelector
    {
        TrackSelector(TrackType, QGstElement selector);
        QGstPad createInputPad();
        void removeInputPad(QGstPad pad);
        void removeAllInputPads();
        QGstPad inputPad(int index);
        int activeInputIndex() const { return isConnected ? tracks.indexOf(activeInputPad()) : -1; }
        QGstPad activeInputPad() const
        {
            return isConnected ? QGstPad{ selector.getObject("active-pad") } : QGstPad{};
        }
        void setActiveInputPad(QGstPad input) { selector.set("active-pad", input); }
        int trackCount() const { return tracks.count(); }

        QGstElement selector;
        TrackType type;
        QList<QGstPad> tracks;
        bool isConnected = false;
    };

    friend class QGstreamerStreamsControl;
    void decoderPadAdded(const QGstElement &src, const QGstPad &pad);
    void decoderPadRemoved(const QGstElement &src, const QGstPad &pad);
    void disconnectDecoderHandlers();
    static void uridecodebinElementAddedCallback(GstElement *uridecodebin, GstElement *child,
                                                 QGstreamerMediaPlayer *that);
    static void sourceSetupCallback(GstElement *uridecodebin, GstElement *source,
                                    QGstreamerMediaPlayer *that);
    static void unknownTypeCallback(GstElement *decodebin, GstPad *pad, GstCaps *caps,
                                    QGstreamerMediaPlayer *self);
    static void decodebinElementAddedCallback(GstBin *decodebin, GstBin *sub_bin,
                                              GstElement *element, QGstreamerMediaPlayer *self);
    static void decodebinElementRemovedCallback(GstBin *decodebin, GstBin *sub_bin,
                                                GstElement *element, QGstreamerMediaPlayer *self);

    void parseStreamsAndMetadata();
    void connectOutput(TrackSelector &ts);
    void removeOutput(TrackSelector &ts);
    void removeAllOutputs();
    void stopOrEOS(bool eos);
    bool canTrackProgress() const { return decodeBinQueues > 0; }
    void detectPipelineIsSeekable();

    std::array<TrackSelector, NTrackTypes> trackSelectors;
    TrackSelector &trackSelector(TrackType type);

    QMediaMetaData m_metaData;

    int m_bufferProgress = 0;
    QUrl m_url;
    QIODevice *m_stream = nullptr;

    enum class ResourceErrorState : uint8_t {
        NoError,
        ErrorOccurred,
        ErrorReported,
    };

    bool prerolling = false;
    bool m_requiresSeekOnPlay = false;
    bool m_initialBufferProgressSent = false;
    ResourceErrorState m_resourceErrorState = ResourceErrorState::NoError;
    qint64 m_duration = 0;
    QTimer positionUpdateTimer;

    QGstAppSource *m_appSrc = nullptr;

    QGstStructure topology;

    // Gst elements
    QGstPipeline playerPipeline;
    QGstElement src;
    QGstElement decoder;

    QGstreamerAudioOutput *gstAudioOutput = nullptr;
    QGstreamerVideoOutput *gstVideoOutput = nullptr;

    //    QGstElement streamSynchronizer;

    QHash<QByteArray, QGstPad> decoderOutputMap;

    // decoder connections
    QGObjectHandlerScopedConnection padAdded;
    QGObjectHandlerScopedConnection padRemoved;
    QGObjectHandlerScopedConnection sourceSetup;
    QGObjectHandlerScopedConnection uridecodebinElementAdded;
    QGObjectHandlerScopedConnection unknownType;
    QGObjectHandlerScopedConnection elementAdded;
    QGObjectHandlerScopedConnection elementRemoved;

    int decodeBinQueues = 0;
};

QT_END_NAMESPACE

#endif
