/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MFTRANSFORM_H
#define MFTRANSFORM_H

#include <mfapi.h>
#include <mfidl.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtMultimedia/qvideosurfaceformat.h>

QT_USE_NAMESPACE

class MFVideoProbeControl;

class MFTransform: public IMFTransform
{
public:
    MFTransform();
    ~MFTransform();

    void addProbe(MFVideoProbeControl* probe);
    void removeProbe(MFVideoProbeControl* probe);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFTransform methods
    STDMETHODIMP GetStreamLimits(DWORD *pdwInputMinimum, DWORD *pdwInputMaximum, DWORD *pdwOutputMinimum, DWORD *pdwOutputMaximum);
    STDMETHODIMP GetStreamCount(DWORD *pcInputStreams, DWORD *pcOutputStreams);
    STDMETHODIMP GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs);
    STDMETHODIMP GetInputStreamInfo(DWORD dwInputStreamID, MFT_INPUT_STREAM_INFO *pStreamInfo);
    STDMETHODIMP GetOutputStreamInfo(DWORD dwOutputStreamID, MFT_OUTPUT_STREAM_INFO *pStreamInfo);
    STDMETHODIMP GetAttributes(IMFAttributes **pAttributes);
    STDMETHODIMP GetInputStreamAttributes(DWORD dwInputStreamID, IMFAttributes **pAttributes);
    STDMETHODIMP GetOutputStreamAttributes(DWORD dwOutputStreamID, IMFAttributes **pAttributes);
    STDMETHODIMP DeleteInputStream(DWORD dwStreamID);
    STDMETHODIMP AddInputStreams(DWORD cStreams, DWORD *adwStreamIDs);
    STDMETHODIMP GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex, IMFMediaType **ppType);
    STDMETHODIMP GetOutputAvailableType(DWORD dwOutputStreamID,DWORD dwTypeIndex, IMFMediaType **ppType);
    STDMETHODIMP SetInputType(DWORD dwInputStreamID, IMFMediaType *pType, DWORD dwFlags);
    STDMETHODIMP SetOutputType(DWORD dwOutputStreamID, IMFMediaType *pType, DWORD dwFlags);
    STDMETHODIMP GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType **ppType);
    STDMETHODIMP GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType **ppType);
    STDMETHODIMP GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags);
    STDMETHODIMP GetOutputStatus(DWORD *pdwFlags);
    STDMETHODIMP SetOutputBounds(LONGLONG hnsLowerBound, LONGLONG hnsUpperBound);
    STDMETHODIMP ProcessEvent(DWORD dwInputStreamID, IMFMediaEvent *pEvent);
    STDMETHODIMP ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
    STDMETHODIMP ProcessInput(DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags);
    STDMETHODIMP ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER *pOutputSamples, DWORD *pdwStatus);

private:
    HRESULT OnFlush();
    static QVideoFrame::PixelFormat formatFromSubtype(const GUID& subtype);
    static QVideoSurfaceFormat videoFormatForMFMediaType(IMFMediaType *mediaType, int *bytesPerLine);
    QVideoFrame makeVideoFrame();
    QByteArray dataFromBuffer(IMFMediaBuffer *buffer, int height, int *bytesPerLine);

    long m_cRef;
    IMFMediaType *m_inputType;
    IMFMediaType *m_outputType;
    IMFSample *m_sample;
    QMutex m_mutex;

    QList<MFVideoProbeControl*> m_videoProbes;
    QMutex m_videoProbeMutex;

    QVideoSurfaceFormat m_format;
    int m_bytesPerLine;
};

#endif
