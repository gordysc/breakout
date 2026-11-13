#include "physics.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

namespace breakout {
namespace {

/// @brief Largest angle, in degrees away from straight up, at which the ball
/// can leave the paddle. Hitting the very edge of the paddle gives this angle.
constexpr float kMaxBounceAngleDegrees = 60.0f;

}  // namespace

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

}  // namespace breakout
