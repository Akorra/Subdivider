#include "App.hpp"

#include <glad/gl.h>   // GLAD1 or GLAD2 depending on what you use
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "control/mesh.hpp"
#include "render_mesh.hpp"

// Simple vertex shader
const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

// Solid fragment shader
const char* fragmentShaderSource = R"(
#version 460 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
}
)";

// Wireframe fragment shader
const char* wireframeFragmentSource = R"(
#version 460 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green wireframe
}
)";

App::App(int w, int h, const char* t) : width(w), height(h), title(t), window(nullptr) {}

App::~App() 
{
    CleanupGL();
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}

bool App::Init() 
{
    if (!glfwInit()) 
    {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) 
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(window, this);

    // Register callbacks
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    // --- Initialize GLAD ---
    if (!gladLoadGL(glfwGetProcAddress)) 
    { 
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";

    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0f); // Thicker wireframe lines

    // Initialize camera matrices
    UpdateProjection();
    UpdateCamera();

    model = glm::mat4(1.0f);

    if (!InitShaders()) return false;
    if (!InitMesh()) return false;

    return true;
}

bool App::InitShaders()
{
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed:\n" << infoLog << "\n";
        return false;
    }

    // Compile fragment shader (solid)
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed:\n" << infoLog << "\n";
        return false;
    }

    // Link solid shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << "\n";
        return false;
    }

    // Compile wireframe fragment shader
    unsigned int wireframeFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(wireframeFragment, 1, &wireframeFragmentSource, nullptr);
    glCompileShader(wireframeFragment);
    
    glGetShaderiv(wireframeFragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(wireframeFragment, 512, nullptr, infoLog);
        std::cerr << "Wireframe fragment shader compilation failed:\n" << infoLog << "\n";
        return false;
    }

    // Link wireframe shader program
    wireframeProgram = glCreateProgram();
    glAttachShader(wireframeProgram, vertexShader);
    glAttachShader(wireframeProgram, wireframeFragment);
    glLinkProgram(wireframeProgram);
    
    glGetProgramiv(wireframeProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(wireframeProgram, 512, nullptr, infoLog);
        std::cerr << "Wireframe shader program linking failed:\n" << infoLog << "\n";
        return false;
    }

    // Clean up shaders (no longer needed after linking)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(wireframeFragment);

    std::cout << "Shaders compiled and linked successfully\n";
    return true;
}

bool App::InitMesh()
{
    using namespace Subdiv;

    mesh = new Control::Mesh();

    // Create a simple cube
    auto v0 = mesh->addVertex({-1, -1, -1});
    auto v1 = mesh->addVertex({ 1, -1, -1});
    auto v2 = mesh->addVertex({ 1,  1, -1});
    auto v3 = mesh->addVertex({-1,  1, -1});
    auto v4 = mesh->addVertex({-1, -1,  1});
    auto v5 = mesh->addVertex({ 1, -1,  1});
    auto v6 = mesh->addVertex({ 1,  1,  1});
    auto v7 = mesh->addVertex({-1,  1,  1});

    // Add faces (quads)
    mesh->addFace({v0, v1, v2, v3}); // Front
    mesh->addFace({v5, v4, v7, v6}); // Back
    mesh->addFace({v4, v0, v3, v7}); // Left
    mesh->addFace({v1, v5, v6, v2}); // Right
    mesh->addFace({v3, v2, v6, v7}); // Top
    mesh->addFace({v4, v5, v1, v0}); // Bottom

    std::cout << "Created cube: " 
              << mesh->numVertices() << " vertices, "
              << mesh->numFaces() << " faces, "
              << mesh->numEdges() << " edges\n";

    // Build topology cache
    mesh->buildCache();
    
    // Create render mesh
    renderMesh = new RenderMesh(*mesh);
    renderMesh->build();

    std::cout << "RenderMesh: "
              << renderMesh->numTriangles() << " triangles, "
              << renderMesh->numWireframeLines() << " lines\n";

    // Setup VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Upload vertex positions
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 mesh->getPositionsBytes(),
                 mesh->getPositionsData(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Upload triangle indices
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 renderMesh->getTriangleIndicesBytes(),
                 renderMesh->getTriangleIndicesData(),
                 GL_STATIC_DRAW);

    // Upload wireframe indices (separate buffer)
    glGenBuffers(1, &wireframeEBO);
    
    glBindVertexArray(0);

    std::cout << "Mesh uploaded to GPU\n";
    return true;
}

void App::CleanupGL()
{
    if(mesh != nullptr)
        delete mesh;
    mesh = nullptr;

    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (wireframeEBO) glDeleteBuffers(1, &wireframeEBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (wireframeProgram) glDeleteProgram(wireframeProgram);
    
    delete renderMesh;
}

void App::UpdateProjection()
{
    float aspect = (float)width / (float)height;
    projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void App::UpdateCamera()
{
    // Convert spherical coordinates to cartesian
    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);
    
    glm::vec3 cameraPos;
    cameraPos.x = cameraTarget.x + cameraDistance * cos(pitchRad) * sin(yawRad);
    cameraPos.y = cameraTarget.y + cameraDistance * sin(pitchRad);
    cameraPos.z = cameraTarget.z + cameraDistance * cos(pitchRad) * cos(yawRad);
    
    view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
}

void App::Run() {
    while (!glfwWindowShouldClose(window)) {
        ProcessInput();
        Update();
        Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void App::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->OnKeyPress(key, action);
    }
}

void App::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->OnWindowResize(width, height);
    }
}

void App::ProcessInput() {
    // Continuous camera movement (hold keys)
    float cameraSpeed = 0.1f;
    
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cameraYaw -= cameraSpeed * 10.0f;
        UpdateCamera();
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraYaw += cameraSpeed * 10.0f;
        UpdateCamera();
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        cameraPitch = glm::clamp(cameraPitch + cameraSpeed * 10.0f, -89.0f, 89.0f);
        UpdateCamera();
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        cameraPitch = glm::clamp(cameraPitch - cameraSpeed * 10.0f, -89.0f, 89.0f);
        UpdateCamera();
    }
}

void App::Update() {
    // Future: update mesh or UI here

    // Rotate the cube
    if (autoRotate)
    {
        rotationAngle += 0.01f;
        model = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0.5f, 1.0f, 0.0f));
    }
}

void App::Render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Future: render meshes here

    // Compute MVP matrix
    glm::mat4 mvp = projection * view * model;

    glBindVertexArray(vao);

    // Render solid mesh
    if (showSolid)
    {
        glUseProgram(shaderProgram);
        
        // Set uniforms
        int mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        
        int colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, 0.3f, 0.3f, 0.8f); // Blue
        
        // Draw triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glDrawElements(GL_TRIANGLES, 
                       renderMesh->numTriangles() * 3, 
                       GL_UNSIGNED_INT, 
                       nullptr);
    }

    // Render wireframe
    if (showWireframe)
    {
        glUseProgram(wireframeProgram);
        
        // Set uniforms
        int mvpLoc = glGetUniformLocation(wireframeProgram, "mvp");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        
        // Draw lines
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireframeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     renderMesh->getWireframeIndicesBytes(),
                     renderMesh->getWireframeIndicesData(),
                     GL_STATIC_DRAW);
        
        glDrawElements(GL_LINES, 
                       renderMesh->numWireframeLines() * 2, 
                       GL_UNSIGNED_INT, 
                       nullptr);
    }

    glBindVertexArray(0);
}

void App::OnKeyPress(int key, int action)
{
    if (action != GLFW_PRESS) return;
    
    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
            
        case GLFW_KEY_W:
            showWireframe = !showWireframe;
            std::cout << "Wireframe: " << (showWireframe ? "ON" : "OFF") << "\n";
            break;
            
        case GLFW_KEY_S:
            showSolid = !showSolid;
            std::cout << "Solid: " << (showSolid ? "ON" : "OFF") << "\n";
            break;
            
        case GLFW_KEY_SPACE:
            autoRotate = !autoRotate;
            std::cout << "Auto-rotate: " << (autoRotate ? "ON" : "OFF") << "\n";
            break;
            
        case GLFW_KEY_R:
            rotationAngle = 0.0f;
            std::cout << "Rotation reset\n";
            break;
            
        // Camera controls
        case GLFW_KEY_UP:
            cameraPitch = glm::clamp(cameraPitch + 5.0f, -89.0f, 89.0f);
            UpdateCamera();
            break;
            
        case GLFW_KEY_DOWN:
            cameraPitch = glm::clamp(cameraPitch - 5.0f, -89.0f, 89.0f);
            UpdateCamera();
            break;
            
        case GLFW_KEY_LEFT:
            cameraYaw -= 10.0f;
            UpdateCamera();
            break;
            
        case GLFW_KEY_RIGHT:
            cameraYaw += 10.0f;
            UpdateCamera();
            break;
            
        case GLFW_KEY_EQUAL: // Plus key (zoom in)
        case GLFW_KEY_KP_ADD:
            cameraDistance = glm::max(1.0f, cameraDistance - 0.5f);
            UpdateCamera();
            break;
            
        case GLFW_KEY_MINUS: // Minus key (zoom out)
        case GLFW_KEY_KP_SUBTRACT:
            cameraDistance = glm::min(20.0f, cameraDistance + 0.5f);
            UpdateCamera();
            break;
            
        case GLFW_KEY_HOME:
            // Reset camera
            cameraYaw = 0.0f;
            cameraPitch = 30.0f;
            cameraDistance = 5.0f;
            UpdateCamera();
            std::cout << "Camera reset\n";
            break;
    }
}

void App::OnWindowResize(int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;
    
    // Update OpenGL viewport
    glViewport(0, 0, width, height);
    
    // Update projection matrix with new aspect ratio
    UpdateProjection();
    
    std::cout << "Window resized to " << width << "x" << height 
              << " (aspect: " << ((float)width / height) << ")\n";
}