#include <iomanip>
#include <iostream>
#include <sstream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <fcntl.h>

#define USE_SSD1351_DISPLAY (0)
#define USE_TP28017_DISPLAY (0)
#define WRITE_IMAGES_TO_FILE (0)
#define SHOW_IMAGE_METADATA (0)

#define BUTTON_GPIO_PIN 27

#include <libcamera/libcamera.h>
#include <wiringPi.h>
#include "event_loop.h"
#include "image.h"
#include "button.hpp"
#include "astro_camera.hpp"

#if USE_SSD1351_DISPLAY
#include "ssd1351.hpp"
#elif USE_TP28017_DISPLAY
#include "tp28017.hpp"
#endif

#if WRITE_IMAGES_TO_SERVER
#include "client.hpp"
#endif

using namespace libcamera;
using namespace std::chrono_literals;

#define TIMEOUT_SEC 3

static std::unique_ptr<AstroCamera> astro_cam;
static EventLoop loop;

#if USE_SSD1351_DISPLAY
static std::unique_ptr<Display> display;
#endif
#if USE_TP28017_DISPLAY
static std::unique_ptr<Tp28017> display;
#endif

static void processRequest(Request *request)
{
#if SHOW_IMAGE_METADATA
    std::cout << std::endl
              << "Request completed: " << request->toString() << std::endl;

    const ControlList &requestMetadata = request->metadata();
    for (const auto &ctrl : requestMetadata)
    {
        const ControlId *id = controls::controls.at(ctrl.first);
        const ControlValue &value = ctrl.second;

        std::cout << "\t" << id->name() << " = " << value.toString()
                  << std::endl;
    }
#endif

    const Request::BufferMap &buffers = request->buffers();
    for (auto bufferPair : buffers)
    {
        const Stream *const stream = bufferPair.first;
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();

#if SHOW_IMAGE_METADATA
        /* Print some information about the buffer which has completed. */
        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence
                  << " timestamp: " << metadata.timestamp
                  << " bytesused: ";

        unsigned int nplane = 0;
        for (const FrameMetadata::Plane &plane : metadata.planes())
        {
            std::cout << plane.bytesused;
            if (++nplane < metadata.planes().size())
                std::cout << "/";
        }

        std::cout << std::endl;
#endif

        /*
         * Image data can be accessed here, but the FrameBuffer
         * must be mapped by the application
         */
        auto &config = stream->configuration();
        std::unique_ptr<Image> image =
            Image::fromFrameBuffer(buffer, Image::MapMode::ReadOnly, config.size.width, config.size.height);

#if USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY
        if (request->cookie() == VIEWFINDER_COOKIE)
        {
            auto rgbData = image->dataAsRGB888();
            auto data = libcamera::Span(rgbData.data(), rgbData.size());
            const unsigned int bytesused = metadata.planes()[0].bytesused;
            const unsigned int length = std::min<unsigned int>(bytesused, data.size());
            display->drawImage(data);
        }
#endif
#if WRITE_IMAGES_TO_FILE
        std::stringstream ss;
        ss << "output/frame" << std::setw(6) << std::setfill('0') << metadata.sequence << ".jpg";
        image->writeToFile(ss.str());
#endif
        if (request->cookie() == STILL_CAPTURE_COOKIE)
        {
            // Get SD Card working. Write it there.
            std::stringstream ss;
            ss << "stills/frame" << std::setw(6) << std::setfill('0') << metadata.sequence << ".jpg";
            image->writeToFile(ss.str());
        }
    }

    /* Re-queue the Request to the camera. */
    request->reuse(Request::ReuseBuffers);
    if (request->cookie() == VIEWFINDER_COOKIE)
    {
        astro_cam->queueRequest(request);
    }
    else
    {
        astro_cam->startPreview();
    }
}

static void requestComplete(Request *request)
{
    if (request->status() == Request::RequestCancelled)
    {
        std::cout << "Request Cancelled" << std::endl;
        return;
    }

    loop.callLater(std::bind(&processRequest, request));
}


void deferredStillRequest()
{
    astro_cam->requestStillFrame();
}

static void shutterButtonPress()
{
    loop.callLater(deferredStillRequest);
}

int main()
{
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();

    auto cameras = cm->cameras();
    if (cameras.empty())
    {
        std::cout << "No cameras were identified on the system."
                  << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }

    std::string cameraId = cameras[0]->id();

    std::shared_ptr<Camera> camera = cm->get(cameraId);

    if (camera->acquire())
    {
        std::cout << "Failed to acquire camera" << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }

#ifdef __ARM_ARCH
    wiringPiSetup();
    wiringPiSetupGpio();

    std::unique_ptr<Button> shutter = std::make_unique<Button>(BUTTON_GPIO_PIN, &shutterButtonPress);
#endif

#if USE_SSD1351_DISPLAY
    //                                                   cs, dc, rst
    display = std::make_unique<Ssd1351>("/dev/spidev0.0", 8,  5,  6);
#elif USE_TP28017_DISPLAY
    //                                  cs, rs, rd, wr, rst)
    display = std::make_unique<Tp28017>(19, 16, 20, 26,   5);
#endif
#if USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY
    display->fillWithColour(0xff0000);
#endif

    astro_cam = std::make_unique<AstroCamera>(camera, &requestComplete);
    astro_cam->start();

    loop.timeout(TIMEOUT_SEC);
    int ret = loop.exec();
    std::cout << "Capture ran for " << TIMEOUT_SEC << " seconds and "
              << "stopped with exit status: " << ret << std::endl;

    astro_cam.reset();

    cm->stop();
#if USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY
    display->displayOff();
#endif

    return EXIT_SUCCESS;
}
