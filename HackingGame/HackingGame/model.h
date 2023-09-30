#pragma once
#include "./words.h"

#define HG_WORD_COUNT 15
#define HG_GRID_ROWS 15
#define HG_GRID_COLS 17

typedef enum hg_game_tile_t {
    HG_WORD,
    HG_WORD_DUD,
    HG_ANGULAR_OPEN, // <
    HG_ANGULAR_CLOSE, // >
    HG_ROUND_OPEN, // (
    HG_ROUND_CLOSE, // )
    HG_SQUARE_OPEN, // [
    HG_SQUARE_CLOSE,// ]
    HG_SQUIGGLE_OPEN, // {
    HG_SQUIGGLE_CLOSE, // }
    __HG_NOISE_SEG_START,
    HG_NOISE_1, // !
    HG_NOISE_2, // ,
    HG_NOISE_3, // .
    HG_NOISE_4, // %
    HG_NOISE_5, // |
    HG_NOISE_6, // #
    HG_NOISE_7, // :
    HG_NOISE_8, // +
    HG_NOISE_9, // /
    HG_NOISE_10, // ?
    HG_NOISE_11, // "
    HG_NOISE_12, // '
    HG_NOISE_13, // ^
    __HG_NOISE_SEG_END
} hg_game_tile_t;

#define HG_NOISE_MAP "!,.%|#:+/?\"'^"
#define HG_NOISE_VARIENTS (__HG_NOISE_SEG_END - __HG_NOISE_SEG_START - 1)

typedef struct hg_game_state_t {
    size_t word_indexes[HG_WORD_COUNT];
    /// The index (of words.h) that is correct
    size_t correct_word_index;
    hg_game_tile_t grid[HG_GRID_ROWS][HG_GRID_COLS];
    int retries;
} hg_game_state_t;

void hg_game_state_init(hg_game_state_t *state);
