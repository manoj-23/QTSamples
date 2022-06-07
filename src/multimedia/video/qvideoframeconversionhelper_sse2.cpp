// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframeconversionhelper_p.h"

#ifdef QT_COMPILER_SUPPORTS_SSE2

QT_BEGIN_NAMESPACE

namespace  {

template<int a, int r, int b, int g>
void convert_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)
    quint32 *argb = reinterpret_cast<quint32*>(output);

    const __m128i zero = _mm_setzero_si128();
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    const uchar shuffle = _MM_SHUFFLE(a, r, b, g);
#else
    const uchar shuffle = _MM_SHUFFLE(3-a, 3-r, 3-b, 3-g);
#endif

    using Pixel = const ArgbPixel<a, r, g, b>;

    for (int y = 0; y < height; ++y) {
        auto *pixel = reinterpret_cast<const Pixel *>(src);

        int x = 0;
        ALIGN(16, argb, x, width) {
            *argb = pixel->convert();
            ++pixel;
            ++argb;
        }

        for (; x < width - 3; x += 4) {
            __m128i pixelData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pixel));
            pixel += 4;
            __m128i lowPixels = _mm_unpacklo_epi8(pixelData, zero);
            __m128i highPixels = _mm_unpackhi_epi8(pixelData, zero);
            lowPixels = _mm_shufflelo_epi16(_mm_shufflehi_epi16(lowPixels, shuffle), shuffle);
            highPixels = _mm_shufflelo_epi16(_mm_shufflehi_epi16(highPixels, shuffle), shuffle);
            pixelData = _mm_packus_epi16(lowPixels, highPixels);
            _mm_store_si128(reinterpret_cast<__m128i*>(argb), pixelData);
            argb += 4;
        }

        // leftovers
        for (; x < width; ++x) {
            *argb = pixel->convert();
            ++pixel;
            ++argb;
        }

        src += stride;
    }
}

}

void QT_FASTCALL qt_convert_ARGB8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_sse2<0, 1, 2, 3>(frame, output);
}

void QT_FASTCALL qt_convert_ABGR8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_sse2<0, 3, 2, 1>(frame, output);
}

void QT_FASTCALL qt_convert_RGBA8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_sse2<3, 0, 1, 2>(frame, output);
}

void QT_FASTCALL qt_convert_BGRA8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_sse2<3, 2, 1, 0>(frame, output);
}

QT_END_NAMESPACE

#endif
