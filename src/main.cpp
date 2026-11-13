#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

/// @brief Window width in pixels.
constexpr int kScreenWidth = 800;
/// @brief Window height in pixels.
constexpr int kScreenHeight = 600;

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

/// @brief Builds a full grid of bricks across the top of the screen.
/// @return kBrickColumns * kBrickRows bricks, all alive, ordered row by row
///         from the top-left.
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

/// @brief Works out the ball's velocity after it hits a rectangle.
///
/// Measures how deep the ball overlaps the rectangle horizontally and
/// vertically. The shallower overlap tells us which side the ball came in
/// through: a shallow horizontal overlap means it hit the left or right side,
/// otherwise it hit the top or bottom.
///
/// @param ball Centre of the ball.
/// @param radius Radius of the ball.
/// @param rect The rectangle the ball is touching.
/// @param velocity The ball's velocity before the bounce.
/// @return The velocity after the bounce, pointing away from the rectangle.
Vector2 BounceOffRect(Vector2 ball, float radius, Rectangle rect,
                      Vector2 velocity) {
  // Vector2 and Rectangle are just a few floats, so passing them by value
  // (as copies) costs no more than passing references. It also lets us change
  // `velocity` here without affecting the caller's variable.
  const float overlap_x = std::min(ball.x + radius - rect.x,
                                   rect.x + rect.width - (ball.x - radius));
  const float overlap_y = std::min(ball.y + radius - rect.y,
                                   rect.y + rect.height - (ball.y - radius));

  // Point the velocity away from the rectangle's centre instead of simply
  // flipping its sign. A plain flip could send the ball back into the
  // rectangle if it was already moving away from it.
  if (overlap_x < overlap_y) {
    const bool left_of_center = ball.x < rect.x + rect.width / 2;
    velocity.x = left_of_center ? -std::abs(velocity.x) : std::abs(velocity.x);
  } else {
    const bool above_center = ball.y < rect.y + rect.height / 2;
    velocity.y = above_center ? -std::abs(velocity.y) : std::abs(velocity.y);
  }
  return velocity;
}

/// @brief Opens the game window and runs the game loop until it is closed.
/// @return 0 on a normal exit.
int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Breakout");
  SetTargetFPS(60);

  Rectangle paddle{350, 560, 100, 15};
  Vector2 ball{400, 300};
  Vector2 vel{200, -200};
  constexpr float kBallRadius = 8.0f;
  constexpr float kPaddleSpeed = 400.0f;
  std::vector<Brick> bricks = MakeBricks();

  while (!WindowShouldClose()) {
    const float dt = GetFrameTime();

    // --- Input ---
    if (IsKeyDown(KEY_LEFT)) paddle.x -= kPaddleSpeed * dt;
    if (IsKeyDown(KEY_RIGHT)) paddle.x += kPaddleSpeed * dt;
    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + paddle.width > kScreenWidth)
      paddle.x = kScreenWidth - paddle.width;

    // --- Update ---
    ball.x += vel.x * dt;
    ball.y += vel.y * dt;

    if (ball.x - kBallRadius < 0 || ball.x + kBallRadius > kScreenWidth)
      vel.x *= -1;
    if (ball.y - kBallRadius < 0) vel.y *= -1;
    if (CheckCollisionCircleRec(ball, kBallRadius, paddle))
      vel.y = -std::abs(vel.y);

    // `Brick&` is a reference to the brick inside the vector, so setting
    // `alive` changes the real brick. A plain `Brick` would be a copy, and the
    // change would be lost at the end of each loop pass.
    //
    // Broken bricks are flagged rather than erased: erasing from a vector
    // while looping over it invalidates the loop's iterators, which is
    // undefined behaviour.
    for (Brick& brick : bricks) {
      if (!brick.alive ||
          !CheckCollisionCircleRec(ball, kBallRadius, brick.rect)) {
        continue;
      }
      brick.alive = false;
      vel = BounceOffRect(ball, kBallRadius, brick.rect, vel);
      // Break at most one brick per frame, so a hit on the seam between two
      // bricks removes only one of them.
      break;
    }

    if (ball.y > kScreenHeight) {  // missed: reset
      ball = {400, 300};
      vel = {200, -200};
    }

    // --- Draw ---
    BeginDrawing();
    ClearBackground(BLACK);
    // `const Brick&` reads each brick without copying it, and the compiler
    // stops us from changing it by accident.
    for (const Brick& brick : bricks) {
      if (brick.alive) DrawRectangleRec(brick.rect, brick.color);
    }
    DrawRectangleRec(paddle, WHITE);
    DrawCircleV(ball, kBallRadius, RED);
    DrawFPS(10, 10);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
