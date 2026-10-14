#pragma once

#include "game_types.h"

#include <filesystem>
#include <string>

// carrega configuração, shaders e imagens usadas pelo jogo
bool loadConfig(const std::filesystem::path& resourceRoot, GameConfig& config);
GLuint createShaderProgram(const std::filesystem::path& resourceRoot);
bool loadImage(const std::string& path, SpriteTexture& sprite);
void advanceSpriteAnimation(SpriteTexture& sprite, float deltaTime);
GLuint createWhiteTexture();
