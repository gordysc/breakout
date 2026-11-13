#include <raylib.h>

#include <cmath>

constexpr int kScreenWidth = 800;
constexpr int kScreenHeight = 600;

int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Breakout");
  SetTargetFPS(60);

  Rectangle paddle{350, 560, 100, 15};
  Vector2 ball{400, 300};
  Vector2 vel{200, -200};
  constexpr float kBallRadius = 8.0f;
  constexpr float kPaddleSpeed = 400.0f;

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

    if (ball.y > kScreenHeight) {  // missed: reset
      ball = {400, 300};
      vel = {200, -200};
    }

    // --- Draw ---
    BeginDrawing();
    ClearBackground(BLACK);
    DrawRectangleRec(paddle, WHITE);
    DrawCircleV(ball, kBallRadius, RED);
    DrawFPS(10, 10);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
