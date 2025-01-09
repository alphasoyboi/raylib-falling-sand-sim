//
// Created by Tom Smale on 02/01/2025.
//

#ifndef RAYLIB_SAND_SIM_SRC_WORLD_H_
#define RAYLIB_SAND_SIM_SRC_WORLD_H_

#include <raylib.h>

#include <cstdlib>
#include <string>

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
    kGas,
  };

  static void LoadElements(const std::string& filename);
  static Type GetType(Element element);
  static Color GetColor(Element element);
  static int GetWeight(Element element);
  static int GetViscosity(Element element);
  static std::string GetName(Element element);

  static const std::array<Color, static_cast<size_t>(Element::kCount)>& GetColorTable() {
    return colors;
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
  const int width;
  const int height;

  AutomataMatrix(int width = 400, int height = 300);

  [[nodiscard]] inline int Above     (const int pos) const;
  [[nodiscard]] inline int Below     (const int pos) const;
  [[nodiscard]] inline int Right     (const int pos) const;
  [[nodiscard]] inline int Left      (const int pos) const;
  [[nodiscard]] inline int AboveRight(const int pos) const;
  [[nodiscard]] inline int AboveLeft (const int pos) const;
  [[nodiscard]] inline int BelowRight(const int pos) const;
  [[nodiscard]] inline int BelowLeft (const int pos) const;

  [[nodiscard]] inline Cell::Element GetCell(const int pos) const;
  [[nodiscard]] inline Cell::Element GetCell(const int x, const int y) const;

  void SetCell(const int pos, const Cell::Element element);
  void SetCell(const int x, const int y, const Cell::Element element);

  void SetCircle(const int x, const int y, const int radius, const Cell::Element element);

  void SwapCells(const int pos1, const int pos2);
  void SwapCells(const int x1, const int y1, const int x2, const int y2);

  void UpdatePowder(int pos, Cell::Element element, const int direction);
  void UpdateWater(int pos, const int direction);
  void ApplyGravity(int pos, Cell::Element element, const int direction);
  void ApplySpread(int& pos, int spread, const int direction);

  void Update();

  void GetPixelArray(std::vector<Color>& pixels);
  const std::vector<uint8_t>& GetMatrix();

 private:
  std::vector<Cell::Element> cell;
  std::vector<uint8_t>       heat;
  std::vector<uint8_t>       shade;
  std::vector<uint8_t>       dirty;

};

#endif //RAYLIB_SAND_SIM_SRC_WORLD_H_
