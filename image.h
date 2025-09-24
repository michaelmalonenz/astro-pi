/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Ideas on Board Oy
 *
 * Multi-planar image with access to pixel data
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include <libcamera/base/class.h>
#include <libcamera/base/flags.h>
#include <libcamera/base/span.h>

#include <libcamera/framebuffer.h>
#include <libcamera/stream.h>

enum PixelColourFormat {
    XRGB8888,
    XBGR8888,
    RGBX8888,
    BGRX8888,
    YUYV,
    YVYU,
    UYVY,
    VYUY,
    AVUY8888,
    XVUY8888,
    YUV420,
    YVU420,
    YUV422,
    YVU422,
    YUV444,
    YVU444,
    MJPEG,
    SRGGB12,
    SGRBG12,
    SGBRG12,
    SBGGR12,
};

class Image
{
public:
    enum class MapMode
    {
        ReadOnly = 1 << 0,
        WriteOnly = 1 << 1,
        ReadWrite = ReadOnly | WriteOnly,
    };

    static std::unique_ptr<Image> fromFrameBuffer(
        const libcamera::FrameBuffer *buffer, MapMode mode, const libcamera::StreamConfiguration& config);

    static std::unique_ptr<Image> copyFromFrameBuffer(
        const libcamera::FrameBuffer *buffer, const libcamera::StreamConfiguration& config);

    ~Image();

    unsigned int numPlanes() const;

    libcamera::Span<uint8_t> data(unsigned int plane);
    libcamera::Span<const uint8_t> data(unsigned int plane) const;
    std::vector<uint8_t> dataAsRGB565();
    std::vector<uint8_t> dataAsRGB888();
    std::vector<uint8_t> dataAsBGR888();
    std::vector<uint8_t> dataAsXXR888();
    void writeToFile(std::string filename);

private:
    LIBCAMERA_DISABLE_COPY(Image)
    int m_width;
    int m_height;
    unsigned int m_stride;

    Image();

    std::vector<libcamera::Span<uint8_t>> maps_;
    std::vector<libcamera::Span<uint8_t>> planes_;
    std::vector<std::vector<uint8_t>> buffers_;
    PixelColourFormat m_format;
};

namespace libcamera
{
    LIBCAMERA_FLAGS_ENABLE_OPERATORS(Image::MapMode)
}