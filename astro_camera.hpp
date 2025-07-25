#pragma once

#include <memory>
#include <libcamera/libcamera.h>

typedef void(*process_request_t)(libcamera::Request *);
#define VIEWFINDER_COOKIE 0x0001
#define STILL_CAPTURE_COOKIE 0x0010

class AstroCamera {

    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;
    std::vector<std::unique_ptr<libcamera::Request>> m_viewfinder_requests;
    std::unique_ptr<libcamera::CameraConfiguration> m_viewfinder_config;
    std::vector<std::unique_ptr<libcamera::Request>> m_still_requests;
    std::unique_ptr<libcamera::CameraConfiguration> m_still_config;

    public:
        AstroCamera(std::shared_ptr<libcamera::Camera>, process_request_t processRequest);
        void requestStillFrame();
        void start();
        void startPreview();
        void queueRequest(libcamera::Request *request);
        ~AstroCamera();

    private:
        std::vector<std::unique_ptr<libcamera::Request>> allocateStream(
            libcamera::StreamConfiguration &cfg, uint64_t cookie = 0);
};
