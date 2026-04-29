#pragma once

/// \file
/// \brief This file introduces a renderer which
/// helps us visualize objects in the scene.

//
//
//
//
//
#include "Scene.h"

/// \brief Let the renderer automatically deduce the color.
#define TK_AUTO_COLOR ((const char *)(NULL))

/// \brief The error message for undefined behaviors.
#define TK_INVALID_COLOR "Invalid Color"

/// \example Consequently call these functions to render a frame.
/// ```c
/// RdrClear();
/// // Update objects in the scene here.
/// RdrRender();
/// RdrFlush();
/// ```
///
/// \details The renderer is implemented by
/// imitating the double buffering technique.
/// See https://en.wikipedia.org/wiki/Multiple_buffering for more details.
typedef struct {
  char *csPrev;      // Characters of the previous frame.
  Color *colorsPrev; // Character colors of the previous frame.
  char *cs;          // Characters of the current frame.
  Color *colors;     // Character colors of the current frame.
} Renderer;

// The renderer singleton.
static Renderer renderer;

//
//
//
/// \brief Render character `c` at position `pos` with color `color`.
///
/// \example ```c
/// // Explicitly specify the color.
/// RdrPutChar(pos, 'o', TK_RED);
/// // Let the renderer automatically deduce the color.
/// RdrPutChar(pos, eFlagWall, TK_AUTO_COLOR);
/// ```
void RdrPutChar(Vec pos, char c, Color color) {
  renderer.cs[Idx(pos)] = c;

  if (color == TK_AUTO_COLOR) {
    Flag flag = (Flag)c;
    color = flag == eFlagNone    ? TK_NORMAL
            : flag == eFlagSolid ? TK_BLUE
            : flag == eFlagWall  ? TK_WHITE

            : flag == eFlagTank ? TK_INVALID_COLOR
                                : TK_INVALID_COLOR;
  }
  renderer.colors[Idx(pos)] = color;
}

//
//
//
/// \brief Clear all the objects in the scene from the frame.
void RdrClear(void) {
  // Clear tanks.
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *tank = RegEntry(regTank, it);
    Vec pos = tank->pos;

    for (int y = -1; y <= 1; ++y)
      for (int x = -1; x <= 1; ++x)
        RdrPutChar(Add(pos, (Vec){x, y}), map.flags[Idx(pos)], TK_AUTO_COLOR);
  }

  // Clear bullets.
  for (RegIterator it = RegBegin(regBullet); it != RegEnd(regBullet); it = RegNext(it)) {
    Bullet *bullet = RegEntry(regBullet, it);
    Vec pos = bullet->pos;

    RdrPutChar(pos, map.flags[Idx(pos)], TK_AUTO_COLOR);
  }
}

/// \brief Render all the objects in the scene to the frame.
void RdrRender(void) {
  // Render tanks.
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *tank = RegEntry(regTank, it);
    Vec pos = tank->pos;
    Color color = tank->color;
    Dir dir = tank->dir;

    // Tank shape: 3x3 grid, varies by direction
    // Center is tank body, edges are tracks
    char shape[3][3];
    for (int y = 0; y < 3; ++y)
      for (int x = 0; x < 3; ++x)
        shape[y][x] = (x == 1 && y == 1) ? '#' : '=';

    // Modify shape based on direction
    if (dir == eDirOP) { // Up
      shape[0][1] = '^'; // Turret points up
      shape[1][1] = '#'; // Body
      shape[2][0] = '='; // Tracks
      shape[2][1] = '=';
      shape[2][2] = '=';
    } else if (dir == eDirON) { // Down
      shape[2][1] = 'v'; // Turret points down
      shape[1][1] = '#';
      shape[0][0] = '=';
      shape[0][1] = '=';
      shape[0][2] = '=';
    } else if (dir == eDirNO) { // Left
      shape[1][0] = '<'; // Turret points left
      shape[1][1] = '#';
      shape[0][2] = '=';
      shape[1][2] = '=';
      shape[2][2] = '=';
    } else if (dir == eDirPO) { // Right
      shape[1][2] = '>'; // Turret points right
      shape[1][1] = '#';
      shape[0][0] = '=';
      shape[1][0] = '=';
      shape[2][0] = '=';
    } else if (dir == eDirOO) { // Center (default)
      shape[1][1] = '#';
    }

    // Render the 3x3 tank
    for (int y = -1; y <= 1; ++y) {
      for (int x = -1; x <= 1; ++x) {
        Vec cellPos = Add(pos, (Vec){x, y});
        int shapeY = y + 1;
        int shapeX = x + 1;
        RdrPutChar(cellPos, shape[shapeY][shapeX], color);
      }
    }
  }

  // Render bullets.
  for (RegIterator it = RegBegin(regBullet); it != RegEnd(regBullet); it = RegNext(it)) {
    Bullet *bullet = RegEntry(regBullet, it);
    Vec pos = bullet->pos;
    Color color = bullet->color;

    RdrPutChar(pos, '*', color);
  }
}

/// \brief Flush the previously rendered frame to screen to
/// make it truly visible.
void RdrFlush(void) {
  char *csPrev = renderer.csPrev;
  Color *colorsPrev = renderer.colorsPrev;
  const char *cs = renderer.cs;
  const Color *colors = renderer.colors;

  for (int y = 0; y < map.size.y; ++y)
    for (int x = 0; x < map.size.x; ++x) {
      Vec pos = {x, y};
      int i = Idx(pos);

      if (cs[i] != csPrev[i] || colors[i] != colorsPrev[i]) {
        MoveCursor(pos);
        printf(TK_TEXT("%c", TK_RUNTIME_COLOR), colors[i], cs[i]);

        csPrev[i] = cs[i];
        colorsPrev[i] = colors[i];
      }
    }
}
