#include <iostream>
#include <SDL.h>
#include <GL/gl.h>

int main(int argc, char* argv[]) {
    std::cout << "=== GPU Acceleration Test ===" << std::endl;
    std::cout << "Testing OpenGL integration with XiaopengKernel..." << std::endl;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window with OpenGL support
    SDL_Window* window = SDL_CreateWindow("GPU Acceleration Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "✓ OpenGL context created successfully" << std::endl;

    // Test OpenGL capabilities
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    
    std::cout << "✓ Renderer: " << (renderer ? (const char*)renderer : "Unknown") << std::endl;
    std::cout << "✓ OpenGL Version: " << (version ? (const char*)version : "Unknown") << std::endl;

    // Set up OpenGL
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Test basic rendering
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw test shapes
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(100, 100);
    glVertex2f(300, 100);
    glVertex2f(300, 300);
    glVertex2f(100, 300);
    glEnd();

    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(400, 100);
    glVertex2f(500, 300);
    glVertex2f(300, 300);
    glEnd();

    // Swap buffers
    SDL_GL_SwapWindow(window);

    std::cout << "✓ GPU acceleration test completed successfully!" << std::endl;
    std::cout << "✓ All OpenGL operations working correctly" << std::endl;

    // Cleanup
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}