/**
 * @brief STUDENT STARTER: implement each task; additional helpers are welcome.
 * Gravity, row clearing and the basic spawn loop are provided.
 */
#include "tetris.h"
#include "main.h"
#include "lcd/lcd.h"

/**
 * @brief PROVIDED: initialize once, then run the student FSM and renderer.
 * @param[in,out] game Game instance to update.
 */
void Tetris_Update(TetrisGame *game)
{
    static bool initialized = false;
    if (!initialized) {
        Tetris_ImplInit(game);
        initialized = true;
    }
    uint32_t now = HAL_GetTick();
    Tetris_UpdateGame(game, now);
    Tetris_Rotate(game);
    Tetris_Move(game);
    Tetris_Drop(game);
    Tetris_Gravity(game, now);
    Tetris_Render(game, now);
}

/**
 * @brief TASK 0: add title, score, Paused and Game Over text to row zero.
 * @param[in] game Use score/state for the display; do not change game data.
 * @note The renderer calls tft_update for you. Keep text within 16 columns.
 */
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

/**
 * @brief TASK 0: complete the two drawing steps for one board cell.
 * @param[in] px Left pixel coordinate supplied by the renderer.
 * @param[in] py Top pixel coordinate supplied by the renderer.
 * @param[in] color Cell colour, including the colour used to erase empty cells.
 * @note Keep the whole square painted. You may add a border or an inset.
 */
void Tetris_DrawCell(uint32_t px, uint32_t py, uint16_t color)
{
    // TODO: draw the cell background or border.
    // TODO: draw the coloured cell.
}

/**
 * @brief TASK 1A: read Button6/7 and move once per new press.
 * @param[in,out] game Active piece to update if its destination is valid.
 * @note GPIO is active-low. Remember previous readings across calls with static
 * variables. Opposite press events cancel. Use Tetris_CanPlace to reject walls
 * and fixed blocks; keep board unchanged. Update history even outside gameplay.
 */
void Tetris_Move(TetrisGame *game)
{
    /** TODO: read the buttons and update previous levels before the guard. */
    if (!game->has_active) {
        /** HINTS: If we return here, the button state will be correctly updated without actually moving anything. */
        return;
    }
    /** TODO: detect press edges and try the requested move. */
}

/**
 * @brief TASK 1B: read Button2/3 and rotate once per new press.
 * @param[in,out] game Active piece to rotate only if the new orientation fits.
 * @note Button2 is anticlockwise; Button3 is clockwise. Orientations are 0..3.
 * Opposite press events cancel. Wall kicks are not required. Keep the previous
 * levels across calls and do not reset them while a button is still held.
 */
void Tetris_Rotate(TetrisGame *game)
{
    /** TODO: read the buttons and update previous levels before the guard. */
    
    if (!game->has_active) {
        /** HINTS: If we return here, the button state will be correctly updated without actually moving anything. */
        return;
    }
    /** TODO: detect press edges and try the requested turn. */
}

/**
 * @brief TASK 2: read Button5 locally and implement short/long drops.
 * @param[in,out] game Active piece to move; FinishPiece handles board bookkeeping.
 * @note Release before TETRIS_LONG_PRESS_MS to step once; hold until the interval
 * to drop to the lowest position and lock once. Read HAL_GetTick and keep your
 * timing and previous input in static variables. Reset last_fall_ms after a manual step.
 * The old gesture must not affect a new piece, including after gravity lands it
 * or a restart. Choose your own state logic.
 */
void Tetris_Drop(TetrisGame *game)
{
    /** TODO: read Button5 and update its previous level before the guard. */
    if (!game->has_active) {
        return;
    }
    /** TODO: track the gesture and apply its downward action. */
}

/**
 * @brief TASK 3: handle Button4 and the complete game-state FSM.
 * @param[in,out] game State, active piece and skip allowance to update.
 * @param[in] now Current software tick in milliseconds.
 * @note Short Button4 pauses/resumes. TETRIS_LONG_PRESS_MS triggers one skip
 * during play or resets Game Over. A replacement must lock before another skip.
 */
void Tetris_UpdateGame(TetrisGame *game, uint32_t now)
{
    /** TODO: add the Button4 pause, replace and restart behaviour. */ 
    
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
