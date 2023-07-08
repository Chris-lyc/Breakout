#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <irrKlang/irrKlang.h>
#include <vector>
#include <sstream>

#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_level.h"
#include "ball_object.h"
#include "particle_generator.h"
#include "post_processor.h"
#include "powerup.h"
#include "text_renderer.h"


// Represents the current state of the game
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

// Game holds all game-related state and functionality.
// Combines all game-related data into a single class for
// easy access to each of the components and manageability.
class Game
{
public:
    // Game state
    GameState              State;
    GLboolean              Keys[1024];
    GLuint                 Width, Height;
    std::vector<GameLevel> Levels;
    GLuint                 Level;
    std::vector<PowerUp> PowerUps;
    unsigned int Lives;
    bool KeyProcessed[1024];

    // Constructor/Destructor
    Game(GLuint width, GLuint height);
    ~Game();
    // Initialize game state (load all shaders/textures/levels)
    void Init();
    // GameLoop
    void ProcessInput(GLfloat dt);
    void Update(GLfloat dt);
    void Render();
    // Collision
    void DoCollisions();
    // Reset
    void ResetLevel();
    void ResetPlayer();
    // Powerup
    void SpawnPowerUps(GameObject& block);
    void UpdatePowerUps(GLfloat dt);
};