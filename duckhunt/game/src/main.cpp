#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef GAME_SOURCE_DIR
#define GAME_SOURCE_DIR "."
#endif

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
    int completedRounds = 0;
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

std::string sourcePath(const std::string& relativePath)
{
    return std::string(GAME_SOURCE_DIR) + "/" + relativePath;
}

std::string loadTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "Nao foi possivel abrir: " << path << '\n';
        return {};
    }

    std::stringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

GameConfig loadConfig()
{
    GameConfig config;
    std::ifstream file(sourcePath("config/game.cfg"));
    std::string line;

    while (std::getline(file, line))
    {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos || line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        try
        {
            if (key == "window_width") config.width = std::stoi(value);
            else if (key == "window_height") config.height = std::stoi(value);
            else if (key == "rounds") config.rounds = std::stoi(value);
            else if (key == "shots_per_round") config.shotsPerRound = std::stoi(value);
            else if (key == "duck_speed") config.duckSpeed = std::stof(value);
            else if (key == "assets_dir") config.assets = value;
            else if (key == "sky_image") config.skyImage = value;
            else if (key == "hills_image") config.hillsImage = value;
            else if (key == "grass_image") config.grassImage = value;
            else if (key == "duck_image") config.duckImage = value;
            else if (key == "dog_image") config.dogImage = value;
            else if (key == "gun_image") config.gunImage = value;
            else if (key == "duck_frames") config.duckFrames = std::stoi(value);
        }
        catch (const std::exception&)
        {
            std::cerr << "Valor invalido na configuracao: " << line << '\n';
        }
    }

    config.width = std::max(config.width, 320);
    config.height = std::max(config.height, 240);
    config.rounds = std::max(config.rounds, 1);
    config.shotsPerRound = std::max(config.shotsPerRound, 1);
    config.duckSpeed = std::max(config.duckSpeed, 20.0f);
    config.duckFrames = std::max(config.duckFrames, 1);
    return config;
}

GLuint compileShader(GLenum type, const std::string& source, const std::string& label)
{
    const char* sourcePointer = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Erro no shader " << label << ":\n" << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createShaderProgram()
{
    const std::string vertexSource = loadTextFile(sourcePath("shaders/vertex.glsl"));
    const std::string fragmentSource = loadTextFile(sourcePath("shaders/fragment.glsl"));
    if (vertexSource.empty() || fragmentSource.empty())
    {
        return 0;
    }

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, "vertex");
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, "fragment");
    if (vertexShader == 0 || fragmentShader == 0)
    {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        char log[1024] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Erro ao linkar shaders:\n" << log << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void uploadSprite(SpriteTexture& sprite, const unsigned char* pixels)
{
    glGenTextures(1, &sprite.id);
    glBindTexture(GL_TEXTURE_2D, sprite.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite.width, sprite.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool loadSprite(const std::string& path, SpriteTexture& sprite)
{
    std::ifstream file(path);
    int paletteSize = 0;
    if (!(file >> sprite.width >> sprite.height >> paletteSize) ||
        sprite.width < 1 || sprite.height < 1 || paletteSize < 1)
    {
        std::cerr << "Cabecalho de sprite invalido: " << path << '\n';
        return false;
    }

    std::unordered_map<char, unsigned int> palette;
    for (int index = 0; index < paletteSize; ++index)
    {
        char key = 0;
        std::string hexColor;
        if (!(file >> key >> hexColor))
        {
            std::cerr << "Paleta invalida: " << path << '\n';
            return false;
        }
        try
        {
            palette[key] = static_cast<unsigned int>(std::stoul(hexColor, nullptr, 16));
        }
        catch (const std::exception&)
        {
            std::cerr << "Cor invalida na paleta: " << path << '\n';
            return false;
        }
    }

    std::string line;
    std::getline(file, line);
    std::getline(file, line);
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(sprite.width * sprite.height * 4), 0);

    for (int y = 0; y < sprite.height; ++y)
    {
        if (!std::getline(file, line) || static_cast<int>(line.size()) != sprite.width)
        {
            std::cerr << "Linha de pixels invalida em " << path << " (linha " << y << ")\n";
            return false;
        }
        for (int x = 0; x < sprite.width; ++x)
        {
            const auto color = palette.find(line[x]);
            if (line[x] == '.' || color == palette.end())
            {
                continue;
            }
            const std::size_t offset = static_cast<std::size_t>((y * sprite.width + x) * 4);
            pixels[offset] = static_cast<unsigned char>((color->second >> 16) & 0xff);
            pixels[offset + 1] = static_cast<unsigned char>((color->second >> 8) & 0xff);
            pixels[offset + 2] = static_cast<unsigned char>(color->second & 0xff);
            pixels[offset + 3] = 255;
        }
    }

    sprite.frameDelaysMs.push_back(100);
    uploadSprite(sprite, pixels.data());
    return true;
}

bool loadImage(const std::string& path, SpriteTexture& sprite)
{
    const std::size_t extensionStart = path.find_last_of('.');
    std::string extension = extensionStart == std::string::npos ? "" : path.substr(extensionStart);
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });

    if (extension == ".sprite")
    {
        return loadSprite(path, sprite);
    }

    if (extension == ".png")
    {
        int channels = 0;
        unsigned char* pixels = stbi_load(path.c_str(), &sprite.width,
            &sprite.height, &channels, STBI_rgb_alpha);
        if (pixels == nullptr)
        {
            std::cerr << "Nao foi possivel carregar PNG: " << path << " ("
                << stbi_failure_reason() << ")\n";
            return false;
        }
        sprite.frameDelaysMs.push_back(100);
        uploadSprite(sprite, pixels);
        stbi_image_free(pixels);
        return true;
    }

    if (extension == ".gif")
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            std::cerr << "Nao foi possivel abrir GIF: " << path << '\n';
            return false;
        }
        std::vector<unsigned char> fileData;
        fileData.assign(std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
        if (fileData.empty() || fileData.size() >
            static_cast<std::size_t>(std::numeric_limits<int>::max()))
        {
            std::cerr << "Arquivo GIF vazio ou grande demais: " << path << '\n';
            return false;
        }

        int frameWidth = 0;
        int frameHeight = 0;
        int channels = 0;
        int* delays = nullptr;
        unsigned char* frames = stbi_load_gif_from_memory(fileData.data(),
            static_cast<int>(fileData.size()), &delays, &frameWidth, &frameHeight,
            &sprite.frameCount, &channels, STBI_rgb_alpha);
        if (frames == nullptr || sprite.frameCount < 1)
        {
            std::cerr << "Nao foi possivel carregar GIF: " << path << " ("
                << stbi_failure_reason() << ")\n";
            stbi_image_free(frames);
            stbi_image_free(delays);
            return false;
        }

        sprite.width = frameWidth * sprite.frameCount;
        sprite.height = frameHeight;
        sprite.frameDelaysMs.resize(sprite.frameCount, 100);
        const std::size_t frameBytes = static_cast<std::size_t>(frameWidth) * frameHeight * 4;
        std::vector<unsigned char> atlas(
            static_cast<std::size_t>(sprite.width) * sprite.height * 4);
        for (int frame = 0; frame < sprite.frameCount; ++frame)
        {
            for (int row = 0; row < frameHeight; ++row)
            {
                const std::size_t sourceOffset =
                    static_cast<std::size_t>(frame) * frameBytes +
                    static_cast<std::size_t>(row) * frameWidth * 4;
                const std::size_t targetOffset =
                    (static_cast<std::size_t>(row) * sprite.width +
                        static_cast<std::size_t>(frame) * frameWidth) * 4;
                std::copy_n(frames + sourceOffset,
                    static_cast<std::size_t>(frameWidth) * 4,
                    atlas.begin() + targetOffset);
            }
            if (delays != nullptr)
            {
                sprite.frameDelaysMs[frame] = std::max(delays[frame], 20);
            }
        }
        uploadSprite(sprite, atlas.data());
        stbi_image_free(frames);
        stbi_image_free(delays);
        return true;
    }

    std::cerr << "Formato de imagem nao suportado: " << path
        << " (use .sprite, .png ou .gif)\n";
    return false;
}

void advanceSpriteAnimation(SpriteTexture& sprite, float deltaTime)
{
    if (sprite.frameCount <= 1)
    {
        return;
    }

    sprite.frameTimerMs += deltaTime * 1000.0f;
    while (sprite.frameTimerMs >= sprite.frameDelaysMs[sprite.currentFrame])
    {
        sprite.frameTimerMs -= sprite.frameDelaysMs[sprite.currentFrame];
        sprite.currentFrame = (sprite.currentFrame + 1) % sprite.frameCount;
    }
}

GLuint createWhiteTexture()
{
    const unsigned char whitePixel[] = { 255, 255, 255, 255 };
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
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

void launchDuck(GameState& game, const GameConfig& config, std::mt19937& random)
{
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
    *context.game = GameState{};
    context.game->shotsLeft = context.config->shotsPerRound;
    context.game->started = startImmediately;
    launchDuck(*context.game, *context.config, *context.random);
}

void startNextRound(GameState& game, const GameConfig& config, std::mt19937& random,
    SpriteTexture& dog)
{
    ++game.completedRounds;
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

    constexpr float duckSize = 72.0f;
    if (mouseX >= game->duckX && mouseX <= game->duckX + duckSize &&
        mouseY >= game->duckY && mouseY <= game->duckY + duckSize)
    {
        ++game->hits;
        game->duckAlive = false;
        game->transitionTime = 0.0f;
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

int main()
{
    const GameConfig config = loadConfig();
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
    glfwSetWindowTitle(window, "Duck Hunt");
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::cerr << "Erro ao inicializar GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwSwapInterval(1);

    const GLuint program = createShaderProgram();
    if (program == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    const float quadVertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };
    const unsigned int quadIndices[] = { 0, 1, 2, 2, 3, 0 };
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
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

    GLuint textVao = 0;
    GLuint textVbo = 0;
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

    const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(config.width),
        static_cast<float>(config.height), 0.0f, -1.0f, 1.0f);
    const std::string assetRoot = sourcePath(config.assets);
    SpriteTexture sky;
    SpriteTexture hills;
    SpriteTexture grass;
    SpriteTexture duck;
    SpriteTexture dog;
    SpriteTexture gun;
    const bool spritesLoaded =
        loadImage(assetRoot + "/" + config.skyImage, sky) &&
        loadImage(assetRoot + "/" + config.hillsImage, hills) &&
        loadImage(assetRoot + "/" + config.grassImage, grass) &&
        loadImage(assetRoot + "/" + config.duckImage, duck) &&
        loadImage(assetRoot + "/" + config.dogImage, dog) &&
        loadImage(assetRoot + "/" + config.gunImage, gun);
    if (!spritesLoaded)
    {
        glDeleteTextures(1, &sky.id);
        glDeleteTextures(1, &hills.id);
        glDeleteTextures(1, &grass.id);
        glDeleteTextures(1, &duck.id);
        glDeleteTextures(1, &dog.id);
        glDeleteTextures(1, &gun.id);
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &textVbo);
        glDeleteVertexArrays(1, &textVao);
        glDeleteProgram(program);
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
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

    SpriteTexture white{ createWhiteTexture(), 1, 1 };
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
    float hillsOffset = 0.0f;
    float grassOffset = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(std::min(currentTime - previousTime, 0.05));
        previousTime = currentTime;

        if (game.started && !game.gameOver)
        {
            if (game.duckAlive)
            {
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
        advanceSpriteAnimation(sky, deltaTime);
        advanceSpriteAnimation(hills, deltaTime);
        advanceSpriteAnimation(grass, deltaTime);
        advanceSpriteAnimation(duck, deltaTime);
        advanceSpriteAnimation(gun, deltaTime);
        hillsOffset = std::fmod(hillsOffset + deltaTime * 7.0f, 200.0f);
        grassOffset = std::fmod(grassOffset + deltaTime * 18.0f, 200.0f);

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
            hitLabel << "HITS " << game.hits;
            drawText(program, textVao, textVbo, white.id, projection,
                hitLabel.str(), config.width * 0.42f, 17.0f, 2.0f,
                glm::vec4(1.0f, 0.87f, 0.38f, 1.0f));
            std::ostringstream shotLabel;
            shotLabel << "SHOTS " << game.shotsLeft;
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
                "PRESS ENTER TO START", config.width * 0.5f,
                config.height * 0.46f, 2.0f, glm::vec4(1.0f));
            drawCenteredText(program, textVao, textVbo, white.id, projection,
                "CLICK THE DUCK TO SCORE", config.width * 0.5f,
                config.height * 0.54f, 1.5f, glm::vec4(0.76f, 0.89f, 0.83f, 1.0f));
            drawCenteredText(program, textVao, textVbo, white.id, projection,
                "R RESTART   ESC QUIT", config.width * 0.5f,
                config.height * 0.61f, 1.5f, glm::vec4(0.76f, 0.89f, 0.83f, 1.0f));
        }
        else if (game.gameOver)
        {
            drawSprite(program, vao, white, projection, config.width * 0.18f,
                config.height * 0.28f, config.width * 0.64f, config.height * 0.36f,
                0.0f, 1.0f, false, glm::vec4(0.05f, 0.12f, 0.13f, 0.88f));
            drawCenteredText(program, textVao, textVbo, white.id, projection,
                game.won ? "YOU WIN" : "YOU LOSE", config.width * 0.5f,
                config.height * 0.35f, 4.0f,
                glm::vec4(1.0f, 0.83f, 0.26f, 1.0f));
            std::ostringstream finalScore;
            finalScore << "HITS " << game.hits << "/" << config.rounds;
            drawCenteredText(program, textVao, textVbo, white.id, projection,
                finalScore.str(), config.width * 0.5f, config.height * 0.47f, 2.0f,
                glm::vec4(1.0f));
            drawCenteredText(program, textVao, textVbo, white.id, projection,
                "ENTER MENU   R RESTART", config.width * 0.5f,
                config.height * 0.55f, 1.5f, glm::vec4(0.76f, 0.89f, 0.83f, 1.0f));
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glDeleteTextures(1, &white.id);
    glDeleteTextures(1, &sky.id);
    glDeleteTextures(1, &hills.id);
    glDeleteTextures(1, &grass.id);
    glDeleteTextures(1, &duck.id);
    glDeleteTextures(1, &dog.id);
    glDeleteTextures(1, &gun.id);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &textVbo);
    glDeleteVertexArrays(1, &textVao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}