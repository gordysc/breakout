#ifndef BREAKOUT_BRICK_H_
#define BREAKOUT_BRICK_H_

// Header guard; see game.h for how it works.

#include <raylib.h>

#include <vector>

/// @brief Everything that belongs to the Breakout game.
namespace breakout {

/// @brief One breakable brick in the grid.
///
/// This is a plain aggregate (a struct with no constructors), so you can
/// create one with braces: `Brick{rect, color}`. `alive` has a default member
/// initializer, so it can be left out of the braces and then starts as `true`.
struct Brick {
  /// @brief Position and size of the brick on screen.
  Rectangle rect;
  /// @brief Fill colour used when drawing the brick.
  Color color;
  /// @brief Whether the brick is still standing. Broken bricks are skipped
  /// when checking collisions and when drawing.
  bool alive = true;
};

// This is only a declaration: it tells other files that the function exists
// and how to call it. The body (the definition) is in brick.cpp. Any file
// that includes this header can call MakeBricks(), and the linker connects
// those calls to the one compiled body.

/// @brief Builds a full grid of bricks across the top of the screen.
/// @return One brick per row and column, all alive, ordered row by row from
///         the top-left.
std::vector<Brick> MakeBricks();

}  // namespace breakout

#endif  // BREAKOUT_BRICK_H_
