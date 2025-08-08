#include <stdexcept>
#include "astro_camera.hpp"

using namespace libcamera;

AstroCamera::AstroCamera(std::shared_ptr<Camera> camera, process_request_t processRequest)
    : m_camera(camera), m_request_processor(processRequest)
{
    m_allocator = std::make_unique<FrameBufferAllocator>(m_camera);
    m_camera->requestCompleted.connect(m_request_processor);
}

AstroCamera::~AstroCamera()
{
    m_camera->stop();
    m_allocator->free(m_viewfinder_config->at(0).stream());
    m_allocator->free(m_viewfinder_config->at(1).stream());
    for (auto &request : m_still_requests)
        request.reset();
    m_still_requests.clear();
    for (auto &request : m_viewfinder_requests)
        request.reset();
    m_viewfinder_requests.clear();
    m_viewfinder_config.reset();
    m_allocator.reset();
    m_camera->release();
    m_camera.reset();
}

void AstroCamera::start()
{
    m_viewfinder_config = m_camera->generateConfiguration({StreamRole::Viewfinder, StreamRole::StillCapture});
    if (m_viewfinder_config == NULL)
    {
        throw std::runtime_error{"Unable to generate configuration"};
    }
    StreamConfiguration &viewFinderStreamConfig = m_viewfinder_config->at(0);
    std::cout << "Default ViewFinder configuration is: " << viewFinderStreamConfig.toString() << std::endl;

    viewFinderStreamConfig.pixelFormat = libcamera::formats::RGB888;
    viewFinderStreamConfig.size.width = 320;
    viewFinderStreamConfig.size.height = 240;

    m_viewfinder_config->validate();
    std::cout << "Validated ViewFinder configuration is: " << viewFinderStreamConfig.toString() << std::endl;
    if (m_camera->configure(m_viewfinder_config.get()) < 0)
    {
        throw std::runtime_error("Failed to configure camera");
    }
    m_viewfinder_requests = allocateStream(m_viewfinder_config->at(0), VIEWFINDER_COOKIE);
    m_still_requests = allocateStream(m_viewfinder_config->at(1), STILL_CAPTURE_COOKIE);
    m_camera->start();
    startPreview();
}

void AstroCamera::startPreview()
{
    for (std::unique_ptr<Request> &request : m_viewfinder_requests)
    {
        m_camera->queueRequest(request.get());
    }
}

std::vector<std::unique_ptr<Request>> AstroCamera::allocateStream(StreamConfiguration &cfg, uint64_t cookie)
{
    std::vector<std::unique_ptr<Request>> requests;
    int ret = m_allocator->allocate(cfg.stream());
    if (ret < 0)
    {
        throw std::runtime_error("Can't allocate buffers");
    }

    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = m_allocator->buffers(cfg.stream());
    size_t allocated = buffers.size();
    std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    for (unsigned int i = 0; i < buffers.size(); ++i)
    {
        std::unique_ptr<Request> request = m_camera->createRequest(cookie);
        if (!request)
        {
            throw std::runtime_error("Can't create request");
        }

        if (cookie == STILL_CAPTURE_COOKIE)
        {
            ControlList &controls = request->controls();
//            controls.set(controls::ExposureTime, (int32_t)10000);
//            controls.set(controls::AnalogueGain, (float)8.0); // ISO800
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(cfg.stream(), buffer.get());
        if (ret < 0)
        {
            throw std::runtime_error("Can't set buffer for request");
        }

        requests.push_back(std::move(request));
    }

    return requests;
}

void AstroCamera::requestStillFrame()
{
    for (std::unique_ptr<Request> &request : m_still_requests)
    {
        std::cout << "Queuing Still Request" << std::endl;
        m_camera->queueRequest(request.get());
    }
}

void AstroCamera::queueRequest(Request *request)
{
    m_camera->queueRequest(request);
}
