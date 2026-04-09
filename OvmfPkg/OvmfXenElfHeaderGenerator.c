/** @file
  This program generates a hex array to be manually coppied into
  OvmfXen.fdf.

  The purpose is for the flash device image to be recognize as an ELF.

  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "elf.h"
#include "fcntl.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"

void
print_hdr (
  FILE    *file,
  void    *s,
  size_t  size,
  bool    end_delimiter
  )
{
  char  *c = s;

  fprintf (file, "  ");
  while (size-- > 1) {
    fprintf (file, "0x%02hhx, ", *(c++));
  }

  if (end_delimiter) {
    fprintf (file, "0x%02hhx,", *c);
  } else {
    fprintf (file, "0x%02hhx", *c);
  }
}

/* Format for the XEN_ELFNOTE_PHYS32_ENTRY program segment */
#define XEN_ELFNOTE_PHYS32_ENTRY  18
typedef struct {
  uint32_t    name_size;
  uint32_t    desc_size;
  uint32_t    type;
  char        name[4];
  uint32_t    desc;
} xen_elfnote_phys32_entry;

#define LICENSE_HDR  "\
## @file\r\n\
#  FDF include file that defines a PVH ELF header.\r\n\
#\r\n\
#  Copyright (c) 2022, Intel Corporation. All rights reserved.\r\n\
#\r\n\
#  SPDX-License-Identifier: BSD-2-Clause-Patent\r\n\
#\r\n\
##\r\n\
\r\n\
"

int
main (
  int   argc,
  char  *argv[]
  )
{
  /* FW_SIZE */
  size_t  ovmf_blob_size = 0x00200000;
  /* Load OVMF at 1MB when running as PVH guest */
  uint32_t  ovmf_base_address = 0x00100000;
  uint32_t  ovmfxen_pvh_entry_point;
  size_t    offset_into_file = 0;
  char      *endptr, *str;
  long      param;
  FILE      *file = stdout;

  /* Parse the size parameter */
  if (argc > 1) {
    str   = argv[1];
    param = strtol (str, &endptr, 10);
    if (endptr != str) {
      ovmf_blob_size = (size_t)param;
    }
  }

  /* Parse the filepath parameter */
  if (argc > 2) {
    file = fopen (argv[2], "w");
    fprintf (file, LICENSE_HDR);
  }

  /* Xen PVH entry point */
  ovmfxen_pvh_entry_point = ovmf_base_address + ovmf_blob_size - 0x30;

  /* ELF file header */
 #ifdef PVH64
  Elf64_Ehdr  hdr = {
 #else
  Elf32_Ehdr  hdr = {
 #endif
    .e_ident   = ELFMAG,
    .e_type    = ET_EXEC,
    .e_machine = EM_386,
    .e_version = EV_CURRENT,
    .e_entry   = ovmfxen_pvh_entry_point,
    .e_flags   = R_386_NONE,
    .e_ehsize  = sizeof (hdr),
 #ifdef PVH64
    .e_phentsize = sizeof (Elf64_Phdr),
 #else
    .e_phentsize = sizeof (Elf32_Phdr),
 #endif
  };

  offset_into_file += sizeof (hdr);

 #ifdef PVH64
  hdr.e_ident[EI_CLASS] = ELFCLASS64;
 #else
  hdr.e_ident[EI_CLASS] = ELFCLASS32;
 #endif
  hdr.e_ident[EI_DATA]    = ELFDATA2LSB;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;
  hdr.e_ident[EI_OSABI]   = ELFOSABI_LINUX;
  /* Placing program headers just after hdr */
  hdr.e_phoff = sizeof (hdr);

  /* program header */
 #ifdef PVH64
  Elf64_Phdr  phdr_load = {
 #else
  Elf32_Phdr  phdr_load = {
 #endif
    .p_type   = PT_LOAD,
    .p_offset = 0, /* load everything */
    .p_paddr  = ovmf_base_address,
    .p_filesz = ovmf_blob_size,
    .p_memsz  = ovmf_blob_size,
    .p_flags  = PF_X | PF_W | PF_R,
 #ifdef PVH64
    .p_align  = 4,
 #else
    .p_align  = 0,
 #endif
  };

  phdr_load.p_vaddr = phdr_load.p_paddr;
  hdr.e_phnum      += 1;
  offset_into_file += sizeof (phdr_load);

  /* Xen ELF Note. */

  xen_elfnote_phys32_entry  xen_elf_note = {
    .type      = XEN_ELFNOTE_PHYS32_ENTRY,
    .name      = "Xen",
    .desc      = ovmfxen_pvh_entry_point,
    .name_size =
      offsetof (xen_elfnote_phys32_entry, desc) -
      offsetof (xen_elfnote_phys32_entry, name),
    .desc_size =
      sizeof (xen_elfnote_phys32_entry) -
      offsetof (xen_elfnote_phys32_entry, desc),
  };
 #ifdef PVH64
  Elf64_Phdr  phdr_note = {
 #else
  Elf32_Phdr  phdr_note = {
 #endif
    .p_type   = PT_NOTE,
    .p_filesz = sizeof (xen_elf_note),
    .p_memsz  = sizeof (xen_elf_note),
    .p_flags  = PF_R,
 #ifdef PVH64
    .p_align  = 4,
 #else
    .p_align  = 0,
 #endif
  };

  hdr.e_phnum       += 1;
  offset_into_file  += sizeof (phdr_note);
  phdr_note.p_offset = offset_into_file;
  phdr_note.p_paddr  = ovmf_base_address + phdr_note.p_offset;
  phdr_note.p_vaddr  = phdr_note.p_paddr;

  /*
   * print elf header
   */

  size_t  i;
  size_t  hdr_size  = sizeof (hdr);
  size_t  entry_off = offsetof (typeof(hdr), e_entry);

  fprintf (file, "DATA = {\r\n");

  fprintf (file, "  # ELF file header\r\n");
  print_hdr (file, &hdr, entry_off, true);
  fprintf (file, "\r\n");
  print_hdr (file, &hdr.e_entry, sizeof (hdr.e_entry), true);
  fprintf (file, " # hdr.e_entry\r\n");
  print_hdr (file, &hdr.e_entry + 1, hdr_size - entry_off - sizeof (hdr.e_entry), true);

  fprintf (file, "\r\n\r\n  # ELF Program segment headers\r\n");
  fprintf (file, "  # - Load segment\r\n");
  for (i = 0; i < sizeof (phdr_load); i += 4) {
    print_hdr (file, ((char *)&phdr_load) + i, 4, true);
    fprintf (file, "\r\n");
  }

  fprintf (file, "  # - ELFNOTE segment\r\n");
  for (i = 0; i < sizeof (phdr_note); i += 4) {
    print_hdr (file, ((char *)&phdr_note) + i, 4, true);
    fprintf (file, "\r\n");
  }

  fprintf (file, "\r\n  # XEN_ELFNOTE_PHYS32_ENTRY\r\n");
  for (i = 0; i < sizeof (xen_elf_note); i += 4) {
    print_hdr (file, ((char *)&xen_elf_note) + i, 4, (sizeof (xen_elf_note) - i) > 4);
    fprintf (file, "\r\n");
  }

  fprintf (file, "}\r\n");

  return 0;
}
