#include <iostream>
#include "Window.h"

int main() {
    try {
        Window Window("DBGui", 1200, 800);

        while (!Window.ShouldWindowClose()) {
            Window.PollEvents();

            // Update logic

            Window.SwapBuffers();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
