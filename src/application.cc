//
// Created by Tom Smale on 02/01/2025.
//

#include "application.h"

#include <raylib.h>
#include <raygui.h>
#include <cmath>

Application::Application(const int screenWidth, const int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight), world(400, 300) {
  // setup raylib
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(screenWidth, screenHeight, "Falling Sand Simulation");
  SetTargetFPS(60);

  Cell::LoadElements(RESOURCES_PATH"elements.json");

  shader = LoadShader(nullptr, (RESOURCES_PATH"shaders/custom_shader.fs"));

  // setup world texture
  const int worldWidth = world.width;
  const int worldHeight = world.height;
  const std::vector<uint8_t> matrix = world.GetMatrix();
  worldImage = (Image){
      .data = (void*)matrix.data(),
      .width = worldWidth,
      .height = worldHeight,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
  };
  worldTexture = LoadTextureFromImage(worldImage);
  SetShaderValueTexture(shader, GetShaderLocation(shader, "texture0"), worldTexture);
  //UnloadImage(worldImage);

  // setup color table
  const std::array<Color, static_cast<size_t>(Cell::Element::kCount)>& colorTable = Cell::GetColorTable();
  colorTableImage = (Image){
    .data = (void*)colorTable.data(),
    .width = static_cast<int>(colorTable.size()),
    .height = 1,
    .mipmaps = 1,
    .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
  };
  colorTableTexture = LoadTextureFromImage(colorTableImage);
  SetShaderValueTexture(shader, GetShaderLocation(shader, "texture1"), colorTableTexture);
}

Application::~Application() {
  UnloadTexture(worldTexture);
  UnloadTexture(colorTableTexture);
  CloseWindow();
}

void Application::DrawMainMenu() {
  DrawRectangleGradientV(0, 0, screenWidth, screenHeight, BLACK, BEIGE);
  DrawRectangle(screenWidth*0.25, screenHeight*0.125, screenWidth*0.5, screenHeight*0.125, GRAY);
  DrawRectangleLinesEx((Rectangle){screenWidth*0.25f, screenHeight*0.125f, screenWidth*0.5f, screenHeight*0.125f}, 5, DARKGRAY);
  DrawText("Falling Sand Simulation", screenWidth/2 - MeasureText("Falling Sand Simulation", 40)/2, screenHeight*0.125 + 24, 40, RAYWHITE);
  GuiSetStyle(DEFAULT, TEXT_SIZE, 30);
  if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.35f, screenWidth*0.33f, screenHeight*0.1f}, "Start")) {
    state = State::kPlaying;
    playState = PlayState::kRunning;
  }
  if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.5f, screenWidth*0.33f, screenHeight*0.1f}, "Options")) {
    state = State::kOptionsMenu;
  }
  if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.65f, screenWidth*0.33f, screenHeight*0.1f}, "Quit")) {
    state = State::kClosing;
  }
}

void Application::DrawWorld() {

  const std::vector<uint8_t>& image = world.GetMatrix();
  UpdateTexture(worldTexture, image.data());

  const auto worldWidth = (float)world.width;
  const auto worldHeight = (float)world.height;

  // Calculate scaling factor based on window size and framebuffer size
  const float scaleFactorX = screenWidth / worldWidth;
  const float scaleFactorY = screenHeight / worldHeight;

  // Determine the smaller scale factor to maintain aspect ratio
  const float scaleFactor = fminf(scaleFactorX, scaleFactorY);

  const float scaledWidth = worldWidth * scaleFactor;
  const float scaledHeight = worldHeight * scaleFactor;
  const float offsetX = (screenWidth - scaledWidth) / 2.0f;
  const float offsetY = (screenHeight - scaledHeight) / 2.0f;

  ClearBackground(BLACK);
  BeginShaderMode(shader);
  SetShaderValueTexture(shader, GetShaderLocation(shader, "texture0"), worldTexture);
  SetShaderValueTexture(shader, GetShaderLocation(shader, "texture1"), colorTableTexture);
  DrawTexturePro(
      worldTexture,
      (Rectangle){ 0, 0, (float)worldTexture.width, -(float)worldTexture.height }, // Source rectangle (inverted Y)
      (Rectangle){ offsetX, offsetY, scaledWidth, scaledHeight },                  // Destination rectangle
      (Vector2){ 0, 0 },                                                           // Origin
      0.0f,                                                                        // Rotation
      WHITE                                                                        // Tint
  );
  EndShaderMode();
}

void Application::Render() {
  BeginDrawing();
  ClearBackground(PURPLE);
  switch (state) {
    case State::kMainMenu:
      DrawMainMenu();
      break;
    case State::kOptionsMenu:
      break;
    case State::kPlaying:
      DrawWorld();
      break;
    default:
      break;
  }
  DrawFPS(10, 10);
  EndDrawing();
}

void Application::Run() {
  double previous = GetTime();
  double lag = 0.0;
  const double ticksMS = 1.0/60.0;
  while (!WindowShouldClose()) {
    double current = GetTime();
    double elapsed = current - previous;
    previous = current;
    lag += elapsed;
    if (lag >= ticksMS) {
      if (playState == PlayState::kRunning) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          Vector2 mousePos = GetMousePosition();
          Vector2 worldPos = ScreenToWorld(mousePos);
          if (world.GetCell((int)worldPos.x, (int)worldPos.y) == Cell::Element::kAir) {
            //world.SetCell((int)worldPos.x, (int)worldPos.y, Cell::Element::kSand);
            world.SetCircle((int)worldPos.x, (int)worldPos.y, 5, Cell::Element::kSand);
          }
        } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
          Vector2 mousePos = GetMousePosition();
          Vector2 worldPos = ScreenToWorld(mousePos);
          if (world.GetCell((int)worldPos.x, (int)worldPos.y) == Cell::Element::kAir) {
            //world.SetCell((int)worldPos.x, (int)worldPos.y, Cell::Element::kWater);
            world.SetCircle((int)worldPos.x, (int)worldPos.y, 5, Cell::Element::kWater);
          }
        }

        world.Update();
      }
      lag -= ticksMS;
    }
    Render();
  }
}

[[nodiscard]] Vector2 Application::ScreenToWorld(const Vector2 screenPos) const {
  const auto worldWidth  = static_cast<float>(world.width);
  const auto worldHeight = static_cast<float>(world.height);

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
