project(HackingGameTests LANGUAGES C)

SET(TESTING_H_FILES
  "../../testing.h/testing.c"
  "../../testing.h/testing.h"
  "../../testing.h/logger.c"
  "../../testing.h/logger.h"
  "../../testing.h/ansi_colour.h")

set(SOURCES
  "./test.c"
  "./test_words.h"
  "./test_words.c"
  "./words.h"
  "./words.c"
  "./test_model.h"
  "./test_model.c"
  "./model.h"
  "./model.c")

add_executable(test ${SOURCES} ${TESTING_H_FILES})
