#include "./test_words.h"
#include "../../testing.h/testing.h"
#include "./model.h"
#include "words.h"
#include <string.h>

static int test_noise_map()
{
    ASSERT(HG_NOISE_VARIENTS == sizeof(HG_NOISE_MAP) - 1);
    for (hg_game_tile_t i = __HG_NOISE_SEG_START + 1; i < __HG_NOISE_SEG_END; i++) {
        ASSERT(HG_NOISE_MAP[i - __HG_NOISE_SEG_START - 1] != 0);
    }
    return 1;
}

static int test_init_game_state()
{
    hg_game_state_t state;
    memset(&state, 0xFF, sizeof(state));

    hg_game_state_init(&state);

    // Test the initial state
    ASSERT(state.retries > 0);
    ASSERT(state.correct_word_index >= 0);
    ASSERT(state.correct_word_index < hg_word_count());

    for (size_t x = 0; x < HG_GRID_ROWS; x++) {
        for(size_t y = 0; y < HG_GRID_COLS; y++) {
            ASSERT(state.grid[x][y] != 0xFF);
            ASSERT(state.grid[x][y] < __HG_NOISE_SEG_END);
            ASSERT(state.grid[x][y] != __HG_NOISE_SEG_START);
            ASSERT(state.grid[x][y] != __HG_NOISE_SEG_END);
        }
    }

    // Test word placement
    size_t words = 0;
    for(size_t y = 0; y < HG_GRID_COLS; y++) {
        for (size_t x = 0; x < HG_GRID_ROWS; x++) {
            if (state.grid[x][y] != HG_WORD) {
                continue;
            }
            words++;

            size_t offset = x + y * HG_GRID_ROWS;
            for (size_t i = 0; i < HG_WORD_LENGTH; i++) {
                size_t grid_offset = offset + i;
                size_t x = grid_offset % HG_GRID_ROWS;
                size_t y = grid_offset / HG_GRID_ROWS;

                ASSERT(state.grid[x][y] == HG_WORD);
            }

            size_t grid_offset = offset + 6;
            x = grid_offset % HG_GRID_ROWS;
            y = grid_offset / HG_GRID_ROWS;
            ASSERT(state.grid[x][y] != HG_WORD);
        }
    }

    ASSERT(words == HG_WORD_COUNT);

    return 1;
}

SUB_TEST(test_model, {&test_noise_map, "Test noise map"},
{&test_init_game_state, "Test game state init"})
