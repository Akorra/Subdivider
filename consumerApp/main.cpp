#include <iostream>

#include "app.hpp"
#include "config.hpp"

int main() 
{
    std::cout << "==============================================\n";
    std::cout << "Subdivision Library Demo\n";
    std::cout << "==============================================\n";
    std::cout << "Build Configuration: " << Subdiv::BuildInfo::getConfigString() << "\n";

    App app(800, 600, "Subdivier Test");

    if (!app.Init())
    {
        std::cout << "----------------------------------------------\n\n";
        std::cout << "==============================================\n";
        std::cout << "Demo failed successfully!\n";
        std::cout << "==============================================\n";
        return -1;
    }

    std::cout << "----------------------------------------------\n";
    std::cout << "Controls \n";
    std::cout << "----------------------------------------------\n";
    std::cout << "  W      - Toggle wireframe\n";
    std::cout << "  S      - Toggle solid\n";
    std::cout << "  SPACE  - Toggle auto-rotate\n";
    std::cout << "  R      - Reset rotation\n";
    std::cout << "\n";
    std::cout << "  Arrow Keys - Rotate camera\n";
    std::cout << "  A/D    - Orbit camera horizontally (smooth)\n";
    std::cout << "  Q/E    - Orbit camera vertically (smooth)\n";
    std::cout << "  +/-    - Zoom in/out\n";
    std::cout << "  HOME   - Reset camera\n";
    std::cout << "\n";
    std::cout << "  ESC    - Exit\n";
    std::cout << "----------------------------------------------\n\n";

    app.Run();

    std::cout << "==============================================\n";
    std::cout << "Demo completed successfully!\n";
    std::cout << "==============================================\n";

    return 0;
}