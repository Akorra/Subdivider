#pragma once

#include "glm/glm.hpp"

struct GLFWwindow;
class RenderMesh;

namespace Subdiv::Control { class Mesh; }

class App {
public:
    App(int width, int height, const char* title);
    ~App();

    bool Init();
    void Run();

private:
    void ProcessInput();
    void Update();
    void Render();

    bool InitShaders();
    bool InitMesh();
    void CleanupGL();

private:
    int width, height;
    const char* title;
    GLFWwindow* window;

    // Mesh data
    Subdiv::Control::Mesh* mesh;
    RenderMesh* renderMesh = nullptr;

    // OpenGL resources
    unsigned int shaderProgram = 0;
    unsigned int wireframeProgram = 0;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int wireframeEBO = 0;

    // Rendering state
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;

    float rotationAngle = 0.0f;
    bool showWireframe = true;
    bool showSolid = true;
};