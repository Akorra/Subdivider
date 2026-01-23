#include <iostream>

#include "subdiv.hpp"
#include "app.hpp"

int main() {
    App app(800, 600, "Subdivier Test");

    if (!app.Init()) return -1;

    app.Run();
    return 0;
}