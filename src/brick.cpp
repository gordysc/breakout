// A .cpp file includes its own header first. If the header forgot to include
// something it needs, this file then fails to compile, so the mistake is
// caught here rather than in some other file.
#include "brick.h"

#include <raylib.h>

#include <array>
#include <vector>

#include "screen.h"

namespace breakout {

// An unnamed namespace makes everything inside it visible only in this .cpp
// file (internal linkage), so these names can't clash with names in other
// files. Google style uses it for helpers that don't belong in the header.
namespace {

/// @brief Number of bricks in each row.
constexpr int kBrickColumns = 10;
/// @brief Number of rows of bricks.
constexpr int kBrickRows = 5;
/// @brief Space in pixels between neighbouring bricks and around the grid.
constexpr float kBrickGap = 4.0f;
/// @brief Height of each brick in pixels.
constexpr float kBrickHeight = 20.0f;
/// @brief Distance in pixels from the top of the window to the first row.
constexpr float kBrickTop = 50.0f;

}  // namespace

std::vector<Brick> MakeBricks() {
  // One colour per row, top to bottom. std::array is the modern replacement
  // for a C array (`Color colors[5]`): it knows its own size and can be
  // copied and passed around like any other value.
  constexpr std::array<Color, kBrickRows> kRowColors = {RED, ORANGE, YELLOW,
                                                        GREEN, BLUE};

  // Choose the width so that the bricks, plus a gap before each one and after
  // the last, exactly fill the screen width.
  const float brick_width =
      (kScreenWidth - kBrickGap * (kBrickColumns + 1)) / kBrickColumns;

  std::vector<Brick> bricks;
  // reserve() allocates room for every brick up front, so push_back never has
  // to grow the vector (allocate a bigger block and move everything) mid-loop.
  bricks.reserve(kBrickColumns * kBrickRows);

  // Index loops rather than range-based `for`, because the row and column
  // numbers are what decide each brick's position and colour.
  for (int row = 0; row < kBrickRows; ++row) {
    for (int col = 0; col < kBrickColumns; ++col) {
      const float x = kBrickGap + col * (brick_width + kBrickGap);
      const float y = kBrickTop + row * (kBrickHeight + kBrickGap);
      bricks.push_back(
          Brick{{x, y, brick_width, kBrickHeight}, kRowColors[row]});
    }
  }

  // Returning a local vector by value is cheap: the compiler either builds it
  // directly in the caller's variable (copy elision) or moves it. The bricks
  // are never copied one by one.
  return bricks;
}

}  // namespace breakout
