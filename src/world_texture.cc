//
// Created by Tom Smale on 02/01/2025.
//

#include "world_texture.h"

void WorldTexture::Draw() {
  for (int i = 0; i < worldWidth * worldHeight; i++) {
    pixels[i] = particleColors[static_cast<int>(world[i])];
  }

  // Calculate scaling factor based on window size and framebuffer size
  float scaleFactorX = (float)GetScreenWidth() / worldWidth;
  float scaleFactorY = (float)GetScreenHeight() / worldHeight;

  // Determine the smaller scale factor to maintain aspect ratio
  float scaleFactor = fminf(scaleFactorX, scaleFactorY);

  float scaledWidth = worldWidth * scaleFactor;
  float scaledHeight = worldHeight * scaleFactor;
  float offsetX = (GetScreenWidth() - scaledWidth) / 2.0f;
  float offsetY = (GetScreenHeight() - scaledHeight) / 2.0f;

  DrawTexturePro(
      worldTexture,
      (Rectangle){ 0, 0, (float)worldTexture.width, -(float)worldTexture.height }, // Source rectangle (inverted Y)
      (Rectangle){ offsetX, offsetY, scaledWidth, scaledHeight },                  // Destination rectangle
      (Vector2){ 0, 0 },                                                           // Origin
      0.0f,                                                                        // Rotation
      WHITE                                                                        // Tint
  );
}
