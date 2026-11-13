#ifndef BREAKOUT_PHYSICS_H_
#define BREAKOUT_PHYSICS_H_

// Header guard; see game.h for how it works.

#include <raylib.h>

/// @brief Everything that belongs to the Breakout game.
namespace breakout {

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
                      Vector2 velocity);

/// @brief Works out the ball's velocity after it hits the paddle.
///
/// Where the ball lands on the paddle sets the bounce angle, which lets the
/// player aim. The centre sends the ball straight up, and the edges send it
/// up to 60 degrees towards that side. The ball keeps its speed.
///
/// @param ball_x Horizontal position of the ball's centre.
/// @param paddle The paddle the ball hit.
/// @param velocity The ball's velocity before the bounce.
/// @return The velocity after the bounce, always pointing upwards.
Vector2 BounceOffPaddle(float ball_x, Rectangle paddle, Vector2 velocity);

}  // namespace breakout

#endif  // BREAKOUT_PHYSICS_H_
