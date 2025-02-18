/** @file
  This file defines the hob structure for bootloader's CFR option menu.

  Copyright (c) 2023, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CFR_SETUP_MENU_GUID_H__
#define __CFR_SETUP_MENU_GUID_H__

///
/// CFR options form GUID
///
extern EFI_GUID gEfiCfrSetupMenuFormGuid;

/*
 * The following tags are for CFR (Cursed Form Representation) entries.
 *
 * CFR records form a tree structure. The size of a record includes
 * the size of its own fields plus the size of all children records.
 * CFR tags can appear multiple times except for `LB_TAG_CFR` which
 * is used for the root record.
 *
 * The following structures have comments that describe the supported
 * children records. These comments cannot be replaced with code! The
 * structures are variable-length, so the offsets won't be valid most
 * of the time. Besides, the implementation uses `sizeof()` to obtain
 * the size of the "record header" (the fixed-length members); adding
 * the children structures as struct members will increase the length
 * returned by `sizeof()`, which complicates things for zero reason.
 *
 * TODO: "This should be documentation instead."
 */
enum cfr_option_flags {
  CFR_OPTFLAG_READONLY  = 1 << 0,
  CFR_OPTFLAG_GRAYOUT   = 1 << 1,
  CFR_OPTFLAG_SUPPRESS  = 1 << 2,
  CFR_OPTFLAG_VOLATILE  = 1 << 3,
};

#define CB_TAG_CFR_VARCHAR_OPT_NAME     0x0007
#define CB_TAG_CFR_VARCHAR_UI_NAME      0x0008
#define CB_TAG_CFR_VARCHAR_UI_HELPTEXT  0x0009
#define CB_TAG_CFR_VARCHAR_DEF_VALUE    0x000a
#pragma pack (1)
typedef struct {
  UINT32  tag;          /*
                         * CFR_TAG_VARCHAR_OPT_NAME, CFR_TAG_VARCHAR_UI_NAME,
                         * CFR_TAG_VARCHAR_UI_HELPTEXT or CFR_TAG_VARCHAR_DEF_VALUE
                         */
  UINT32  size;         /* Length of the entire structure */
  UINT32  data_length;  /* Length of data, including NULL terminator for strings */
  UINT8   data[];
} CFR_VARBINARY;

#define CB_TAG_CFR_ENUM_VALUE  0x0002
typedef struct {
  UINT32          tag;      /* CFR_TAG_ENUM_VALUE */
  UINT32          size;
  UINT32          value;
  /*
   * CFR_UI_NAME  ui_name
   */
} CFR_ENUM_VALUE;

#define CB_TAG_CFR_OPTION_ENUM    0x0003
#define CB_TAG_CFR_OPTION_NUMBER  0x0004
#define CB_TAG_CFR_OPTION_BOOL    0x0005
typedef struct {
  UINT32                      tag;            /*
                                               * CFR_TAG_OPTION_ENUM, CFR_TAG_OPTION_NUMBER or
                                               * CFR_TAG_OPTION_BOOL
                                               */
  UINT32                      size;
  UINT64                      object_id;      /* Unique ID */
  UINT64                      dependency_id;  /* Dependent object ID, or 0 */
  UINT32                      flags;          /* enum cfr_option_flags */
  UINT32                      default_value;
  /*
   * CFR_VARCHAR_OPT_NAME     opt_name
   * CFR_VARCHAR_UI_NAME      ui_name
   * CFR_VARCHAR_UI_HELPTEXT  ui_helptext (Optional)
   * CFR_ENUM_VALUE           enum_values[]
   */
} CFR_OPTION_NUMERIC;

#define CB_TAG_CFR_OPTION_VARCHAR  0x0006
typedef struct {
  UINT32              tag;            /* CFR_OPTION_VARCHAR */
  UINT32              size;
  UINT64              object_id;      /* Unique ID */
  UINT64              dependency_id;  /* Dependent object ID, or 0 */
  UINT32              flags;          /* enum cfr_option_flags */
  /*
   * CFR_OPT_NAME     opt_name
   * CFR_UI_NAME      ui_name
   * CFR_UI_HELPTEXT  ui_helptext (Optional)
   * CFR_VARCHAR      default_value
   */
} CFR_OPTION_VARCHAR;

/*
 * A CFR option comment is roughly equivalent to a Kconfig comment.
 * Option comments are *NOT* string options (see CFR_OPTION_VARCHAR
 * instead) but they're considered an option for simplicity's sake.
 */
#define CB_TAG_CFR_OPTION_COMMENT  0x000b
typedef struct {
  UINT32              tag;              /* CFR_OPTION_COMMENT */
  UINT32              size;
  UINT64              object_id;        /* Unique ID */
  UINT64              dependency_id;    /* Dependent object ID, or 0 */
  UINT32              flags;            /* enum cfr_option_flags */
  /*
   * CFR_UI_NAME      ui_name
   * CFR_UI_HELPTEXT  ui_helptext (Optional)
   */
} CFR_OPTION_COMMENT;

/* CFR forms are considered options as they can be nested inside other forms */
#define CB_TAG_CFR_OPTION_FORM  0x0001
typedef struct {
  UINT32                tag;              /* CFR_OPTION_FORM */
  UINT32                size;
  UINT64                object_id;        /* Unique ID */
  UINT64                dependency_id;    /* Dependent object ID, or 0 */
  UINT32                flags;            /* enum cfr_option_flags */
  /*
   * CFR_UI_NAME        ui_name
   * <T in CFR_OPTION>  options[]
   */
} CFR_OPTION_FORM;
#pragma pack ()

#endif // __CFR_SETUP_MENU_GUID_H__
