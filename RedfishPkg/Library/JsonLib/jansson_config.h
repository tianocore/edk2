/** @file This is the configuration file for building jansson library.

 (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

    SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#ifndef JANSSON_CONFIG_H_
#define JANSSON_CONFIG_H_

///
/// We don't support inline JSON on edk2
///
#define JSON_INLINE

///
/// We support long long on edk2
///
#define JSON_INTEGER_IS_LONG_LONG 1

///
/// We don't support locale on edk2
///
#define JSON_HAVE_LOCALECONV 0

///
/// We don't support atomic builtins on edk2
///
#define JSON_HAVE_ATOMIC_BUILTINS 0

///
/// We don't support sync builtins on edk2
///
#define JSON_HAVE_SYNC_BUILTINS 0

///
/// Mzximum deepth is set to 2048
///
#define JSON_PARSER_MAX_DEPTH 2048

#endif
