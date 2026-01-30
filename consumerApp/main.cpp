#include <iostream>

#include "app.hpp"
#include "config.hpp"

int main() 
{
    std::cout << "==============================================\n";
    std::cout << "Subdivision Library Demo\n";
    std::cout << "==============================================\n";
    std::cout << "Build Configuration: " << Subdiv::BuildInfo::getConfigString() << "\n";
    std::cout << "----------------------------------------------\n\n";

    App app(800, 600, "Subdivier Test");

    if (!app.Init()) return -1;

    app.Run();

    std::cout << "==============================================\n";
    std::cout << "Demo completed successfully!\n";
    std::cout << "==============================================\n";

    return 0;
}