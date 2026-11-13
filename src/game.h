// A header guard. The first time a .cpp file includes this header,
// BREAKOUT_GAME_H_ is not defined yet, so the contents are compiled and the
// macro gets defined. If the same .cpp file includes the header again (often
// indirectly, through another header), the #ifndef is false and the contents
// are skipped, which avoids "redefinition" errors. Google style uses guards
// named after the file instead of `#pragma once`.
#ifndef BREAKOUT_GAME_H_
#define BREAKOUT_GAME_H_

#include <raylib.h>

#include <vector>

#include "brick.h"

/// @brief Everything that belongs to the Breakout game.
namespace breakout {

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

/// @brief The whole game. Owns the paddle, ball, bricks, score, lives and
/// current screen, and knows how to update and draw them.
///
/// main() creates one Game, then calls Update() and Draw() once per frame.
class Game {
 public:
  /// @brief Sets up a fresh game on the title screen.
  Game();

  // `= delete` removes the copy constructor and copy assignment, so copying a
  // Game is a compile error. There is only ever one game, so a copy (for
  // example from passing a Game by value where you meant a reference) would
  // be a bug. Declaring these also stops the compiler from generating move
  // operations, so a Game can't be moved either.

  /// @brief Not copyable: there is only ever one game.
  Game(const Game&) = delete;
  /// @brief Not copy-assignable: there is only ever one game.
  Game& operator=(const Game&) = delete;

  /// @brief Advances the game by one frame. Handles key presses and, during
  /// play, moves the paddle and ball and checks collisions.
  /// @param dt Seconds since the last frame.
  void Update(float dt);

  // `const` after the parentheses promises that the method doesn't change the
  // Game. The compiler enforces it: assigning to a data member inside a const
  // method is an error. Every Draw method is const, because drawing should
  // only ever read the game, never change it.

  /// @brief Draws the current frame. Call it between raylib's BeginDrawing()
  /// and EndDrawing().
  void Draw() const;

 private:
  /// @brief Moves between states when the player presses SPACE or P.
  void HandleStateKeys();
  /// @brief Moves the paddle and ball and handles every collision. Only
  /// called in the Playing state.
  /// @param dt Seconds since the last frame.
  void UpdatePlaying(float dt);
  /// @brief Puts the ball back in the middle of the screen with its starting
  /// velocity.
  void ResetBall();
  /// @brief Resets the paddle, bricks and ball for a new level. Score and
  /// lives are left alone.
  void StartLevel();

  /// @brief Draws the title screen.
  void DrawTitle() const;
  /// @brief Draws the bricks, the paddle and, during play, the ball.
  void DrawField() const;
  /// @brief Draws the score and lives along the top of the screen.
  void DrawHud() const;
  /// @brief Draws the centred message for the paused, game over and level
  /// cleared screens.
  void DrawMessage() const;

  // Google style ends data member names with an underscore, so inside the
  // methods you can tell `score_` (part of the Game) from a local variable.

  /// @brief The player's paddle.
  Rectangle paddle_;
  /// @brief Centre of the ball.
  Vector2 ball_;
  /// @brief The ball's velocity in pixels per second.
  Vector2 velocity_;
  /// @brief Every brick in the level, including broken ones.
  std::vector<Brick> bricks_;
  /// @brief Points scored so far this game.
  int score_;
  /// @brief Lives left. The game ends when this reaches 0.
  int lives_;
  /// @brief Which screen the game is on.
  GameState state_;
};

}  // namespace breakout

#endif  // BREAKOUT_GAME_H_
