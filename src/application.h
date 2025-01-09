//
// Created by Tom Smale on 02/01/2025.
//

#ifndef RAYLIB_SAND_SIM_SRC_APPLICATION_H_
#define RAYLIB_SAND_SIM_SRC_APPLICATION_H_

#include "automata_matrix.h"
#include <raylib.h>

class Application {
 public:
  enum class State {
    kMainMenu,
    kOptionsMenu,
    kPlaying,
    kClosing,
  };

  enum class PlayState {
    kRunning,
    kPaused,
    kStopped,
  };

  Application(int screenWidth = 1280, int screenHeight = 720);
  ~Application();

  void DrawMainMenu();
  void DrawWorld();
  void Render();
  void Run();
  [[nodiscard]] Vector2 ScreenToWorld(const Vector2 screenPos) const;

 private:
  State state = State::kMainMenu;
  PlayState playState = PlayState::kStopped;

  Shader shader;

  int screenWidth = 1280;
  int screenHeight = 720;

  AutomataMatrix world;

  std::vector<Color> pixels;
  Image worldImage;
  Texture2D worldTexture;

  Image colorTableImage;
  Texture colorTableTexture;
};

#endif //RAYLIB_SAND_SIM_SRC_APPLICATION_H_
