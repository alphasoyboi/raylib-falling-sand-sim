//
// Created by tomsmale on 28/12/24.
//

#include <raylib.h>
#include "raygui.h"
#include <memory>
#include <iostream>

enum class GameState {
  kMainMenu,
  kOptionsMenu,
  kPlaying,
  kClosing

};

enum class PlayState {
  kRunning,
  kPaused,
};

/*class Particle {
 public:
  enum class Type {
    kVoid,
    kAir,
    kSand,
    kDirt,
    kStone,
    kWater,
    kLava,
    kCount,
  };
 private:
};*/

enum class Particle {
  kVoid,
  kAir,
  kSand,
  kDirt,
  kStone,
  kWater,
  kLava,
  kBedrock,
  kCount,
};

Color particleColors[] = {
    BLANK, // VOID
    BLACK, // AIR
    BEIGE, // SAND
    BROWN, // DIRT
    GRAY,  // STONE
    BLUE,  // WATER
    RED,   // LAVA
    DARKGRAY, // BEDROCK
};

class World {

};

class Application {
public:
  Application() {
    // setup raylib
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Falling Sand Simulation");
    SetTargetFPS(60);

    // allocate particle array to represent world and pixel array for rendering
    world = std::make_unique<Particle[]>(worldWidth * worldHeight);
    for (int y = 0; y < worldHeight; y++) {
      for (int x = 0; x < worldWidth; x++) {
        if (x == 0 || x == worldWidth - 1 || y == 0 || y == worldHeight - 1) {
          world[y * worldWidth + x] = Particle::kBedrock;
        } else if (y < worldHeight / 3 && x < worldWidth / 2) {
          world[y * worldWidth + x] = Particle::kSand;
        } else if (y%2 == 0 && x == worldWidth/2) {//x > worldWidth/2-3 && x < worldWidth/2+3) {
          world[y*worldWidth + x] = Particle::kWater;
        }
        else {
          world[y*worldWidth + x] = Particle::kAir;
        }
      }
    }

    std::cout << static_cast<int>(world[worldWidth-1]) << std::endl;

    // setup world texture
    pixels = std::make_unique<Color[]>(worldWidth * worldHeight);
    for (int y = 0; y < worldHeight; y++) {
      for (int x = 0; x < worldWidth; x++) {
        pixels[y*worldWidth + x] = particleColors[static_cast<int>(world[y*worldWidth + x])];
      }
    }
    worldImage = {
        .data = pixels.get(),
        .width = worldWidth,
        .height = worldHeight,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    worldTexture = LoadTextureFromImage(worldImage);
    //UnloadImage(worldImage);
  }

  ~Application() {
    UnloadTexture(worldTexture);
    CloseWindow();
  }

  void DrawMainMenu() {
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, BLACK, BEIGE);
    DrawRectangle(screenWidth*0.25, screenHeight*0.125, screenWidth*0.5, screenHeight*0.125, GRAY);
    DrawRectangleLinesEx((Rectangle){screenWidth*0.25f, screenHeight*0.125f, screenWidth*0.5f, screenHeight*0.125f}, 5, DARKGRAY);
    DrawText("Falling Sand Simulation", screenWidth/2 - MeasureText("Falling Sand Simulation", 40)/2, screenHeight*0.125 + 24, 40, RAYWHITE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 30);
    if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.35f, screenWidth*0.33f, screenHeight*0.1f}, "Start")) {
      state = GameState::kPlaying;
    }
    if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.5f, screenWidth*0.33f, screenHeight*0.1f}, "Options")) {
      state = GameState::kOptionsMenu;
    }
    if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.65f, screenWidth*0.33f, screenHeight*0.1f}, "Quit")) {
      state = GameState::kClosing;
    }
  }

  void UpdateWorldTexture() {
    for (int y = 0; y < worldHeight; y++) {
      for (int x = 0; x < worldWidth; x++) {
        pixels[y*worldWidth + x] = particleColors[static_cast<int>(world[y*worldWidth + x])];
      }
    }
    UpdateTexture(worldTexture, pixels.get());
  }

  void DrawWorld() {
    ClearBackground(BLACK);

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

  void Render() {
    BeginDrawing();
    ClearBackground(PURPLE);
    switch (state) {
      case GameState::kMainMenu:
        DrawMainMenu();
        break;
      case GameState::kOptionsMenu:
        break;
      case GameState::kPlaying:
        UpdateWorldTexture();
        DrawWorld();
        break;
    }
    DrawFPS(10, 10);
    EndDrawing();
  }

  [[nodiscard]] inline int Above     (int pos) const { return pos + worldWidth; }
  [[nodiscard]] inline int Below     (int pos) const { return pos - worldWidth; }
  [[nodiscard]] inline int Right     (int pos) const { return pos + 1; }
  [[nodiscard]] inline int Left      (int pos) const { return pos - 1; }
  [[nodiscard]] inline int AboveRight(int pos) const { return pos + worldWidth + 1; }
  [[nodiscard]] inline int AboveLeft (int pos) const { return pos + worldWidth - 1; }
  [[nodiscard]] inline int BelowRight(int pos) const { return pos - worldWidth + 1; }
  [[nodiscard]] inline int BelowLeft (int pos) const { return pos - worldWidth - 1; }

  inline void PlaceParticle(int pos, Particle type) {
    world[pos] = type;
  }

  inline void SwapParticle(int newPos, Particle newType, int oldPos, Particle oldType) {
    world[newPos] = newType;
    world[oldPos] = oldType;
  }

  void CalculateGravity(int& pos, Particle type, int& fallFactor, int fallDirection, bool calculateSpread = false) {
    while (fallFactor-- != 0) {
      int below = Below(pos); // Store the result in a temporary variable
      if (world[below] == Particle::kAir) {
        SwapParticle(pos, Particle::kAir, below, type);
        pos = below;
        continue;
      }
      int belowRight = BelowRight(pos);
      int belowLeft = BelowLeft(pos);
      if (fallDirection == 0) {
        if (world[belowRight] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, belowRight, type);
          pos = belowRight;
        } else if (world[belowLeft] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, belowLeft, type);
          pos = belowLeft;
        } else {
          if (calculateSpread) {
            CalculateSpread(pos, type, 1, fallDirection);
          }
        }
      } else {
        if (world[belowLeft] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, belowLeft, type);
          pos = belowLeft;
        } else if (world[belowRight] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, belowRight, type);
          pos = belowRight;
        } else {
          if (calculateSpread) {
            CalculateSpread(pos, type, 1, fallDirection);
          }
        }
      }
    }
  }

  void CalculateSpread(int& pos, Particle type, int spreadFactor, int spreadDirection) {
    while (spreadFactor-- != 0) {
      int right = Right(pos);
      int left = Left(pos);
      if (spreadDirection == 0) {
        if (world[right] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, right, type);
          pos = right;
        } else if (world[left] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, left, type);
          pos = left;
        }
      } else {
        if (world[left] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, left, type);
          pos = left;
        } else if (world[right] == Particle::kAir) {
          SwapParticle(pos, Particle::kAir, right, type);
          pos = right;
        }
      }
    }
  }

  void UpdatePowder(int pos, Particle type, int fallFactor, int fallDirection) {
    CalculateGravity(pos, type, fallFactor, fallDirection);
  }

  void UpdateLiquid(int pos, Particle type, int fallFactor, int fallDirection) {
    CalculateGravity(pos, type, fallFactor, fallDirection, true);
  }

  void UpdateWorld(double elapsedTicks) {
    if (state != GameState::kPlaying) {
      return;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      Vector2 mousePos = GetMousePosition();
      Vector2 worldPos = ScreenToWorld(mousePos);
      if (world[GetIndex(worldPos.x, worldPos.y)] != Particle::kBedrock) {
        PlaceParticle(GetIndex(worldPos.x, worldPos.y), Particle::kSand);
      }
    } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 mousePos = GetMousePosition();
      Vector2 worldPos = ScreenToWorld(mousePos);
      if (world[GetIndex(worldPos.x, worldPos.y)] != Particle::kBedrock) {
        PlaceParticle(GetIndex(worldPos.x, worldPos.y), Particle::kWater);
      }
    }

    int fallDirection = GetRandomValue(0, 1);
    for (int i = worldWidth; i < worldWidth*worldHeight-worldWidth; i++) {
      switch(world[i]) {
        case Particle::kSand: {
          UpdatePowder(i, Particle::kSand, 4, fallDirection);
          break;
        }
        case Particle::kWater: {
          UpdateLiquid(i, Particle::kWater, 8, fallDirection);
          break;
        }
        default:
          break;
      }
    }
  }

  void Run() {
    double previous = GetTime();
    double lag = 0.0;
    const double ticksMS = 1.0/60.0;
    while (!WindowShouldClose()) {
      double current = GetTime();
      double elapsed = current - previous;
      previous = current;
      lag += elapsed;
      if (lag >= ticksMS) {
        UpdateWorld(elapsed);
        lag -= ticksMS;
      }
      Render();
    }
  }

  void BoundsCheck(int& x, int& y) const {
    x = BoundsCheckX(x);
    y = BoundsCheckY(y);
  }

  [[nodiscard]] int BoundsCheckX(int x) const {
    return x < 0 ? 0 : x >= worldWidth ? worldWidth - 1 : x;
  }

  [[nodiscard]] int BoundsCheckY(int y) const {
    return y < 0 ? 0 : y >= worldHeight ? worldHeight - 1 : y;
  }

  void GetXY(int index, int& x, int& y) const {
    x = index % worldWidth;
    y = index / worldWidth;
  }

  [[nodiscard]] int GetIndex(int x, int y) const {
    return y*worldWidth + x;
  }

  Vector2 ScreenToWorld(Vector2 screenPos) {
    // Calculate scaling factor based on window size and framebuffer size
    float scaleFactorX = (float)screenWidth / worldWidth;
    float scaleFactorY = (float)screenHeight / worldHeight;

    // Determine the smaller scale factor to maintain aspect ratio
    float scaleFactor = fminf(scaleFactorX, scaleFactorY);

    // Calculate the offset
    float scaledWidth = worldWidth * scaleFactor;
    float scaledHeight = worldHeight * scaleFactor;
    float offsetX = (screenWidth - scaledWidth) / 2.0f;
    float offsetY = (screenHeight - scaledHeight) / 2.0f;

    // Convert screen coordinates to world coordinates
    float worldX = (screenPos.x - offsetX) / scaleFactor;
    float worldY = (screenHeight - screenPos.y - offsetY) / scaleFactor; // Adjust for flipped texture

    // Clamp the coordinates to the world bounds
    worldX = fmaxf(0, fminf(worldX, worldWidth - 1));
    worldY = fmaxf(0, fminf(worldY, worldHeight - 1));

    return (Vector2){ worldX, worldY };
  }

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

  GameState state = GameState::kMainMenu;
};

int main(int argc, char* argv[]) {
  Application app;
  app.Run();

  return 0;
}