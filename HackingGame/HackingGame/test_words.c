#include "./test_words.h"
#include "../../testing.h/testing.h"
#include "./words.h"
#include <string.h>

static int test_words_count()
{
    ASSERT(hg_word_count() > 0);
    return 1;
}

static int test_get_word_at()
{
    hg_word_t zero_word;
    hg_word_t word;
    for (size_t i = 0; i < hg_word_count(); i++) {
        memset(word, 0, sizeof(word));
        memset(zero_word, 0, sizeof(zero_word));

        ASSERT(hg_word_at(i, word));
        ASSERT(memcmp(word, zero_word, sizeof(word)) == 0);
    }
    return 1;
}

SUB_TEST(test_words, {&test_words_count, "Test words count"},
{&test_get_word_at, "Test get word at"})
