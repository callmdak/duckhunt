#include "renderer.h"

#include "resources.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

bool initializeRenderer(Renderer& renderer, const GameConfig& config,
    const std::filesystem::path& resourceRoot)
{
    auto& program = renderer.program;
    auto& vao = renderer.vao;
    auto& vbo = renderer.vbo;
    auto& ebo = renderer.ebo;
    auto& textVao = renderer.textVao;
    auto& textVbo = renderer.textVbo;
    auto& projection = renderer.projection;
    auto& sky = renderer.sky;
    auto& hills = renderer.hills;
    auto& grass = renderer.grass;
    auto& duck = renderer.duck;
    auto& dog = renderer.dog;
    auto& gun = renderer.gun;
    auto& white = renderer.white;
    program = createShaderProgram(resourceRoot);
    if (program == 0)
    {
        return false;
    }

    const float quadVertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };
    const unsigned int quadIndices[] = { 0, 1, 2, 2, 3, 0 };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenVertexArrays(1, &textVao);
    glGenBuffers(1, &textVbo);
    glBindVertexArray(textVao);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    projection = glm::ortho(0.0f, static_cast<float>(config.width),
        static_cast<float>(config.height), 0.0f, -1.0f, 1.0f);
    const std::filesystem::path assetRoot = resourceRoot / config.assets;
    const bool spritesLoaded =
        loadImage((assetRoot / config.skyImage).string(), sky) &&
        loadImage((assetRoot / config.hillsImage).string(), hills) &&
        loadImage((assetRoot / config.grassImage).string(), grass) &&
        loadImage((assetRoot / config.duckImage).string(), duck) &&
        loadImage((assetRoot / config.dogImage).string(), dog) &&
        loadImage((assetRoot / config.gunImage).string(), gun);
    if (!spritesLoaded)
    {
        destroyRenderer(renderer);
        return false;
    }

    std::string duckExtension = config.duckImage.substr(
        config.duckImage.find_last_of('.') == std::string::npos ? 0 :
            config.duckImage.find_last_of('.'));
    std::transform(duckExtension.begin(), duckExtension.end(), duckExtension.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (duckExtension != ".gif" && duck.frameCount == 1 && config.duckFrames > 1)
    {
        if (duck.width % config.duckFrames == 0)
        {
            duck.frameCount = config.duckFrames;
            duck.frameDelaysMs.resize(duck.frameCount, 100);
        }
        else
        {
            std::cerr << "duck_frames nao divide a largura de " << config.duckImage
                << "; o pato sera tratado como imagem estatica.\n";
        }
    }

    white = SpriteTexture{ createWhiteTexture(), 1, 1 };
    return true;
}

void drawSprite(GLuint program, GLuint vao, const SpriteTexture& texture,
    const glm::mat4& projection, float x, float y, float width, float height,
    float u0 = 0.0f, float u1 = 1.0f, bool flipX = false,
    const glm::vec4& tint = glm::vec4(1.0f), float rotationRadians = 0.0f,
    bool rotateAtBottom = false)
{
    float frameU0 = u0;
    float frameU1 = u1;
    if (texture.frameCount > 1 && u0 == 0.0f && u1 == 1.0f)
    {
        const float frameWidth = 1.0f / texture.frameCount;
        frameU0 = texture.currentFrame * frameWidth;
        frameU1 = frameU0 + frameWidth;
    }
    const float selectedLeftU = flipX ? frameU1 : frameU0;
    const float selectedRightU = flipX ? frameU0 : frameU1;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    if (rotationRadians != 0.0f)
    {
        const glm::vec3 pivot(x + width * 0.5f,
            rotateAtBottom ? y + height : y + height * 0.5f, 0.0f);
        model = glm::translate(glm::mat4(1.0f), pivot) *
            glm::rotate(glm::mat4(1.0f), rotationRadians, glm::vec3(0.0f, 0.0f, 1.0f)) *
            glm::translate(glm::mat4(1.0f), -pivot) * model;
    }
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1,
        GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1,
        GL_FALSE, glm::value_ptr(model));
    const glm::vec4 uvRect(selectedLeftU, 0.0f, selectedRightU, 1.0f);
    glUniform4fv(glGetUniformLocation(program, "uvRect"), 1, glm::value_ptr(uvRect));
    glUniform4fv(glGetUniformLocation(program, "tint"), 1, glm::value_ptr(tint));
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

const std::array<unsigned char, 7>& glyphFor(char character)
{
    static const std::array<unsigned char, 7> blank{};
    static const std::unordered_map<char, std::array<unsigned char, 7>> glyphs = {
        {'A', {14, 17, 17, 31, 17, 17, 17}}, {'B', {30, 17, 17, 30, 17, 17, 30}},
        {'C', {14, 17, 16, 16, 16, 17, 14}}, {'D', {30, 17, 17, 17, 17, 17, 30}},
        {'E', {31, 16, 16, 30, 16, 16, 31}}, {'F', {31, 16, 16, 30, 16, 16, 16}},
        {'G', {14, 17, 16, 23, 17, 17, 15}}, {'H', {17, 17, 17, 31, 17, 17, 17}},
        {'I', {14, 4, 4, 4, 4, 4, 14}}, {'J', {7, 2, 2, 2, 18, 18, 12}},
        {'K', {17, 18, 20, 24, 20, 18, 17}}, {'L', {16, 16, 16, 16, 16, 16, 31}},
        {'M', {17, 27, 21, 21, 17, 17, 17}}, {'N', {17, 25, 25, 21, 19, 19, 17}},
        {'O', {14, 17, 17, 17, 17, 17, 14}}, {'P', {30, 17, 17, 30, 16, 16, 16}},
        {'Q', {14, 17, 17, 17, 21, 18, 13}}, {'R', {30, 17, 17, 30, 20, 18, 17}},
        {'S', {15, 16, 16, 14, 1, 1, 30}}, {'T', {31, 4, 4, 4, 4, 4, 4}},
        {'U', {17, 17, 17, 17, 17, 17, 14}}, {'V', {17, 17, 17, 17, 17, 10, 4}},
        {'W', {17, 17, 17, 21, 21, 21, 10}}, {'X', {17, 17, 10, 4, 10, 17, 17}},
        {'Y', {17, 17, 10, 4, 4, 4, 4}}, {'Z', {31, 1, 2, 4, 8, 16, 31}},
        {'0', {14, 17, 19, 21, 25, 17, 14}}, {'1', {4, 12, 4, 4, 4, 4, 14}},
        {'2', {14, 17, 1, 2, 4, 8, 31}}, {'3', {30, 1, 1, 14, 1, 1, 30}},
        {'4', {2, 6, 10, 18, 31, 2, 2}}, {'5', {31, 16, 16, 30, 1, 1, 30}},
        {'6', {14, 16, 16, 30, 17, 17, 14}}, {'7', {31, 1, 2, 4, 8, 8, 8}},
        {'8', {14, 17, 17, 14, 17, 17, 14}}, {'9', {14, 17, 17, 15, 1, 1, 14}},
        {':', {0, 4, 4, 0, 4, 4, 0}}, {'/', {1, 2, 2, 4, 8, 8, 16}},
        {'-', {0, 0, 0, 31, 0, 0, 0}}, {'!', {4, 4, 4, 4, 4, 0, 4}},
        {'.', {0, 0, 0, 0, 0, 0, 4}}, {' ', {0, 0, 0, 0, 0, 0, 0}}
    };
    const auto found = glyphs.find(character);
    return found == glyphs.end() ? blank : found->second;
}

void drawText(GLuint program, GLuint vao, GLuint vbo, GLuint whiteTexture,
    const glm::mat4& projection, const std::string& text, float x, float y,
    float pixelSize, const glm::vec4& color)
{
    std::vector<float> vertices;
    vertices.reserve(text.size() * 5 * 7 * 6 * 4);
    for (std::size_t characterIndex = 0; characterIndex < text.size(); ++characterIndex)
    {
        const auto& glyph = glyphFor(text[characterIndex]);
        for (int row = 0; row < 7; ++row)
        {
            for (int column = 0; column < 5; ++column)
            {
                if ((glyph[row] & (1 << (4 - column))) == 0)
                {
                    continue;
                }
                const float left = x + (static_cast<float>(characterIndex) * 6.0f + column) * pixelSize;
                const float top = y + row * pixelSize;
                const float right = left + pixelSize;
                const float bottom = top + pixelSize;
                const float quad[] = {
                    left, top, 0.0f, 0.0f, right, top, 1.0f, 0.0f,
                    right, bottom, 1.0f, 1.0f, left, top, 0.0f, 0.0f,
                    right, bottom, 1.0f, 1.0f, left, bottom, 0.0f, 1.0f
                };
                vertices.insert(vertices.end(), std::begin(quad), std::end(quad));
            }
        }
    }

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1,
        GL_FALSE, glm::value_ptr(projection));
    const glm::mat4 identity(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1,
        GL_FALSE, glm::value_ptr(identity));
    const glm::vec4 uvRect(0.0f, 0.0f, 1.0f, 1.0f);
    glUniform4fv(glGetUniformLocation(program, "uvRect"), 1, glm::value_ptr(uvRect));
    glUniform4fv(glGetUniformLocation(program, "tint"), 1, glm::value_ptr(color));
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
        vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
}

void drawCenteredText(GLuint program, GLuint vao, GLuint vbo, GLuint whiteTexture,
    const glm::mat4& projection, const std::string& text, float centerX, float y,
    float pixelSize, const glm::vec4& color)
{
    const float textWidth = text.empty() ? 0.0f :
        (static_cast<float>(text.size()) * 6.0f - 1.0f) * pixelSize;
    drawText(program, vao, vbo, whiteTexture, projection, text,
        centerX - textWidth * 0.5f, y, pixelSize, color);
}

void updateRenderer(Renderer& renderer, float deltaTime)
{
    advanceSpriteAnimation(renderer.sky, deltaTime);
    advanceSpriteAnimation(renderer.hills, deltaTime);
    advanceSpriteAnimation(renderer.grass, deltaTime);
    advanceSpriteAnimation(renderer.duck, deltaTime);
    advanceSpriteAnimation(renderer.gun, deltaTime);
    renderer.hillsOffset = std::fmod(renderer.hillsOffset + deltaTime * 7.0f, 200.0f);
    renderer.grassOffset = std::fmod(renderer.grassOffset + deltaTime * 18.0f, 200.0f);
}

void renderFrame(Renderer& renderer, const GameConfig& config,
    const GameState& game, GLFWwindow* window)
{
    const auto& program = renderer.program;
    const auto& vao = renderer.vao;
    const auto& projection = renderer.projection;
    const auto& sky = renderer.sky;
    const auto& hills = renderer.hills;
    const auto& grass = renderer.grass;
    const auto& duck = renderer.duck;
    const auto& dog = renderer.dog;
    const auto& gun = renderer.gun;
    const auto& white = renderer.white;
    const auto& textVao = renderer.textVao;
    const auto& textVbo = renderer.textVbo;
    const auto hillsOffset = renderer.hillsOffset;
    const auto grassOffset = renderer.grassOffset;
    glViewport(0, 0, config.width, config.height);
    glClearColor(0.25f, 0.65f, 0.88f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    drawSprite(program, vao, sky, projection, 0.0f, 0.0f,
        static_cast<float>(config.width), static_cast<float>(config.height));

    for (int tile = -1; tile < config.width / 200 + 2; ++tile)
    {
        const float tileX = tile * 200.0f;
        drawSprite(program, vao, hills, projection, tileX - hillsOffset,
            config.height * 0.54f,
            200.0f, config.height * 0.30f);
    }

    if (game.dogCelebration)
    {
        const float rise = std::clamp(game.dogCelebrationTimer / 0.5f, 0.0f, 1.0f);
        const float dogSize = 100.0f;
        const float bushTop = config.height * 0.83f;
        const float dogY = bushTop + 6.0f - rise * dogSize;
        drawSprite(program, vao, dog, projection, config.width * 0.5f - dogSize * 0.5f,
            dogY, dogSize, dogSize);
    }
    for (int tile = -1; tile < config.width / 200 + 2; ++tile)
    {
        const float tileX = tile * 200.0f;
        drawSprite(program, vao, grass, projection, tileX - grassOffset,
            config.height * 0.83f, 200.0f, config.height * 0.17f);
    }
    if (game.duckAlive)
    {
        drawSprite(program, vao, duck, projection, game.duckX, game.duckY,
            72.0f, 72.0f, 0.0f, 1.0f, game.duckVelocityX < 0.0f);
    }

    if (game.started && !game.gameOver && !game.dogCelebration)
    {
        drawSprite(program, vao, white, projection, 0.0f, 0.0f,
            static_cast<float>(config.width), 48.0f, 0.0f, 1.0f, false,
            glm::vec4(0.08f, 0.13f, 0.14f, 0.82f));
        std::ostringstream roundLabel;
        roundLabel << "ROUND " << game.round << "/" << config.rounds;
        drawText(program, textVao, textVbo, white.id, projection,
            roundLabel.str(), 20.0f, 17.0f, 2.0f, glm::vec4(1.0f));
        std::ostringstream hitLabel;
        hitLabel << "ACERTOS " << game.hits;
        drawText(program, textVao, textVbo, white.id, projection,
            hitLabel.str(), config.width * 0.42f, 17.0f, 2.0f,
            glm::vec4(1.0f, 0.87f, 0.38f, 1.0f));
        std::ostringstream shotLabel;
        shotLabel << "TIROS " << game.shotsLeft;
        drawText(program, textVao, textVbo, white.id, projection,
            shotLabel.str(), config.width - 160.0f, 17.0f, 2.0f,
            glm::vec4(1.0f));

        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        const glm::vec4 crosshair(0.9f, 0.12f, 0.08f, 0.95f);
        drawSprite(program, vao, white, projection, static_cast<float>(mouseX) - 12.0f,
            static_cast<float>(mouseY) - 1.0f, 8.0f, 2.0f, 0.0f, 1.0f, false, crosshair);
        drawSprite(program, vao, white, projection, static_cast<float>(mouseX) + 4.0f,
            static_cast<float>(mouseY) - 1.0f, 8.0f, 2.0f, 0.0f, 1.0f, false, crosshair);
        drawSprite(program, vao, white, projection, static_cast<float>(mouseX) - 1.0f,
            static_cast<float>(mouseY) - 12.0f, 2.0f, 8.0f, 0.0f, 1.0f, false, crosshair);
        drawSprite(program, vao, white, projection, static_cast<float>(mouseX) - 1.0f,
            static_cast<float>(mouseY) + 4.0f, 2.0f, 8.0f, 0.0f, 1.0f, false, crosshair);

        const float gunWidth = 64.0f;
        const float gunHeight = 96.0f;
        const float gunX = std::clamp(static_cast<float>(mouseX) - gunWidth * 0.5f,
            0.0f, static_cast<float>(config.width) - gunWidth);
        const float gunY = static_cast<float>(config.height) - gunHeight;
        const float gunCenterX = gunX + gunWidth * 0.5f;
        const float aimAngle = std::clamp(
            std::atan2(gunCenterX - static_cast<float>(mouseX),
                static_cast<float>(config.height) - static_cast<float>(mouseY)),
            -1.25f, 1.25f);
        drawSprite(program, vao, gun, projection, gunX, gunY, gunWidth, gunHeight,
            0.0f, 1.0f, false, glm::vec4(1.0f), aimAngle, true);
    }

    if (!game.started)
    {
        drawSprite(program, vao, white, projection, config.width * 0.12f,
            config.height * 0.22f, config.width * 0.76f, config.height * 0.47f,
            0.0f, 1.0f, false, glm::vec4(0.05f, 0.12f, 0.13f, 0.84f));
        drawCenteredText(program, textVao, textVbo, white.id, projection,
            "DUCK HUNT", config.width * 0.5f, config.height * 0.30f, 5.0f,
            glm::vec4(1.0f, 0.83f, 0.26f, 1.0f));
        drawCenteredText(program, textVao, textVbo, white.id, projection,
            "ENTER PRA COMECAR", config.width * 0.5f,
            config.height * 0.46f, 2.0f, glm::vec4(1.0f));
        drawCenteredText(program, textVao, textVbo, white.id, projection,
            "CLICA NO PATO PARA FAZER UM PONTO!", config.width * 0.5f,
            config.height * 0.54f, 1.5f, glm::vec4(0.76f, 0.89f, 0.83f, 1.0f));
        drawCenteredText(program, textVao, textVbo, white.id, projection,
            "R PARA RESTARTAR   ESC PARA SAIR", config.width * 0.5f,
            config.height * 0.61f, 1.5f, glm::vec4(0.76f, 0.89f, 0.83f, 1.0f));
    }
    else if (game.gameOver)
    {
        drawSprite(program, vao, white, projection, config.width * 0.18f,
            config.height * 0.28f, config.width * 0.64f, config.height * 0.36f,
            0.0f, 1.0f, false, glm::vec4(0.05f, 0.12f, 0.13f, 0.88f));
        drawCenteredText(program, textVao, textVbo, white.id, projection,
            game.won ? "SEM PIEDADE: VENCEU" : "HESITOU: PERDEU", config.width * 0.5f,
            config.height * 0.35f, 4.0f,
            glm::vec4(1.0f, 0.83f, 0.26f, 1.0f));
        std::ostringstream finalScore;
        finalScore << "ACERTOS " << game.hits << "/" << config.rounds;
        drawCenteredText(program, textVao, textVbo, white.id, projection,
            finalScore.str(), config.width * 0.5f, config.height * 0.47f, 2.0f,
            glm::vec4(1.0f));
        drawCenteredText(program, textVao, textVbo, white.id, projection,
            "ENTER MENU   R PARA RESTARTAR", config.width * 0.5f,
            config.height * 0.55f, 1.5f, glm::vec4(0.76f, 0.89f, 0.83f, 1.0f));
    }
    glfwSwapBuffers(window);
}

void destroyRenderer(Renderer& renderer)
{
    glDeleteTextures(1, &renderer.white.id);
    glDeleteTextures(1, &renderer.sky.id);
    glDeleteTextures(1, &renderer.hills.id);
    glDeleteTextures(1, &renderer.grass.id);
    glDeleteTextures(1, &renderer.duck.id);
    glDeleteTextures(1, &renderer.dog.id);
    glDeleteTextures(1, &renderer.gun.id);
    glDeleteBuffers(1, &renderer.ebo);
    glDeleteBuffers(1, &renderer.vbo);
    glDeleteVertexArrays(1, &renderer.vao);
    glDeleteBuffers(1, &renderer.textVbo);
    glDeleteVertexArrays(1, &renderer.textVao);
    glDeleteProgram(renderer.program);
}