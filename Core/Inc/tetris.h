#ifndef TETRIS_H
#define TETRIS_H

#include <stdbool.h>
#include <stdint.h>

#define TETRIS_ROWS          20    // Logical board height
#define TETRIS_COLS          10    // Logical board width
#define TETRIS_CELL_SIZE      7    // Cell width and height in TFT pixels
#define TETRIS_FALL_MS       700U  // Interval between gravity steps
#define TETRIS_LONG_PRESS_MS 500U  // Minimum time for a long press
#define TETRIS_RENDER_MS      50U  // Minimum interval between TFT updates

typedef enum {
    PIECE_I, PIECE_O, PIECE_T, PIECE_S,
    PIECE_Z, PIECE_J, PIECE_L,
    PIECE_COUNT  // Number of available piece types
} TetrisPieceType;

typedef enum {
    TETRIS_SPAWN,     // Generate the next active piece
    TETRIS_FALLING,   // Accept controls and apply gravity
    TETRIS_PAUSED,    // Keep the board visible but stop gameplay
    TETRIS_GAME_OVER  // Wait for Button4 to reset the game
} TetrisState;


typedef struct {
    uint8_t type;      // One of the seven TetrisPieceType values
    uint8_t rotation;  // Orientation from 0 to 3
    int16_t x;         // Horizontal origin used by the shape offsets
    int16_t y;         // Vertical origin used by the shape offsets
} TetrisPiece;

typedef struct {
    // Landed cells only: 0 is empty and 1..7 select a piece colour.
    uint8_t board[TETRIS_ROWS][TETRIS_COLS];

    // The falling piece stays separate from board until it locks.
    TetrisPiece active;
    bool has_active;       // Whether active currently contains a falling piece
    bool skip_available;   // Whether Button4 may replace the current piece
    TetrisState state;     // Current state of the game FSM
    uint32_t last_fall_ms; // Reference tick used to schedule the next gravity step
    uint32_t lines;        // Total number of cleared rows
    uint32_t score;        // Score awarded by the provided baseline

    // Internal state used by the provided seven-piece bag generator.
    uint32_t random_state;
    uint8_t piece_bag[PIECE_COUNT];
    uint8_t bag_index;
} TetrisGame;

/**
 * @brief Run one application update after HAL, GPIO and SPI initialization.
 * @param[in,out] game Game instance owned by main.c.
 * @note The starter shows one piece so movement and rotation can be tested first.
 */
void Tetris_Update(TetrisGame *game);

/**
 * @brief Initialize the game data, display cache and board graphics.
 * @param[out] game Game data to initialize.
 * @note main.c must initialize the TFT first. This function does not configure GPIO.
 */
void Tetris_ImplInit(TetrisGame *game);

/**
 * @brief Clear the board, counters and gravity timer, ready for a new game.
 * @param[out] game Game to reset.
 * @param[in] now Current tick in milliseconds.
 * @note Restores the one-skip allowance. Button history remains function-local.
 */
void Tetris_Reset(TetrisGame *game, uint32_t now);

/**
 * @brief Test a position and rotation against bounds and fixed cells.
 * @param[in] game Fixed board to inspect.
 * @param[in] piece Candidate to inspect without changing it.
 * @return true if all four cells fit.
 */
bool Tetris_CanPlace(const TetrisGame *game, const TetrisPiece *piece);

/**
 * @brief Take the next shape from a shuffled seven-piece bag and spawn it.
 * @param[in,out] game Updates active, has_active, bag and generator state.
 * @return true if placed; false with no active piece if the spawn is blocked.
 */
bool Tetris_Spawn(TetrisGame *game);

/**
 * @brief Copy the active piece's four cells into the fixed board.
 * @param[in,out] game Board and active flag to update.
 * @return true if locked; false if no valid active piece exists.
 * @note Does not decide when to land or alter input history.
 */
bool Tetris_Lock(TetrisGame *game);

/**
 * @brief Finish a landed piece, clear completed rows and update the score.
 * @param[in,out] game Game to advance to SPAWN, or GAME_OVER on invalid data.
 * @note Awards 100 points per cleared row. Does not cancel button gestures.
 */
void Tetris_FinishPiece(TetrisGame *game);

/**
 * @brief Move the active piece down once every TETRIS_FALL_MS.
 * @param[in,out] game Active piece and gravity timer to update.
 * @param[in] now Current tick in milliseconds.
 * @note Lock a blocked piece with Tetris_FinishPiece. Do not use HAL_Delay.
 */
void Tetris_Gravity(TetrisGame *game, uint32_t now);

/**
 * @brief Task 3: handle Button4 and the SPAWN/FALLING/PAUSED/GAME_OVER FSM.
 * @param[in,out] game State, active piece and timers to update.
 * @param[in] now Current tick in milliseconds.
 * @note Short Button4 toggles pause; TETRIS_LONG_PRESS_MS triggers skip/reset.
 */
void Tetris_UpdateGame(TetrisGame *game, uint32_t now);

/**
 * @brief Compose the fixed board and active piece; call Task 0 to draw changes.
 * @param[in] game Game to display without changing it.
 * @param[in] now Current tick in milliseconds for redraw throttling.
 */
void Tetris_Render(const TetrisGame *game, uint32_t now);

/**
 * @brief Task 0: draw the title, score, Paused and Game Over status in row zero.
 * @param[in] game Current score and game state.
 * @note The renderer calls tft_update after this hook. Leave the board area free.
 */
void Tetris_DrawHUD(const TetrisGame *game);

/**
 * @brief Task 0: draw one whole board cell.
 * @param[in] px Left pixel coordinate.
 * @param[in] py Top pixel coordinate.
 * @param[in] color Requested RGB565 cell colour, including empty-cell colour.
 * @note Paint the full TETRIS_CELL_SIZE square to erase old contents reliably.
 */
void Tetris_DrawCell(uint32_t px, uint32_t py, uint16_t color);

/**
 * @brief Task 1A: read Button6/7, detect presses and try moving left/right.
 * @param[in,out] game Active piece to update only if the destination is valid.
 * @note Keep previous readings in local static variables across calls.
 */
void Tetris_Move(TetrisGame *game);

/**
 * @brief Task 1B: read Button2/3, detect presses and try rotating the piece.
 * @param[in,out] game Active piece to rotate only when all four cells fit.
 */
void Tetris_Rotate(TetrisGame *game);

/**
 * @brief Task 2: read Button5 and handle short/long drops and gesture lifetime.
 * @param[in,out] game Piece to drop; update the gravity timer after a short step.
 * @note Read HAL_GetTick locally and keep previous input/timing in static variables.
 */
void Tetris_Drop(TetrisGame *game);

/**
 * @brief Remove full rows and compact survivors downwards.
 * @param[in,out] game Modify board only; scoring is handled by FinishPiece.
 * @return Number of rows removed.
 * @note Surviving rows move as whole rows; individual cells do not fall separately.
 */
int Tetris_ClearFullRows(TetrisGame *game);

#endif
