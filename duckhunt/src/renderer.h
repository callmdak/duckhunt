#pragma once

#include "game_types.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <filesystem>

// reúne os recursos gráficos usados para desenhar uma cena
struct Renderer
{
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint textVao = 0;
    GLuint textVbo = 0;
    glm::mat4 projection{1.0f};
    SpriteTexture sky;
    SpriteTexture hills;
    SpriteTexture grass;
    SpriteTexture duck;
    SpriteTexture dog;
    SpriteTexture gun;
    SpriteTexture white;
    float hillsOffset = 0.0f;
    float grassOffset = 0.0f;
};

bool initializeRenderer(Renderer& renderer, const GameConfig& config,
    const std::filesystem::path& resourceRoot);
void updateRenderer(Renderer& renderer, float deltaTime);
void renderFrame(Renderer& renderer, const GameConfig& config,
    const GameState& game, GLFWwindow* window);
void destroyRenderer(Renderer& renderer);
