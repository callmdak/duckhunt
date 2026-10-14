#include "resources.h"

#include <glad/gl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

std::string resourcePath(const std::filesystem::path& resourceRoot,
    const std::string& relativePath)
{
    return (resourceRoot / relativePath).string();
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

bool loadConfig(const std::filesystem::path& resourceRoot, GameConfig& config)
{
    const std::string configPath = resourcePath(resourceRoot, "config/game.cfg");
    std::ifstream file(configPath);
    if (!file)
    {
        std::cerr << "Nao foi possivel abrir a configuracao: " << configPath << '\n';
        return false;
    }

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
    return true;
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

GLuint createShaderProgram(const std::filesystem::path& resourceRoot)
{
    const std::string vertexSource = loadTextFile(
        resourcePath(resourceRoot, "shaders/vertex.glsl"));
    const std::string fragmentSource = loadTextFile(
        resourcePath(resourceRoot, "shaders/fragment.glsl"));
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
