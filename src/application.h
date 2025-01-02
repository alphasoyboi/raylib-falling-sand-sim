//
// Created by Tom Smale on 02/01/2025.
//

#ifndef RAYLIB_SAND_SIM_SRC_APPLICATION_H_
#define RAYLIB_SAND_SIM_SRC_APPLICATION_H_

class Application {
 public:


 private:
  int screenWidth = 1280;
  int screenHeight = 720;

  int worldWidth = 400;
  int worldHeight = 300;
  std::unique_ptr<Particle[]> world;
  std::unique_ptr<Color[]> pixels;
  Image worldImage = {
      .data = nullptr,
      .width = worldWidth,
      .height = worldHeight,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
  };
  Texture2D worldTexture;

  GameState state = GameState::MAIN_MENU;

};

#endif //RAYLIB_SAND_SIM_SRC_APPLICATION_H_
