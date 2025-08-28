#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <iomanip>
#include "image_writer.hpp"

static std::mutex queue_lock;
static std::condition_variable cond_var;
static std::queue<std::unique_ptr<Image>> queue;
static std::unique_ptr<std::thread> worker;
static int frame_number;

void enqueue_image(std::unique_ptr<Image> image)
{
    std::unique_lock lock(queue_lock);
    queue.push(std::move(image));

    lock.unlock();
    cond_var.notify_one();
}

static void process_images()
{
    std::unique_lock lock(queue_lock);
    while (true)
    {
        cond_var.wait(lock);
        if (queue.empty()) // we've been notified with nothing in the queue - that means we're shutting down
            return;
        while (!queue.empty())
        {
            std::unique_ptr<Image> &image = queue.front();
            // figure out the filename
            std::stringstream ss;
            ss << "stills/frame" << std::setw(6) << std::setfill('0') << frame_number << ".jpg";
            image->writeToFile(ss.str());
            frame_number++;
            queue.pop();
        }
        lock.unlock();
    }
}

void start_image_processing()
{
    // need to initialise frame_number to a reasonable value.
    worker = std::make_unique<std::thread>(process_images);
}

void stop_image_processing()
{
    cond_var.notify_one();
    worker->join();
    worker.reset();
}