#pragma once

#include "game_types.h"

#include <GLFW/glfw3.h>

void launchDuck(GameState& game, const GameConfig& config, std::mt19937& random);
void resetGame(GameContext& context, bool startImmediately);
void startNextRound(GameState& game, const GameConfig& config, std::mt19937& random,
    SpriteTexture& dog);
void updateGame(GameState& game, const GameConfig& config, std::mt19937& random,
    SpriteTexture& dog, float deltaTime);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int modifiers);
