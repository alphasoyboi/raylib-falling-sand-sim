//
// Created by Tom Smale on 02/01/2025.
//

#include "automata_matrix.h"

#include <raylib.h>
#include <nlohmann/json.hpp>

#include <fstream>

Color particleColors[] = {
    BLACK, // AIR
    BEIGE, // SAND
    GRAY,  // STONE
    BLUE,  // WATER
    DARKGRAY, // BEDROCK
};

void Cell::LoadElements(const std::string& filename) {
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

Cell::Type Cell::GetType(Cell::Element element) {
  return types[static_cast<size_t>(element)];
}

Color Cell::GetColor(Cell::Element element) {
  return colors[static_cast<size_t>(element)];
}

int Cell::GetWeight(Cell::Element element) {
  return weights[static_cast<size_t>(element)];
}

int Cell::GetViscosity(Cell::Element element) {
  return viscosity[static_cast<size_t>(element)];
}

std::string Cell::GetName(Cell::Element element) {
  return names[static_cast<size_t>(element)];
}

std::array<Cell::Type,  static_cast<size_t>(Cell::Element::kCount)> Cell::types;
std::array<Color,       static_cast<size_t>(Cell::Element::kCount)> Cell::colors;
std::array<int,         static_cast<size_t>(Cell::Element::kCount)> Cell::weights;
std::array<int,         static_cast<size_t>(Cell::Element::kCount)> Cell::viscosity;
std::array<std::string, static_cast<size_t>(Cell::Element::kCount)> Cell::names;

AutomataMatrix::AutomataMatrix(const int width, const int height) : width(width), height(height) {
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

inline int AutomataMatrix::Above     (const int pos) const { return pos + width; }
inline int AutomataMatrix::Below     (const int pos) const { return pos - width; }
inline int AutomataMatrix::Right     (const int pos) const { return pos + 1; }
inline int AutomataMatrix::Left      (const int pos) const { return pos - 1; }
inline int AutomataMatrix::AboveRight(const int pos) const { return pos + width + 1; }
inline int AutomataMatrix::AboveLeft (const int pos) const { return pos + width - 1; }
inline int AutomataMatrix::BelowRight(const int pos) const { return pos - width + 1; }
inline int AutomataMatrix::BelowLeft (const int pos) const { return pos - width - 1; }

inline Cell::Element AutomataMatrix::GetCell(const int pos) const {
  return cell[pos];
}

inline Cell::Element AutomataMatrix::GetCell(const int x, const int y) const {
  return cell[y*width + x];
}

void AutomataMatrix::SetCell(const int pos, const Cell::Element element) {
  cell[pos] = element;
}

void AutomataMatrix::SetCircle(const int x, const int y, const int radius, const Cell::Element element) {
  // Iterate over a square area centered at (x, y) with the given radius
  for (int i = -radius; i <= radius; i++) {
    for (int j = -radius; j <= radius; j++) {
      // Check if the current position (i, j) is within the circle's radius
      if (i*i + j*j <= radius*radius) {
        int newX = x + i;
        int newY = y + j;
        // Check if the new coordinates are within bounds
        if (newX >= 0 && newX < width && newY >= 0 && newY < height) {
          // Set the cell at the current position to the specified element
          if (GetCell(newX, newY) == Cell::Element::kAir) {
            SetCell(newX, newY, element);
          }
        }
      }
    }
  }
}
void AutomataMatrix::SetCell(const int x, const int y, const Cell::Element element) {
  cell[y*width + x] = element;
}

void AutomataMatrix::SwapCells(const int pos1, const int pos2) {
  std::swap(cell[pos1], cell[pos2]);
}

void AutomataMatrix::SwapCells(const int x1, const int y1, const int x2, const int y2) {
  std::swap(cell[y1*width + x1], cell[y2*width + x2]);
}

void AutomataMatrix::UpdatePowder(int pos, Cell::Element element, const int direction) {
  int weight = Cell::GetWeight(element);

  while (weight-- != 0) {
    const int below = Below(pos);
    int directionA = direction ? BelowRight(pos) : BelowLeft(pos);
    int directionB = direction ? BelowLeft(pos) : BelowRight(pos);
    if (Cell::GetType(cell[below]) == Cell::Type::kEmpty || Cell::GetType(cell[below]) == Cell::Type::kLiquid) {
      SwapCells(pos, below);
      pos = below;
    } else if (Cell::GetType(cell[directionA]) == Cell::Type::kEmpty || Cell::GetType(cell[directionA]) == Cell::Type::kLiquid) {
      SwapCells(pos, directionA);
      pos = directionA;
    } else if (Cell::GetType(cell[directionB]) == Cell::Type::kEmpty || Cell::GetType(cell[directionB]) == Cell::Type::kLiquid) {
      SwapCells(pos, directionB);
      pos = directionB;
    } else {
      // spread
      break;
    }
  }
  dirty[pos] = 0;
}

void AutomataMatrix::UpdateWater(int pos, const int direction) {
  int weight = Cell::GetWeight(Cell::Element::kWater);
  int spread = Cell::GetViscosity(Cell::Element::kWater);
  while (weight-- != 0) {
    const int below = Below(pos);
    int diagonalA = direction ? BelowRight(pos) : BelowLeft(pos);
    int diagonalB = direction ? BelowLeft(pos) : BelowRight(pos);
    int sideA = direction ? Right(pos) : Left(pos);
    int sideB = direction ? Left(pos) : Right(pos);
    if (Cell::GetType(cell[below]) == Cell::Type::kEmpty) {
      SwapCells(pos, below);
      pos = below;
    } else if (Cell::GetType(cell[diagonalA]) == Cell::Type::kEmpty) {
      SwapCells(pos, diagonalA);
      pos = diagonalA;
    } else if (Cell::GetType(cell[diagonalB]) == Cell::Type::kEmpty) {
      SwapCells(pos, diagonalB);
      pos = diagonalB;
    } else if (Cell::GetType(cell[sideA]) == Cell::Type::kEmpty) {
      SwapCells(pos, sideA);
      pos = sideA;
    } else if (Cell::GetType(cell[sideB]) == Cell::Type::kEmpty) {
      SwapCells(pos, sideB);
      pos = sideB;
    } else {
      break;
    }
  }
  dirty[pos] = 0;
}

void AutomataMatrix::ApplyGravity(int pos, Cell::Element element, const int direction) {
  Cell::Type type = Cell::GetType(element);
  int weight = Cell::GetWeight(element);
  int spread = Cell::GetViscosity(element);
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
      if (type == Cell::Type::kLiquid) {
        ApplySpread(pos, spread, direction);
      }
      break;
    }
  }
  dirty[pos] = 0;
}

void AutomataMatrix::ApplySpread(int& pos, int spread, const int direction) {
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

void AutomataMatrix::Update() {
  const int direction = GetRandomValue(0, 1);
  for (int i = 0; i < width*height; i++) {
    switch(cell[i]) {
      case Cell::Element::kSand: {
        //ApplyGravity(i, Cell::Element::kSand, direction);
        UpdatePowder(i, Cell::Element::kSand, direction);
        break;
      }
      case Cell::Element::kWater: {
        ApplyGravity(i, Cell::Element::kWater, direction);
        //UpdateWater(i, direction);
        break;
      }
      default:
        break;
    }
  }
  std::ranges::fill(dirty, 1);
}

void AutomataMatrix::GetPixelArray(std::vector<Color>& pixels) {
  for (int i = 0; i < width*height; i++) {
    pixels[i] = Cell::GetColor(cell[i]);
  }
}

const std::vector<uint8_t>& AutomataMatrix::GetMatrix() {
  return reinterpret_cast<const std::vector<uint8_t>&>(cell);
}