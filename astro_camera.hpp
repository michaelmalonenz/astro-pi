#pragma once

#include <memory>
#include <libcamera/libcamera.h>


class AstroCamera {

    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;

    public:
        AstroCamera(std::shared_ptr<libcamera::Camera>);
        void requestStillFrame();
};
