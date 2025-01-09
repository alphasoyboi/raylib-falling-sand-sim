#version 330

in vec2 fragTexCoord;         // Texture coordinates
out vec4 finalColor;          // Final fragment color

uniform sampler2D texture0;   // Grayscale index map
uniform sampler1D texture1;   // Color lookup table

void main() {
    // Sample the grayscale value from texture0
    float index = texture(texture0, fragTexCoord).r;

    // Use the grayscale value as an index to sample the color from texture1
    vec4 color = texture(texture1, index);

    // Output the color
    finalColor = color;
}