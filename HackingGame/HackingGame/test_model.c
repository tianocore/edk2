#include "./test_words.h"
#include "../../testing.h/testing.h"
#include "./model.h"

static int test_noise_map()
{
    ASSERT(__HG_NOISE_SEG_END - __HG_NOISE_SEG_START - 1 == sizeof(HG_NOISE_MAP) - 1);
    for (hg_game_tile_t i = __HG_NOISE_SEG_START + 1; i < __HG_NOISE_SEG_END; i++) {
        ASSERT(HG_NOISE_MAP[i - __HG_NOISE_SEG_START - 1] != 0);
    }
    return 1;
}

SUB_TEST(test_model, {&test_noise_map, "Test noise map"})
