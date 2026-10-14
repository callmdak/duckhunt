#pragma once

#include <glad/gl.h>

#include <random>
#include <string>
#include <vector>

struct GameConfig
{
    int width = 800;
    int height = 600;
    int rounds = 10;
    int shotsPerRound = 3;
    float duckSpeed = 210.0f;
    std::string assets = "assets";
    std::string skyImage = "sky.sprite";
    std::string hillsImage = "hills.sprite";
    std::string grassImage = "grass.sprite";
    std::string duckImage = "duck.sprite";
    std::string dogImage = "dog.sprite";
    std::string gunImage = "gun.sprite";
    int duckFrames = 4;
};

struct SpriteTexture
{
    GLuint id = 0;
    int width = 0;
    int height = 0;
    int frameCount = 1;
    int currentFrame = 0;
    float frameTimerMs = 0.0f;
    std::vector<int> frameDelaysMs;
};

struct GameState
{
    float duckX = 0.0f;
    float duckY = 0.0f;
    float duckVelocityX = 0.0f;
    float duckVelocityY = 0.0f;
    float transitionTime = 0.0f;
    int round = 1;
    int shotsLeft = 3;
    int hits = 0;
    bool duckAlive = true;
    bool gameOver = false;
    bool started = false;
    bool dogCelebration = false;
    bool won = false;
    float dogCelebrationTimer = 0.0f;
};

struct GameContext
{
    GameState* game;
    const GameConfig* config;
    std::mt19937* random;
};
