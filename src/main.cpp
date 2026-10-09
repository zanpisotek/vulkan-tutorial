//
// Created by zanpisotek on 10/7/26.
//
#include "HelloTriangleApplication.h"
int main() {

    try {
        HelloTriangleApplication app;
        app.Run();
    }catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
