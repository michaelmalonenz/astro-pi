#pragma once

#include <memory>
#include "image.h"

void enqueue_image(std::unique_ptr<Image> image);

void start_image_processing();

void stop_image_processing();