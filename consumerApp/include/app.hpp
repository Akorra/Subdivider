#pragma once

#include "glm/glm.hpp"
#include <unordered_map>

struct GLFWwindow;
class RenderMesh;

namespace Subdiv::Control { class Mesh; }

class App {
public:
    App(int width, int height, const char* title);
    ~App();

    bool Init();
    void Run();

    // Callbacks
    void OnKeyPress(int key, int action);
    void OnWindowResize(int width, int height);
    void OnMouseButton(int button, int action, int mods);
    void OnCursorPos(double xpos, double ypos);
    void OnScroll(double xoffset, double yoffset);

private:
    void ProcessInput();
    void Update();
    void Render();

    bool InitShaders();
    bool InitMesh();
    void CleanupGL();

    void UpdateProjection();
    void UpdateCamera();

    // Static callback wrappers
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    int width, height;
    const char* title;
    GLFWwindow* window;

    // Key state tracking
    std::unordered_map<int, bool> keyPressed;
    std::unordered_map<int, bool> keyPressedLastFrame;

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

    // Camera parameters
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // Look at target
    float cameraDistance = 5.0f;                           // Distance from target
    float cameraYaw = 0.0f;                                // Horizontal angle
    float cameraPitch = 30.0f;                             // Vertical angle (degrees)
    float fov = 45.0f;                                     // Field of view
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    // Rendering state
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;

    float rotationAngle = 0.0f;
    bool  showWireframe = true;
    bool  showSolid     = true;
    bool  animate       = true;

    // Mouse state
    bool   isTumbling = false;   // LMB held
    bool   isPanning  = false;   // MMB held (or Alt+LMB)
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    
    // Sensitivity tuning
    float tumbleSensitivity = 0.3f;  // degrees per pixel
    float panSensitivity    = 0.005f; // world units per pixel (scaled by distance)
    float zoomSensitivity   = 0.3f;  // distance units per scroll tick
};