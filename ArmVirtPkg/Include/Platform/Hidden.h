/** @file

  Copyright (c) 2018, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLATFORM_HIDDEN_H
#define __PLATFORM_HIDDEN_H

//
// Setting the GCC -fvisibility=hidden command line option is not quite the same
// as setting the pragma below: the former only affects definitions, whereas the
// pragma affects extern declarations as well. So if we want to ensure that no
// GOT indirected symbol references are emitted, we need to use the pragma, or
// GOT based cross object references could be emitted, e.g., in libraries, and
// these cannot be relaxed to ordinary symbol references at link time.
//
#pragma GCC visibility push (hidden)

#endif
