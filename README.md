[Tutorial 1](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/tutorial-1-basic-io) 

# Homework: Tetris

## Initial Setup

### Project overview

Complete a Tetris game on the RDC Controller. The starter already handles piece generation, gravity, collision checking, locking, row clearing, scoring, and rendering. Even without button controls, pieces will appear, fall, and stack automatically. Each shuffled group of seven pieces contains every piece type exactly once.

### Controller buttons

The game uses six buttons (Button2 to Button7). Button1 and Button8 are not used.

You must implement all six controls below. You are free to assign any control to any button — there is no fixed mapping. (The demo videos show one example layout; your layout may differ.)

1. Move the active piece one cell left.
2. Move the active piece one cell right.
3. Rotate the active piece clockwise.
4. Rotate the active piece anticlockwise.
5. Soft drop: move down one cell (short press) / hard drop: drop to the bottom and lock (long press).
6. Pause/resume (short press), replace the active piece (long press), restart after Game Over (long press).

### Coordinate system

The logical game board uses whole-cell coordinates. `(0, 0)` is the **top-left cell of the Tetris board**:

```text
                 x increases →
              0  1  2  ...  9
            +------------------+
 y = 0  --> | (0,0)            |  top
 y = 1  --> |                  |
     ...    |                  |
 y = 19 --> |                  |  bottom
            +------------------+
                 y increases downward
```

- Moving right increases `x`; moving left decreases `x`.
- Moving down increases `y`; there are no half-cell positions.
- Access the board using `board[y][x]`. Valid coordinates are `x = 0..9` and `y = 0..19`.

The HUD occupies text row zero of the TFT, but it is outside the logical Tetris board. The board is drawn lower on the screen, so a piece at logical `y = 0` does not overlap the title or score.

### How to start

Please download the skeleton code for Tutorial 1 Homework here:

[Link](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Homework1-skeleton)

Set up the project the same way as you were taught in Tutorial 1! [Review the slides again](https://canva.link/oon1xmuchxtwbk8) for the steps if you can't remember.


## Task 0: Display Housekeeping

```c
void Tetris_DrawHUD(const TetrisGame *game)
{
    if (game->state == TETRIS_GAME_OVER) {
        // TODO: display Game Over.
    } else if (game->state == TETRIS_PAUSED) {
        // TODO: display Paused.
    } else {
        // TODO: display the title and score.
    }
}

void Tetris_DrawCell(uint32_t px, uint32_t py, uint16_t color)
{
    // TODO: draw the cell background or border.
    // TODO: draw the coloured cell.
}
```

You are required to do the following:

1. Show the title and score during play, **Paused** while paused, and **Game Over** when the game ends.
2. Draw a **7 × 7 pixel** background for each cell, then draw a **5 × 5 pixel** coloured block in the centre. This leaves a 1-pixel border on every side. Moving pieces must leave no trail.

Marking Scheme:

* The title, score, Paused, and Game Over status are displayed correctly and remain within text row zero @1
* Each cell has a 7 × 7 pixel background and a centred 5 × 5 pixel coloured block, with no trail after movement @1

### Demo video

<video src="https://github.com/user-attachments/assets/3a3b01f4-0d80-4e12-8df3-b3daf3770cde" muted autoplay loop controls width="100%">
</video>

## Task 1A: Move the Piece

```c
void Tetris_Move(TetrisGame *game)
{
    /** TODO: read the buttons and update previous levels before the guard. */
    if (!game->has_active) {
        /** HINTS: If we return here, the button state will be correctly updated without actually moving anything. */
        return;
    }
    /** TODO: detect press edges and try the requested move. */
}
```

1. Read the two buttons you assigned to moving left and right, and move only once for each new press.

   <details>
   <summary>Hints</summary>

   > [!TIP]
   > Review the example at the end of [02-clock.md](./02-clock.md). Treat each new press as a rising edge of the logical pressed state. You need to detect every new press, not only the first press after startup. What value can you remember between function calls to compare the current and previous button states?

   </details>

2. Move only when the new position is valid. Opposite presses cancel each other.

   <details>
   <summary>Hints</summary>

   > [!TIP]
   > Copy `game->active` into a new `TetrisPiece`, change its position, and update the active piece only if the new position is valid.

   </details>

You might find this useful:

```c
bool Tetris_CanPlace(const TetrisGame *game, const TetrisPiece *piece);
```

Marking Scheme:

* The piece moves left and right only when the destination is valid @1
* Each new press moves the piece once @1

### Demo video

<video src="https://github.com/user-attachments/assets/6e3d3663-7d86-4d1e-bf0a-bd37044a9f7d" muted autoplay loop controls width="100%">
</video>

## Task 1B: Rotate the Piece

```c
void Tetris_Rotate(TetrisGame *game)
{
    /** TODO: read the buttons and update previous levels before the guard. */

    if (!game->has_active) {
        /** HINTS: If we return here, the button state will be correctly updated without actually moving anything. */
        return;
    }
    /** TODO: detect press edges and try the requested turn. */
}
```

You are required to do the following:

1. Rotate once for each new press of the two buttons you assigned to rotation. Opposite presses cancel each other.
2. Accept the rotation only when the piece still fits. Wall kicks are not required.

You might find this useful:

```c
bool Tetris_CanPlace(const TetrisGame *game, const TetrisPiece *piece);
```

Marking Scheme:

* The block can rotate clockwise and anticlockwise when the proposed orientation is valid @1
* Each press rotates only once, opposite presses cancel, and blocked rotations do not pass through walls or landed blocks @1

### Demo video

<video src="https://github.com/user-attachments/assets/ae0460d2-b0d6-4fa1-9a8f-54d8f6b53211" muted autoplay loop controls width="100%">
</video>


## Task 2: Drop the Piece

```c
void Tetris_Drop(TetrisGame *game)
{
    /** TODO: read your drop button and update its previous level before the guard. */
    if (!game->has_active) {
        return;
    }
    /** TODO: track the gesture and apply its downward action. */
}
```

You are required to do the following:

1. Distinguish a short press from a long press on the button you assigned to dropping.
2. A short press moves the active piece down by one valid cell, or locks it if it cannot move down.

   <details>
   <summary>Hints</summary>

   > [!TIP]
   > Handle a short press when the button is released before the long-press interval. In the logical pressed state used here, releasing the button produces a falling edge.

   </details>

3. A long press moves the active piece to its lowest valid position and locks it exactly once.

   <details>
   <summary>Hints</summary>

   > [!TIP]
   > A straightforward method is to move the piece down one cell at a time until the next position is invalid, then keep the last valid position. Call `Tetris_FinishPiece(game)` to lock the piece stored inside `game->active`.

   </details>

4. Releasing after a long press must not cause a short drop, and continuing to hold the drop button must not affect the next piece.

You might find these useful:

```c
bool Tetris_CanPlace(const TetrisGame *game, const TetrisPiece *piece);
void Tetris_FinishPiece(TetrisGame *game);
```

Marking Scheme:

* A short press moves down once or locks when blocked @1
* A long press drops to the lowest valid position and locks once @1
* Holding or releasing after a long press does not affect the next piece @1

### Demo video

<video src="https://github.com/user-attachments/assets/2ce04627-8b56-41a7-a35f-3c8fd97ac7a5" muted autoplay loop controls width="100%">
</video>

## Task 3: Game FSM and Pause/Replace/Restart

```c
void Tetris_UpdateGame(TetrisGame *game, uint32_t now)
{
    /** TODO: add the pause, replace and restart behaviour. */

    switch (game->state) {
    case TETRIS_SPAWN:
        game->state = Tetris_Spawn(game) ? TETRIS_FALLING : TETRIS_GAME_OVER;
        game->last_fall_ms = now;
        break;

    case TETRIS_FALLING:
        if (!game->has_active) {
            game->state = TETRIS_SPAWN;
        }
        break;

    case TETRIS_PAUSED:
    case TETRIS_GAME_OVER:
        break;
    }
}
```

You are required to do the following:

1. A short press on the button you assigned to pause pauses or resumes the game. Nothing moves while paused.
2. A long press on that button replaces the active piece once. Another replacement is allowed after the new piece locks.
3. A long press on that button restarts the game after Game Over.

You might find these useful:

```c
void Tetris_Reset(TetrisGame *game, uint32_t now);
bool Tetris_Spawn(TetrisGame *game);
```

Marking Scheme:

* Short press pauses or resumes the game @1
* Long press replaces the active piece once @1
* Replacement is available again only after the piece locks @1
* Long press restarts the game after Game Over @1

### Demo video

<video src="https://github.com/user-attachments/assets/093a5148-9529-4029-84bb-c7d138f3ab36" muted autoplay loop controls width="100%">
</video>


