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

#define IMAGE_COLOUR_SPACE_BYTES 3
#define JPEG_IMAGE_QUALITY 90

using namespace libcamera;

std::unique_ptr<Image> Image::fromFrameBuffer(const FrameBuffer *buffer, MapMode mode, int width, int height)
{
    std::unique_ptr<Image> image{new Image()};
    image->m_width = width;
    image->m_height = height;

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
        uint16_t rgb = ((red & 0b11111000) << 8) | ((green & 0b11111100) << 3) | (blue >> 3);
        result.push_back((uint8_t)rgb >> 8);
        result.push_back((uint8_t)rgb & 0xFF);
    }
    return result;
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
    auto plane = planes_[0];
    std::vector<uint8_t> result;
    for (int i = 0; i < plane.size(); i+=4) {
        uint16_t red = plane[i+2];
        uint16_t green = plane[i+1];
        uint16_t blue = plane[i];
        uint16_t rgb = ((red & 0xF8) << 8) | ((green & 0xFB) << 3) | (blue >> 3);
        result.push_back(color565_to_r(rgb));
        result.push_back(color565_to_g(rgb));
        result.push_back(color565_to_b(rgb));
    }

    stbi_write_jpg(filename.c_str(), m_width, m_height, IMAGE_COLOUR_SPACE_BYTES, result.data(), JPEG_IMAGE_QUALITY);
}
