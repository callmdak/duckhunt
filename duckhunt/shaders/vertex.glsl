#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 model;
uniform vec4 uvRect;

void main()
{
    // transforma a posição do quadrado e escolhe a parte da imagem
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = vec2(
        mix(uvRect.x, uvRect.z, aTexCoord.x),
        mix(uvRect.y, uvRect.w, aTexCoord.y)
    );
}