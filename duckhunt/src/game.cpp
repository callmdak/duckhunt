#include "game.h"

#include "resources.h"

#include <algorithm>
#include <cmath>
#include <random>

void launchDuck(GameState& game, const GameConfig& config, std::mt19937& random)
{
    // escolhe uma posição e direção aleatórias para o pato
    std::uniform_real_distribution<float> xPosition(80.0f,
        static_cast<float>(config.width) - 152.0f);
    std::uniform_real_distribution<float> yPosition(70.0f,
        std::max(70.0f, std::min(330.0f, config.height * 0.62f - 80.0f)));
    std::uniform_int_distribution<int> direction(0, 1);
    std::uniform_real_distribution<float> vertical(-0.65f, 0.65f);
    game.duckX = xPosition(random);
    game.duckY = yPosition(random);
    game.duckVelocityX = config.duckSpeed * (direction(random) == 0 ? -1.0f : 1.0f);
    game.duckVelocityY = config.duckSpeed * vertical(random);
    game.duckAlive = true;
}

void resetGame(GameContext& context, bool startImmediately)
{
    // recria o estado inicial sem substituir a configuração
    *context.game = GameState{};
    context.game->shotsLeft = context.config->shotsPerRound;
    context.game->started = startImmediately;
    launchDuck(*context.game, *context.config, *context.random);
}

void startNextRound(GameState& game, const GameConfig& config, std::mt19937& random,
    SpriteTexture& dog)
{
    // avança a rodada ou decide se a partida terminou
    ++game.round;
    game.transitionTime = 0.0f;
    game.shotsLeft = config.shotsPerRound;
    if (game.round > config.rounds)
    {
        game.won = game.hits >= config.rounds;
        if (game.won)
        {
            game.dogCelebration = true;
            game.dogCelebrationTimer = 0.0f;
            dog.currentFrame = 0;
            dog.frameTimerMs = 0.0f;
        }
        else
        {
            game.gameOver = true;
        }
        return;
    }
    launchDuck(game, config, random);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
    {
        return;
    }

    auto* context = static_cast<GameContext*>(glfwGetWindowUserPointer(window));
    if (context == nullptr || context->game == nullptr || !context->game->started ||
        context->game->gameOver ||
        context->game->shotsLeft <= 0 || !context->game->duckAlive)
    {
        return;
    }

    GameState* game = context->game;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    --game->shotsLeft;

    // considera acerto quando o clique cai dentro do quadrado do pato
    constexpr float duckSize = 72.0f;
    if (mouseX >= game->duckX && mouseX <= game->duckX + duckSize &&
        mouseY >= game->duckY && mouseY <= game->duckY + duckSize)
    {
        ++game->hits;
        game->duckAlive = false;
        game->transitionTime = 0.0f;
    }
    else if (game->shotsLeft == 0)
    {
        game->gameOver = true;
        game->won = false;
    }
}

void keyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (action == GLFW_PRESS && (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER ||
        key == GLFW_KEY_SPACE))
    {
        auto* context = static_cast<GameContext*>(glfwGetWindowUserPointer(window));
        if (context != nullptr && context->game != nullptr &&
            context->config != nullptr && context->random != nullptr)
        {
            if (context->game->gameOver)
            {
                resetGame(*context, false);
            }
            else
            {
                context->game->started = true;
            }
        }
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        auto* context = static_cast<GameContext*>(glfwGetWindowUserPointer(window));
        if (context != nullptr && context->game != nullptr &&
            context->config != nullptr && context->random != nullptr)
        {
            resetGame(*context, true);
        }
    }
}

void updateGame(GameState& game, const GameConfig& config, std::mt19937& random,
    SpriteTexture& dog, float deltaTime)
{
    if (game.started && !game.gameOver)
    {
        if (game.duckAlive)
        {
            // move o pato e inverte a direção ao tocar nos limites
            game.duckX += game.duckVelocityX * deltaTime;
            game.duckY += game.duckVelocityY * deltaTime;
            if (game.duckX < 0.0f || game.duckX > config.width - 72.0f)
            {
                game.duckVelocityX = -game.duckVelocityX;
                game.duckX = std::clamp(game.duckX, 0.0f,
                    static_cast<float>(config.width) - 72.0f);
            }
            if (game.duckY < 35.0f || game.duckY > config.height * 0.62f - 72.0f)
            {
                game.duckVelocityY = -game.duckVelocityY;
                game.duckY = std::clamp(game.duckY, 35.0f,
                    config.height * 0.62f - 72.0f);
            }
        }

        if ((!game.duckAlive || game.shotsLeft == 0) && !game.dogCelebration)
        {
            // aguarda uma pausa curta antes de iniciar a próxima rodada
            game.transitionTime += deltaTime;
            if (game.transitionTime >= 0.65f)
            {
                startNextRound(game, config, random, dog);
            }
        }

        if (game.dogCelebration)
        {
            game.dogCelebrationTimer += deltaTime;
            advanceSpriteAnimation(dog, deltaTime);
            if (game.dogCelebrationTimer >= 2.0f)
            {
                game.dogCelebration = false;
                game.gameOver = true;
            }
        }
    }
}