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

class Image
{
public:
    enum class MapMode
    {
        ReadOnly = 1 << 0,
        WriteOnly = 1 << 1,
        ReadWrite = ReadOnly | WriteOnly,
    };

    static std::unique_ptr<Image> fromFrameBuffer(const libcamera::FrameBuffer *buffer,
                                                  MapMode mode, int width, int height);

    ~Image();

    unsigned int numPlanes() const;

    libcamera::Span<uint8_t> data(unsigned int plane);
    libcamera::Span<const uint8_t> data(unsigned int plane) const;
    std::vector<uint8_t> dataAsRGB565();
    std::vector<uint8_t> dataAsRGB888();
    void writeToFile(std::string filename);

private:
    LIBCAMERA_DISABLE_COPY(Image)
    int m_width;
    int m_height;

    Image();

    std::vector<libcamera::Span<uint8_t>> maps_;
    std::vector<libcamera::Span<uint8_t>> planes_;
};

namespace libcamera
{
    LIBCAMERA_FLAGS_ENABLE_OPERATORS(Image::MapMode)
}