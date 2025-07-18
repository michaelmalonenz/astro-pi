#pragma once

#include <memory>
#include <libcamera/libcamera.h>

typedef void(*process_request_t)(libcamera::Request *);


class AstroCamera {

    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;
    std::vector<std::unique_ptr<libcamera::Request>> m_viewfinder_requests;
    std::unique_ptr<libcamera::CameraConfiguration> m_viewfinder_config;

    public:
        AstroCamera(std::shared_ptr<libcamera::Camera>);
        void requestStillFrame();
        void start(process_request_t processRequest);
        ~AstroCamera();

    private:
        std::vector<std::unique_ptr<libcamera::Request>> allocateStream(
            libcamera::StreamConfiguration &cfg, uint64_t cookie = 0);
};
