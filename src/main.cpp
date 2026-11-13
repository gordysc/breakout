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

/// @brief Largest angle, in degrees away from straight up, at which the ball
/// can leave the paddle. Hitting the very edge of the paddle gives this angle.
constexpr float kMaxBounceAngleDegrees = 60.0f;

/// @brief Paddle position and size at the start of each game.
constexpr Rectangle kPaddleStart{350, 560, 100, 15};
/// @brief Where the ball starts each life: the middle of the screen.
constexpr Vector2 kBallStartPosition{400, 300};
/// @brief Ball velocity at the start of each life, in pixels per second.
constexpr Vector2 kBallStartVelocity{200, -200};

/// @brief Number of lives the player starts each game with.
constexpr int kStartingLives = 3;
/// @brief Points awarded for each brick broken.
constexpr int kPointsPerBrick = 10;
/// @brief Font size in pixels for the score and lives display.
constexpr int kHudFontSize = 20;

/// @brief Which screen the game is on. Decides what each frame updates and
/// draws.
///
/// `enum class` is the modern replacement for a plain `enum`. Its names are
/// scoped (you write `GameState::kPlaying`, so they can't clash with other
/// names like raylib's), and it never silently converts to an int.
enum class GameState {
  /// @brief Title screen, shown when the game starts.
  kTitle,
  /// @brief Normal play: the ball moves and bricks can break.
  kPlaying,
  /// @brief Play is frozen until the player unpauses.
  kPaused,
  /// @brief The player has lost their last life.
  kGameOver,
  /// @brief Every brick in the level is broken.
  kLevelCleared,
};

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

/// @brief Works out the ball's velocity after it hits the paddle.
///
/// Where the ball lands on the paddle sets the bounce angle, which lets the
/// player aim. The centre sends the ball straight up, and the edges send it
/// up to kMaxBounceAngleDegrees towards that side. The ball keeps its speed.
///
/// @param ball_x Horizontal position of the ball's centre.
/// @param paddle The paddle the ball hit.
/// @param velocity The ball's velocity before the bounce.
/// @return The velocity after the bounce, always pointing upwards.
Vector2 BounceOffPaddle(float ball_x, Rectangle paddle, Vector2 velocity) {
  // `offset` is -1 at the paddle's left edge, 0 at its centre and +1 at its
  // right edge. When the ball clips a corner its centre can be slightly past
  // the edge, so std::clamp (C++17) keeps the value within -1 to +1.
  const float half_width = paddle.width / 2;
  const float paddle_center = paddle.x + half_width;
  const float offset =
      std::clamp((ball_x - paddle_center) / half_width, -1.0f, 1.0f);

  // std::sin and std::cos work in radians. DEG2RAD is a raylib macro equal
  // to PI / 180, which converts degrees to radians.
  const float angle = offset * kMaxBounceAngleDegrees * DEG2RAD;

  // std::hypot(x, y) is sqrt(x * x + y * y): the length of the velocity
  // vector, which is the ball's speed.
  const float speed = std::hypot(velocity.x, velocity.y);

  // Split the speed into horizontal and vertical parts for the new angle.
  // Screen y grows downwards, so a negative y velocity moves the ball up.
  return Vector2{speed * std::sin(angle), -speed * std::cos(angle)};
}

/// @brief Draws a line of text centred horizontally on the screen.
/// @param text The text to draw, as a C string.
/// @param y Distance in pixels from the top of the window to the text.
/// @param font_size Height of the text in pixels.
/// @param color Colour of the text.
void DrawCenteredText(const char* text, int y, int font_size, Color color) {
  // raylib is a C library, so it takes text as `const char*` (a pointer to
  // null-terminated characters), not std::string. The pointer is only
  // borrowed: this function reads the text during the call and never frees
  // it or keeps it.
  //
  // MeasureText returns how many pixels wide the text will be at this size.
  const int x = (kScreenWidth - MeasureText(text, font_size)) / 2;
  DrawText(text, x, y, font_size, color);
}

/// @brief Opens the game window and runs the game loop until it is closed.
/// @return 0 on a normal exit.
int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Breakout");
  SetTargetFPS(60);

  constexpr float kBallRadius = 8.0f;
  constexpr float kPaddleSpeed = 400.0f;

  Rectangle paddle = kPaddleStart;
  Vector2 ball = kBallStartPosition;
  Vector2 vel = kBallStartVelocity;
  std::vector<Brick> bricks = MakeBricks();
  int score = 0;
  int lives = kStartingLives;
  GameState state = GameState::kTitle;

  // These are lambdas: small unnamed functions written inline. `[&]` captures
  // the surrounding local variables by reference, so the lambda changes the
  // real `ball`, `bricks` and so on in main(). That is safe here because the
  // lambdas are only used inside main(), while those variables still exist. A
  // lambda that captured by reference and outlived them would be left holding
  // dangling references.
  //
  // A lambda's type has no name you can write, which is why `auto` is needed.
  auto reset_ball = [&] {
    ball = kBallStartPosition;
    vel = kBallStartVelocity;
  };
  auto start_level = [&] {
    paddle = kPaddleStart;
    // MakeBricks() returns a temporary vector, so this is a move assignment:
    // `bricks` takes over the new vector's memory and its old memory is freed
    // automatically. Nothing is copied, and there is no manual cleanup.
    bricks = MakeBricks();
    reset_ball();
  };

  while (!WindowShouldClose()) {
    const float dt = GetFrameTime();

    // --- Key presses that change the state ---
    // IsKeyPressed is true only on the frame the key goes down, unlike
    // IsKeyDown, which stays true for as long as the key is held.
    //
    // There is a `case` for every GameState and no `default`, so if a new
    // state is added later, the compiler warns (-Wswitch) that it is missing.
    switch (state) {
      case GameState::kTitle:
        if (IsKeyPressed(KEY_SPACE)) state = GameState::kPlaying;
        break;
      case GameState::kPlaying:
        if (IsKeyPressed(KEY_P)) state = GameState::kPaused;
        break;
      case GameState::kPaused:
        if (IsKeyPressed(KEY_P)) state = GameState::kPlaying;
        break;
      case GameState::kGameOver:
        if (IsKeyPressed(KEY_SPACE)) {
          score = 0;
          lives = kStartingLives;
          start_level();
          state = GameState::kPlaying;
        }
        break;
      case GameState::kLevelCleared:
        // Score and lives carry over into the next level.
        if (IsKeyPressed(KEY_SPACE)) {
          start_level();
          state = GameState::kPlaying;
        }
        break;
    }

    if (state == GameState::kPlaying) {
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
      // Only bounce while the ball is moving down. The ball can still overlap
      // the paddle for a frame after bouncing, and without this check the
      // bounce would be worked out again from a slightly different spot.
      if (vel.y > 0 && CheckCollisionCircleRec(ball, kBallRadius, paddle)) {
        vel = BounceOffPaddle(ball.x, paddle, vel);
      }

      // `Brick&` is a reference to the brick inside the vector, so setting
      // `alive` changes the real brick. A plain `Brick` would be a copy, and
      // the change would be lost at the end of each loop pass.
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
        score += kPointsPerBrick;
        vel = BounceOffRect(ball, kBallRadius, brick.rect, vel);
        // Break at most one brick per frame, so a hit on the seam between two
        // bricks removes only one of them.
        break;
      }

      // std::ranges::none_of (C++20) is true if the test is false for every
      // brick. The test is a lambda with an empty capture list `[]`, because
      // it only needs its own parameter, not any of main()'s variables.
      if (std::ranges::none_of(
              bricks, [](const Brick& brick) { return brick.alive; })) {
        state = GameState::kLevelCleared;
      }

      if (ball.y > kScreenHeight) {  // missed: lose a life
        --lives;
        if (lives == 0) state = GameState::kGameOver;
        reset_ball();
      }
    }

    // --- Draw ---
    BeginDrawing();
    ClearBackground(BLACK);

    if (state == GameState::kTitle) {
      DrawCenteredText("BREAKOUT", 200, 60, WHITE);
      DrawCenteredText("Press SPACE to start", 300, kHudFontSize, GRAY);
      DrawCenteredText("Arrow keys move, P pauses", 330, kHudFontSize, GRAY);
    } else {
      // `const Brick&` reads each brick without copying it, and the compiler
      // stops us from changing it by accident.
      for (const Brick& brick : bricks) {
        if (brick.alive) DrawRectangleRec(brick.rect, brick.color);
      }
      DrawRectangleRec(paddle, WHITE);
      // The ball only matters during play. On the other screens it would sit
      // behind the message text.
      if (state == GameState::kPlaying || state == GameState::kPaused) {
        DrawCircleV(ball, kBallRadius, RED);
      }

      // TextFormat works like printf: `%d` is replaced by an int argument.
      // The placeholder must match the argument's type, or the behaviour is
      // undefined. It returns a pointer into a small buffer that raylib owns
      // and reuses on later calls, so use the text straight away and never
      // keep the pointer.
      DrawText(TextFormat("Score: %d", score), 10, 10, kHudFontSize, WHITE);
      const char* lives_text = TextFormat("Lives: %d", lives);
      DrawText(lives_text,
               kScreenWidth - MeasureText(lives_text, kHudFontSize) - 10, 10,
               kHudFontSize, WHITE);

      switch (state) {
        case GameState::kTitle:
        case GameState::kPlaying:
          // No message: the title has its own screen above, and during play
          // the field stays clear. Two `case` labels in a row share one body.
          break;
        case GameState::kPaused:
          DrawCenteredText("PAUSED", 220, 40, WHITE);
          DrawCenteredText("Press P to resume", 280, kHudFontSize, GRAY);
          break;
        case GameState::kGameOver:
          DrawCenteredText("GAME OVER", 220, 40, RED);
          DrawCenteredText(TextFormat("Final score: %d", score), 280,
                           kHudFontSize, WHITE);
          DrawCenteredText("Press SPACE to play again", 320, kHudFontSize,
                           GRAY);
          break;
        case GameState::kLevelCleared:
          DrawCenteredText("LEVEL CLEARED", 220, 40, GREEN);
          DrawCenteredText(TextFormat("Score: %d", score), 280, kHudFontSize,
                           WHITE);
          DrawCenteredText("Press SPACE for the next level", 320, kHudFontSize,
                           GRAY);
          break;
      }
    }

    DrawFPS(10, kScreenHeight - 25);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
