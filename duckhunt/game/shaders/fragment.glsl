#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D image;
uniform vec4 tint;

void main()
{
    FragColor = texture(image, TexCoord) * tint;
}