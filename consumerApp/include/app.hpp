#pragma once

class GLFWwindow;

class App {
public:
    App(int width, int height, const char* title);
    ~App();

    bool Init();
    void Run();

private:
    int width, height;
    const char* title;
    GLFWwindow* window;

    void ProcessInput();
    void Update();
    void Render();
};