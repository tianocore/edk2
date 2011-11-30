/** @file
  Worker functions for the setlocale function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

    setlocale.c 8.1 (Berkeley) 7/4/93
 *  NetBSD: setlocale.c,v 1.50 2006/02/16 19:19:49 tnozaki Exp
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#if defined(_MSC_VER)
  // Disable warnings about assignment within conditional expressions.
  #pragma warning ( disable : 4706 )
#endif

#define _CTYPE_PRIVATE

#include  "namespace.h"
#include  <sys/localedef.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <assert.h>
#include  <limits.h>
#include  <ctype.h>
#define __SETLOCALE_SOURCE__
#include  <locale.h>
#include  <paths.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>
#include  "rune.h"
#ifdef WITH_RUNE
  #include "rune_local.h"
#else
  #include "ctypeio.h"
#endif

#ifdef CITRUS
  #include <citrus/citrus_namespace.h>
  #include <citrus/citrus_region.h>
  #include <citrus/citrus_lookup.h>
  #include <citrus/citrus_bcs.h>
#else
  #include "aliasname_local.h"
  #define _lookup_alias(p, a, b, s, c)  __unaliasname((p), (a), (b), (s))
  #define _bcs_strcasecmp(a, b)   strcasecmp((a), (b))
#endif

#define _LOCALE_SYM_FORCE "/force"

#ifndef WITH_RUNE
  const char *_PathLocale = NULL;
#endif

/** Category names for getenv(). **/
static const char *const categories[_LC_LAST] = {
    "LC_ALL",
    "LC_COLLATE",
    "LC_CTYPE",
    "LC_MONETARY",
    "LC_NUMERIC",
    "LC_TIME",
    "LC_MESSAGES"
};

/** Current locales for each category.  **/
static char current_categories[_LC_LAST][32] = {
    "C",
    "C",
    "C",
    "C",
    "C",
    "C",
    "C"
};

/*
 * The locales we are going to try and load
 */
static char new_categories[_LC_LAST][32];

static char current_locale_string[_LC_LAST * 33];

static char *currentlocale(void);
static void revert_to_default(int);
static int force_locale_enable(int);
static int load_locale_sub(int, const char *, int);
static char *loadlocale(int);
static const char *__get_locale_env(int);

char *
__setlocale(int category, const char *locale)
{
  int i, loadlocale_success;
  size_t len;
  const char *env, *r;

  //if (issetugid() ||
  //    (!_PathLocale && !(_PathLocale = getenv("PATH_LOCALE"))))
  //  _PathLocale = _PATH_LOCALE;

  if (category < 0 || category >= _LC_LAST)
    return (NULL);

  if (!locale)
    return (category ?
        current_categories[category] : currentlocale());

  /*
   * Default to the current locale for everything.
   */
  for (i = 1; i < _LC_LAST; ++i)
    (void)strncpyX(new_categories[i], current_categories[i],
        sizeof(new_categories[i]));

  /*
   * Now go fill up new_categories from the locale argument
   */
  if (!*locale) {
    if (category == LC_ALL) {
      for (i = 1; i < _LC_LAST; ++i) {
        env = __get_locale_env(i);
        (void)strncpyX(new_categories[i], env,
            sizeof(new_categories[i]));
      }
    }
    else {
      env = __get_locale_env(category);
      (void)strncpyX(new_categories[category], env,
        sizeof(new_categories[category]));
    }
  } else if (category) {
    (void)strncpyX(new_categories[category], locale,
        sizeof(new_categories[category]));
  } else {
    if ((r = strchr(locale, '/')) == 0) {
      for (i = 1; i < _LC_LAST; ++i) {
        (void)strncpyX(new_categories[i], locale,
            sizeof(new_categories[i]));
      }
    } else {
      for (i = 1;;) {
        _DIAGASSERT(*r == '/' || *r == 0);
        _DIAGASSERT(*locale != 0);
        if (*locale == '/')
          return (NULL);  /* invalid format. */
        len = r - locale;
        if (len + 1 > sizeof(new_categories[i]))
          return (NULL);  /* too long */
        (void)memcpy(new_categories[i], locale, len);
        new_categories[i][len] = '\0';
        if (*r == 0)
          break;
        _DIAGASSERT(*r == '/');
        if (*(locale = ++r) == 0)
          /* slash followed by NUL */
          return (NULL);
        /* skip until NUL or '/' */
        while (*r && *r != '/')
          r++;
        if (++i == _LC_LAST)
          return (NULL);  /* too many slashes. */
      }
      if (i + 1 != _LC_LAST)
        return (NULL);  /* too few slashes. */
    }
  }

  if (category)
    return (loadlocale(category));

  loadlocale_success = 0;
  for (i = 1; i < _LC_LAST; ++i) {
    if (loadlocale(i) != NULL)
      loadlocale_success = 1;
  }

  /*
   * If all categories failed, return NULL; we don't need to back
   * changes off, since none happened.
   */
  if (!loadlocale_success)
    return NULL;

  return (currentlocale());
}

static char *
currentlocale()
{
  int i;

  (void)strncpyX(current_locale_string, current_categories[1],
      sizeof(current_locale_string));

  for (i = 2; i < _LC_LAST; ++i)
    if (strcmp(current_categories[1], current_categories[i])) {
      (void)snprintf(current_locale_string,
          sizeof(current_locale_string), "%s/%s/%s/%s/%s/%s",
          current_categories[1], current_categories[2],
          current_categories[3], current_categories[4],
          current_categories[5], current_categories[6]);
      break;
    }
  return (current_locale_string);
}

static void
revert_to_default(int category)
{
  switch (category) {
  case LC_CTYPE:
#ifdef WITH_RUNE
    (void)_xpg4_setrunelocale("C");
    (void)__runetable_to_netbsd_ctype("C");
#else
    if (_cClass != _C_CharClassTable) {
      /* LINTED const castaway */
      free((void *)_cClass);
      _cClass = _C_CharClassTable;
    }
    if (_uConvT != _C_ToUpperTable) {
      /* LINTED const castaway */
      free((void *)_uConvT);
      _uConvT = _C_ToUpperTable;
    }
    if (_lConvT != _C_ToLowerTable) {
      /* LINTED const castaway */
      free((void *)_lConvT);
      _lConvT = _C_ToLowerTable;
    }
#endif
    break;
  case LC_MESSAGES:
  case LC_COLLATE:
  case LC_MONETARY:
  case LC_NUMERIC:
  case LC_TIME:
    break;
  }
}

static int
force_locale_enable(int category)
{
  revert_to_default(category);

  return 0;
}

static int
load_locale_sub(
  int category,
  const char *locname,
  int isspecial
  )
{
  char name[PATH_MAX];

  /* check for the default locales */
  if (!strcmp(new_categories[category], "C") ||
      !strcmp(new_categories[category], "POSIX")) {
    revert_to_default(category);
    return 0;
  }

  /* check whether special symbol */
  if (isspecial && _bcs_strcasecmp(locname, _LOCALE_SYM_FORCE) == 0)
    return force_locale_enable(category);

  /* sanity check */
  if (strchr(locname, '/') != NULL)
    return -1;

  (void)snprintf(name, sizeof(name), "%s/%s/%s",
           _PathLocale, locname, categories[category]);

  switch (category) {
  case LC_CTYPE:
#ifdef WITH_RUNE
    if (_xpg4_setrunelocale(__UNCONST(locname)))
      return -1;
    if (__runetable_to_netbsd_ctype(locname)) {
      /* very unfortunate, but need to go to "C" locale */
      revert_to_default(category);
      return -1;
    }
#else
    if (!__loadctype(name))
      return -1;
#endif
    break;

  case LC_MESSAGES:
    /*
     * XXX we don't have LC_MESSAGES support yet,
     * but catopen may use the value of LC_MESSAGES category.
     * so return successfully if locale directory is present.
     */
    (void)snprintf(name, sizeof(name), "%s/%s",
      _PathLocale, locname);
    /* local */
    {
      struct stat st;
      if (stat(name, &st) < 0)
        return -1;
      if (!S_ISDIR(st.st_mode))
        return -1;
    }
    break;

  case LC_COLLATE:
  case LC_MONETARY:
  case LC_NUMERIC:
  case LC_TIME:
    return -1;
  }

  return 0;
}

static char *
loadlocale(int category)
{
  //char aliaspath[PATH_MAX], loccat[PATH_MAX], buf[PATH_MAX];
  //const char *alias;

  _DIAGASSERT(0 < category && category < _LC_LAST);

  if (strcmp(new_categories[category], current_categories[category]) == 0)
    return (current_categories[category]);

  /* (1) non-aliased file */
  if (!load_locale_sub(category, new_categories[category], 0))
    goto success;

  ///* (2) lookup locname/catname type alias */
  //(void)snprintf(aliaspath, sizeof(aliaspath),
  //         "%s/" _LOCALE_ALIAS_NAME, _PathLocale);
  //(void)snprintf(loccat, sizeof(loccat), "%s/%s",
  //         new_categories[category], categories[category]);
  //alias = _lookup_alias(aliaspath, loccat, buf, sizeof(buf),
  //          _LOOKUP_CASE_SENSITIVE);
  //if (!load_locale_sub(category, alias, 1))
  //  goto success;

  ///* (3) lookup locname type alias */
  //alias = _lookup_alias(aliaspath, new_categories[category],
  //          buf, sizeof(buf), _LOOKUP_CASE_SENSITIVE);
  //if (!load_locale_sub(category, alias, 1))
  //  goto success;

  return NULL;

success:
  (void)strncpyX(current_categories[category],
    new_categories[category],
    sizeof(current_categories[category]));
  return current_categories[category];
}

static const char *
__get_locale_env(int category)
{
  const char *env;

  //_DIAGASSERT(category != LC_ALL);

  ///* 1. check LC_ALL. */
  //env = getenv(categories[0]);

  ///* 2. check LC_* */
  //if (!env || !*env)
  //  env = getenv(categories[category]);

  ///* 3. check LANG */
  //if (!env || !*env)
  //  env = getenv("LANG");

  ///* 4. if none is set, fall to "C" */
  //if (!env || !*env || strchr(env, '/'))
    env = "C";

  return env;
}
