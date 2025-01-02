//
// Created by tomsmale on 28/12/24.
//

#include <raylib.h>
#include "raygui.h"
#include <memory>

enum class GameState {
  MAIN_MENU,
  OPTIONS_MENU,
  PLAYING,
};

enum class PlayState {
  RUNNING,
  PAUSED,
};

enum class Particle {
  VOID,
  AIR,
  SAND,
  DIRT,
  STONE,
  WATER,
  LAVA,
  COUNT,
};

Color particleColors[] = {
    BLANK, // VOID
    BLACK, // AIR
    BEIGE, // SAND
    BROWN, // DIRT
    GRAY,  // STONE
    BLUE,  // WATER
    RED,   // LAVA
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
    pixels = std::make_unique<Color[]>(worldWidth * worldHeight);

    for (int y = 0; y < worldHeight; y++) {
      for (int x = 0; x < worldWidth; x++) {
        if (y > worldHeight/2) {
          world[y * worldWidth + x] = Particle::AIR;
        }
        else {
          world[y * worldWidth + x] = Particle::SAND;
        }
        pixels[y * worldWidth + x] = particleColors[static_cast<int>(world[y * worldWidth + x])];
      }
    }

    // setup world texture
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
      state = GameState::PLAYING;
    }
    if (GuiButton((Rectangle){screenWidth*0.335f, screenHeight*0.5f, screenWidth*0.33f, screenHeight*0.1f}, "Options")) {
      state = GameState::OPTIONS_MENU;
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

  void Run() {
    while (!WindowShouldClose()) {
      Render();
    }
  }

  static size_t GetIndex(int x, int y, int width) {
    return y * width + x;
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

  GameState state = GameState::MAIN_MENU;
};

int main(int argc, char* argv[]) {
  Application app;
  app.Run();

  return 0;
}

/*

//
// Created by Tom Smale on 27/12/2024.
//

#include <raylib.h>
#include <cstdlib>
#include <cmath>

enum class Particle {
  VOID,
  AIR,
  WATER,
  SAND,
  DIRT,
  STONE,
  COUNT,
};

struct Particle {
  ParticleType type;
  Color color;
};

int main(int argc, char** argv) {
  // Initialization
  //--------------------------------------------------------------------------------------
  int screenWidth = 1280;
  int screenHeight = 720;

  InitWindow(screenWidth, screenHeight, "raylib [textures] example - texture from raw data");

  // Generate a checked texture by code
  int width = 400;
  int height = 225;

  // Dynamic memory allocation to store pixels data (Color type)
  Color *pixels = (Color *)malloc(width*height*sizeof(Color));

  Color pixelColor = (Color){255, 0, 0, 255};
  float hue = 0.0f;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {

      // Convert hue to RGB
      float r = fabs(hue * 6.0f - 3.0f) - 1.0f;
      float g = 2.0f - fabs(hue * 6.0f - 2.0f);
      float b = 2.0f - fabs(hue * 6.0f - 4.0f);

      pixelColor.r = (unsigned char)(fmax(0.0f, fmin(1.0f, r)) * 255.0f);
      pixelColor.g = (unsigned char)(fmax(0.0f, fmin(1.0f, g)) * 255.0f);
      pixelColor.b = (unsigned char)(fmax(0.0f, fmin(1.0f, b)) * 255.0f);

      pixels[y*width + x] = pixelColor;

      hue += 0.125f; // Increment hue
      if (hue > 1.0f) hue = 0.0f; // Reset hue after one full cycle
    }
  }

  // Load pixels data into an image structure and create texture
  Image checkedIm = {
      .data = pixels,             // We can assign pixels directly to data
      .width = width,
      .height = height,
      .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
      .mipmaps = 1
  };

  Texture2D checked = LoadTextureFromImage(checkedIm);
  UnloadImage(checkedIm);         // Unload CPU (RAM) image data (pixels)

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
  //---------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose())    // Detect window close button or ESC key
  {
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(BLACK);

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
        checked,
        (Rectangle){ 0, 0, (float)checked.width, -(float)checked.height }, // Source rectangle (inverted Y)
        (Rectangle){ offsetX, offsetY, scaledWidth, scaledHeight },                      // Destination rectangle
        (Vector2){ 0, 0 },                                                              // Origin
        0.0f,                                                                           // Rotation
        WHITE                                                                           // Tint
    );

    DrawText("CHECKED TEXTURE ", 84, 85, 30, BROWN);
    DrawText("GENERATED by CODE", 72, 148, 30, BROWN);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  UnloadTexture(checked);     // Texture unloading

  CloseWindow();              // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}

*/