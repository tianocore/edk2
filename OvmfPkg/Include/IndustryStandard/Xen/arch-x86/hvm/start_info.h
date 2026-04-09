/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2016, Citrix Systems, Inc.
 */

#ifndef __XEN_PUBLIC_ARCH_X86_HVM_START_INFO_H__
#define __XEN_PUBLIC_ARCH_X86_HVM_START_INFO_H__

/*
 * Start of day structure passed to PVH guests and to HVM guests in %ebx.
 *
 * NOTE: nothing will be loaded at physical address 0, so a 0 value in any
 * of the address fields should be treated as not present.
 *
 *  0 +----------------+
 *    | magic          | Contains the magic value XEN_HVM_START_MAGIC_VALUE
 *    |                | ("xEn3" with the 0x80 bit of the "E" set).
 *  4 +----------------+
 *    | version        | Version of this structure. Current version is 1. New
 *    |                | versions are guaranteed to be backwards-compatible.
 *  8 +----------------+
 *    | flags          | SIF_xxx flags.
 * 12 +----------------+
 *    | nr_modules     | Number of modules passed to the kernel.
 * 16 +----------------+
 *    | modlist_paddr  | Physical address of an array of modules
 *    |                | (layout of the structure below).
 * 24 +----------------+
 *    | cmdline_paddr  | Physical address of the command line,
 *    |                | a zero-terminated ASCII string.
 * 32 +----------------+
 *    | rsdp_paddr     | Physical address of the RSDP ACPI data structure.
 * 40 +----------------+
 *    | memmap_paddr   | Physical address of the (optional) memory map. Only
 *    |                | present in version 1 and newer of the structure.
 * 48 +----------------+
 *    | memmap_entries | Number of entries in the memory map table. Zero
 *    |                | if there is no memory map being provided. Only
 *    |                | present in version 1 and newer of the structure.
 * 52 +----------------+
 *    | reserved       | Version 1 and newer only.
 * 56 +----------------+
 *
 * The layout of each entry in the module structure is the following:
 *
 *  0 +----------------+
 *    | paddr          | Physical address of the module.
 *  8 +----------------+
 *    | size           | Size of the module in bytes.
 * 16 +----------------+
 *    | cmdline_paddr  | Physical address of the command line,
 *    |                | a zero-terminated ASCII string.
 * 24 +----------------+
 *    | reserved       |
 * 32 +----------------+
 *
 * The layout of each entry in the memory map table is as follows:
 *
 *  0 +----------------+
 *    | addr           | Base address
 *  8 +----------------+
 *    | size           | Size of mapping in bytes
 * 16 +----------------+
 *    | type           | Type of mapping as defined between the hypervisor
 *    |                | and guest. See XEN_HVM_MEMMAP_TYPE_* values below.
 * 20 +----------------|
 *    | reserved       |
 * 24 +----------------+
 *
 * The address and sizes are always a 64bit little endian unsigned integer.
 *
 * NB: Xen on x86 will always try to place all the data below the 4GiB
 * boundary.
 *
 * Version numbers of the hvm_start_info structure have evolved like this:
 *
 * Version 0:  Initial implementation.
 *
 * Version 1:  Added the memmap_paddr/memmap_entries fields (plus 4 bytes of
 *             padding) to the end of the hvm_start_info struct. These new
 *             fields can be used to pass a memory map to the guest. The
 *             memory map is optional and so guests that understand version 1
 *             of the structure must check that memmap_entries is non-zero
 *             before trying to read the memory map.
 */
#define XEN_HVM_START_MAGIC_VALUE  0x336ec578

/*
 * The values used in the type field of the memory map table entries are
 * defined below and match the Address Range Types as defined in the "System
 * Address Map Interfaces" section of the ACPI Specification. Please refer to
 * section 15 in version 6.2 of the ACPI spec: http://uefi.org/specifications
 */
#define XEN_HVM_MEMMAP_TYPE_RAM       1
#define XEN_HVM_MEMMAP_TYPE_RESERVED  2
#define XEN_HVM_MEMMAP_TYPE_ACPI      3
#define XEN_HVM_MEMMAP_TYPE_NVS       4
#define XEN_HVM_MEMMAP_TYPE_UNUSABLE  5
#define XEN_HVM_MEMMAP_TYPE_DISABLED  6
#define XEN_HVM_MEMMAP_TYPE_PMEM      7

/*
 * C representation of the x86/HVM start info layout.
 *
 * The canonical definition of this layout is above, this is just a way to
 * represent the layout described there using C types.
 */
struct hvm_start_info {
  UINT32    magic;            /* Contains the magic value 0x336ec578       */
                              /* ("xEn3" with the 0x80 bit of the "E" set).*/
  UINT32    version;          /* Version of this structure.                */
  UINT32    flags;            /* SIF_xxx flags.                            */
  UINT32    nr_modules;       /* Number of modules passed to the kernel.   */
  UINT64    modlist_paddr;    /* Physical address of an array of           */
                              /* hvm_modlist_entry.                        */
  UINT64    cmdline_paddr;    /* Physical address of the command line.     */
  UINT64    rsdp_paddr;       /* Physical address of the RSDP ACPI data    */
                              /* structure.                                */
  /* All following fields only present in version 1 and newer */
  UINT64    memmap_paddr;     /* Physical address of an array of           */
                              /* hvm_memmap_table_entry.                   */
  UINT32    memmap_entries;   /* Number of entries in the memmap table.    */
                              /* Value will be zero if there is no memory  */
                              /* map being provided.                       */
  UINT32    reserved;         /* Must be zero.                             */
};

struct hvm_modlist_entry {
  UINT64    paddr;            /* Physical address of the module.           */
  UINT64    size;             /* Size of the module in bytes.              */
  UINT64    cmdline_paddr;    /* Physical address of the command line.     */
  UINT64    reserved;
};

struct hvm_memmap_table_entry {
  UINT64    addr;             /* Base address of the memory region         */
  UINT64    size;             /* Size of the memory region in bytes        */
  UINT32    type;             /* Mapping type                              */
  UINT32    reserved;         /* Must be zero for Version 1.               */
};

#endif /* __XEN_PUBLIC_ARCH_X86_HVM_START_INFO_H__ */
