#include <stdexcept>
#include "astro_camera.hpp"

using namespace libcamera;

AstroCamera::AstroCamera(std::shared_ptr<Camera> camera) : m_camera(camera)
{
    m_allocator = std::make_unique<FrameBufferAllocator>(m_camera);
}

AstroCamera::~AstroCamera()
{
    m_allocator->free(m_viewfinder_config->at(0).stream());
    m_allocator.release();
}

void AstroCamera::start(process_request_t processRequest)
{
    m_viewfinder_config = m_camera->generateConfiguration({StreamRole::Viewfinder});
    StreamConfiguration &viewFinderStreamConfig = m_viewfinder_config->at(0);
    std::cout << "Default ViewFinder configuration is: " << viewFinderStreamConfig.toString() << std::endl;

    viewFinderStreamConfig.size.width = 128;
    viewFinderStreamConfig.size.height = 128;

    m_viewfinder_config->validate();
    std::cout << "Validated ViewFinder configuration is: " << viewFinderStreamConfig.toString() << std::endl;

    if (m_camera->configure(m_viewfinder_config.get()) < 0)
    {
        throw std::runtime_error("Failed to configure camera");
    }

    m_viewfinder_requests = allocateStream(m_viewfinder_config->at(0), VIEWFINDER_COOKIE);

    m_camera->requestCompleted.connect(processRequest);
    m_camera->start();
    for (std::unique_ptr<Request> &request : m_viewfinder_requests)
        m_camera->queueRequest(request.get());
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

void AstroCamera::requestStillFrame(process_request_t processRequest)
{
    auto config = m_camera->generateConfiguration({StreamRole::Viewfinder});
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default Still configuration is: " << streamConfig.toString() << std::endl;
    config->validate();
    std::cout << "Validated Still configuration is: " << streamConfig.toString() << std::endl;

    if (m_camera->configure(config.get()) < 0)
    {
        throw std::runtime_error("Failed to configure camera");
    }
    m_still_requests = allocateStream(config->at(0), STILL_CAPTURE_COOKIE);
    m_camera->requestCompleted.connect(processRequest);
    for (std::unique_ptr<Request> &request : m_still_requests)
        m_camera->queueRequest(request.get());
}