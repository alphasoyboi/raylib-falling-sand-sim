//
// Created by Tom Smale on 02/01/2025.
//

#ifndef RAYLIB_SAND_SIM_SRC_WORLD_TEXTURE_H_
#define RAYLIB_SAND_SIM_SRC_WORLD_TEXTURE_H_

#include <raylib.h>
#include <memory.h>

class WorldTexture {
 public:
  WorldTexture(int textureWidth, int textureHeight) : width(textureWidth), height(textureHeight) {
    pixels = std::make_unique<Color[]>(width * height);
    image = {
        .data = nullptr,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    texture = LoadTextureFromImage(image);
  }

  void Draw() {
    for (int i = 0; i < width * height; i++) {
      pixels[i] = particleColors[static_cast<int>(world[i])];
    }

    // Calculate scaling factor based on window size and framebuffer size
    float scaleFactorX = (float)GetScreenWidth() / width;
    float scaleFactorY = (float)GetScreenHeight() / height;

    // Determine the smaller scale factor to maintain aspect ratio
    float scaleFactor = fminf(scaleFactorX, scaleFactorY);

    float scaledWidth = width * scaleFactor;
    float scaledHeight = height * scaleFactor;
    float offsetX = (GetScreenWidth() - scaledWidth) / 2.0f;
    float offsetY = (GetScreenHeight() - scaledHeight) / 2.0f;

    DrawTexturePro(
        texture,
        (Rectangle){ 0, 0, (float)worldTexture.width, -(float)worldTexture.height }, // Source rectangle (inverted Y)
        (Rectangle){ offsetX, offsetY, scaledWidth, scaledHeight },                  // Destination rectangle
        (Vector2){ 0, 0 },                                                           // Origin
        0.0f,                                                                        // Rotation
        WHITE                                                                        // Tint
    );
  }

 private:
  int width;
  int height;
  std::unique_ptr<Color[]> pixels;
  Image image;
  Texture2D texture;
};

#endif //RAYLIB_SAND_SIM_SRC_WORLD_TEXTURE_H_
