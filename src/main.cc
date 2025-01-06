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
    if (json["elements"].size() != static_cast<size_t>(Cell::Element::kCount)) {
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

std::array<Cell::Type,  static_cast<size_t>(Cell::Element::kCount)> Cell::types;
std::array<Color,       static_cast<size_t>(Cell::Element::kCount)> Cell::colors;
std::array<int,         static_cast<size_t>(Cell::Element::kCount)> Cell::weights;
std::array<int,         static_cast<size_t>(Cell::Element::kCount)> Cell::viscosity;
std::array<std::string, static_cast<size_t>(Cell::Element::kCount)> Cell::names;

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
        } else if (x == width/2) {
          cell[y*width + x] = Cell::Element::kSand;
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

    [[nodiscard]] inline Cell::Element GetCell(const int pos) const {
        return cell[pos];
    }

    [[nodiscard]] inline Cell::Element GetCell(const int x, const int y) const {
        return cell[y*width + x];
    }

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

  void GetPixelArray(std::vector<Color>& pixels) {
    for (int i = 0; i < width*height; i++) {
      pixels[i] = Cell::GetColor(cell[i]);
    }
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
  Application(const int screenWidth = 1280, const int screenHeight = 720)
    : screenWidth(screenWidth), screenHeight(screenHeight), world(400, 300) {
    // setup raylib
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Falling Sand Simulation");
    SetTargetFPS(60);

    Cell::LoadElements(RESOURCES_PATH"elements.json");

    const int worldWidth = world.GetWidth();
    const int worldHeight = world.GetHeight();

    // setup world texture
    pixels.resize(worldWidth * worldHeight);
    world.GetPixelArray(pixels);
    worldImage = {
        .data = pixels.data(),
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

  void DrawWorld() {
    ClearBackground(BLACK);

    world.GetPixelArray(pixels);
    UpdateTexture(worldTexture, pixels.data());

    const auto worldWidth = (float)world.GetWidth();
    const auto worldHeight = (float)world.GetHeight();

    // Calculate scaling factor based on window size and framebuffer size
    const float scaleFactorX = screenWidth / worldWidth;
    const float scaleFactorY = screenHeight / worldHeight;

    // Determine the smaller scale factor to maintain aspect ratio
    const float scaleFactor = fminf(scaleFactorX, scaleFactorY);

    const float scaledWidth = worldWidth * scaleFactor;
    const float scaledHeight = worldHeight * scaleFactor;
    const float offsetX = (screenWidth - scaledWidth) / 2.0f;
    const float offsetY = (screenHeight - scaledHeight) / 2.0f;

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
        DrawWorld();
        break;
      default:
        break;
    }
    DrawFPS(10, 10);
    EndDrawing();
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
        if (state == GameState::kPlaying) {
          if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            Vector2 worldPos = ScreenToWorld(mousePos);
            if (world.GetCell((int)worldPos.x, (int)worldPos.y) == Cell::Element::kAir) {
              world.SetCell((int)worldPos.x, (int)worldPos.y, Cell::Element::kSand);
            }
          } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mousePos = GetMousePosition();
            Vector2 worldPos = ScreenToWorld(mousePos);
            if (world.GetCell((int)worldPos.x, (int)worldPos.y) == Cell::Element::kAir) {
              world.SetCell((int)worldPos.x, (int)worldPos.y, Cell::Element::kWater);
            }
          }

          world.Update();
        }
        lag -= ticksMS;
      }
      Render();
    }
  }

  [[nodiscard]] Vector2 ScreenToWorld(const Vector2 screenPos) const {
    const auto worldWidth  = static_cast<float>(world.GetWidth());
    const auto worldHeight = static_cast<float>(world.GetHeight());

    // Calculate scaling factor based on window size and framebuffer size
    const float scaleFactorX = screenWidth / worldWidth;
    const float scaleFactorY = screenHeight / worldHeight;

    // Determine the smaller scale factor to maintain aspect ratio
    const float scaleFactor = fminf(scaleFactorX, scaleFactorY);

    // Calculate the offset
    const float scaledWidth = worldWidth * scaleFactor;
    const float scaledHeight = worldHeight * scaleFactor;
    const float offsetX = (screenWidth - scaledWidth) / 2.0f;
    const float offsetY = (screenHeight - scaledHeight) / 2.0f;

    // Convert screen coordinates to world coordinates
    float worldX = (screenPos.x - offsetX) / scaleFactor;
    float worldY = (screenHeight - screenPos.y - offsetY) / scaleFactor; // Adjust for flipped texture

    // Clamp the coordinates to the world bounds
    worldX = fmaxf(0, fminf(worldX, worldWidth - 1));
    worldY = fmaxf(0, fminf(worldY, worldWidth - 1));

    return (Vector2){ worldX, worldY };
  }

private:
  GameState state = GameState::kMainMenu;

  int screenWidth = 1280;
  int screenHeight = 720;

  AutomataMatrix world;

  std::vector<Color> pixels;
  Image worldImage;
  Texture2D worldTexture;
};

int main(int argc, char* argv[]) {
  Application app(1920, 1080);
  app.Run();

  return 0;
}