#include <condition_variable>
#include <mutex>
#include <queue>
#include <memory>
#include "image.h"

std::mutex queue_lock;
std::condition_variable cond_var;
std::queue<std::unique_ptr<Image>> queue;
std::unique_ptr<std::thread> worker;

void enqueue_image(std::unique_ptr<Image> image)
{
    const std::unique_lock lock(queue_lock);
    queue.push(image);

    lock.unlock();
    cond_var.notify_one();
}

static void process_images()
{
    const std::unique_lock lock(queue_lock);
    while (true)
    {
        cond_var.wait(lock);
        if (queue.empty()) // we've been notified with nothing in the queue - that means we're shutting down
            return;
        while (!queue.empty())
        {
            std::unique_ptr<Image> image = queue.pop();
            // figure out the filename
            image->writeToFile("stills/output.jpg");
        }
        lock.unlock();
    }
}

void start_image_processing()
{
    worker = std::make_unique<std::thread>(process_images);
}

void stop_image_processing()
{
    cond_var.notify_one();
    worker->join();
}