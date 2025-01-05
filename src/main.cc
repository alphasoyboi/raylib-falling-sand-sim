//
// Created by tomsmale on 28/12/24.
//

#include <raylib.h>
#include "raygui.h"
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <fstream>

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

Color particleColors[] = {
    BLACK, // AIR
    BEIGE, // SAND
    GRAY,  // STONE
    BLUE,  // WATER
    DARKGRAY, // BEDROCK
};

class Cell {
public:
  enum class Element : uint8_t {
    kAir,
    kSand,
    kStone,
    kWater,
    //kLava,
    //kDirt,
    kBedrock,
    kCount,
  };

  enum class Type : uint8_t {
    kEmpty,
    kPowder,
    kSolid,
    kLiquid,
    kFire,
    kGas,
  };

  static void LoadElements(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + filename);
    }

    nlohmann::json json;
    file >> json;

    if (!json.contains("elements") || !json["elements"].is_array()) {
      throw std::runtime_error("Invalid JSON config format: " + filename);
    }
    if (json["elements"].size() != static_cast<size_t>(Cell::Element::kCount)-1) {
      throw std::runtime_error("Invalid number of elements in JSON config: " + filename);
    }

    for (const auto& element : json["elements"]) {
      auto i = static_cast<size_t>(element["index"]);
      types[i] = static_cast<Cell::Type>(element["type"]);
      weights[i] = element["weight"];
      viscosity[i] = element["viscosity"];
      names[i] = element["name"];
      colors[i] = particleColors[i];
    }
  }

  static Type GetType(Element element) {
    return types[static_cast<size_t>(element)];
  }

  static Color GetColor(Element element) {
    return colors[static_cast<size_t>(element)];
  }

  static int GetWeight(Element element) {
    return weights[static_cast<size_t>(element)];
  }

  static int GetViscosity(Element element) {
    return viscosity[static_cast<size_t>(element)];
  }

  static std::string GetName(Element element) {
    return names[static_cast<size_t>(element)];
  }

private:
  static std::array<Type,        static_cast<size_t>(Element::kCount)> types;
  static std::array<Color,       static_cast<size_t>(Element::kCount)> colors;
  static std::array<int,         static_cast<size_t>(Element::kCount)> weights;
  static std::array<int,         static_cast<size_t>(Element::kCount)> viscosity;
  static std::array<std::string, static_cast<size_t>(Element::kCount)> names;
};

class AutomataMatrix {
public:
  AutomataMatrix(int width = 400, int height = 300) : width(width), height(height) {
    cell.resize(width * height);
    heat.resize(width * height);
    shade.resize(width * height);
    dirty.resize(width * height);

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
          cell[y*width + x] = Cell::Element::kBedrock;
        } else {
          cell[y*width + x] = Cell::Element::kAir;
        }

        heat[y*width + x] = 0;
        shade[y*width + x] = 0;
        dirty[y*width + x] = 1;
      }
    }
  }

  [[nodiscard]] inline int GetWidth()  const { return width; }
  [[nodiscard]] inline int GetHeight() const { return height; }

  [[nodiscard]] inline int Above     (const int pos) const { return pos + width; }
  [[nodiscard]] inline int Below     (const int pos) const { return pos - width; }
  [[nodiscard]] inline int Right     (const int pos) const { return pos + 1; }
  [[nodiscard]] inline int Left      (const int pos) const { return pos - 1; }
  [[nodiscard]] inline int AboveRight(const int pos) const { return pos + width + 1; }
  [[nodiscard]] inline int AboveLeft (const int pos) const { return pos + width - 1; }
  [[nodiscard]] inline int BelowRight(const int pos) const { return pos - width + 1; }
  [[nodiscard]] inline int BelowLeft (const int pos) const { return pos - width - 1; }

  void SetCell(const int pos, const Cell::Element element) {
    cell[pos] = element;
  }

  void SetCell(const int x, const int y, const Cell::Element element) {
    cell[y*width + x] = element;
  }

  void SwapCells(const int pos1, const int pos2) {
    std::swap(cell[pos1], cell[pos2]);
  }

  void SwapCells(const int x1, const int y1, const int x2, const int y2) {
    std::swap(cell[y1*width + x1], cell[y2*width + x2]);
  }

  void ApplyGravity(int pos, Cell::Element element, const int direction, const bool liquid) {
    int weight = Cell::GetWeight(element);
    while (weight-- != 0) {
      const int below = Below(pos);
      const int directionA = direction ? BelowRight(pos) : BelowLeft(pos);
      const int directionB = direction ? BelowLeft(pos) : BelowRight(pos);
      if (Cell::GetType(cell[below]) == Cell::Type::kEmpty) {
        SwapCells(pos, below);
        pos = below;
      } else if (Cell::GetType(cell[directionA]) == Cell::Type::kEmpty) {
        SwapCells(pos, directionA);
        pos = directionA;
      } else if (Cell::GetType(cell[directionB]) == Cell::Type::kEmpty) {
        SwapCells(pos, directionB);
        pos = directionB;
      } else {
        if (liquid) {
          ApplySpread(pos, weight, direction);
        }
        break;
      }
    }
    dirty[pos] = 0;
  }

  void ApplySpread(int& pos, int spread, const int direction) {
    while (spread-- != 0) {
      const int directionA = direction ? Right(pos) : Left(pos);
      const int directionB = direction ? Left(pos) : Right(pos);
      if (Cell::GetType(cell[directionA]) == Cell::Type::kEmpty) {
        SwapCells(pos, directionA);
        pos = directionA;
      } else if (Cell::GetType(cell[directionB]) == Cell::Type::kEmpty) {
        SwapCells(pos, directionB);
        pos = directionB;
      }
    }
  }

  void Update() {
    const int direction = GetRandomValue(0, 1);
    for (int i = 0; i < width*height; i++) {
      switch(cell[i]) {
        case Cell::Element::kSand: {
          ApplyGravity(i, Cell::Element::kSand, direction, false);
          break;
        }
        case Cell::Element::kWater: {
          ApplyGravity(i, Cell::Element::kWater, direction, true);
          break;
        }
        default:
          break;
      }
    }
    std::ranges::fill(dirty, 1);
  }

private:
  int width;
  int height;

  std::vector<Cell::Element> cell;
  std::vector<uint8_t>       heat;
  std::vector<uint8_t>       shade;
  std::vector<uint8_t>       dirty;
};


enum class Particle {
  kAir,
  kSand,
  kStone,
  kWater,
  kBedrock,
  kCount,
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

    // allocate and initialize dirty map
    dirtyMap = std::make_unique<uint8_t[]>(worldWidth * worldHeight);
    std::fill_n(dirtyMap.get(), worldWidth * worldHeight, 1);

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

  inline void SetParticle(int pos, Particle type) {
    world[pos] = type;
  }

  inline void SwapParticle(int newPos, Particle newType, int oldPos, Particle oldType) {
    world[newPos] = newType;
    world[oldPos] = oldType;
  }

  void ApplyGravity(int pos, Particle type, int weight, int direction) {
    int tempPos = pos;
    while (weight-- != 0) {
      int below = Below(pos);
      if (world[below] == Particle::kAir) {
        SwapParticle(pos, Particle::kAir, below, type);
        tempPos = below;
        continue;
      } else {
        int belowRight = BelowRight(pos);
        int belowLeft = BelowLeft(pos);
        if (direction == 0) {
          if (world[belowRight] == Particle::kAir) {
            SwapParticle(pos, Particle::kAir, belowRight, type);
            pos = belowRight;
          } else if (world[belowLeft] == Particle::kAir) {
            SwapParticle(pos, Particle::kAir, belowLeft, type);
            pos = belowLeft;
          }
        } else {
          if (world[belowLeft] == Particle::kAir) {
            SwapParticle(pos, Particle::kAir, belowLeft, type);
            pos = belowLeft;
          } else if (world[belowRight] == Particle::kAir) {
            SwapParticle(pos, Particle::kAir, belowRight, type);
            pos = belowRight;
          }
        }
      }
    }
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
        SetParticle(GetIndex(worldPos.x, worldPos.y), Particle::kSand);
      }
    } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 mousePos = GetMousePosition();
      Vector2 worldPos = ScreenToWorld(mousePos);
      if (world[GetIndex(worldPos.x, worldPos.y)] != Particle::kBedrock) {
        SetParticle(GetIndex(worldPos.x, worldPos.y), Particle::kWater);
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
  std::unique_ptr<uint8_t[]> dirtyMap;
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