/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTVIDEORENDERERSINK_P_H
#define QGSTVIDEORENDERERSINK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qqueue.h>
#include <QtCore/qpointer.h>
#include <QtCore/qwaitcondition.h>
#include <qvideosurfaceformat.h>
#include <qvideoframe.h>

#include <private/qgst_p.h>

#if QT_CONFIG(gstreamer_gl)
#ifndef GST_USE_UNSTABLE_API
#define GST_USE_UNSTABLE_API
#endif
#include <gst/gl/gl.h>
#endif

QT_BEGIN_NAMESPACE
class QVideoSink;

class QGstVideoRenderer
{
public:
    QGstVideoRenderer();
    ~QGstVideoRenderer();

    QGstMutableCaps getCaps();
    bool start(GstCaps *caps);
    void stop();

    bool proposeAllocation(GstQuery *query);

    bool present(QVideoSink *surface, GstBuffer *buffer);
    void flush(QVideoSink *surface);

private:
    QVideoSurfaceFormat m_format;
    GstVideoInfo m_videoInfo;
    bool m_flushed = true;
    QVideoFrame::HandleType m_handleType = QVideoFrame::NoHandle;
};

class QVideoSurfaceGstDelegate : public QObject
{
    Q_OBJECT
public:
    QVideoSurfaceGstDelegate(QVideoSink *sink);
    ~QVideoSurfaceGstDelegate();

    QGstMutableCaps caps();

    bool start(GstCaps *caps);
    void stop();
    void unlock();
    bool proposeAllocation(GstQuery *query);

    void flush();

    GstFlowReturn render(GstBuffer *buffer);

    bool event(QEvent *event) override;
    bool query(GstQuery *query);

private slots:
    bool handleEvent(QMutexLocker<QMutex> *locker);
    void updateSupportedFormats();

private:
    void notify();
    bool waitForAsyncEvent(QMutexLocker<QMutex> *locker, QWaitCondition *condition, unsigned long time);

    QPointer<QVideoSink> m_sink;

    QMutex m_mutex;
    QWaitCondition m_setupCondition;
    QWaitCondition m_renderCondition;
    GstFlowReturn m_renderReturn = GST_FLOW_OK;
    QGstVideoRenderer *m_renderer = nullptr;
    QGstVideoRenderer *m_activeRenderer = nullptr;

    QGstMutableCaps m_surfaceCaps;
    QGstMutableCaps m_startCaps;
    GstBuffer *m_renderBuffer = nullptr;
#if QT_CONFIG(gstreamer_gl)
    GstGLContext *m_gstGLDisplayContext = nullptr;
#endif

    bool m_notified = false;
    bool m_stop = false;
    bool m_flush = false;
};

class Q_MULTIMEDIA_EXPORT QGstVideoRendererSink
{
public:
    GstVideoSink parent;

    static QGstVideoRendererSink *createSink(QVideoSink *surface);
    static void setSink(QVideoSink *surface);

private:
    static GType get_type();
    static void class_init(gpointer g_class, gpointer class_data);
    static void base_init(gpointer g_class);
    static void instance_init(GTypeInstance *instance, gpointer g_class);

    static void finalize(GObject *object);

    static void handleShowPrerollChange(GObject *o, GParamSpec *p, gpointer d);

    static GstStateChangeReturn change_state(GstElement *element, GstStateChange transition);

    static GstCaps *get_caps(GstBaseSink *sink, GstCaps *filter);
    static gboolean set_caps(GstBaseSink *sink, GstCaps *caps);

    static gboolean propose_allocation(GstBaseSink *sink, GstQuery *query);

    static gboolean stop(GstBaseSink *sink);

    static gboolean unlock(GstBaseSink *sink);

    static GstFlowReturn show_frame(GstVideoSink *sink, GstBuffer *buffer);
    static gboolean query(GstBaseSink *element, GstQuery *query);

private:
    QVideoSurfaceGstDelegate *delegate = nullptr;
};


class QGstVideoRendererSinkClass
{
public:
    GstVideoSinkClass parent_class;
};

QT_END_NAMESPACE

#endif
