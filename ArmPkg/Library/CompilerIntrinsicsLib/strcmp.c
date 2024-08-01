// ------------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// ------------------------------------------------------------------------------

int
strcmp (
  const char  *,
  const char  *
  );

#if defined (_MSC_VER)
  #pragma intrinsic(strcmp)
  #pragma function(strcmp)
#endif

int
strcmp (
  const char  *s1,
  const char  *s2
  )
{
  while ((*s1 != '\0') && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return *s1 - *s2;
}
