#pragma once
#include <string>

#define VERSION_MAJOR 2
#define VERSION_MINOR 0
#define VERSION_PATCH 0

const std::string PRODUCT_NAME    = "DeltaVParser";
const std::string PRODUCT_VERSION = std::to_string(VERSION_MAJOR) + "." +
                                    std::to_string(VERSION_MINOR) + "." +
                                    std::to_string(VERSION_PATCH);