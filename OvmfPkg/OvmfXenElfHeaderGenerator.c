/** @file
  This program generates a hex array to be manually coppied into
  OvmfXen.fdf.

  The purpose is for the flash device image to be recognize as an ELF.

  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "elf.h"
#include "stdio.h"
#include "stddef.h"

void
print_hdr (
  void    *s,
  size_t  size
  )
{
  char  *c = s;

  while (size--) {
    printf ("0x%02hhx, ", *(c++));
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

int
main (
  void
  )
{
  /* FW_SIZE */
  size_t  ovmf_blob_size = 0x00200000;
  /* Load OVMF at 1MB when running as PVH guest */
  uint32_t  ovmf_base_address = 0x00100000;
  /* Xen PVH entry point */
  uint32_t  ovmfxen_pvh_entry_point = ovmf_base_address + ovmf_blob_size - 0x30;
  size_t    offset_into_file        = 0;

  /* ELF file header */
  Elf32_Ehdr  hdr = {
    .e_ident     = ELFMAG,
    .e_type      = ET_EXEC,
    .e_machine   = EM_386,
    .e_version   = EV_CURRENT,
    .e_entry     = ovmfxen_pvh_entry_point,
    .e_flags     = R_386_NONE,
    .e_ehsize    = sizeof (hdr),
    .e_phentsize = sizeof (Elf32_Phdr),
  };

  offset_into_file += sizeof (hdr);

  hdr.e_ident[EI_CLASS]   = ELFCLASS32;
  hdr.e_ident[EI_DATA]    = ELFDATA2LSB;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;
  hdr.e_ident[EI_OSABI]   = ELFOSABI_LINUX;
  /* Placing program headers just after hdr */
  hdr.e_phoff = sizeof (hdr);

  /* program header */
  Elf32_Phdr  phdr_load = {
    .p_type   = PT_LOAD,
    .p_offset = 0, /* load everything */
    .p_paddr  = ovmf_base_address,
    .p_filesz = ovmf_blob_size,
    .p_memsz  = ovmf_blob_size,
    .p_flags  = PF_X | PF_W | PF_R,
    .p_align  = 0,
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
  Elf32_Phdr                phdr_note = {
    .p_type   = PT_NOTE,
    .p_filesz = sizeof (xen_elf_note),
    .p_memsz  = sizeof (xen_elf_note),
    .p_flags  = PF_R,
    .p_align  = 0,
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

  printf ("# ELF file header\n");
  print_hdr (&hdr, entry_off);
  printf ("\n");
  print_hdr (&hdr.e_entry, sizeof (hdr.e_entry));
  printf (" # hdr.e_entry\n");
  print_hdr (&hdr.e_entry + 1, hdr_size - entry_off - sizeof (hdr.e_entry));

  printf ("\n\n# ELF Program segment headers\n");
  printf ("# - Load segment\n");
  for (i = 0; i < sizeof (phdr_load); i += 4) {
    print_hdr (((char *)&phdr_load) + i, 4);
    printf ("\n");
  }

  printf ("# - ELFNOTE segment\n");
  for (i = 0; i < sizeof (phdr_note); i += 4) {
    print_hdr (((char *)&phdr_note) + i, 4);
    printf ("\n");
  }

  printf ("\n# XEN_ELFNOTE_PHYS32_ENTRY\n");
  for (i = 0; i < sizeof (xen_elf_note); i += 4) {
    print_hdr (((char *)&xen_elf_note) + i, 4);
    printf ("\n");
  }

  return 0;
}
