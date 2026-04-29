#pragma once

/// \file
/// \brief This file contains the game lifecycle and logics.
/// There are 5 important functions:
/// `GameInit`, `GameInput`, `GameUpdate`, `GameTerminate`, and
/// the most important: `GameLifecycle`.
/// Please read the corresponding comments to understand their relationships.

#include "Config.h"
#include "Renderer.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  char keyHit; // The keyboard key hit by the player at this frame.
} Game;

// The game singleton.
static Game game;

// The keyboard key "ESC".
static const char keyESC = '\033';

// Keyboard keys for tank control
static const char keyUp = 'w';
static const char keyDown = 's';
static const char keyLeft = 'a';
static const char keyRight = 'd';
static const char keyShoot = 'k';

// Game state for original feature (win/lose condition)
static bool gameOver = false;
static bool playerWon = false;

//
//
//

/// \brief Check if player won or lost.
void CheckGameState(void) {
  int playerCount = 0;
  int enemyCount = 0;
  
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *tank = RegEntry(regTank, it);
    if (tank->isPlayer) {
      playerCount++;
    } else {
      enemyCount++;
    }
  }
  
  if (playerCount == 0) {
    gameOver = true;
    playerWon = false;
  } else if (enemyCount == 0) {
    gameOver = true;
    playerWon = true;
  }
}

/// \brief Get the direction vector from Dir enum.
Vec GetDirVec(Dir dir) {
  switch (dir) {
    case eDirOP: return (Vec){0, 1};  // Up
    case eDirON: return (Vec){0, -1}; // Down
    case eDirNO: return (Vec){-1, 0}; // Left
    case eDirPO: return (Vec){1, 0};  // Right
    default: return (Vec){0, 0};
  }
}

/// \brief Check if position is valid (inside map and not blocked).
bool CanMoveTo(Vec pos) {
  if (pos.x < 0 || pos.x >= map.size.x || pos.y < 0 || pos.y >= map.size.y)
    return false;
  Flag flag = map.flags[Idx(pos)];
  if (flag == eFlagSolid || flag == eFlagWall)
    return false;
  return true;
}

/// \brief Check if position collides with any tank.
bool CollidesWithTank(Vec pos, Tank *exclude) {
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *tank = RegEntry(regTank, it);
    if (tank == exclude) continue;
    // Check all 3x3 cells of tank
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        Vec tankPos = Add(tank->pos, (Vec){dx, dy});
        if (Eq(pos, tankPos))
          return true;
      }
    }
  }
  return false;
}

/// \brief Check if position is blocked (map or tank).
bool IsBlocked(Vec pos, Tank *exclude) {
  return !CanMoveTo(pos) || CollidesWithTank(pos, exclude);
}

/// \brief Configure the scene (See `Scene.h`) with `config` (See `Config.h`), and
/// perform initializations including:
/// 1. Terminal setup.
/// 2. Memory allocations.
/// 3. Map and object generations.
/// 4. Rendering of the initialized scene.
///
/// \note This function should be called at the very beginning of `GameLifecycle`.
void GameInit(void) {
  // Setup terminal.
  TermSetupGameEnvironment();
  TermClearScreen();

  // Configure scene.
  map.size = config.mapSize;
  int nEnemies = config.nEnemies;
  int nSolids = config.nSolids;
  int nWalls = config.nWalls;

  // Initialize scene.
  RegInit(regTank);
  RegInit(regBullet);

  map.flags = (Flag *)malloc(sizeof(Flag) * map.size.x * map.size.y);
  for (int y = 0; y < map.size.y; ++y)
    for (int x = 0; x < map.size.x; ++x) {
      Vec pos = {x, y};

      Flag flag = eFlagNone;
      if (x == 0 || y == 0 || x == map.size.x - 1 || y == map.size.y - 1)
        flag = eFlagSolid;

      map.flags[Idx(pos)] = flag;
    }

  // Randomly generate solids (3x3 blocks)
  for (int i = 0; i < nSolids; ++i) {
    Vec pos = RandVec(map.size);
    // Try to place a 3x3 solid block
    bool canPlace = true;
    for (int dy = -1; dy <= 1 && canPlace; ++dy)
      for (int dx = -1; dx <= 1 && canPlace; ++dx) {
        Vec testPos = Add(pos, (Vec){dx, dy});
        if (testPos.x <= 0 || testPos.x >= map.size.x - 1 ||
            testPos.y <= 0 || testPos.y >= map.size.y - 1 ||
            map.flags[Idx(testPos)] != eFlagNone)
          canPlace = false;
      }
    if (canPlace) {
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
          Vec solidPos = Add(pos, (Vec){dx, dy});
          map.flags[Idx(solidPos)] = eFlagSolid;
        }
    }
  }

  // Randomly generate walls (3x3 blocks)
  for (int i = 0; i < nWalls; ++i) {
    Vec pos = RandVec(map.size);
    bool canPlace = true;
    for (int dy = -1; dy <= 1 && canPlace; ++dy)
      for (int dx = -1; dx <= 1 && canPlace; ++dx) {
        Vec testPos = Add(pos, (Vec){dx, dy});
        if (testPos.x <= 0 || testPos.x >= map.size.x - 1 ||
            testPos.y <= 0 || testPos.y >= map.size.y - 1 ||
            map.flags[Idx(testPos)] != eFlagNone)
          canPlace = false;
      }
    if (canPlace) {
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
          Vec wallPos = Add(pos, (Vec){dx, dy});
          map.flags[Idx(wallPos)] = eFlagWall;
        }
    }
  }

  // Create player tank
  {
    Tank *tank = RegNew(regTank);
    tank->pos = (Vec){2, 2};
    tank->dir = eDirOP;
    tank->color = TK_GREEN;
    tank->isPlayer = true;
    tank->shootCooldown = 0;
    tank->moveCooldown = 0;
    tank->hp = 3; // Player has 3 HP (original feature)
  }

  // Create enemy tanks
  for (int i = 0; i < nEnemies; ++i) {
    Vec pos = RandVec(map.size);
    // Keep trying until find valid position
    while (IsBlocked(pos, NULL) || Eq(pos, (Vec){2, 2})) {
      pos = RandVec(map.size);
    }
    Tank *tank = RegNew(regTank);
    tank->pos = pos;
    tank->dir = (Rand(2) == 0) ? eDirON : eDirPO;
    tank->color = TK_RED;
    tank->isPlayer = false;
    tank->shootCooldown = Rand(30);
    tank->moveCooldown = Rand(10);
    tank->hp = 1; // Enemies have 1 HP
  }

  // Initialize renderer.
  renderer.csPrev = (char *)malloc(sizeof(char) * map.size.x * map.size.y);
  renderer.colorsPrev = (Color *)malloc(sizeof(Color) * map.size.x * map.size.y);
  renderer.cs = (char *)malloc(sizeof(char) * map.size.x * map.size.y);
  renderer.colors = (Color *)malloc(sizeof(Color) * map.size.x * map.size.y);

  for (int i = 0; i < map.size.x * map.size.y; ++i) {
    renderer.csPrev[i] = renderer.cs[i] = ' ';
    renderer.colorsPrev[i] = renderer.colors[i] = TK_NORMAL;
  }

  // Render scene.
  for (int y = 0; y < map.size.y; ++y)
    for (int x = 0; x < map.size.x; ++x) {
      Vec pos = {x, y};
      RdrPutChar(pos, map.flags[Idx(pos)], TK_AUTO_COLOR);
    }
  RdrRender();
  RdrFlush();
}

//
//
//

/// \brief Read input from the player.
///
/// \note This function should be called in the loop of `GameLifecycle` before `GameUpdate`.
void GameInput(void) {
  game.keyHit = kbhit_t() ? (char)getch_t() : '\0';
}

//
//
//

/// \brief Shoot a bullet from a tank.
void TankShoot(Tank *tank) {
  if (tank->shootCooldown > 0) return;

  Bullet *bullet = RegNew(regBullet);
  bullet->pos = tank->pos;
  bullet->dir = tank->dir;
  bullet->color = tank->color;
  bullet->isPlayer = tank->isPlayer;
  tank->shootCooldown = 15; // 15 frame cooldown
}

/// \brief Move a tank in its current direction.
void MoveTank(Tank *tank) {
  if (tank->moveCooldown > 0) {
    tank->moveCooldown--;
    return;
  }

  Vec dirVec = GetDirVec(tank->dir);
  Vec newPos = Add(tank->pos, dirVec);

  // Check if can move (check 3x3 area)
  bool canMove = true;
  for (int dy = -1; dy <= 1 && canMove; ++dy) {
    for (int dx = -1; dx <= 1 && canMove; ++dx) {
      Vec testPos = Add(newPos, (Vec){dx, dy});
      if (IsBlocked(testPos, tank))
        canMove = false;
    }
  }

  if (canMove) {
    tank->pos = newPos;
  }
  tank->moveCooldown = 10; // 10 frame cooldown for AI
}

/// \brief Update all bullets (movement and collision).
void UpdateBullets(void) {
  // Use a list to store bullets to be removed
  int bulletCount = RegSize(regBullet);
  Bullet **toRemove = (Bullet **)malloc(sizeof(Bullet *) * bulletCount);
  int removeCount = 0;

  for (RegIterator it = RegBegin(regBullet); it != RegEnd(regBullet); it = RegNext(it)) {
    Bullet *bullet = RegEntry(regBullet, it);
    Vec dirVec = GetDirVec(bullet->dir);
    Vec newPos = Add(bullet->pos, dirVec);

    // Check boundary
    if (newPos.x < 0 || newPos.x >= map.size.x ||
        newPos.y < 0 || newPos.y >= map.size.y) {
      toRemove[removeCount++] = bullet;
      continue;
    }

    // Check map flags
    Flag flag = map.flags[Idx(newPos)];
    if (flag == eFlagSolid) {
      // Cannot destroy solid
      toRemove[removeCount++] = bullet;
      continue;
    } else if (flag == eFlagWall) {
      // Destroy wall
      map.flags[Idx(newPos)] = eFlagNone;
      toRemove[removeCount++] = bullet;
      continue;
    }

    // Check tank collision
    bool hitTank = false;
    for (RegIterator tit = RegBegin(regTank); tit != RegEnd(regTank); tit = RegNext(tit)) {
      Tank *tank = RegEntry(regTank, tit);
      // Check all 3x3 cells
      for (int dy = -1; dy <= 1 && !hitTank; ++dy) {
        for (int dx = -1; dx <= 1 && !hitTank; ++dx) {
          Vec tankPos = Add(tank->pos, (Vec){dx, dy});
          if (Eq(newPos, tankPos)) {
            // Player bullet hits enemy, or enemy bullet hits player
            if (bullet->isPlayer != tank->isPlayer) {
              // Decrease HP (original feature: HP system)
              tank->hp--;
              if (tank->hp <= 0) {
                RegDelete(tank);
              }
              hitTank = true;
              toRemove[removeCount++] = bullet;
            }
            break;
          }
        }
      }
    }

    if (!hitTank) {
      bullet->pos = newPos;
    }
  }

  // Remove bullets
  for (int i = 0; i < removeCount; ++i) {
    RegDelete(toRemove[i]);
  }
  free(toRemove);
}

/// \brief Update AI tanks (random movement and shooting).
void UpdateAI(void) {
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *tank = RegEntry(regTank, it);
    if (tank->isPlayer) continue;

    // Update cooldown
    if (tank->shootCooldown > 0) tank->shootCooldown--;
    if (tank->moveCooldown > 0) tank->moveCooldown--;

    // Randomly change direction (10% chance)
    if (Rand(100) < 10) {
      Dir dirs[] = {eDirOP, eDirON, eDirNO, eDirPO};
      tank->dir = dirs[Rand(4)];
    }

    // Try to move
    MoveTank(tank);

    // Randomly shoot (5% chance)
    if (Rand(100) < 5) {
      TankShoot(tank);
    }
  }
}

/// \brief Perform all tasks required for a frame update, including:
/// 1. Game logics of `Tank`s, `Bullet`s, etc.
/// 2. Rerendering all objects in the scene.
///
/// \note This function should be called in the loop of `GameLifecycle` after `GameInput`.
void GameUpdate(void) {
  RdrClear();

  // Handle player input
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *tank = RegEntry(regTank, it);
    if (!tank->isPlayer) continue;

    // Update cooldown
    if (tank->shootCooldown > 0) tank->shootCooldown--;
    if (tank->moveCooldown > 0) tank->moveCooldown--;

    // Change direction
    if (game.keyHit == keyUp) {
      tank->dir = eDirOP;
    } else if (game.keyHit == keyDown) {
      tank->dir = eDirON;
    } else if (game.keyHit == keyLeft) {
      tank->dir = eDirNO;
    } else if (game.keyHit == keyRight) {
      tank->dir = eDirPO;
    }

    // Move on key press (after direction change)
    if (tank->moveCooldown == 0 && 
        (game.keyHit == keyUp || game.keyHit == keyDown || 
         game.keyHit == keyLeft || game.keyHit == keyRight)) {
      MoveTank(tank);
    }

    // Shoot
    if (game.keyHit == keyShoot) {
      TankShoot(tank);
    }
  }

  // Update AI
  UpdateAI();

  // Update bullets
  UpdateBullets();

  // Check win/lose condition (original feature)
  CheckGameState();

  RdrRender();
  RdrFlush();
}

//
//
//

/// \brief Terminate the game and free all the resources.
///
/// \note This function should be called at the very end of `GameLifecycle`.
void GameTerminate(void) {
  while (RegSize(regTank) > 0)
    RegDelete(RegEntry(regTank, RegBegin(regTank)));

  while (RegSize(regBullet) > 0)
    RegDelete(RegEntry(regBullet, RegBegin(regBullet)));

  free(map.flags);

  free(renderer.csPrev);
  free(renderer.colorsPrev);
  free(renderer.cs);
  free(renderer.colors);

  TermClearScreen();
}

//
//
//

/// \brief Lifecycle of the game, defined by calling the 4 important functions:
/// `GameInit`, `GameInput`, `GameUpdate`, and `GameTerminate`.
///
/// \note This function should be called by `main`.
void GameLifecycle(void) {
  GameInit();

  double frameTime = (double)1000 / (double)config.fps;
  clock_t frameBegin = clock();

  while (true) {
    GameInput();
    if (game.keyHit == keyESC)
      break;

    GameUpdate();

    // Check for game over (original feature)
    if (gameOver) {
      // Show game over message
      RdrClear();
      for (int y = 0; y < map.size.y; ++y)
        for (int x = 0; x < map.size.x; ++x) {
          Vec pos = {x, y};
          RdrPutChar(pos, map.flags[Idx(pos)], TK_AUTO_COLOR);
        }
      
      // Render tanks and bullets one more time
      for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
        Tank *tank = RegEntry(regTank, it);
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx) {
            Vec pos = Add(tank->pos, (Vec){dx, dy});
            RdrPutChar(pos, '#', tank->color);
          }
      }
      
      // Display win/lose message at center
      Vec msgPos = {map.size.x / 2 - 5, map.size.y / 2};
      if (playerWon) {
        RdrPutChar(msgPos, 'Y', TK_YELLOW);
        RdrPutChar((Vec){msgPos.x + 1, msgPos.y}, 'O', TK_YELLOW);
        RdrPutChar((Vec){msgPos.x + 2, msgPos.y}, 'U', TK_YELLOW);
        RdrPutChar((Vec){msgPos.x + 3, msgPos.y}, ' ', TK_YELLOW);
        RdrPutChar((Vec){msgPos.x + 4, msgPos.y}, 'W', TK_YELLOW);
        RdrPutChar((Vec){msgPos.x + 5, msgPos.y}, 'I', TK_YELLOW);
        RdrPutChar((Vec){msgPos.x + 6, msgPos.y}, 'N', TK_YELLOW);
        RdrPutChar((Vec){msgPos.x + 7, msgPos.y}, '!', TK_YELLOW);
      } else {
        RdrPutChar(msgPos, 'G', TK_RED);
        RdrPutChar((Vec){msgPos.x + 1, msgPos.y}, 'A', TK_RED);
        RdrPutChar((Vec){msgPos.x + 2, msgPos.y}, 'M', TK_RED);
        RdrPutChar((Vec){msgPos.x + 3, msgPos.y}, 'E', TK_RED);
        RdrPutChar((Vec){msgPos.x + 4, msgPos.y}, ' ', TK_RED);
        RdrPutChar((Vec){msgPos.x + 5, msgPos.y}, 'O', TK_RED);
        RdrPutChar((Vec){msgPos.x + 6, msgPos.y}, 'V', TK_RED);
        RdrPutChar((Vec){msgPos.x + 7, msgPos.y}, 'E', TK_RED);
        RdrPutChar((Vec){msgPos.x + 8, msgPos.y}, 'R', TK_RED);
      }
      RdrFlush();
      
      // Wait a bit then exit
      SleepMs(2000);
      break;
    }

    while (((double)(clock() - frameBegin) / CLOCKS_PER_SEC) * 1000.0 < frameTime - 0.5)
      Daze();
    frameBegin = clock();
  }

  GameTerminate();
}