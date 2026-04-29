# CS100-Tank Agent Instructions

This repository contains a C-based terminal game (Tank 1990 clone).

## Build & Run
- **Compile command**: `gcc Tank/Main.c -Wall -Wextra -Wpedantic -Wno-unused-variable`
- **Run**: Execute the resulting binary (e.g., `./a.out` or `.\a.exe`). The game is terminal-based.

## Architecture
- **Entrypoint**: `Tank/Main.c`. It contains `main()` and calls the game lifecycle.
- **Game Logic**: `Tank/Game.h` (GameInit, GameInput, GameUpdate, GameTerminate).
- **Rendering**: `Tank/Renderer.h` (handles clearing, drawing characters via `RdrPutChar`, and flushing to the terminal).
- **Data & Singletons**: `Tank/Scene.h`. Contains structures (`Tank`, `Bullet`, `Map`) and global instances (`regTank`, `regBullet`, `map`). The map uses a 1D array; always use the `Idx(Vec pos)` function to access tiles.
- **Container System**: `Tank/Registry.h` is a heavily macro-based custom container registry. **Always** follow its strict usage conventions (e.g., `TK_REG_AUTH` in structs, `TK_REG_DEF`, `RegInit`, `RegAdd`, `RegRemove`, and `RegIterator` loops). Do not try to bypass it or change its design.

## Conventions & Quirks
- **Singletons**: Global static variables are the intended way to access the current map and active entities.
- **Memory & Lifecycle**: Be extremely careful to free allocated memory upon `ESC` (handled in `GameTerminate`). Memory leaks are strictly penalized.
- **Formatting**: Use `.clang-format` to format all code.
- **Documentation**: Any custom function >= 20 lines **must** have a Doxygen-style `/// \brief ...` comment.
- **Randomization**: Use `Rand`, `Rand01`, and `RandVec` from `Base.h`. Never remove the `srand(time(NULL));` call in `Main.c`.