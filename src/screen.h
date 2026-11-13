#ifndef BREAKOUT_SCREEN_H_
#define BREAKOUT_SCREEN_H_

// Header guard; see game.h for how it works.

/// @brief Everything that belongs to the Breakout game.
namespace breakout {

// `inline` (C++17) on a variable in a header means every .cpp file that
// includes the header shares one variable. Without it, each .cpp file would
// get its own private copy.

/// @brief Window width in pixels.
inline constexpr int kScreenWidth = 800;
/// @brief Window height in pixels.
inline constexpr int kScreenHeight = 600;

}  // namespace breakout

#endif  // BREAKOUT_SCREEN_H_
