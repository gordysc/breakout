#include "game.h"

#include <raylib.h>

#include <algorithm>
#include <vector>

#include "brick.h"
#include "physics.h"
#include "screen.h"

namespace breakout {

// An unnamed namespace makes everything inside it visible only in this .cpp
// file; see brick.cpp.
namespace {

/// @brief Radius of the ball in pixels.
constexpr float kBallRadius = 8.0f;
/// @brief How fast the paddle moves, in pixels per second.
constexpr float kPaddleSpeed = 400.0f;
/// @brief Paddle position and size at the start of each level.
constexpr Rectangle kPaddleStart{350, 560, 100, 15};
/// @brief Where the ball starts each life: the middle of the screen.
constexpr Vector2 kBallStartPosition{400, 300};
/// @brief Ball velocity at the start of each life, in pixels per second.
constexpr Vector2 kBallStartVelocity{200, -200};

/// @brief Number of lives the player starts each game with.
constexpr int kStartingLives = 3;
/// @brief Points awarded for each brick broken.
constexpr int kPointsPerBrick = 10;
/// @brief Font size in pixels for the score, lives and smaller messages.
constexpr int kHudFontSize = 20;

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

}  // namespace

// The part after the colon is the member initializer list. It sets each data
// member as it is created, rather than creating it empty and then assigning
// to it in the body. Members are always initialized in the order they are
// declared in game.h, whatever order this list uses, so keep the two orders
// the same to avoid surprises.
Game::Game()
    : paddle_(kPaddleStart),
      ball_(kBallStartPosition),
      velocity_(kBallStartVelocity),
      bricks_(MakeBricks()),
      score_(0),
      lives_(kStartingLives),
      state_(GameState::kTitle) {}

void Game::Update(float dt) {
  HandleStateKeys();
  // Only the Playing state moves anything, so pausing needs no extra code.
  if (state_ == GameState::kPlaying) UpdatePlaying(dt);
}

void Game::HandleStateKeys() {
  // IsKeyPressed is true only on the frame the key goes down, unlike
  // IsKeyDown, which stays true for as long as the key is held.
  //
  // There is a `case` for every GameState and no `default`, so if a new state
  // is added later, the compiler warns (-Wswitch) that it is missing.
  switch (state_) {
    case GameState::kTitle:
      if (IsKeyPressed(KEY_SPACE)) state_ = GameState::kPlaying;
      break;
    case GameState::kPlaying:
      if (IsKeyPressed(KEY_P)) state_ = GameState::kPaused;
      break;
    case GameState::kPaused:
      if (IsKeyPressed(KEY_P)) state_ = GameState::kPlaying;
      break;
    case GameState::kGameOver:
      if (IsKeyPressed(KEY_SPACE)) {
        score_ = 0;
        lives_ = kStartingLives;
        StartLevel();
        state_ = GameState::kPlaying;
      }
      break;
    case GameState::kLevelCleared:
      // Score and lives carry over into the next level.
      if (IsKeyPressed(KEY_SPACE)) {
        StartLevel();
        state_ = GameState::kPlaying;
      }
      break;
  }
}

void Game::UpdatePlaying(float dt) {
  // --- Paddle ---
  if (IsKeyDown(KEY_LEFT)) paddle_.x -= kPaddleSpeed * dt;
  if (IsKeyDown(KEY_RIGHT)) paddle_.x += kPaddleSpeed * dt;
  // std::clamp keeps the paddle fully on screen.
  paddle_.x = std::clamp(paddle_.x, 0.0f, kScreenWidth - paddle_.width);

  // --- Ball ---
  ball_.x += velocity_.x * dt;
  ball_.y += velocity_.y * dt;

  if (ball_.x - kBallRadius < 0 || ball_.x + kBallRadius > kScreenWidth) {
    velocity_.x *= -1;
  }
  if (ball_.y - kBallRadius < 0) velocity_.y *= -1;
  // Only bounce while the ball is moving down. The ball can still overlap the
  // paddle for a frame after bouncing, and without this check the bounce
  // would be worked out again from a slightly different spot.
  if (velocity_.y > 0 && CheckCollisionCircleRec(ball_, kBallRadius, paddle_)) {
    velocity_ = BounceOffPaddle(ball_.x, paddle_, velocity_);
  }

  // --- Bricks ---
  // `Brick&` is a reference to the brick inside the vector, so setting `alive`
  // changes the real brick. A plain `Brick` would be a copy, and the change
  // would be lost at the end of each loop pass.
  //
  // Broken bricks are flagged rather than erased: erasing from a vector while
  // looping over it invalidates the loop's iterators, which is undefined
  // behaviour.
  for (Brick& brick : bricks_) {
    if (!brick.alive ||
        !CheckCollisionCircleRec(ball_, kBallRadius, brick.rect)) {
      continue;
    }
    brick.alive = false;
    score_ += kPointsPerBrick;
    velocity_ = BounceOffRect(ball_, kBallRadius, brick.rect, velocity_);
    // Break at most one brick per frame, so a hit on the seam between two
    // bricks removes only one of them.
    break;
  }

  // std::ranges::none_of (C++20) is true if the test is false for every
  // brick. The test is a lambda: a small unnamed function written inline. Its
  // capture list `[]` is empty because it only needs its own parameter, not
  // any variables from around it.
  if (std::ranges::none_of(bricks_,
                           [](const Brick& brick) { return brick.alive; })) {
    state_ = GameState::kLevelCleared;
  }

  // --- Missed ball ---
  if (ball_.y > kScreenHeight) {
    --lives_;
    if (lives_ == 0) state_ = GameState::kGameOver;
    ResetBall();
  }
}

void Game::ResetBall() {
  ball_ = kBallStartPosition;
  velocity_ = kBallStartVelocity;
}

void Game::StartLevel() {
  paddle_ = kPaddleStart;
  // MakeBricks() returns a temporary vector, so this is a move assignment:
  // `bricks_` takes over the new vector's memory and its old memory is freed
  // automatically. Nothing is copied, and there is no manual cleanup.
  bricks_ = MakeBricks();
  ResetBall();
}

void Game::Draw() const {
  ClearBackground(BLACK);
  if (state_ == GameState::kTitle) {
    DrawTitle();
    return;
  }
  DrawField();
  DrawHud();
  DrawMessage();
}

void Game::DrawTitle() const {
  DrawCenteredText("BREAKOUT", 200, 60, WHITE);
  DrawCenteredText("Press SPACE to start", 300, kHudFontSize, GRAY);
  DrawCenteredText("Arrow keys move, P pauses", 330, kHudFontSize, GRAY);
}

void Game::DrawField() const {
  // `const Brick&` reads each brick without copying it. It has to be const
  // here anyway: inside a const method, every data member (including
  // `bricks_`) is treated as const.
  for (const Brick& brick : bricks_) {
    if (brick.alive) DrawRectangleRec(brick.rect, brick.color);
  }
  DrawRectangleRec(paddle_, WHITE);
  // The ball only matters during play. On the other screens it would sit
  // behind the message text.
  if (state_ == GameState::kPlaying || state_ == GameState::kPaused) {
    DrawCircleV(ball_, kBallRadius, RED);
  }
}

void Game::DrawHud() const {
  // TextFormat works like printf: `%d` is replaced by an int argument. The
  // placeholder must match the argument's type, or the behaviour is
  // undefined. It returns a pointer into a small buffer that raylib owns and
  // reuses on later calls, so use the text straight away and never keep the
  // pointer.
  DrawText(TextFormat("Score: %d", score_), 10, 10, kHudFontSize, WHITE);
  const char* lives_text = TextFormat("Lives: %d", lives_);
  DrawText(lives_text,
           kScreenWidth - MeasureText(lives_text, kHudFontSize) - 10, 10,
           kHudFontSize, WHITE);
}

void Game::DrawMessage() const {
  switch (state_) {
    case GameState::kTitle:
    case GameState::kPlaying:
      // No message: the title has its own screen, and during play the field
      // stays clear. Two `case` labels in a row share one body.
      break;
    case GameState::kPaused:
      DrawCenteredText("PAUSED", 220, 40, WHITE);
      DrawCenteredText("Press P to resume", 280, kHudFontSize, GRAY);
      break;
    case GameState::kGameOver:
      DrawCenteredText("GAME OVER", 220, 40, RED);
      DrawCenteredText(TextFormat("Final score: %d", score_), 280, kHudFontSize,
                       WHITE);
      DrawCenteredText("Press SPACE to play again", 320, kHudFontSize, GRAY);
      break;
    case GameState::kLevelCleared:
      DrawCenteredText("LEVEL CLEARED", 220, 40, GREEN);
      DrawCenteredText(TextFormat("Score: %d", score_), 280, kHudFontSize,
                       WHITE);
      DrawCenteredText("Press SPACE for the next level", 320, kHudFontSize,
                       GRAY);
      break;
  }
}

}  // namespace breakout
