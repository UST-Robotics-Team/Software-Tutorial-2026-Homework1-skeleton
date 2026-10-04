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
    char buf[17];
    if (game->state == TETRIS_GAME_OVER) {
        // TODO: display Game Over.
        tft_print_string(0, 0, RGB888TO565(0xFF0000), RGB888TO565(0x000000), "   GAME OVER!   ");
    } else if (game->state == TETRIS_PAUSED) {
        // TODO: display Paused.
        tft_print_string(0, 0, RGB888TO565(0xFFFF00), RGB888TO565(0x000000), "    PAUSED      ");
    } else {
        // TODO: display the title and score.
        snprintf(buf, sizeof(buf), "TETRIS  S:%-6lu", (unsigned long)game->score);
        tft_print_string(0, 0, RGB888TO565(0xFFFFFF), RGB888TO565(0x000000), buf);
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
    tft_print_rectangle(color, px, py, TETRIS_CELL_SIZE, TETRIS_CELL_SIZE);
    
  
    
    // TODO: draw the coloured cell.
     
    if (color != RGB888TO565(0x101820)) {
        tft_print_rectangle(RGB888TO565(0x000000), px, py, TETRIS_CELL_SIZE, 1);
        tft_print_rectangle(RGB888TO565(0x000000), px, py, 1, TETRIS_CELL_SIZE);
    }

}

/**
 * @brief TASK 1A: read your chosen left/right buttons and move once per new press.
 * @param[in,out] game Active piece to update if its destination is valid.
 * @note GPIO is active-low. Remember previous readings across calls with static
 * variables. Opposite press events cancel. Use Tetris_CanPlace to reject walls
 * and fixed blocks; keep board unchanged. Update history even outside gameplay.
 */
void Tetris_Move(TetrisGame *game)
{
    /** TODO: read the buttons and update previous levels before the guard. */

    static uint8_t prev_left = 1;
    static uint8_t prev_right = 1;

    uint8_t curr_left = HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin);
    uint8_t curr_right = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);

    bool left_press = (prev_left == 1 && curr_left == 0);
    bool right_press = (prev_right == 1 && curr_right == 0);

    prev_left = curr_left;
    prev_right = curr_right;
    if (!game->has_active || game->state != TETRIS_FALLING) {
        /** HINTS: If we return here, the button state will be correctly updated without actually moving anything. */
        return;
    }
    /** TODO: detect press edges and try the requested move. */
    if (left_press && !right_press) {
        TetrisPiece cand = game->active;
        cand.x--;
        if (Tetris_CanPlace(game, &cand)) {
            game->active = cand;
        }
    } else if (right_press && !left_press) {
        TetrisPiece cand = game->active;
        cand.x++;
        if (Tetris_CanPlace(game, &cand)) {
            game->active = cand;
        }
    }
}

/**
 * @brief TASK 1B: read your chosen rotate buttons and rotate once per new press.
 * @param[in,out] game Active piece to rotate only if the new orientation fits.
 * @note Assign one button to clockwise and one to anticlockwise. Orientations are 0..3.
 * Opposite press events cancel. Wall kicks are not required. Keep the previous
 * levels across calls and do not reset them while a button is still held.
 */
void Tetris_Rotate(TetrisGame *game)
{
    /** TODO: read the buttons and update previous levels before the guard. */
    static uint8_t prev_cw = 1;
    static uint8_t prev_ccw = 1;

    uint8_t curr_cw = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
    uint8_t curr_ccw = HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin);

    bool cw_press = (prev_cw == 1 && curr_cw == 0);
    bool ccw_press = (prev_ccw == 1 && curr_ccw == 0);

    prev_cw = curr_cw;
    prev_ccw = curr_ccw;
    
    if (!game->has_active || game->state != TETRIS_FALLING) {
        /** HINTS: If we return here, the button state will be correctly updated without actually moving anything. */
        return;
    }
    /** TODO: detect press edges and try the requested turn. */
    if (cw_press && !ccw_press) {
        TetrisPiece cand = game->active;
        cand.rotation = (cand.rotation + 1) % 4;
        if (Tetris_CanPlace(game, &cand)) {
            game->active = cand;
        }
    } else if (ccw_press && !cw_press) {
        TetrisPiece cand = game->active;
        cand.rotation = (cand.rotation + 3) % 4;
        if (Tetris_CanPlace(game, &cand)) {
            game->active = cand;
        }
    }
}

/**
 * @brief TASK 2: read your drop button locally and implement short/long drops.
 * @param[in,out] game Active piece to move; FinishPiece handles board bookkeeping.
 * @note Release before TETRIS_LONG_PRESS_MS to step once; hold until the interval
 * to drop to the lowest position and lock once. Read HAL_GetTick and keep your
 * timing and previous input in static variables. Reset last_fall_ms after a manual step.
 * The old gesture must not affect a new piece, including after gravity lands it
 * or a restart. Choose your own state logic.
 */
void Tetris_Drop(TetrisGame *game)
{
    /** TODO: read your drop button and update its previous level before the guard. */
    static uint8_t prev_drop = 1;
    static uint32_t press_start_ms = 0;
    static bool long_handled = false;

    uint8_t curr_drop = HAL_GPIO_ReadPin(WK_UP_GPIO_Port, WK_UP_Pin);
    uint32_t now = HAL_GetTick();

    if (prev_drop == 1 && curr_drop == 0) {
        press_start_ms = now;
        long_handled = false;
    }

    if (!game->has_active || game->state != TETRIS_FALLING) {
      prev_drop = curr_drop;
        return;
    }
    /** TODO: track the gesture and apply its downward action. */
    if (curr_drop == 0 && !long_handled && (now - press_start_ms >= TETRIS_LONG_PRESS_MS)) {
        long_handled = true;
        while (1) {
            TetrisPiece cand = game->active;
            cand.y++;
            if (Tetris_CanPlace(game, &cand)) {
                game->active = cand;
            } else {
                break;
            }
        }
        Tetris_FinishPiece(game);
    }

    else if (prev_drop == 0 && curr_drop == 1) {
        if (!long_handled && (now - press_start_ms < TETRIS_LONG_PRESS_MS)) {
            TetrisPiece cand = game->active;
            cand.y++;
            if (Tetris_CanPlace(game, &cand)) {
                game->active = cand;
                game->last_fall_ms = now;
            } else {
                Tetris_FinishPiece(game);
            }
        }
    }

    prev_drop = curr_drop;
}

/**
 * @brief TASK 3: handle the pause/replace/restart button and the complete game-state FSM.
 * @param[in,out] game State, active piece and skip allowance to update.
 * @param[in] now Current software tick in milliseconds.
 * @note A short press pauses/resumes. TETRIS_LONG_PRESS_MS triggers one skip
 * during play or resets Game Over. A replacement must lock before another skip.
 */
void Tetris_UpdateGame(TetrisGame *game, uint32_t now)
{
    /** TODO: add the pause, replace and restart behaviour. */
    static uint8_t prev_btn = 1;
    static uint32_t press_start = 0;
    static bool long_done = false;

    uint8_t curr_btn = HAL_GPIO_ReadPin(WK_UP_GPIO_Port, WK_UP_Pin);

    if (prev_btn == 1 && curr_btn == 0) {
        press_start = now;
        long_done = false;
    }

    if (curr_btn == 0 && !long_done && (now - press_start >= TETRIS_LONG_PRESS_MS)) {
        long_done = true;
        if (game->state == TETRIS_FALLING && game->skip_available) {
            game->skip_available = false;
            game->has_active = false;
            game->state = TETRIS_SPAWN;
        } else if (game->state == TETRIS_GAME_OVER) {
            Tetris_Reset(game, now);
        }
    }

    else if (prev_btn == 0 && curr_btn == 1) {
        if (!long_done && (now - press_start < TETRIS_LONG_PRESS_MS)) {
            if (game->state == TETRIS_FALLING) {
                game->state = TETRIS_PAUSED;
            } else if (game->state == TETRIS_PAUSED) {
                game->state = TETRIS_FALLING;
            }
        }
    }

    prev_btn = curr_btn;
    
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
