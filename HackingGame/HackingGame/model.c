#include "./model.h"
#include "words.h"

#define HG_STARTING_RETRIES 5
#define HG_WORD_MIN_GAP 3
/// The subtraction now means you can do HG_WORD_MIN_GAP + random() % HG_WORD_MAX_GAP
#define HG_WORD_MAX_GAP (7 - HG_WORD_MIN_GAP)

#define random() 42

void hg_game_state_init(hg_game_state_t *state)
{
    state->retries = HG_STARTING_RETRIES;

    // Generate random words
    for (size_t i = 0; i < HG_WORD_COUNT; i++)
    {
        state->word_indexes[i] = random() % hg_word_count();
    }
    state->correct_word_index = state->word_indexes[random() % HG_WORD_COUNT];

    // Populate the grid with noise
    for (size_t x = 0; x < HG_GRID_ROWS; x++)
    {
        for (size_t y = 0; y < HG_GRID_COLS; y++)
        {
            state->grid[x][y] = __HG_NOISE_SEG_START + 1 + random() % HG_NOISE_VARIENTS;
        }
    }

    // Add words to the grid
    size_t grid_offset = -HG_WORD_MIN_GAP; // This allows the start to be a word
    for (size_t i = 0; i < HG_WORD_COUNT; i++)
    {
        grid_offset += HG_WORD_MIN_GAP + random() % HG_WORD_MAX_GAP;
        for (size_t j = 0; j < HG_WORD_LENGTH; j++)
        {
            size_t x = grid_offset % HG_GRID_ROWS;
            size_t y = grid_offset / HG_GRID_ROWS;
            state->grid[x][y] = HG_WORD;
            grid_offset++;
        }
    }
}
