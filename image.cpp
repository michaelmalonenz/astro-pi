/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Ideas on Board Oy
 *
 * Multi-planar image with access to pixel data
 */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include "stb_image.h"
#include "stb_image_write.h"

#include "image.h"

#include <assert.h>
#include <errno.h>
#include <iostream>
#include <map>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>

#include <libcamera/formats.h>

#define IMAGE_COLOUR_SPACE_BYTES 3
#define JPEG_IMAGE_QUALITY 90

#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)

// RGB -> YUV
#define RGB2Y(R, G, B) CLIP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
#define RGB2U(R, G, B) CLIP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLIP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)

// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )

#define YUV2R(Y, U, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U, V) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)

using namespace libcamera;

static PixelColourFormat getPixelFormat(const libcamera::PixelFormat& format)
{
    switch (format)
    {
        case libcamera::formats::XBGR8888: return PixelColourFormat::XBGR8888;
        case libcamera::formats::RGBX8888: return PixelColourFormat::RGBX8888;
        case libcamera::formats::BGRX8888: return PixelColourFormat::BGRX8888;
        case libcamera::formats::YUYV: return PixelColourFormat::YUYV;
        case libcamera::formats::YVYU: return PixelColourFormat::YVYU;
        case libcamera::formats::UYVY: return PixelColourFormat::UYVY;
        case libcamera::formats::VYUY: return PixelColourFormat::VYUY;
        case libcamera::formats::AVUY8888: return PixelColourFormat::AVUY8888;
        case libcamera::formats::XVUY8888: return PixelColourFormat::XVUY8888;
        case libcamera::formats::YUV420: return PixelColourFormat::YUV420;
        case libcamera::formats::YVU420: return PixelColourFormat::YVU420;
        case libcamera::formats::YUV422: return PixelColourFormat::YUV422;
        case libcamera::formats::YVU422: return PixelColourFormat::YVU422;
        case libcamera::formats::YUV444: return PixelColourFormat::YUV444;
        case libcamera::formats::YVU444: return PixelColourFormat::YVU444;
        case libcamera::formats::MJPEG: return PixelColourFormat::MJPEG;
        case libcamera::formats::SRGGB12: return PixelColourFormat::SRGGB12;
        case libcamera::formats::SGRBG12: return PixelColourFormat::SGRBG12;
        case libcamera::formats::SGBRG12: return PixelColourFormat::SGBRG12;
        case libcamera::formats::SBGGR12: return PixelColourFormat::SBGGR12;
    }
    return PixelColourFormat::XRGB8888;
}


std::unique_ptr<Image> Image::fromFrameBuffer(const FrameBuffer *buffer, MapMode mode, const libcamera::StreamConfiguration& config)
{
    std::unique_ptr<Image> image{new Image()};
    image->m_width = config.size.width;
    image->m_height = config.size.height;
    image->m_format = getPixelFormat(config.pixelFormat);
    image->m_stride = config.stride;

    assert(!buffer->planes().empty());

    int mmapFlags = 0;

    if (mode & MapMode::ReadOnly)
        mmapFlags |= PROT_READ;

    if (mode & MapMode::WriteOnly)
        mmapFlags |= PROT_WRITE;

    struct MappedBufferInfo
    {
        uint8_t *address = nullptr;
        size_t mapLength = 0;
        size_t dmabufLength = 0;
    };
    std::map<int, MappedBufferInfo> mappedBuffers;

    for (const FrameBuffer::Plane &plane : buffer->planes())
    {
        const int fd = plane.fd.get();
        if (mappedBuffers.find(fd) == mappedBuffers.end())
        {
            const size_t length = lseek(fd, 0, SEEK_END);
            mappedBuffers[fd] = MappedBufferInfo{nullptr, 0, length};
        }

        const size_t length = mappedBuffers[fd].dmabufLength;

        if (plane.offset > length ||
            plane.offset + plane.length > length)
        {
            std::cerr << "plane is out of buffer: buffer length="
                      << length << ", plane offset=" << plane.offset
                      << ", plane length=" << plane.length
                      << std::endl;
            return nullptr;
        }
        size_t &mapLength = mappedBuffers[fd].mapLength;
        mapLength = std::max(mapLength,
                             static_cast<size_t>(plane.offset + plane.length));
    }

    for (const FrameBuffer::Plane &plane : buffer->planes())
    {
        const int fd = plane.fd.get();
        auto &info = mappedBuffers[fd];
        if (!info.address)
        {
            void *address = mmap(nullptr, info.mapLength, mmapFlags,
                                 MAP_SHARED, fd, 0);
            if (address == MAP_FAILED)
            {
                int error = -errno;
                std::cerr << "Failed to mmap plane: "
                          << strerror(-error) << std::endl;
                return nullptr;
            }

            info.address = static_cast<uint8_t *>(address);
            image->maps_.emplace_back(info.address, info.mapLength);
        }

        image->planes_.emplace_back(info.address + plane.offset, plane.length);
    }

    return image;
}

Image::Image() = default;

Image::~Image()
{
    for (Span<uint8_t> &map : maps_)
        munmap(map.data(), map.size());
}

unsigned int Image::numPlanes() const
{
    return planes_.size();
}

Span<uint8_t> Image::data(unsigned int plane)
{
    assert(plane < planes_.size());
    return planes_[plane];
}

Span<const uint8_t> Image::data(unsigned int plane) const
{
    assert(plane < planes_.size());
    return planes_[plane];
}

std::vector<uint8_t> Image::dataAsRGB565()
{
    std::vector<uint8_t> result;
    auto plane = planes_[0];
    for (int i = 0; i < plane.size(); i+=4) {
        uint16_t red = plane[i+2];
        uint16_t green = plane[i+1];
        uint16_t blue = plane[i];
        uint16_t rgb = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue >> 3) & 0x1F);
        result.push_back((uint8_t)rgb >> 8);
        result.push_back((uint8_t)rgb & 0xFF);
    }
    return result;
}

// Drops the 'X' component
std::vector<uint8_t> Image::dataAsRGB888()
{
    std::vector<uint8_t> result;
    auto plane = planes_[0];
    for (int i = 0; i < plane.size(); i+=4) {
        result.push_back((uint8_t)plane[i+2] >> 2 & 0x3F); // SSD1351 Format
        result.push_back((uint8_t)plane[i+1] >> 2 & 0x3F); // SSD1351 Format
        result.push_back((uint8_t)plane[i]   >> 2 & 0x3F); // SSD1351 Format
    }
    return result;
}

#define Y_OFFSET   16
#define UV_OFFSET 128
#define YUV2RGB_11  298
#define YUV2RGB_12   -1
#define YUV2RGB_13  409
#define YUV2RGB_22 -100
#define YUV2RGB_23 -210
#define YUV2RGB_32  519
#define YUV2RGB_33    0

// Drops the 'X' component
std::vector<uint8_t> Image::dataAsBGR888()
{
    std::vector<uint8_t> result;
    auto plane = planes_[0];
    if (m_format == PixelColourFormat::XRGB8888)
    {
        for (int i = 0; i < plane.size(); i+=4) {
            result.push_back((uint8_t)plane[i]);
            result.push_back((uint8_t)plane[i+1]);
            result.push_back((uint8_t)plane[i+2]);
        }
    }
    else if (m_format == PixelColourFormat::YUYV)
    {
        int y, u, v;
        int uv_r, uv_g, uv_b;
        for (int i = 0; i < plane.size(); i+=4) {
            u=plane[i+1]-UV_OFFSET;
            v=plane[i+3]-UV_OFFSET;
            uv_r=YUV2RGB_12*u+YUV2RGB_13*v;
            uv_g=YUV2RGB_22*u+YUV2RGB_23*v;
            uv_b=YUV2RGB_32*u+YUV2RGB_33*v;

            // 1st pixel
            y=YUV2RGB_11*(plane[i] -Y_OFFSET);
            result.push_back(CLIP((y + uv_r) >> 8)); // r
            result.push_back(CLIP((y + uv_g) >> 8)); // g
            result.push_back(CLIP((y + uv_b) >> 8)); // b

            // 2nd pixel
            y=YUV2RGB_11*(plane[i+2] -Y_OFFSET);
            result.push_back(CLIP((y + uv_r) >> 8)); // r
            result.push_back(CLIP((y + uv_g) >> 8)); // g
            result.push_back(CLIP((y + uv_b) >> 8)); // b
        }
    }
    return result;
    //320x240-YUYV/sYCC
}


static uint8_t color565_to_r(uint16_t color) {
    return ((color & 0xF800) >> 8);  // transform to rrrrrxxx
}
static uint8_t color565_to_g(uint16_t color) {
    return ((color & 0x07E0) >> 3);  // transform to ggggggxx
}
static uint8_t color565_to_b(uint16_t color) {
    return ((color & 0x001F) << 3);  // transform to bbbbbxxx
}

void Image::writeToFile(std::string filename)
{
    auto thread = std::thread([this, filename]{
        auto plane = planes_[0];
       std::vector<uint8_t> result;
       for (int i = 0; i < plane.size(); i+=3) {
           uint8_t red = plane[i+2];
           uint8_t green = plane[i+1];
           uint8_t blue = plane[i];
           result.push_back(red);
           result.push_back(green);
           result.push_back(blue);
       }

       std::cout << "Width: " << m_width << " Height: " << m_height << " Stride: " << m_stride << std::endl;

        stbi_write_jpg(filename.c_str(), m_width, m_height, IMAGE_COLOUR_SPACE_BYTES, result.data(), JPEG_IMAGE_QUALITY);

    });
    thread.detach();
}
