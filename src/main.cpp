#include <raylib.h>

#include "game.h"
#include "screen.h"

/// @brief Opens the game window and runs the game loop until it is closed.
/// @return 0 on a normal exit.
int main() {
  InitWindow(breakout::kScreenWidth, breakout::kScreenHeight, "Breakout");
  SetTargetFPS(60);

  // `game` is a local variable, so its destructor runs when main() returns,
  // which is after CloseWindow(). That is fine for now, because Game holds no
  // raylib resources. If it ever loads textures or sounds, wrap the next few
  // lines in their own `{ }` block, so that the Game is destroyed (and can
  // unload them) before the window closes.
  breakout::Game game;
  while (!WindowShouldClose()) {
    game.Update(GetFrameTime());

    BeginDrawing();
    game.Draw();
    DrawFPS(10, breakout::kScreenHeight - 25);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
