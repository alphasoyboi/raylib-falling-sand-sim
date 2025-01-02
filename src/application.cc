//
// Created by Tom Smale on 02/01/2025.
//

#include "application.h"

class Application {
 public:
  Application::Application() {
    InitGraphics();
    AllocateWorld();
  }

  Application::~Application() {
    CloseGraphics();
  }

  void Application::InitGraphics() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Falling Sand Simulation");
    SetTargetFPS(60);
  }

  void Application::CloseGraphics() {
    CloseWindow();
  }

  void Application::AllocateWorld() {
    world = std::make_unique<Particle[]>(worldWidth * worldHeight);
    pixels = std::make_unique<Color[]>(worldWidth * worldHeight);

    for (int y = 0; y < worldHeight; y++) {
      for (int x = 0; x < worldWidth; x++) {
        if (y < worldHeight/2) {
          world[y * worldWidth + x] = Particle::AIR;
        }
        else {
          world[y * worldWidth + x] = Particle::SAND;
        }
        pixels[y * worldWidth + x] = particleColors[static_cast<int>(world[y * worldWidth + x])];
      }
    }
  }

  void Application::DrawMainMenu() {
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, BLACK, BEIGE);
    DrawRectangle(screenWidth*0.25, screenHeight*0.125, screenWidth*0.5, screenHeight*0.125, GRAY);
    DrawRectangleLinesEx((Rectangle){screenWidth*0.25f, screenHeight*0.125f, screenWidth*0.5f, screenHeight*0.125f}, 5, DARKGRAY);
    DrawText("Falling Sand Simulation", screenWidth/2 - MeasureText("Falling Sand Simulation", 40)/2, screenHeight*0.125 + 24, 40, RAYWHITE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 30);
    if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.35f, screenWidth*0.33f, screenHeight*0.1f}, "Start")) {
      state = GameState::PLAYING;
      InitWorldTexture();
    }
    if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.5f, screenWidth*0.33f, screenHeight*0.1f}, "Options")) {
      state = GameState::OPTIONS_MENU;
    }
  }

  void Application::InitWorldTexture() {
    worldImage.data = pixels.get();             // We can assign pixels directly to data
    worldTexture = LoadTextureFromImage(worldImage);
  }

  void Application::UpdateWorldTexture() {
    for (int y = 0; y < worldHeight; y++) {
      for (int x = 0; x < worldWidth; x++) {
        pixels[y*worldWidth + x] = particleColors[static_cast<int>(world[y*worldWidth + x])];
      }
    }
    UpdateTexture(worldTexture, pixels.get());
  }

  void Application::DrawWorld() {
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

  void Application::Render() {
    BeginDrawing();
    ClearBackground(PURPLE);
    switch (state) {
      case GameState::MAIN_MENU:
        DrawMainMenu();
        break;
      case GameState::OPTIONS_MENU:
        break;
      case GameState::PLAYING:
        UpdateWorldTexture();
        DrawWorld();
        break;
    }
    DrawFPS(10, 10);
    EndDrawing();
  }

  void Application::Run() {
    while (!WindowShouldClose()) {
      Render();
    }
  }

  static size_t Application::GetIndex(int x, int y, int width) {
    return y * width + x;
  }
};
