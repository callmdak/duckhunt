#include "game.h"
#include "renderer.h"
#include "resources.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>

int main(int argc, char* argv[])
{
    const std::filesystem::path resourceRoot = argc > 0 && argv[0] != nullptr
        ? std::filesystem::absolute(argv[0]).parent_path()
        : std::filesystem::current_path();
    GameConfig config;
    if (!loadConfig(resourceRoot, config))
    {
        return EXIT_FAILURE;
    }

    if (!glfwInit())
    {
        std::cerr << "Erro ao inicializar GLFW\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(config.width, config.height,
        "Duck Hunt", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Erro ao criar janela\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::cerr << "Erro ao inicializar GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwSwapInterval(1);

    Renderer renderer;
    if (!initializeRenderer(renderer, config, resourceRoot))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GameState game;
    game.shotsLeft = config.shotsPerRound;
    std::mt19937 random(std::random_device{}());
    GameContext context{ &game, &config, &random };
    launchDuck(game, config, random);
    glfwSetWindowUserPointer(window, &context);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(std::min(currentTime - previousTime, 0.05));
        previousTime = currentTime;

        updateGame(game, config, random, renderer.dog, deltaTime);
        updateRenderer(renderer, deltaTime);
        renderFrame(renderer, config, game, window);
        glfwPollEvents();
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    destroyRenderer(renderer);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}