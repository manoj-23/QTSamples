/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtCore/qdebug.h>
#include <QtCore/qmap.h>

#include "qmediaservice.h"
#include "qmediaserviceprovider_p.h"
#include "qmediaserviceproviderplugin.h"
#include "qmediapluginloader_p.h"
#include "qmediaplayer.h"
#include "qvideodeviceselectorcontrol.h"

QT_BEGIN_NAMESPACE

QMediaServiceProviderFactoryInterface::~QMediaServiceProviderFactoryInterface()
{
}

Q_GLOBAL_STATIC_WITH_ARGS(QMediaPluginLoader, loader,
        (QMediaServiceProviderFactoryInterface_iid, QLatin1String("mediaservice"), Qt::CaseInsensitive))


class QPluginServiceProvider : public QMediaServiceProvider
{
    struct MediaServiceData {
        QByteArray type;
        QMediaServiceProviderPlugin *plugin;

        MediaServiceData() : plugin(nullptr) { }
    };

    QMap<const QMediaService*, MediaServiceData> mediaServiceData;

public:
    QMediaService* requestService(const QByteArray &type) override
    {
        QString key(QLatin1String(type.constData()));

        QList<QMediaServiceProviderPlugin *>plugins;
        const auto instances = loader()->instances(key);
        for (QObject *obj : instances) {
            QMediaServiceProviderPlugin *plugin = qobject_cast<QMediaServiceProviderPlugin*>(obj);
            if (plugin)
                plugins << plugin;
        }

        if (!plugins.isEmpty()) {
            QMediaServiceProviderPlugin *plugin = plugins[0];

            if (plugin != nullptr) {
                QMediaService *service = plugin->create(key);
                if (service != nullptr) {
                    MediaServiceData d;
                    d.type = type;
                    d.plugin = plugin;
                    mediaServiceData.insert(service, d);
                }

                return service;
            }
        }

        qWarning() << "defaultServiceProvider::requestService(): no service found for -" << key;
        return nullptr;
    }

    void releaseService(QMediaService *service) override
    {
        if (service != nullptr) {
            MediaServiceData d = mediaServiceData.take(service);

            if (d.plugin != nullptr)
                d.plugin->release(service);
        }
    }

    QMediaServiceFeaturesInterface::Features supportedFeatures(const QMediaService *service) const override
    {
        if (service) {
            MediaServiceData d = mediaServiceData.value(service);

            if (d.plugin) {
                QMediaServiceFeaturesInterface *iface =
                        qobject_cast<QMediaServiceFeaturesInterface*>(d.plugin);

                if (iface)
                    return iface->supportedFeatures(d.type);
            }
        }

        return QMediaServiceFeaturesInterface::Features();
    }

    QMultimedia::SupportEstimate hasSupport(const QByteArray &serviceType,
                                     const QString &mimeType,
                                     const QStringList& codecs,
                                     int flags) const override
    {
        const QList<QObject*> instances = loader()->instances(QLatin1String(serviceType));

        if (instances.isEmpty())
            return QMultimedia::NotSupported;

        bool allServicesProvideInterface = true;
        QMultimedia::SupportEstimate supportEstimate = QMultimedia::NotSupported;

        for (QObject *obj : instances) {
            QMediaServiceSupportedFormatsInterface *iface =
                    qobject_cast<QMediaServiceSupportedFormatsInterface*>(obj);


            if (flags) {
                QMediaServiceFeaturesInterface *iface =
                        qobject_cast<QMediaServiceFeaturesInterface*>(obj);

                if (iface) {
                    QMediaServiceFeaturesInterface::Features features = iface->supportedFeatures(serviceType);

                    //if low latency playback was asked, skip services known
                    //not to provide low latency playback
                    if ((flags & QMediaPlayer::LowLatency) &&
                        !(features & QMediaServiceFeaturesInterface::LowLatencyPlayback))
                            continue;

                    //the same for QIODevice based streams support
                    if ((flags & QMediaPlayer::StreamPlayback) &&
                        !(features & QMediaServiceFeaturesInterface::StreamPlayback))
                            continue;
                }
            }

            if (iface)
                supportEstimate = qMax(supportEstimate, iface->hasSupport(mimeType, codecs));
            else
                allServicesProvideInterface = false;
        }

        //don't return PreferredService
        supportEstimate = qMin(supportEstimate, QMultimedia::ProbablySupported);

        //Return NotSupported only if no services are available of serviceType
        //or all the services returned NotSupported, otherwise return at least MaybeSupported
        if (!allServicesProvideInterface)
            supportEstimate = qMax(QMultimedia::MaybeSupported, supportEstimate);

        return supportEstimate;
    }

    QStringList supportedMimeTypes(const QByteArray &serviceType, int flags) const override
    {
        const QList<QObject*> instances = loader()->instances(QLatin1String(serviceType));

        QStringList supportedTypes;

        for (QObject *obj : instances) {
            QMediaServiceSupportedFormatsInterface *iface =
                    qobject_cast<QMediaServiceSupportedFormatsInterface*>(obj);


            if (flags) {
                QMediaServiceFeaturesInterface *iface =
                        qobject_cast<QMediaServiceFeaturesInterface*>(obj);

                if (iface) {
                    QMediaServiceFeaturesInterface::Features features = iface->supportedFeatures(serviceType);

                    // If low latency playback was asked for, skip MIME types from services known
                    // not to provide low latency playback
                    if ((flags & QMediaPlayer::LowLatency) &&
                        !(features & QMediaServiceFeaturesInterface::LowLatencyPlayback))
                        continue;

                    //the same for QIODevice based streams support
                    if ((flags & QMediaPlayer::StreamPlayback) &&
                        !(features & QMediaServiceFeaturesInterface::StreamPlayback))
                            continue;

                    //the same for QAbstractVideoSurface support
                    if ((flags & QMediaPlayer::VideoSurface) &&
                        !(features & QMediaServiceFeaturesInterface::VideoSurface))
                            continue;
                }
            }

            if (iface) {
                supportedTypes << iface->supportedMimeTypes();
            }
        }

        // Multiple services may support the same MIME type
        supportedTypes.removeDuplicates();

        return supportedTypes;
    }

    QByteArray defaultDevice(const QByteArray &serviceType) const override
    {
        const auto instances = loader()->instances(QLatin1String(serviceType));
        for (QObject *obj : instances) {
            const QMediaServiceDefaultDeviceInterface *iface =
                    qobject_cast<QMediaServiceDefaultDeviceInterface*>(obj);

            if (iface) {
                QByteArray name = iface->defaultDevice(serviceType);
                if (!name.isEmpty())
                    return name;
            }
        }

        // if QMediaServiceDefaultDeviceInterface is not implemented, return the
        // first available device.
        QList<QByteArray> devs = devices(serviceType);
        if (!devs.isEmpty())
            return devs.first();

        return QByteArray();
    }

    QList<QByteArray> devices(const QByteArray &serviceType) const override
    {
        QList<QByteArray> res;

        const auto instances = loader()->instances(QLatin1String(serviceType));
        for (QObject *obj : instances) {
            QMediaServiceSupportedDevicesInterface *iface =
                    qobject_cast<QMediaServiceSupportedDevicesInterface*>(obj);

            if (iface) {
                res.append(iface->devices(serviceType));
            }
        }

        return res;
    }

    QString deviceDescription(const QByteArray &serviceType, const QByteArray &device) override
    {
        const auto instances = loader()->instances(QLatin1String(serviceType));
        for (QObject *obj : instances) {
            QMediaServiceSupportedDevicesInterface *iface =
                    qobject_cast<QMediaServiceSupportedDevicesInterface*>(obj);

            if (iface) {
                if (iface->devices(serviceType).contains(device))
                    return iface->deviceDescription(serviceType, device);
            }
        }

        return QString();
    }

    QCamera::Position cameraPosition(const QByteArray &device) const override
    {
        QMediaService *service = const_cast<QPluginServiceProvider *>(this)->requestService(Q_MEDIASERVICE_CAMERA);
        auto *deviceControl = qobject_cast<QVideoDeviceSelectorControl *>(service->requestControl(QVideoDeviceSelectorControl_iid));
        auto pos = QCamera::UnspecifiedPosition;
        for (int i = 0; i < deviceControl->deviceCount(); i++) {
            if (deviceControl->deviceName(i) == QString::fromUtf8(device)) {
                pos = deviceControl->cameraPosition(i);
                break;
            }
        }
        service->releaseControl(deviceControl);
        return pos;
    }

    int cameraOrientation(const QByteArray &device) const override
    {
        QMediaService *service = const_cast<QPluginServiceProvider *>(this)->requestService(Q_MEDIASERVICE_CAMERA);
        auto *deviceControl = qobject_cast<QVideoDeviceSelectorControl *>(service->requestControl(QVideoDeviceSelectorControl_iid));
        int orientation = 0;
        for (int i = 0; i < deviceControl->deviceCount(); i++) {
            if (deviceControl->deviceName(i) == QString::fromUtf8(device)) {
                orientation = deviceControl->cameraOrientation(i);
                break;
            }
        }
        service->releaseControl(deviceControl);
        return orientation;
    }
};

Q_GLOBAL_STATIC(QPluginServiceProvider, pluginProvider);

/*!
    \class QMediaServiceProvider
    \obsolete
    \ingroup multimedia
    \ingroup multimedia_control
    \ingroup multimedia_core

    \internal

    \brief The QMediaServiceProvider class provides an abstract allocator for media services.
*/

/*!
    \internal
    \fn QMediaServiceProvider::requestService(const QByteArray &type)

    Requests an instance of a \a type service which best matches the given \a
    hint.

    Returns a pointer to the requested service, or a null pointer if there is
    no suitable service.

    The returned service must be released with releaseService when it is
    finished with.
*/

/*!
    \internal
    \fn QMediaServiceProvider::releaseService(QMediaService *service)

    Releases a media \a service requested with requestService().
*/

/*!
    \internal
    \fn QMediaServiceProvider::supportedFeatures(const QMediaService *service) const

    Returns the features supported by a given \a service.
*/
QMediaServiceFeaturesInterface::Features QMediaServiceProvider::supportedFeatures(const QMediaService *service) const
{
    Q_UNUSED(service);

    return {};
}

/*!
    \internal
    Returns how confident a media service provider is that is can provide a \a
    serviceType service that is able to play media of a specific \a mimeType
    that is encoded using the listed \a codecs while adhering to constraints
    identified in \a flags.
*/
QMultimedia::SupportEstimate QMediaServiceProvider::hasSupport(const QByteArray &serviceType,
                                                        const QString &mimeType,
                                                        const QStringList& codecs,
                                                        int flags) const
{
    Q_UNUSED(serviceType);
    Q_UNUSED(mimeType);
    Q_UNUSED(codecs);
    Q_UNUSED(flags);

    return QMultimedia::MaybeSupported;
}

/*!
    \internal
    \fn QStringList QMediaServiceProvider::supportedMimeTypes(const QByteArray &serviceType, int flags) const

    Returns a list of MIME types supported by the service provider for the
    specified \a serviceType.

    The resultant list is restricted to MIME types which can be supported given
    the constraints in \a flags.
*/
QStringList QMediaServiceProvider::supportedMimeTypes(const QByteArray &serviceType, int flags) const
{
    Q_UNUSED(serviceType);
    Q_UNUSED(flags);

    return QStringList();
}

/*!
  \internal
  \since 5.3

  Returns the default device for a \a service type.
*/
QByteArray QMediaServiceProvider::defaultDevice(const QByteArray &serviceType) const
{
    Q_UNUSED(serviceType);
    return QByteArray();
}

/*!
  \internal
  Returns the list of devices related to \a service type.
*/
QList<QByteArray> QMediaServiceProvider::devices(const QByteArray &service) const
{
    Q_UNUSED(service);
    return QList<QByteArray>();
}

/*!
    \internal
    Returns the description of \a device related to \a serviceType, suitable for use by
    an application for display.
*/
QString QMediaServiceProvider::deviceDescription(const QByteArray &serviceType, const QByteArray &device)
{
    Q_UNUSED(serviceType);
    Q_UNUSED(device);
    return QString();
}

/*!
    \internal
    \since 5.3

    Returns the physical position of a camera \a device on the system hardware.
*/
QCamera::Position QMediaServiceProvider::cameraPosition(const QByteArray &device) const
{
    Q_UNUSED(device);
    return QCamera::UnspecifiedPosition;
}

/*!
    \internal
    \since 5.3

    Returns the physical orientation of the camera \a device. The value is the angle by which the
    camera image should be rotated anti-clockwise (in steps of 90 degrees) so it shows correctly on
    the display in its natural orientation.
*/
int QMediaServiceProvider::cameraOrientation(const QByteArray &device) const
{
    Q_UNUSED(device);
    return 0;
}

static QMediaServiceProvider *qt_defaultMediaServiceProvider = nullptr;

/*!
    Sets a media service \a provider as the default.
    It's useful for unit tests to provide mock service.

    \internal
*/
void QMediaServiceProvider::setDefaultServiceProvider(QMediaServiceProvider *provider)
{
    qt_defaultMediaServiceProvider = provider;
}


/*!
    \internal
    Returns a default provider of media services.
*/
QMediaServiceProvider *QMediaServiceProvider::defaultServiceProvider()
{
    return qt_defaultMediaServiceProvider != nullptr
            ? qt_defaultMediaServiceProvider
            : static_cast<QMediaServiceProvider *>(pluginProvider());
}

/*!
    \class QMediaServiceProviderPlugin
    \obsolete
    \inmodule QtMultimedia
    \brief The QMediaServiceProviderPlugin class interface provides an interface for QMediaService
    plug-ins.

    A media service provider plug-in may implement one or more of
    QMediaServiceSupportedFormatsInterface,
    QMediaServiceSupportedDevicesInterface, and QMediaServiceFeaturesInterface
    to identify the features it supports.
*/

/*!
    \fn QMediaServiceProviderPlugin::create(const QString &key)

    Constructs a new instance of the QMediaService identified by \a key.

    The QMediaService returned must be destroyed with release().
*/

/*!
    \fn QMediaServiceProviderPlugin::release(QMediaService *service)

    Destroys a media \a service constructed with create().
*/


/*!
    \class QMediaServiceSupportedFormatsInterface
    \obsolete
    \inmodule QtMultimedia
    \brief The QMediaServiceSupportedFormatsInterface class interface
    identifies if a media service plug-in supports a media format.

    A QMediaServiceProviderPlugin may implement this interface.
*/

/*!
    \fn QMediaServiceSupportedFormatsInterface::~QMediaServiceSupportedFormatsInterface()

    Destroys a media service supported formats interface.
*/

/*!
    \fn QMediaServiceSupportedFormatsInterface::hasSupport(const QString &mimeType, const QStringList& codecs) const

    Returns the level of support a media service plug-in has for a \a mimeType
    and set of \a codecs.
*/

/*!
    \fn QMediaServiceSupportedFormatsInterface::supportedMimeTypes() const

    Returns a list of MIME types supported by the media service plug-in.
*/

/*!
    \class QMediaServiceSupportedDevicesInterface
    \obsolete
    \inmodule QtMultimedia
    \brief The QMediaServiceSupportedDevicesInterface class interface
    identifies the devices supported by a media service plug-in.

    A QMediaServiceProviderPlugin may implement this interface.
*/

/*!
    \fn QMediaServiceSupportedDevicesInterface::~QMediaServiceSupportedDevicesInterface()

    Destroys a media service supported devices interface.
*/

/*!
    \fn QList<QByteArray> QMediaServiceSupportedDevicesInterface::devices(const QByteArray &service) const

    Returns a list of devices available for a \a service type.
*/

/*!
    \fn QString QMediaServiceSupportedDevicesInterface::deviceDescription(const QByteArray &service, const QByteArray &device)

    Returns the description of a \a device available for a \a service type.
*/

/*!
    \class QMediaServiceDefaultDeviceInterface
    \obsolete
    \inmodule QtMultimedia
    \brief The QMediaServiceDefaultDeviceInterface class interface
    identifies the default device used by a media service plug-in.

    A QMediaServiceProviderPlugin may implement this interface.

    \since 5.3
*/

/*!
    \fn QMediaServiceDefaultDeviceInterface::~QMediaServiceDefaultDeviceInterface()

    Destroys a media service default device interface.
*/

/*!
    \fn QByteArray QMediaServiceDefaultDeviceInterface::defaultDevice(const QByteArray &service) const

    Returns the default device for a \a service type.
*/

/*!
    \class QMediaServiceFeaturesInterface
    \obsolete
    \inmodule QtMultimedia
    \brief The QMediaServiceFeaturesInterface class interface identifies
    features supported by a media service plug-in.

    A QMediaServiceProviderPlugin may implement this interface.
*/

/*!
    \fn QMediaServiceFeaturesInterface::~QMediaServiceFeaturesInterface()

    Destroys a media service features interface.
*/
/*!
    \fn QMediaServiceFeaturesInterface::supportedFeatures(const QByteArray &service) const

    Returns a set of features supported by a plug-in \a service.
*/

QT_END_NAMESPACE

#include "moc_qmediaserviceprovider_p.cpp"
#include "moc_qmediaserviceproviderplugin.cpp"
