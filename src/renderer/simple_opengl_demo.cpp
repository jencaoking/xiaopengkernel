#include <iostream>
#include <SDL.h>
#include <GL/gl.h>
#include <cmath>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("OpenGL GPU Acceleration Demo",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "OpenGL context created successfully!" << std::endl;
    std::cout << "GPU Acceleration Demo - Rendering with OpenGL" << std::endl;

    // Set up OpenGL
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    bool running = true;
    SDL_Event event;
    int frameCount = 0;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw a red rectangle (GPU accelerated)
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(100, 100);
        glVertex2f(700, 100);
        glVertex2f(700, 500);
        glVertex2f(100, 500);
        glEnd();

        // Draw a green triangle (GPU accelerated)
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(400, 200);
        glVertex2f(500, 400);
        glVertex2f(300, 400);
        glEnd();

        // Draw a blue circle (GPU accelerated)
        glColor3f(0.0f, 0.0f, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(600, 300); // Center
        for (int i = 0; i <= 360; i += 10) {
            float angle = i * 3.14159f / 180.0f;
            float x = 600 + 50 * cos(angle);
            float y = 300 + 50 * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();

        // Swap buffers
        SDL_GL_SwapWindow(window);

        // Calculate FPS
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - startTime >= 1000) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            startTime = currentTime;
        }

        // Limit to 60 FPS
        SDL_Delay(16);
    }

    std::cout << "GPU Acceleration Demo completed successfully!" << std::endl;

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}