#pragma once
#include <stddef.h>

#define HG_WORD_LENGTH 5

typedef char hg_word_t[HG_WORD_LENGTH];

extern size_t hg_word_count();
extern int hg_word_at(size_t index, hg_word_t ret);

