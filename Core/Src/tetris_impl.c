/**
 * @brief PROVIDED: hardware support, shapes, collision and rendering.
 */
#include "tetris.h"
#include "main.h"
#include "lcd/lcd.h"

#include <string.h>

#define BOARD_X   29
#define BOARD_Y   18
#define BACKGROUND_COLOR RGB888TO565(0x000000)
#define EMPTY_COLOR      RGB888TO565(0x101820)
#define BORDER_COLOR     RGB888TO565(0x52606D)

/**
 * @brief Private shape offset; students only need TetrisGame and TetrisPiece.
 */
typedef struct {
    int8_t x;
    int8_t y;
} TetrisCell;

/**
 * @brief [piece type][clockwise orientation][occupied cell].
 * I rotates in a 4x4 box; J/L/S/T/Z rotate in a 3x3 box.
 * O retains exactly the same occupied cells in every orientation.
 */
static const TetrisCell shapes[PIECE_COUNT][4][4] = {
    /**
     * @brief I
     */
    {
        {{0,1}, {1,1}, {2,1}, {3,1}},
        {{2,0}, {2,1}, {2,2}, {2,3}},
        {{0,2}, {1,2}, {2,2}, {3,2}},
        {{1,0}, {1,1}, {1,2}, {1,3}}
    },
    /**
     * @brief O
     */
    {
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}}
    },
    /**
     * @brief T
     */
    {
        {{1,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {1,2}},
        {{1,0}, {0,1}, {1,1}, {1,2}}
    },
    /**
     * @brief S
     */
    {
        {{1,0}, {2,0}, {0,1}, {1,1}},
        {{1,0}, {1,1}, {2,1}, {2,2}},
        {{1,1}, {2,1}, {0,2}, {1,2}},
        {{0,0}, {0,1}, {1,1}, {1,2}}
    },
    /**
     * @brief Z
     */
    {
        {{0,0}, {1,0}, {1,1}, {2,1}},
        {{2,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {1,2}, {2,2}},
        {{1,0}, {0,1}, {1,1}, {0,2}}
    },
    /**
     * @brief J
     */
    {
        {{0,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {2,2}},
        {{1,0}, {1,1}, {0,2}, {1,2}}
    },
    /**
     * @brief L
     */
    {
        {{2,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {1,2}, {2,2}},
        {{0,1}, {1,1}, {2,1}, {0,2}},
        {{0,0}, {1,0}, {1,1}, {1,2}}
    }
};

/**
 * @brief Explicit RGB values: do not rely on the library's CYAN/PURPLE names.
 */
static const uint16_t colors[PIECE_COUNT + 1] = {
    EMPTY_COLOR,
    /**
     * @brief I: cyan
     */
    RGB888TO565(0x00D9E8),
    /**
     * @brief O: yellow
     */
    RGB888TO565(0xFFE052),
    /**
     * @brief T: purple
     */
    RGB888TO565(0xB65CFF),
    /**
     * @brief S: green
     */
    RGB888TO565(0x42D66B),
    /**
     * @brief Z: red
     */
    RGB888TO565(0xFF5263),
    /**
     * @brief J: blue
     */
    RGB888TO565(0x4285FF),
    /**
     * @brief L: orange
     */
    RGB888TO565(0xFF9B42)
};

/**
 * @brief Keep two cell buffers (400 bytes total), not pixel framebuffers.
 */
static uint8_t previous_frame[TETRIS_ROWS][TETRIS_COLS];
static uint8_t frame[TETRIS_ROWS][TETRIS_COLS];
static uint32_t last_render_ms;

/**
 * @brief Advance the local pseudo-random sequence used to shuffle the bag.
 * @param[in,out] game Holds the nonzero generator state.
 * @return Next pseudo-random value.
 */
static uint32_t next_random(TetrisGame *game)
{
    uint32_t value = game->random_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    game->random_state = value;
    return value;
}

/**
 * @brief Refill and shuffle one copy of every piece type.
 * @param[in,out] game Bag, bag index and random generator state to update.
 */
static void refill_bag(TetrisGame *game)
{
    for (uint8_t i = 0; i < PIECE_COUNT; ++i) {
        game->piece_bag[i] = i;
    }
    for (uint8_t i = PIECE_COUNT - 1; i > 0; --i) {
        uint8_t j = (uint8_t)(next_random(game) % (i + 1U));
        uint8_t temporary = game->piece_bag[i];
        game->piece_bag[i] = game->piece_bag[j];
        game->piece_bag[j] = temporary;
    }
    game->bag_index = 0;
}

/**
 * @brief Check the candidate against board bounds and fixed cells.
 * @param[in] game Fixed board.
 * @param[in] piece Candidate to test without modifying it.
 * @return true when all four cells fit.
 */
bool Tetris_CanPlace(const TetrisGame *game, const TetrisPiece *piece)
{
    if (piece->type >= PIECE_COUNT || piece->rotation >= 4) {
        return false;
    }
    for (int i = 0; i < 4; ++i) {
        TetrisCell cell = shapes[piece->type][piece->rotation][i];
        int x = piece->x + cell.x;
        int y = piece->y + cell.y;
        /**
         * @brief Bounds must be checked before indexing board[y][x].
         */
        if (x < 0 || x >= TETRIS_COLS || y < 0 || y >= TETRIS_ROWS) {
            return false;
        }
        if (game->board[y][x] != 0) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Create the next piece from the shuffled bag near the top centre.
 * @param[in,out] game Piece, bag and generator state to update.
 * @return false if the starting position is blocked.
 */
bool Tetris_Spawn(TetrisGame *game)
{
    if (game->bag_index >= PIECE_COUNT) {
        refill_bag(game);
    }
    TetrisPiece piece = {
        .type = game->piece_bag[game->bag_index++],
        .rotation = 0,
        .x = (TETRIS_COLS - 4) / 2,
        .y = 0
    };
    game->has_active = false;
    if (!Tetris_CanPlace(game, &piece)) {
        return false;
    }
    game->active = piece;
    game->has_active = true;
    return true;
}

/**
 * @brief Store the active piece as fixed board cells.
 * @param[in,out] game Board and active-piece flag to update.
 * @return true on success; false if the active piece is missing or invalid.
 */
bool Tetris_Lock(TetrisGame *game)
{
    if (!game->has_active || !Tetris_CanPlace(game, &game->active)) {
        return false;
    }
    const TetrisPiece *piece = &game->active;
    for (int i = 0; i < 4; ++i) {
        TetrisCell cell = shapes[piece->type][piece->rotation][i];
        game->board[piece->y + cell.y][piece->x + cell.x] =
            (uint8_t)(piece->type + 1);
    }
    game->has_active = false;
    return true;
}

/**
 * @brief Reset game data without reading or modifying the buttons.
 * @param[out] game Empty game ready to spawn.
 * @param[in] now Current tick in milliseconds.
 */
void Tetris_Reset(TetrisGame *game, uint32_t now)
{
    memset(game, 0, sizeof(*game));
    game->skip_available = true;
    game->state = TETRIS_SPAWN;
    game->last_fall_ms = now;
    game->random_state = now ^ 0xA341316CU;
    if (game->random_state == 0) {
        game->random_state = 1;
    }
    game->bag_index = PIECE_COUNT;
}

/**
 * @brief Lock, clear completed rows and prepare the next spawn.
 * @param[in,out] game Game data; input/gesture history is never touched.
 */
void Tetris_FinishPiece(TetrisGame *game)
{
    if (!Tetris_Lock(game)) {
        game->has_active = false;
        game->state = TETRIS_GAME_OVER;
        return;
    }
    game->skip_available = true;
    int cleared = Tetris_ClearFullRows(game);
    if (cleared > 0 && cleared <= TETRIS_ROWS) {
        game->lines += (uint32_t)cleared;
        game->score += (uint32_t)cleared * 100U;
    }
    game->state = TETRIS_SPAWN;
}

/**
 * @brief Apply automatic gravity without blocking the main loop.
 * @param[in,out] game Active piece and gravity timer to update.
 * @param[in] now Current software tick in milliseconds.
 */
void Tetris_Gravity(TetrisGame *game, uint32_t now)
{
    if (game->state != TETRIS_FALLING || !game->has_active) {
        return;
    }
    if ((uint32_t)(now - game->last_fall_ms) < TETRIS_FALL_MS) {
        return;
    }

    game->last_fall_ms = now;
    TetrisPiece candidate = game->active;
    ++candidate.y;
    if (Tetris_CanPlace(game, &candidate)) {
        game->active = candidate;
    } else {
        Tetris_FinishPiece(game);
    }
}

/**
 * @brief Clear each completed row and shift every row above it down.
 * @param[in,out] game Fixed board to update.
 * @return Number of removed rows.
 */
int Tetris_ClearFullRows(TetrisGame *game)
{
    int cleared = 0;
    int row = TETRIS_ROWS - 1;

    while (row >= 0) {
        bool full = true;
        for (int col = 0; col < TETRIS_COLS; ++col) {
            if (game->board[row][col] == 0) {
                full = false;
                break;
            }
        }

        if (!full) {
            --row;
            continue;
        }

        for (int move_row = row; move_row > 0; --move_row) {
            for (int col = 0; col < TETRIS_COLS; ++col) {
                game->board[move_row][col] =
                    game->board[move_row - 1][col];
            }
        }
        for (int col = 0; col < TETRIS_COLS; ++col) {
            game->board[0][col] = 0;
        }
        ++cleared;
    }
    return cleared;
}

/**
 * @brief Initialize game data, display cache and board graphics.
 * @param[out] game Game to initialize.
 * @note main.c must call tft_init before this function runs.
 */
void Tetris_ImplInit(TetrisGame *game)
{
    memset(previous_frame, 0xFF, sizeof(previous_frame));
    uint32_t now = HAL_GetTick();
    Tetris_Reset(game, now);
    game->state = Tetris_Spawn(game) ? TETRIS_FALLING : TETRIS_GAME_OVER;
    last_render_ms = now - TETRIS_RENDER_MS;
    tft_print_rectangle(BORDER_COLOR, BOARD_X - 1, BOARD_Y - 1,
                        TETRIS_COLS * TETRIS_CELL_SIZE + 2,
                        TETRIS_ROWS * TETRIS_CELL_SIZE + 2);
}

/**
 * @brief Compose whole-cell graphics and delegate painting to Task 0.
 * @param[in] game Game data to display without changing it.
 * @param[in] now Current tick in milliseconds for redraw throttling.
 */
void Tetris_Render(const TetrisGame *game, uint32_t now)
{
    if ((uint32_t)(now - last_render_ms) < TETRIS_RENDER_MS) {
        return;
    }
    last_render_ms = now;
    Tetris_DrawHUD(game);
    tft_update(TETRIS_RENDER_MS);
    memcpy(frame, game->board, sizeof(frame));
    if (game->has_active && Tetris_CanPlace(game, &game->active)) {
        const TetrisPiece *piece = &game->active;
        for (int i = 0; i < 4; ++i) {
            TetrisCell cell = shapes[piece->type][piece->rotation][i];
            frame[piece->y + cell.y][piece->x + cell.x] =
                (uint8_t)(piece->type + 1);
        }
    }
    for (int row = 0; row < TETRIS_ROWS; ++row) {
        for (int col = 0; col < TETRIS_COLS; ++col) {
            if (frame[row][col] != previous_frame[row][col]) {
                uint8_t id = frame[row][col];
                Tetris_DrawCell(BOARD_X + col * TETRIS_CELL_SIZE,
                                BOARD_Y + row * TETRIS_CELL_SIZE,
                                colors[id <= PIECE_COUNT ? id : 0]);
                previous_frame[row][col] = frame[row][col];
            }
        }
    }
}
