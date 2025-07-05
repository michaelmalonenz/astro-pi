#include <iomanip>
#include <iostream>
#include <sstream>
#include <memory>
#include <thread>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <Magick++.h>

#define USE_SSD1351_DISPLAY (0)
#define USE_TP28017_DISPLAY (0)
#define WRITE_IMAGES_TO_FILE (!(USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY))
#define SHOW_IMAGE_METADATA (0)

#include <libcamera/libcamera.h>
#include "event_loop.h"
#include "image.h"

#if USE_SSD1351_DISPLAY
#include "ssd1351.hpp"
#elif USE_TP28017_DISPLAY
#include "tp28017.hpp"
#endif

using namespace libcamera;
using namespace std::chrono_literals;

#define TIMEOUT_SEC 3


static std::shared_ptr<Camera> camera;
static EventLoop loop;
static int _width;
static int _height;

#if USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY
static std::unique_ptr<Display> display;
#endif


static void processRequest(Request *request)
{
#if SHOW_IMAGE_METADATA
    std::cout << std::endl
              << "Request completed: " << request->toString() << std::endl;
#endif

    /*
     * When a request has completed, it is populated with a metadata control
     * list that allows an application to determine various properties of
     * the completed request. This can include the timestamp of the Sensor
     * capture, or its gain and exposure values, or properties from the IPA
     * such as the state of the 3A algorithms.
     *
     * ControlValue types have a toString, so to examine each request, print
     * all the metadata for inspection. A custom application can parse each
     * of these items and process them according to its needs.
     */
#if SHOW_IMAGE_METADATA
    const ControlList &requestMetadata = request->metadata();
    for (const auto &ctrl : requestMetadata)
    {
        const ControlId *id = controls::controls.at(ctrl.first);
        const ControlValue &value = ctrl.second;

        std::cout << "\t" << id->name() << " = " << value.toString()
                  << std::endl;
    }
#endif

    /*
     * Each buffer has its own FrameMetadata to describe its state, or the
     * usage of each buffer. While in our simple capture we only provide one
     * buffer per request, a request can have a buffer for each stream that
     * is established when configuring the camera.
     *
     * This allows a viewfinder and a still image to be processed at the
     * same time, or to allow obtaining the RAW capture buffer from the
     * sensor along with the image as processed by the ISP.
     */
    const Request::BufferMap &buffers = request->buffers();
    for (auto bufferPair : buffers)
    {
        // (Unused) Stream *stream = bufferPair.first;
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
        std::unique_ptr<Image> image =
            Image::fromFrameBuffer(buffer, Image::MapMode::ReadOnly);
        const unsigned int bytesused = metadata.planes()[0].bytesused;
        auto rgbData = image->dataAsRGB565();
        auto data = libcamera::Span(rgbData.data(), rgbData.size());
        const unsigned int length = std::min<unsigned int>(bytesused, data.size());

#if USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY
        display->drawImage(data);
#elif WRITE_IMAGES_TO_FILE
        auto imageData = image->data(0);
        Magick::Blob blob(imageData.data(), imageData.size());
        Magick::Geometry size(128, 128);
        Magick::Image my_image(blob, size);
        std::stringstream ss;
        ss << "output/frame" << std::setw(6) << std::setfill('0') << metadata.sequence << ".jpg";
        my_image.write(ss.str());
#endif
    }

    /* Re-queue the Request to the camera. */
    request->reuse(Request::ReuseBuffers);
    camera->queueRequest(request);
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

    camera = cm->get(cameraId);

    if (camera->acquire())
    {
        std::cout << "Failed to acquire camera" << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }

#if USE_SSD1351_DISPLAY
    //                                                   cs, dc, rst
    display = std::make_unique<Ssd1351>("/dev/spidev0.0", 8,  5,  6);
#elif USE_TP28017_DISPLAY
    //                                                   cs, dc, rst
    display = std::make_unique<Tp28017>("/dev/spidev0.0", 8,  5,  6);
#endif
#if USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY
    display->fillWithColour(0xf81e);
#endif

    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({StreamRole::Viewfinder});
    StreamConfiguration &viewFinderStreamConfig = config->at(0);
    std::cout << "Default ViewFinder configuration is: " << viewFinderStreamConfig.toString() << std::endl;

    // StreamConfiguration &stillCaptureStreamConfig = config->at(1);
    // std::cout << "Default StillCapture configuration is: " << stillCaptureStreamConfig.toString() << std::endl;

    viewFinderStreamConfig.size.width = 128;
    viewFinderStreamConfig.size.height = 128;

    config->validate();
    std::cout << "Validated ViewFinder configuration is: " << viewFinderStreamConfig.toString() << std::endl;
    // std::cout << "Validated StillCapture configuration is: " << stillCaptureStreamConfig.toString() << std::endl;
    // if (stillCaptureStreamConfig.colorSpace.has_value())
    // {
    //     std::cout << "Still Capture Colour Space: " << stillCaptureStreamConfig.colorSpace.value().toString() << std::endl;
    // }

    _width = viewFinderStreamConfig.size.width;
    _height = viewFinderStreamConfig.size.height;

    if (camera->configure(config.get()) < 0)
    {
        std::cout << "Failed to configure camera" << std::endl;
    }

    FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);

    for (StreamConfiguration &cfg : *config)
    {
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0)
        {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -ENOMEM;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }

    /*
     * --------------------------------------------------------------------
     * Frame Capture
     *
     * libcamera frames capture model is based on the 'Request' concept.
     * For each frame a Request has to be queued to the Camera.
     *
     * A Request refers to (at least one) Stream for which a Buffer that
     * will be filled with image data shall be added to the Request.
     *
     * A Request is associated with a list of Controls, which are tunable
     * parameters (similar to v4l2_controls) that have to be applied to
     * the image.
     *
     * Once a request completes, all its buffers will contain image data
     * that applications can access and for each of them a list of metadata
     * properties that reports the capture parameters applied to the image.
     */
    Stream *stream = viewFinderStreamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);
    std::vector<std::unique_ptr<Request>> requests;

    for (unsigned int i = 0; i < buffers.size(); ++i)
    {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request)
        {
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0)
        {
            std::cerr << "Can't set buffer for request"
                      << std::endl;
            return ret;
        }

        requests.push_back(std::move(request));
    }

    camera->requestCompleted.connect(requestComplete);

    camera->start();
    for (std::unique_ptr<Request> &request : requests)
        camera->queueRequest(request.get());

    /*
     * --------------------------------------------------------------------
     * Run an EventLoop
     *
     * In order to dispatch events received from the video devices, such
     * as buffer completions, an event loop has to be run.
     */
    loop.timeout(TIMEOUT_SEC);
    int ret = loop.exec();
    std::cout << "Capture ran for " << TIMEOUT_SEC << " seconds and "
              << "stopped with exit status: " << ret << std::endl;

    camera->stop();
    allocator->free(stream);
    delete allocator;
    camera->release();
    camera.reset();
    cm->stop();
#if USE_SSD1351_DISPLAY || USE_TP28017_DISPLAY
    display->displayOff();
#endif

    return EXIT_SUCCESS;
}
