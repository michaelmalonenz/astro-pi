#pragma once

#include <memory>
#include <libcamera/libcamera.h>

typedef void(*process_request_t)(libcamera::Request *);
#define VIEWFINDER_COOKIE 0x0001
#define STILL_CAPTURE_COOKIE 0x0010

class AstroCamera {

    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;
    std::unique_ptr<libcamera::CameraConfiguration> m_config;
    std::vector<std::unique_ptr<libcamera::Request>> m_viewfinder_requests;
    std::vector<std::unique_ptr<libcamera::Request>> m_still_requests;
    process_request_t m_request_processor;
    uint16_t m_display_width;
    uint16_t m_display_height;

    public:
        AstroCamera(std::shared_ptr<libcamera::Camera>, process_request_t processRequest, uint16_t width, uint16_t height);
        void requestStillFrame();
        void start();
        void startPreview();
        void queueRequest(libcamera::Request *request);
        ~AstroCamera();

    private:
        std::vector<std::unique_ptr<libcamera::Request>> allocateStream(
            libcamera::StreamConfiguration &cfg, uint64_t cookie = 0);
};
