/** @file
  UEFI Application to display CPUID leaf information.

  Copyright (c) 2016 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Register/Intel/Cpuid.h>

///
/// Macro used to display the value of a bit field in a register returned by CPUID.
///
#define PRINT_BIT_FIELD(Variable, FieldName) \
  Print (L"%5a%42a: %x\n", #Variable, #FieldName, Variable.Bits.FieldName);

///
/// Macro used to display the value of a register returned by CPUID.
///
#define PRINT_VALUE(Variable, Description) \
  Print (L"%5a%42a: %x\n", #Variable, #Description, Variable);

///
/// Structure for cache description lookup table
///
typedef struct {
  UINT8    CacheDescriptor;
  CHAR8    *Type;
  CHAR8    *Description;
} CPUID_CACHE_INFO_DESCRIPTION;

///
/// Cache description lookup table
///
CPUID_CACHE_INFO_DESCRIPTION  mCpuidCacheInfoDescription[] = {
  { 0x00, "General",  "Null descriptor, this byte contains no information"                                                                                                                                    },
  { 0x01, "TLB",      "Instruction TLB: 4 KByte pages, 4-way set associative, 32 entries"                                                                                                                     },
  { 0x02, "TLB",      "Instruction TLB: 4 MByte pages, fully associative, 2 entries"                                                                                                                          },
  { 0x03, "TLB",      "Data TLB: 4 KByte pages, 4-way set associative, 64 entries"                                                                                                                            },
  { 0x04, "TLB",      "Data TLB: 4 MByte pages, 4-way set associative, 8 entries"                                                                                                                             },
  { 0x05, "TLB",      "Data TLB1: 4 MByte pages, 4-way set associative, 32 entries"                                                                                                                           },
  { 0x06, "Cache",    "1st-level instruction cache: 8 KBytes, 4-way set associative, 32 byte line size"                                                                                                       },
  { 0x08, "Cache",    "1st-level instruction cache: 16 KBytes, 4-way set associative, 32 byte line size"                                                                                                      },
  { 0x09, "Cache",    "1st-level instruction cache: 32KBytes, 4-way set associative, 64 byte line size"                                                                                                       },
  { 0x0A, "Cache",    "1st-level data cache: 8 KBytes, 2-way set associative, 32 byte line size"                                                                                                              },
  { 0x0B, "TLB",      "Instruction TLB: 4 MByte pages, 4-way set associative, 4 entries"                                                                                                                      },
  { 0x0C, "Cache",    "1st-level data cache: 16 KBytes, 4-way set associative, 32 byte line size"                                                                                                             },
  { 0x0D, "Cache",    "1st-level data cache: 16 KBytes, 4-way set associative, 64 byte line size"                                                                                                             },
  { 0x0E, "Cache",    "1st-level data cache: 24 KBytes, 6-way set associative, 64 byte line size"                                                                                                             },
  { 0x1D, "Cache",    "2nd-level cache: 128 KBytes, 2-way set associative, 64 byte line size"                                                                                                                 },
  { 0x21, "Cache",    "2nd-level cache: 256 KBytes, 8-way set associative, 64 byte line size"                                                                                                                 },
  { 0x22, "Cache",    "3rd-level cache: 512 KBytes, 4-way set associative, 64 byte line size, 2 lines per sector"                                                                                             },
  { 0x23, "Cache",    "3rd-level cache: 1 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector"                                                                                               },
  { 0x24, "Cache",    "2nd-level cache: 1 MBytes, 16-way set associative, 64 byte line size"                                                                                                                  },
  { 0x25, "Cache",    "3rd-level cache: 2 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector"                                                                                               },
  { 0x29, "Cache",    "3rd-level cache: 4 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector"                                                                                               },
  { 0x2C, "Cache",    "1st-level data cache: 32 KBytes, 8-way set associative, 64 byte line size"                                                                                                             },
  { 0x30, "Cache",    "1st-level instruction cache: 32 KBytes, 8-way set associative, 64 byte line size"                                                                                                      },
  { 0x40, "Cache",    "No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache"                                                                                              },
  { 0x41, "Cache",    "2nd-level cache: 128 KBytes, 4-way set associative, 32 byte line size"                                                                                                                 },
  { 0x42, "Cache",    "2nd-level cache: 256 KBytes, 4-way set associative, 32 byte line size"                                                                                                                 },
  { 0x43, "Cache",    "2nd-level cache: 512 KBytes, 4-way set associative, 32 byte line size"                                                                                                                 },
  { 0x44, "Cache",    "2nd-level cache: 1 MByte, 4-way set associative, 32 byte line size"                                                                                                                    },
  { 0x45, "Cache",    "2nd-level cache: 2 MByte, 4-way set associative, 32 byte line size"                                                                                                                    },
  { 0x46, "Cache",    "3rd-level cache: 4 MByte, 4-way set associative, 64 byte line size"                                                                                                                    },
  { 0x47, "Cache",    "3rd-level cache: 8 MByte, 8-way set associative, 64 byte line size"                                                                                                                    },
  { 0x48, "Cache",    "2nd-level cache: 3MByte, 12-way set associative, 64 byte line size"                                                                                                                    },
  { 0x49, "Cache",    "3rd-level cache: 4MB, 16-way set associative, 64-byte line size (Intel Xeon processor MP, Family 0FH, Model 06H). 2nd-level cache: 4 MByte, 16-way set associative, 64 byte line size" },
  { 0x4A, "Cache",    "3rd-level cache: 6MByte, 12-way set associative, 64 byte line size"                                                                                                                    },
  { 0x4B, "Cache",    "3rd-level cache: 8MByte, 16-way set associative, 64 byte line size"                                                                                                                    },
  { 0x4C, "Cache",    "3rd-level cache: 12MByte, 12-way set associative, 64 byte line size"                                                                                                                   },
  { 0x4D, "Cache",    "3rd-level cache: 16MByte, 16-way set associative, 64 byte line size"                                                                                                                   },
  { 0x4E, "Cache",    "2nd-level cache: 6MByte, 24-way set associative, 64 byte line size"                                                                                                                    },
  { 0x4F, "TLB",      "Instruction TLB: 4 KByte pages, 32 entries"                                                                                                                                            },
  { 0x50, "TLB",      "Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 64 entries"                                                                                                                     },
  { 0x51, "TLB",      "Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 128 entries"                                                                                                                    },
  { 0x52, "TLB",      "Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 256 entries"                                                                                                                    },
  { 0x55, "TLB",      "Instruction TLB: 2-MByte or 4-MByte pages, fully associative, 7 entries"                                                                                                               },
  { 0x56, "TLB",      "Data TLB0: 4 MByte pages, 4-way set associative, 16 entries"                                                                                                                           },
  { 0x57, "TLB",      "Data TLB0: 4 KByte pages, 4-way associative, 16 entries"                                                                                                                               },
  { 0x59, "TLB",      "Data TLB0: 4 KByte pages, fully associative, 16 entries"                                                                                                                               },
  { 0x5A, "TLB",      "Data TLB0: 2 MByte or 4 MByte pages, 4-way set associative, 32 entries"                                                                                                                },
  { 0x5B, "TLB",      "Data TLB: 4 KByte and 4 MByte pages, 64 entries"                                                                                                                                       },
  { 0x5C, "TLB",      "Data TLB: 4 KByte and 4 MByte pages,128 entries"                                                                                                                                       },
  { 0x5D, "TLB",      "Data TLB: 4 KByte and 4 MByte pages,256 entries"                                                                                                                                       },
  { 0x60, "Cache",    "1st-level data cache: 16 KByte, 8-way set associative, 64 byte line size"                                                                                                              },
  { 0x61, "TLB",      "Instruction TLB: 4 KByte pages, fully associative, 48 entries"                                                                                                                         },
  { 0x63, "TLB",      "Data TLB: 2 MByte or 4 MByte pages, 4-way set associative, 32 entries and a separate array with 1 GByte pages, 4-way set associative, 4 entries"                                       },
  { 0x64, "TLB",      "Data TLB: 4 KByte pages, 4-way set associative, 512 entries"                                                                                                                           },
  { 0x66, "Cache",    "1st-level data cache: 8 KByte, 4-way set associative, 64 byte line size"                                                                                                               },
  { 0x67, "Cache",    "1st-level data cache: 16 KByte, 4-way set associative, 64 byte line size"                                                                                                              },
  { 0x68, "Cache",    "1st-level data cache: 32 KByte, 4-way set associative, 64 byte line size"                                                                                                              },
  { 0x6A, "Cache",    "uTLB: 4 KByte pages, 8-way set associative, 64 entries"                                                                                                                                },
  { 0x6B, "Cache",    "DTLB: 4 KByte pages, 8-way set associative, 256 entries"                                                                                                                               },
  { 0x6C, "Cache",    "DTLB: 2M/4M pages, 8-way set associative, 128 entries"                                                                                                                                 },
  { 0x6D, "Cache",    "DTLB: 1 GByte pages, fully associative, 16 entries"                                                                                                                                    },
  { 0x70, "Cache",    "Trace cache: 12 K-uop, 8-way set associative"                                                                                                                                          },
  { 0x71, "Cache",    "Trace cache: 16 K-uop, 8-way set associative"                                                                                                                                          },
  { 0x72, "Cache",    "Trace cache: 32 K-uop, 8-way set associative"                                                                                                                                          },
  { 0x76, "TLB",      "Instruction TLB: 2M/4M pages, fully associative, 8 entries"                                                                                                                            },
  { 0x78, "Cache",    "2nd-level cache: 1 MByte, 4-way set associative, 64byte line size"                                                                                                                     },
  { 0x79, "Cache",    "2nd-level cache: 128 KByte, 8-way set associative, 64 byte line size, 2 lines per sector"                                                                                              },
  { 0x7A, "Cache",    "2nd-level cache: 256 KByte, 8-way set associative, 64 byte line size, 2 lines per sector"                                                                                              },
  { 0x7B, "Cache",    "2nd-level cache: 512 KByte, 8-way set associative, 64 byte line size, 2 lines per sector"                                                                                              },
  { 0x7C, "Cache",    "2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size, 2 lines per sector"                                                                                                },
  { 0x7D, "Cache",    "2nd-level cache: 2 MByte, 8-way set associative, 64byte line size"                                                                                                                     },
  { 0x7F, "Cache",    "2nd-level cache: 512 KByte, 2-way set associative, 64-byte line size"                                                                                                                  },
  { 0x80, "Cache",    "2nd-level cache: 512 KByte, 8-way set associative, 64-byte line size"                                                                                                                  },
  { 0x82, "Cache",    "2nd-level cache: 256 KByte, 8-way set associative, 32 byte line size"                                                                                                                  },
  { 0x83, "Cache",    "2nd-level cache: 512 KByte, 8-way set associative, 32 byte line size"                                                                                                                  },
  { 0x84, "Cache",    "2nd-level cache: 1 MByte, 8-way set associative, 32 byte line size"                                                                                                                    },
  { 0x85, "Cache",    "2nd-level cache: 2 MByte, 8-way set associative, 32 byte line size"                                                                                                                    },
  { 0x86, "Cache",    "2nd-level cache: 512 KByte, 4-way set associative, 64 byte line size"                                                                                                                  },
  { 0x87, "Cache",    "2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size"                                                                                                                    },
  { 0xA0, "DTLB",     "DTLB: 4k pages, fully associative, 32 entries"                                                                                                                                         },
  { 0xB0, "TLB",      "Instruction TLB: 4 KByte pages, 4-way set associative, 128 entries"                                                                                                                    },
  { 0xB1, "TLB",      "Instruction TLB: 2M pages, 4-way, 8 entries or 4M pages, 4-way, 4 entries"                                                                                                             },
  { 0xB2, "TLB",      "Instruction TLB: 4KByte pages, 4-way set associative, 64 entries"                                                                                                                      },
  { 0xB3, "TLB",      "Data TLB: 4 KByte pages, 4-way set associative, 128 entries"                                                                                                                           },
  { 0xB4, "TLB",      "Data TLB1: 4 KByte pages, 4-way associative, 256 entries"                                                                                                                              },
  { 0xB5, "TLB",      "Instruction TLB: 4KByte pages, 8-way set associative, 64 entries"                                                                                                                      },
  { 0xB6, "TLB",      "Instruction TLB: 4KByte pages, 8-way set associative, 128 entries"                                                                                                                     },
  { 0xBA, "TLB",      "Data TLB1: 4 KByte pages, 4-way associative, 64 entries"                                                                                                                               },
  { 0xC0, "TLB",      "Data TLB: 4 KByte and 4 MByte pages, 4-way associative, 8 entries"                                                                                                                     },
  { 0xC1, "STLB",     "Shared 2nd-Level TLB: 4 KByte/2MByte pages, 8-way associative, 1024 entries"                                                                                                           },
  { 0xC2, "DTLB",     "DTLB: 4 KByte/2 MByte pages, 4-way associative, 16 entries"                                                                                                                            },
  { 0xC3, "STLB",     "Shared 2nd-Level TLB: 4 KByte /2 MByte pages, 6-way associative, 1536 entries. Also 1GBbyte pages, 4-way, 16 entries."                                                                 },
  { 0xC4, "DTLB",     "DTLB: 2M/4M Byte pages, 4-way associative, 32 entries"                                                                                                                                 },
  { 0xCA, "STLB",     "Shared 2nd-Level TLB: 4 KByte pages, 4-way associative, 512 entries"                                                                                                                   },
  { 0xD0, "Cache",    "3rd-level cache: 512 KByte, 4-way set associative, 64 byte line size"                                                                                                                  },
  { 0xD1, "Cache",    "3rd-level cache: 1 MByte, 4-way set associative, 64 byte line size"                                                                                                                    },
  { 0xD2, "Cache",    "3rd-level cache: 2 MByte, 4-way set associative, 64 byte line size"                                                                                                                    },
  { 0xD6, "Cache",    "3rd-level cache: 1 MByte, 8-way set associative, 64 byte line size"                                                                                                                    },
  { 0xD7, "Cache",    "3rd-level cache: 2 MByte, 8-way set associative, 64 byte line size"                                                                                                                    },
  { 0xD8, "Cache",    "3rd-level cache: 4 MByte, 8-way set associative, 64 byte line size"                                                                                                                    },
  { 0xDC, "Cache",    "3rd-level cache: 1.5 MByte, 12-way set associative, 64 byte line size"                                                                                                                 },
  { 0xDD, "Cache",    "3rd-level cache: 3 MByte, 12-way set associative, 64 byte line size"                                                                                                                   },
  { 0xDE, "Cache",    "3rd-level cache: 6 MByte, 12-way set associative, 64 byte line size"                                                                                                                   },
  { 0xE2, "Cache",    "3rd-level cache: 2 MByte, 16-way set associative, 64 byte line size"                                                                                                                   },
  { 0xE3, "Cache",    "3rd-level cache: 4 MByte, 16-way set associative, 64 byte line size"                                                                                                                   },
  { 0xE4, "Cache",    "3rd-level cache: 8 MByte, 16-way set associative, 64 byte line size"                                                                                                                   },
  { 0xEA, "Cache",    "3rd-level cache: 12MByte, 24-way set associative, 64 byte line size"                                                                                                                   },
  { 0xEB, "Cache",    "3rd-level cache: 18MByte, 24-way set associative, 64 byte line size"                                                                                                                   },
  { 0xEC, "Cache",    "3rd-level cache: 24MByte, 24-way set associative, 64 byte line size"                                                                                                                   },
  { 0xF0, "Prefetch", "64-Byte prefetching"                                                                                                                                                                   },
  { 0xF1, "Prefetch", "128-Byte prefetching"                                                                                                                                                                  },
  { 0xFE, "General",  "CPUID leaf 2 does not report TLB descriptor information; use CPUID leaf 18H to query TLB and other address translation parameters."                                                    },
  { 0xFF, "General",  "CPUID leaf 2 does not report cache descriptor information, use CPUID leaf 4 to query cache parameters"                                                                                 }
};

///
/// The maximum supported CPUID leaf index starting from leaf 0x00000000.
///
UINT32  gMaximumBasicFunction = CPUID_SIGNATURE;

///
/// The maximum supported CPUID leaf index starting from leaf 0x80000000.
///
UINT32  gMaximumExtendedFunction = CPUID_EXTENDED_FUNCTION;

/**
  Display CPUID_SIGNATURE leaf.

**/
VOID
CpuidSignature (
  VOID
  )
{
  UINT32  Eax;
  UINT32  Ebx;
  UINT32  Ecx;
  UINT32  Edx;
  CHAR8   Signature[13];

  AsmCpuid (CPUID_SIGNATURE, &Eax, &Ebx, &Ecx, &Edx);

  Print (L"CPUID_SIGNATURE (Leaf %08x)\n", CPUID_SIGNATURE);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx, Ecx, Edx);
  PRINT_VALUE (Eax, MaximumLeaf);
  *(UINT32 *)(Signature + 0) = Ebx;
  *(UINT32 *)(Signature + 4) = Edx;
  *(UINT32 *)(Signature + 8) = Ecx;
  Signature[12]              = 0;
  Print (L"  Signature = %a\n", Signature);

  gMaximumBasicFunction = Eax;
}

/**
  Display CPUID_VERSION_INFO leaf.

**/
VOID
CpuidVersionInfo (
  VOID
  )
{
  CPUID_VERSION_INFO_EAX  Eax;
  CPUID_VERSION_INFO_EBX  Ebx;
  CPUID_VERSION_INFO_ECX  Ecx;
  CPUID_VERSION_INFO_EDX  Edx;
  UINT32                  DisplayFamily;
  UINT32                  DisplayModel;

  if (CPUID_VERSION_INFO > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);

  Print (L"CPUID_VERSION_INFO (Leaf %08x)\n", CPUID_VERSION_INFO);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);

  DisplayFamily = Eax.Bits.FamilyId;
  if (Eax.Bits.FamilyId == 0x0F) {
    DisplayFamily += Eax.Bits.ExtendedFamilyId;
  }

  DisplayModel = Eax.Bits.Model;
  if ((Eax.Bits.FamilyId == 0x06) || (Eax.Bits.FamilyId == 0x0f)) {
    DisplayModel += (Eax.Bits.ExtendedModelId << 4);
  }

  Print (L"  Family = %x  Model = %x  Stepping = %x\n", DisplayFamily, DisplayModel, Eax.Bits.SteppingId);

  PRINT_BIT_FIELD (Eax, SteppingId);
  PRINT_BIT_FIELD (Eax, Model);
  PRINT_BIT_FIELD (Eax, FamilyId);
  PRINT_BIT_FIELD (Eax, ProcessorType);
  PRINT_BIT_FIELD (Eax, ExtendedModelId);
  PRINT_BIT_FIELD (Eax, ExtendedFamilyId);
  PRINT_BIT_FIELD (Ebx, BrandIndex);
  PRINT_BIT_FIELD (Ebx, CacheLineSize);
  PRINT_BIT_FIELD (Ebx, MaximumAddressableIdsForLogicalProcessors);
  PRINT_BIT_FIELD (Ebx, InitialLocalApicId);
  PRINT_BIT_FIELD (Ecx, SSE3);
  PRINT_BIT_FIELD (Ecx, PCLMULQDQ);
  PRINT_BIT_FIELD (Ecx, DTES64);
  PRINT_BIT_FIELD (Ecx, MONITOR);
  PRINT_BIT_FIELD (Ecx, DS_CPL);
  PRINT_BIT_FIELD (Ecx, VMX);
  PRINT_BIT_FIELD (Ecx, SMX);
  PRINT_BIT_FIELD (Ecx, TM2);
  PRINT_BIT_FIELD (Ecx, SSSE3);
  PRINT_BIT_FIELD (Ecx, CNXT_ID);
  PRINT_BIT_FIELD (Ecx, SDBG);
  PRINT_BIT_FIELD (Ecx, FMA);
  PRINT_BIT_FIELD (Ecx, CMPXCHG16B);
  PRINT_BIT_FIELD (Ecx, xTPR_Update_Control);
  PRINT_BIT_FIELD (Ecx, PDCM);
  PRINT_BIT_FIELD (Ecx, PCID);
  PRINT_BIT_FIELD (Ecx, DCA);
  PRINT_BIT_FIELD (Ecx, SSE4_1);
  PRINT_BIT_FIELD (Ecx, SSE4_2);
  PRINT_BIT_FIELD (Ecx, x2APIC);
  PRINT_BIT_FIELD (Ecx, MOVBE);
  PRINT_BIT_FIELD (Ecx, POPCNT);
  PRINT_BIT_FIELD (Ecx, TSC_Deadline);
  PRINT_BIT_FIELD (Ecx, AESNI);
  PRINT_BIT_FIELD (Ecx, XSAVE);
  PRINT_BIT_FIELD (Ecx, OSXSAVE);
  PRINT_BIT_FIELD (Ecx, AVX);
  PRINT_BIT_FIELD (Ecx, F16C);
  PRINT_BIT_FIELD (Ecx, RDRAND);
  PRINT_BIT_FIELD (Edx, FPU);
  PRINT_BIT_FIELD (Edx, VME);
  PRINT_BIT_FIELD (Edx, DE);
  PRINT_BIT_FIELD (Edx, PSE);
  PRINT_BIT_FIELD (Edx, TSC);
  PRINT_BIT_FIELD (Edx, MSR);
  PRINT_BIT_FIELD (Edx, PAE);
  PRINT_BIT_FIELD (Edx, MCE);
  PRINT_BIT_FIELD (Edx, CX8);
  PRINT_BIT_FIELD (Edx, APIC);
  PRINT_BIT_FIELD (Edx, SEP);
  PRINT_BIT_FIELD (Edx, MTRR);
  PRINT_BIT_FIELD (Edx, PGE);
  PRINT_BIT_FIELD (Edx, MCA);
  PRINT_BIT_FIELD (Edx, CMOV);
  PRINT_BIT_FIELD (Edx, PAT);
  PRINT_BIT_FIELD (Edx, PSE_36);
  PRINT_BIT_FIELD (Edx, PSN);
  PRINT_BIT_FIELD (Edx, CLFSH);
  PRINT_BIT_FIELD (Edx, DS);
  PRINT_BIT_FIELD (Edx, ACPI);
  PRINT_BIT_FIELD (Edx, MMX);
  PRINT_BIT_FIELD (Edx, FXSR);
  PRINT_BIT_FIELD (Edx, SSE);
  PRINT_BIT_FIELD (Edx, SSE2);
  PRINT_BIT_FIELD (Edx, SS);
  PRINT_BIT_FIELD (Edx, HTT);
  PRINT_BIT_FIELD (Edx, TM);
  PRINT_BIT_FIELD (Edx, PBE);
}

/**
  Lookup a cache description string from the mCpuidCacheInfoDescription table.

  @param[in] CacheDescriptor  Cache descriptor value from CPUID_CACHE_INFO.

**/
CPUID_CACHE_INFO_DESCRIPTION *
LookupCacheDescription (
  UINT8  CacheDescriptor
  )
{
  UINTN  NumDescriptors;
  UINTN  Descriptor;

  if (CacheDescriptor == 0x00) {
    return NULL;
  }

  NumDescriptors = sizeof (mCpuidCacheInfoDescription)/sizeof (mCpuidCacheInfoDescription[0]);
  for (Descriptor = 0; Descriptor < NumDescriptors; Descriptor++) {
    if (CacheDescriptor == mCpuidCacheInfoDescription[Descriptor].CacheDescriptor) {
      return &mCpuidCacheInfoDescription[Descriptor];
    }
  }

  return NULL;
}

/**
  Display CPUID_CACHE_INFO leaf for each supported cache descriptor.

**/
VOID
CpuidCacheInfo (
  VOID
  )
{
  CPUID_CACHE_INFO_CACHE_TLB    Eax;
  CPUID_CACHE_INFO_CACHE_TLB    Ebx;
  CPUID_CACHE_INFO_CACHE_TLB    Ecx;
  CPUID_CACHE_INFO_CACHE_TLB    Edx;
  UINTN                         Index;
  CPUID_CACHE_INFO_DESCRIPTION  *CacheDescription;

  if (CPUID_CACHE_INFO > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_CACHE_INFO, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);

  Print (L"CPUID_CACHE_INFO (Leaf %08x)\n", CPUID_CACHE_INFO);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
  if (Eax.Bits.NotValid == 0) {
    //
    // Process Eax.CacheDescriptor[1..3].  Ignore Eax.CacheDescriptor[0]
    //
    for (Index = 1; Index < 4; Index++) {
      CacheDescription = LookupCacheDescription (Eax.CacheDescriptor[Index]);
      if (CacheDescription != NULL) {
        Print (
          L"  %-8a %a\n",
          CacheDescription->Type,
          CacheDescription->Description
          );
      }
    }
  }

  if (Ebx.Bits.NotValid == 0) {
    //
    // Process Ebx.CacheDescriptor[0..3]
    //
    for (Index = 0; Index < 4; Index++) {
      CacheDescription = LookupCacheDescription (Ebx.CacheDescriptor[Index]);
      if (CacheDescription != NULL) {
        Print (
          L"  %-8a %a\n",
          CacheDescription->Type,
          CacheDescription->Description
          );
      }
    }
  }

  if (Ecx.Bits.NotValid == 0) {
    //
    // Process Ecx.CacheDescriptor[0..3]
    //
    for (Index = 0; Index < 4; Index++) {
      CacheDescription = LookupCacheDescription (Ecx.CacheDescriptor[Index]);
      if (CacheDescription != NULL) {
        Print (
          L"  %-8a %a\n",
          CacheDescription->Type,
          CacheDescription->Description
          );
      }
    }
  }

  if (Edx.Bits.NotValid == 0) {
    //
    // Process Edx.CacheDescriptor[0..3]
    //
    for (Index = 0; Index < 4; Index++) {
      CacheDescription = LookupCacheDescription (Edx.CacheDescriptor[Index]);
      if (CacheDescription != NULL) {
        Print (
          L"  %-8a %a\n",
          CacheDescription->Type,
          CacheDescription->Description
          );
      }
    }
  }
}

/**
  Display CPUID_SERIAL_NUMBER leaf if it is supported.

**/
VOID
CpuidSerialNumber (
  VOID
  )
{
  CPUID_VERSION_INFO_EDX  VersionInfoEdx;
  UINT32                  Ecx;
  UINT32                  Edx;

  Print (L"CPUID_SERIAL_NUMBER (Leaf %08x)\n", CPUID_SERIAL_NUMBER);

  if (CPUID_SERIAL_NUMBER > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &VersionInfoEdx.Uint32);
  if (VersionInfoEdx.Bits.PSN == 0) {
    Print (L"  Not Supported\n");
    return;
  }

  AsmCpuid (CPUID_SERIAL_NUMBER, NULL, NULL, &Ecx, &Edx);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", 0, 0, Ecx, Edx);
  Print (L"  Processor Serial Number = %08x%08x%08x\n", 0, Edx, Ecx);
}

/**
  Display CPUID_CACHE_PARAMS for all supported sub-leafs.

**/
VOID
CpuidCacheParams (
  VOID
  )
{
  UINT32                  CacheLevel;
  CPUID_CACHE_PARAMS_EAX  Eax;
  CPUID_CACHE_PARAMS_EBX  Ebx;
  UINT32                  Ecx;
  CPUID_CACHE_PARAMS_EDX  Edx;

  if (CPUID_CACHE_PARAMS > gMaximumBasicFunction) {
    return;
  }

  CacheLevel = 0;
  do {
    AsmCpuidEx (
      CPUID_CACHE_PARAMS,
      CacheLevel,
      &Eax.Uint32,
      &Ebx.Uint32,
      &Ecx,
      &Edx.Uint32
      );
    if (Eax.Bits.CacheType != CPUID_CACHE_PARAMS_CACHE_TYPE_NULL) {
      Print (L"CPUID_CACHE_PARAMS (Leaf %08x, Sub-Leaf %08x)\n", CPUID_CACHE_PARAMS, CacheLevel);
      Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx, Edx.Uint32);
      PRINT_BIT_FIELD (Eax, CacheType);
      PRINT_BIT_FIELD (Eax, CacheLevel);
      PRINT_BIT_FIELD (Eax, SelfInitializingCache);
      PRINT_BIT_FIELD (Eax, FullyAssociativeCache);
      PRINT_BIT_FIELD (Eax, MaximumAddressableIdsForLogicalProcessors);
      PRINT_BIT_FIELD (Eax, MaximumAddressableIdsForProcessorCores);
      PRINT_BIT_FIELD (Ebx, LineSize);
      PRINT_BIT_FIELD (Ebx, LinePartitions);
      PRINT_BIT_FIELD (Ebx, Ways);
      PRINT_VALUE (Ecx, NumberOfSets);
      PRINT_BIT_FIELD (Edx, Invalidate);
      PRINT_BIT_FIELD (Edx, CacheInclusiveness);
      PRINT_BIT_FIELD (Edx, ComplexCacheIndexing);
    }

    CacheLevel++;
  } while (Eax.Bits.CacheType != CPUID_CACHE_PARAMS_CACHE_TYPE_NULL);
}

/**
  Display CPUID_MONITOR_MWAIT leaf.

**/
VOID
CpuidMonitorMwait (
  VOID
  )
{
  CPUID_MONITOR_MWAIT_EAX  Eax;
  CPUID_MONITOR_MWAIT_EBX  Ebx;
  CPUID_MONITOR_MWAIT_ECX  Ecx;
  CPUID_MONITOR_MWAIT_EDX  Edx;

  if (CPUID_MONITOR_MWAIT > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_MONITOR_MWAIT, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);

  Print (L"CPUID_MONITOR_MWAIT (Leaf %08x)\n", CPUID_MONITOR_MWAIT);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);

  PRINT_BIT_FIELD (Eax, SmallestMonitorLineSize);
  PRINT_BIT_FIELD (Ebx, LargestMonitorLineSize);
  PRINT_BIT_FIELD (Ecx, ExtensionsSupported);
  PRINT_BIT_FIELD (Ecx, InterruptAsBreak);
  PRINT_BIT_FIELD (Edx, C0States);
  PRINT_BIT_FIELD (Edx, C1States);
  PRINT_BIT_FIELD (Edx, C2States);
  PRINT_BIT_FIELD (Edx, C3States);
  PRINT_BIT_FIELD (Edx, C4States);
  PRINT_BIT_FIELD (Edx, C5States);
  PRINT_BIT_FIELD (Edx, C6States);
  PRINT_BIT_FIELD (Edx, C7States);
}

/**
  Display CPUID_THERMAL_POWER_MANAGEMENT leaf.

**/
VOID
CpuidThermalPowerManagement (
  VOID
  )
{
  CPUID_THERMAL_POWER_MANAGEMENT_EAX  Eax;
  CPUID_THERMAL_POWER_MANAGEMENT_EBX  Ebx;
  CPUID_THERMAL_POWER_MANAGEMENT_ECX  Ecx;

  if (CPUID_THERMAL_POWER_MANAGEMENT > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_THERMAL_POWER_MANAGEMENT, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, NULL);

  Print (L"CPUID_THERMAL_POWER_MANAGEMENT (Leaf %08x)\n", CPUID_THERMAL_POWER_MANAGEMENT);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, 0);

  PRINT_BIT_FIELD (Eax, DigitalTemperatureSensor);
  PRINT_BIT_FIELD (Eax, TurboBoostTechnology);
  PRINT_BIT_FIELD (Eax, ARAT);
  PRINT_BIT_FIELD (Eax, PLN);
  PRINT_BIT_FIELD (Eax, ECMD);
  PRINT_BIT_FIELD (Eax, PTM);
  PRINT_BIT_FIELD (Eax, HWP);
  PRINT_BIT_FIELD (Eax, HWP_Notification);
  PRINT_BIT_FIELD (Eax, HWP_Activity_Window);
  PRINT_BIT_FIELD (Eax, HWP_Energy_Performance_Preference);
  PRINT_BIT_FIELD (Eax, HWP_Package_Level_Request);
  PRINT_BIT_FIELD (Eax, HDC);
  PRINT_BIT_FIELD (Eax, TurboBoostMaxTechnology30);
  PRINT_BIT_FIELD (Eax, HWPCapabilities);
  PRINT_BIT_FIELD (Eax, HWPPECIOverride);
  PRINT_BIT_FIELD (Eax, FlexibleHWP);
  PRINT_BIT_FIELD (Eax, FastAccessMode);
  PRINT_BIT_FIELD (Eax, IgnoringIdleLogicalProcessorHWPRequest);
  PRINT_BIT_FIELD (Ebx, InterruptThresholds);
  PRINT_BIT_FIELD (Ecx, HardwareCoordinationFeedback);
  PRINT_BIT_FIELD (Ecx, PerformanceEnergyBias);
}

/**
  Display CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS for all supported sub-leafs.

**/
VOID
CpuidStructuredExtendedFeatureFlags (
  VOID
  )
{
  UINT32                                       Eax;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX  Ebx;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX  Ecx;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EDX  Edx;
  UINT32                                       SubLeaf;

  if (CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO,
    &Eax,
    NULL,
    NULL,
    NULL
    );
  for (SubLeaf = 0; SubLeaf <= Eax; SubLeaf++) {
    AsmCpuidEx (
      CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
      SubLeaf,
      NULL,
      &Ebx.Uint32,
      &Ecx.Uint32,
      &Edx.Uint32
      );
    if ((Ebx.Uint32 != 0) || (Ecx.Uint32 != 0) || (Edx.Uint32 != 0)) {
      Print (L"CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS (Leaf %08x, Sub-Leaf %08x)\n", CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, SubLeaf);
      Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
      PRINT_BIT_FIELD (Ebx, FSGSBASE);
      PRINT_BIT_FIELD (Ebx, IA32_TSC_ADJUST);
      PRINT_BIT_FIELD (Ebx, SGX);
      PRINT_BIT_FIELD (Ebx, BMI1);
      PRINT_BIT_FIELD (Ebx, HLE);
      PRINT_BIT_FIELD (Ebx, AVX2);
      PRINT_BIT_FIELD (Ebx, FDP_EXCPTN_ONLY);
      PRINT_BIT_FIELD (Ebx, SMEP);
      PRINT_BIT_FIELD (Ebx, BMI2);
      PRINT_BIT_FIELD (Ebx, EnhancedRepMovsbStosb);
      PRINT_BIT_FIELD (Ebx, INVPCID);
      PRINT_BIT_FIELD (Ebx, RTM);
      PRINT_BIT_FIELD (Ebx, RDT_M);
      PRINT_BIT_FIELD (Ebx, DeprecateFpuCsDs);
      PRINT_BIT_FIELD (Ebx, MPX);
      PRINT_BIT_FIELD (Ebx, RDT_A);
      PRINT_BIT_FIELD (Ebx, AVX512F);
      PRINT_BIT_FIELD (Ebx, AVX512DQ);
      PRINT_BIT_FIELD (Ebx, RDSEED);
      PRINT_BIT_FIELD (Ebx, ADX);
      PRINT_BIT_FIELD (Ebx, SMAP);
      PRINT_BIT_FIELD (Ebx, AVX512_IFMA);
      PRINT_BIT_FIELD (Ebx, CLFLUSHOPT);
      PRINT_BIT_FIELD (Ebx, CLWB);
      PRINT_BIT_FIELD (Ebx, IntelProcessorTrace);
      PRINT_BIT_FIELD (Ebx, AVX512PF);
      PRINT_BIT_FIELD (Ebx, AVX512ER);
      PRINT_BIT_FIELD (Ebx, AVX512CD);
      PRINT_BIT_FIELD (Ebx, SHA);
      PRINT_BIT_FIELD (Ebx, AVX512BW);
      PRINT_BIT_FIELD (Ebx, AVX512VL);

      PRINT_BIT_FIELD (Ecx, PREFETCHWT1);
      PRINT_BIT_FIELD (Ecx, AVX512_VBMI);
      PRINT_BIT_FIELD (Ecx, UMIP);
      PRINT_BIT_FIELD (Ecx, PKU);
      PRINT_BIT_FIELD (Ecx, OSPKE);
      PRINT_BIT_FIELD (Ecx, AVX512_VPOPCNTDQ);
      PRINT_BIT_FIELD (Ecx, MAWAU);
      PRINT_BIT_FIELD (Ecx, RDPID);
      PRINT_BIT_FIELD (Ecx, SGX_LC);

      PRINT_BIT_FIELD (Edx, AVX512_4VNNIW);
      PRINT_BIT_FIELD (Edx, AVX512_4FMAPS);
      PRINT_BIT_FIELD (Edx, EnumeratesSupportForIBRSAndIBPB);
      PRINT_BIT_FIELD (Edx, EnumeratesSupportForSTIBP);
      PRINT_BIT_FIELD (Edx, EnumeratesSupportForL1D_FLUSH);
      PRINT_BIT_FIELD (Edx, EnumeratesSupportForCapability);
      PRINT_BIT_FIELD (Edx, EnumeratesSupportForSSBD);
    }
  }
}

/**
  Display CPUID_DIRECT_CACHE_ACCESS_INFO leaf.

**/
VOID
CpuidDirectCacheAccessInfo (
  VOID
  )
{
  UINT32  Eax;

  if (CPUID_DIRECT_CACHE_ACCESS_INFO > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_DIRECT_CACHE_ACCESS_INFO, &Eax, NULL, NULL, NULL);
  Print (L"CPUID_DIRECT_CACHE_ACCESS_INFO (Leaf %08x)\n", CPUID_DIRECT_CACHE_ACCESS_INFO);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, 0, 0, 0);
}

/**
  Display CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING leaf.

**/
VOID
CpuidArchitecturalPerformanceMonitoring (
  VOID
  )
{
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EAX  Eax;
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EBX  Ebx;
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EDX  Edx;

  if (CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING, &Eax.Uint32, &Ebx.Uint32, NULL, &Edx.Uint32);
  Print (L"CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING (Leaf %08x)\n", CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, 0, Edx.Uint32);
  PRINT_BIT_FIELD (Eax, ArchPerfMonVerID);
  PRINT_BIT_FIELD (Eax, PerformanceMonitorCounters);
  PRINT_BIT_FIELD (Eax, PerformanceMonitorCounterWidth);
  PRINT_BIT_FIELD (Eax, EbxBitVectorLength);
  PRINT_BIT_FIELD (Ebx, UnhaltedCoreCycles);
  PRINT_BIT_FIELD (Ebx, InstructionsRetired);
  PRINT_BIT_FIELD (Ebx, UnhaltedReferenceCycles);
  PRINT_BIT_FIELD (Ebx, LastLevelCacheReferences);
  PRINT_BIT_FIELD (Ebx, LastLevelCacheMisses);
  PRINT_BIT_FIELD (Ebx, BranchInstructionsRetired);
  PRINT_BIT_FIELD (Ebx, AllBranchMispredictRetired);
  PRINT_BIT_FIELD (Edx, FixedFunctionPerformanceCounters);
  PRINT_BIT_FIELD (Edx, FixedFunctionPerformanceCounterWidth);
  PRINT_BIT_FIELD (Edx, AnyThreadDeprecation);
}

/**
  Display CPUID_EXTENDED_TOPOLOGY leafs for all supported levels.

  @param[in] LeafFunction  Leaf function index for CPUID_EXTENDED_TOPOLOGY.

**/
VOID
CpuidExtendedTopology (
  UINT32  LeafFunction
  )
{
  CPUID_EXTENDED_TOPOLOGY_EAX  Eax;
  CPUID_EXTENDED_TOPOLOGY_EBX  Ebx;
  CPUID_EXTENDED_TOPOLOGY_ECX  Ecx;
  UINT32                       Edx;
  UINT32                       LevelNumber;

  if (LeafFunction > gMaximumBasicFunction) {
    return;
  }

  if ((LeafFunction != CPUID_EXTENDED_TOPOLOGY) && (LeafFunction != CPUID_V2_EXTENDED_TOPOLOGY)) {
    return;
  }

  LevelNumber = 0;
  for (LevelNumber = 0; ; LevelNumber++) {
    AsmCpuidEx (
      LeafFunction,
      LevelNumber,
      &Eax.Uint32,
      &Ebx.Uint32,
      &Ecx.Uint32,
      &Edx
      );
    if (Ecx.Bits.LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID) {
      break;
    }

    Print (
      L"%a (Leaf %08x, Sub-Leaf %08x)\n",
      LeafFunction == CPUID_EXTENDED_TOPOLOGY ? "CPUID_EXTENDED_TOPOLOGY" : "CPUID_V2_EXTENDED_TOPOLOGY",
      LeafFunction,
      LevelNumber
      );
    Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx);
    PRINT_BIT_FIELD (Eax, ApicIdShift);
    PRINT_BIT_FIELD (Ebx, LogicalProcessors);
    PRINT_BIT_FIELD (Ecx, LevelNumber);
    PRINT_BIT_FIELD (Ecx, LevelType);
    PRINT_VALUE (Edx, x2APIC_ID);
  }
}

/**
  Display CPUID_EXTENDED_STATE sub-leaf.

**/
VOID
CpuidExtendedStateSubLeaf (
  VOID
  )
{
  CPUID_EXTENDED_STATE_SUB_LEAF_EAX  Eax;
  UINT32                             Ebx;
  CPUID_EXTENDED_STATE_SUB_LEAF_ECX  Ecx;
  UINT32                             Edx;

  AsmCpuidEx (
    CPUID_EXTENDED_STATE,
    CPUID_EXTENDED_STATE_SUB_LEAF,
    &Eax.Uint32,
    &Ebx,
    &Ecx.Uint32,
    &Edx
    );
  Print (L"CPUID_EXTENDED_STATE (Leaf %08x, Sub-Leaf %08x)\n", CPUID_EXTENDED_STATE, CPUID_EXTENDED_STATE_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx, Ecx.Uint32, Edx);
  PRINT_BIT_FIELD (Eax, XSAVEOPT);
  PRINT_BIT_FIELD (Eax, XSAVEC);
  PRINT_BIT_FIELD (Eax, XGETBV);
  PRINT_BIT_FIELD (Eax, XSAVES);
  PRINT_VALUE (Ebx, EnabledSaveStateSize_XCR0_IA32_XSS);
  PRINT_BIT_FIELD (Ecx, XCR0);
  PRINT_BIT_FIELD (Ecx, HWPState);
  PRINT_BIT_FIELD (Ecx, PT);
  PRINT_BIT_FIELD (Ecx, XCR0_1);
  PRINT_VALUE (Edx, IA32_XSS_Supported_32_63);
}

/**
  Display CPUID_EXTENDED_STATE size and offset information sub-leaf.

**/
VOID
CpuidExtendedStateSizeOffset (
  VOID
  )
{
  UINT32                                Eax;
  UINT32                                Ebx;
  CPUID_EXTENDED_STATE_SIZE_OFFSET_ECX  Ecx;
  UINT32                                Edx;
  UINT32                                SubLeaf;

  for (SubLeaf = CPUID_EXTENDED_STATE_SIZE_OFFSET; SubLeaf < 32; SubLeaf++) {
    AsmCpuidEx (
      CPUID_EXTENDED_STATE,
      SubLeaf,
      &Eax,
      &Ebx,
      &Ecx.Uint32,
      &Edx
      );
    if (Edx != 0) {
      Print (L"CPUID_EXTENDED_STATE (Leaf %08x, Sub-Leaf %08x)\n", CPUID_EXTENDED_STATE, SubLeaf);
      Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx, Ecx.Uint32, Edx);
      PRINT_VALUE (Eax, FeatureSaveStateSize);
      PRINT_VALUE (Ebx, FeatureSaveStateOffset);
      PRINT_BIT_FIELD (Ecx, XSS);
      PRINT_BIT_FIELD (Ecx, Compacted);
    }
  }
}

/**
  Display CPUID_EXTENDED_STATE main leaf and sub-leafs.

**/
VOID
CpuidExtendedStateMainLeaf (
  VOID
  )
{
  CPUID_EXTENDED_STATE_MAIN_LEAF_EAX  Eax;
  UINT32                              Ebx;
  UINT32                              Ecx;
  UINT32                              Edx;

  if (CPUID_EXTENDED_STATE > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_EXTENDED_STATE,
    CPUID_EXTENDED_STATE_MAIN_LEAF,
    &Eax.Uint32,
    &Ebx,
    &Ecx,
    &Edx
    );
  Print (L"CPUID_EXTENDED_STATE (Leaf %08x, Sub-Leaf %08x)\n", CPUID_EXTENDED_STATE, CPUID_EXTENDED_STATE_MAIN_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx, Ecx, Edx);
  PRINT_BIT_FIELD (Eax, x87);
  PRINT_BIT_FIELD (Eax, SSE);
  PRINT_BIT_FIELD (Eax, AVX);
  PRINT_BIT_FIELD (Eax, MPX);
  PRINT_BIT_FIELD (Eax, AVX_512);
  PRINT_BIT_FIELD (Eax, IA32_XSS);
  PRINT_BIT_FIELD (Eax, PKRU);
  PRINT_BIT_FIELD (Eax, IA32_XSS_2);
  PRINT_VALUE (Ebx, EnabledSaveStateSize);
  PRINT_VALUE (Ecx, SupportedSaveStateSize);
  PRINT_VALUE (Edx, XCR0_Supported_32_63);

  CpuidExtendedStateSubLeaf ();
  CpuidExtendedStateSizeOffset ();
}

/**
  Display CPUID_INTEL_RDT_MONITORING enumeration sub-leaf.

**/
VOID
CpuidIntelRdtMonitoringEnumerationSubLeaf (
  VOID
  )
{
  UINT32                                               Ebx;
  CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF_EDX  Edx;

  if (CPUID_INTEL_RDT_MONITORING > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_INTEL_RDT_MONITORING,
    CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF,
    NULL,
    &Ebx,
    NULL,
    &Edx.Uint32
    );
  Print (L"CPUID_INTEL_RDT_MONITORING (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_RDT_MONITORING, CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", 0, Ebx, 0, Edx.Uint32);
  PRINT_VALUE (Ebx, Maximum_RMID_Range);
  PRINT_BIT_FIELD (Edx, L3CacheRDT_M);
}

/**
  Display CPUID_INTEL_RDT_MONITORING L3 cache capability sub-leaf.

**/
VOID
CpuidIntelRdtMonitoringL3CacheCapabilitySubLeaf (
  VOID
  )
{
  UINT32                                            Ebx;
  UINT32                                            Ecx;
  CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF_EDX  Edx;

  if (CPUID_INTEL_RDT_MONITORING > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_INTEL_RDT_MONITORING,
    CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF,
    NULL,
    &Ebx,
    &Ecx,
    &Edx.Uint32
    );
  Print (L"CPUID_INTEL_RDT_MONITORING (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_RDT_MONITORING, CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", 0, Ebx, Ecx, Edx.Uint32);
  PRINT_VALUE (Ebx, OccupancyConversionFactor);
  PRINT_VALUE (Ecx, Maximum_RMID_Range);
  PRINT_BIT_FIELD (Edx, L3CacheOccupancyMonitoring);
  PRINT_BIT_FIELD (Edx, L3CacheTotalBandwidthMonitoring);
  PRINT_BIT_FIELD (Edx, L3CacheLocalBandwidthMonitoring);
}

/**
  Display CPUID_INTEL_RDT_ALLOCATION memory bandwidth allocation technology enumeration
  sub-leaf.

**/
VOID
CpuidIntelRdtAllocationMemoryBandwidthSubLeaf (
  VOID
  )
{
  CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EAX  Eax;
  UINT32                                                    Ebx;
  CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_ECX  Ecx;
  CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION,
    CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF,
    &Eax.Uint32,
    &Ebx,
    &Ecx.Uint32,
    &Edx.Uint32
    );
  Print (L"CPUID_INTEL_RDT_ALLOCATION (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx, Ecx.Uint32, Edx.Uint32);
  PRINT_BIT_FIELD (Eax, MaximumMBAThrottling);
  PRINT_VALUE (Ebx, AllocationUnitBitMap);
  PRINT_BIT_FIELD (Ecx, Liner);
  PRINT_BIT_FIELD (Edx, HighestCosNumber);
}

/**
  Display CPUID_INTEL_RDT_ALLOCATION L3 cache allocation technology enumeration
  sub-leaf.

**/
VOID
CpuidIntelRdtAllocationL3CacheSubLeaf (
  VOID
  )
{
  CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EAX  Eax;
  UINT32                                            Ebx;
  CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_ECX  Ecx;
  CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION,
    CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF,
    &Eax.Uint32,
    &Ebx,
    &Ecx.Uint32,
    &Edx.Uint32
    );
  Print (L"CPUID_INTEL_RDT_ALLOCATION (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx, Ecx.Uint32, Edx.Uint32);
  PRINT_BIT_FIELD (Eax, CapacityLength);
  PRINT_VALUE (Ebx, AllocationUnitBitMap);
  PRINT_BIT_FIELD (Ecx, CodeDataPrioritization);
  PRINT_BIT_FIELD (Edx, HighestCosNumber);
}

/**
  Display CPUID_INTEL_RDT_ALLOCATION L2 cache allocation technology enumeration
  sub-leaf.

**/
VOID
CpuidIntelRdtAllocationL2CacheSubLeaf (
  VOID
  )
{
  CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EAX  Eax;
  UINT32                                            Ebx;
  CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION,
    CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF,
    &Eax.Uint32,
    &Ebx,
    NULL,
    &Edx.Uint32
    );
  Print (L"CPUID_INTEL_RDT_ALLOCATION (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx, 0, Edx.Uint32);
  PRINT_BIT_FIELD (Eax, CapacityLength);
  PRINT_VALUE (Ebx, AllocationUnitBitMap);
  PRINT_BIT_FIELD (Edx, HighestCosNumber);
}

/**
  Display CPUID_INTEL_RDT_ALLOCATION main leaf and sub-leaves.

**/
VOID
CpuidIntelRdtAllocationMainLeaf (
  VOID
  )
{
  CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF_EBX  Ebx;

  if (CPUID_INTEL_RDT_ALLOCATION > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION,
    CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF,
    NULL,
    &Ebx.Uint32,
    NULL,
    NULL
    );
  Print (L"CPUID_INTEL_RDT_ALLOCATION (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", 0, Ebx.Uint32, 0, 0);
  PRINT_BIT_FIELD (Ebx, L3CacheAllocation);
  PRINT_BIT_FIELD (Ebx, L2CacheAllocation);
  PRINT_BIT_FIELD (Ebx, MemoryBandwidth);
  CpuidIntelRdtAllocationMemoryBandwidthSubLeaf ();
  CpuidIntelRdtAllocationL3CacheSubLeaf ();
  CpuidIntelRdtAllocationL2CacheSubLeaf ();
}

/**
  Display Sub-Leaf 0 Enumeration of Intel SGX Capabilities.

**/
VOID
CpuidEnumerationOfIntelSgxCapabilities0SubLeaf (
  VOID
  )
{
  CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EAX  Eax;
  UINT32                                       Ebx;
  CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_SGX,
    CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF,
    &Eax.Uint32,
    &Ebx,
    NULL,
    &Edx.Uint32
    );
  Print (L"CPUID_INTEL_SGX (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_SGX, CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx, 0, Edx.Uint32);
  PRINT_BIT_FIELD (Eax, SGX1);
  PRINT_BIT_FIELD (Eax, SGX2);
  PRINT_BIT_FIELD (Eax, ENCLV);
  PRINT_BIT_FIELD (Eax, ENCLS);
  PRINT_BIT_FIELD (Edx, MaxEnclaveSize_Not64);
  PRINT_BIT_FIELD (Edx, MaxEnclaveSize_64);
}

/**
  Display Sub-Leaf 1 Enumeration of Intel SGX Capabilities.

**/
VOID
CpuidEnumerationOfIntelSgxCapabilities1SubLeaf (
  VOID
  )
{
  UINT32  Eax;
  UINT32  Ebx;
  UINT32  Ecx;
  UINT32  Edx;

  AsmCpuidEx (
    CPUID_INTEL_SGX,
    CPUID_INTEL_SGX_CAPABILITIES_1_SUB_LEAF,
    &Eax,
    &Ebx,
    &Ecx,
    &Edx
    );
  Print (L"CPUID_INTEL_SGX (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_SGX, CPUID_INTEL_SGX_CAPABILITIES_1_SUB_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx, Ecx, Edx);
}

/**
  Display Sub-Leaf Index 2 or Higher Enumeration of Intel SGX Resources.

**/
VOID
CpuidEnumerationOfIntelSgxResourcesSubLeaf (
  VOID
  )
{
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EAX  Eax;
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EBX  Ebx;
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_ECX  Ecx;
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EDX  Edx;
  UINT32                                               SubLeaf;

  SubLeaf = CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF;
  do {
    AsmCpuidEx (
      CPUID_INTEL_SGX,
      SubLeaf,
      &Eax.Uint32,
      &Ebx.Uint32,
      &Ecx.Uint32,
      &Edx.Uint32
      );
    if (Eax.Bits.SubLeafType == 0x1) {
      Print (L"CPUID_INTEL_SGX (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_SGX, SubLeaf);
      Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
      PRINT_BIT_FIELD (Eax, SubLeafType);
      PRINT_BIT_FIELD (Eax, LowAddressOfEpcSection);
      PRINT_BIT_FIELD (Ebx, HighAddressOfEpcSection);
      PRINT_BIT_FIELD (Ecx, EpcSection);
      PRINT_BIT_FIELD (Ecx, LowSizeOfEpcSection);
      PRINT_BIT_FIELD (Edx, HighSizeOfEpcSection);
    }

    SubLeaf++;
  } while (Eax.Bits.SubLeafType == 0x1);
}

/**
  Display Intel SGX Resource Enumeration.

**/
VOID
CpuidEnumerationOfIntelSgx (
  VOID
  )
{
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX  Ebx;

  if (CPUID_INTEL_SGX > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO,
    NULL,
    &Ebx.Uint32,
    NULL,
    NULL
    );
  if (Ebx.Bits.SGX != 1) {
    //
    // Only if CPUID.(EAX=07H, ECX=0H):EBX.SGX = 1, the processor has support
    // for Intel SGX.
    //
    return;
  }

  CpuidEnumerationOfIntelSgxCapabilities0SubLeaf ();
  CpuidEnumerationOfIntelSgxCapabilities1SubLeaf ();
  CpuidEnumerationOfIntelSgxResourcesSubLeaf ();
}

/**
  Display CPUID_INTEL_PROCESSOR_TRACE sub-leafs.

  @param[in] MaximumSubLeaf  Maximum sub-leaf index for CPUID_INTEL_PROCESSOR_TRACE.

**/
VOID
CpuidIntelProcessorTraceSubLeaf (
  UINT32  MaximumSubLeaf
  )
{
  UINT32                                    SubLeaf;
  CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EAX  Eax;
  CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EBX  Ebx;

  for (SubLeaf = CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF; SubLeaf <= MaximumSubLeaf; SubLeaf++) {
    AsmCpuidEx (
      CPUID_INTEL_PROCESSOR_TRACE,
      SubLeaf,
      &Eax.Uint32,
      &Ebx.Uint32,
      NULL,
      NULL
      );
    Print (L"CPUID_INTEL_PROCESSOR_TRACE (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_PROCESSOR_TRACE, SubLeaf);
    Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, 0, 0);
    PRINT_BIT_FIELD (Eax, ConfigurableAddressRanges);
    PRINT_BIT_FIELD (Eax, MtcPeriodEncodings);
    PRINT_BIT_FIELD (Ebx, CycleThresholdEncodings);
    PRINT_BIT_FIELD (Ebx, PsbFrequencyEncodings);
  }
}

/**
  Display CPUID_INTEL_PROCESSOR_TRACE main leaf and sub-leafs.

**/
VOID
CpuidIntelProcessorTraceMainLeaf (
  VOID
  )
{
  UINT32                                     Eax;
  CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_EBX  Ebx;
  CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_ECX  Ecx;

  if (CPUID_INTEL_PROCESSOR_TRACE > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_INTEL_PROCESSOR_TRACE,
    CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF,
    &Eax,
    &Ebx.Uint32,
    &Ecx.Uint32,
    NULL
    );
  Print (L"CPUID_INTEL_PROCESSOR_TRACE (Leaf %08x, Sub-Leaf %08x)\n", CPUID_INTEL_PROCESSOR_TRACE, CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx.Uint32, Ecx.Uint32, 0);
  PRINT_VALUE (Eax, MaximumSubLeaf);
  PRINT_BIT_FIELD (Ebx, Cr3Filter);
  PRINT_BIT_FIELD (Ebx, ConfigurablePsb);
  PRINT_BIT_FIELD (Ebx, IpTraceStopFiltering);
  PRINT_BIT_FIELD (Ebx, Mtc);
  PRINT_BIT_FIELD (Ebx, PTWrite);
  PRINT_BIT_FIELD (Ebx, PowerEventTrace);
  PRINT_BIT_FIELD (Ecx, RTIT);
  PRINT_BIT_FIELD (Ecx, ToPA);
  PRINT_BIT_FIELD (Ecx, SingleRangeOutput);
  PRINT_BIT_FIELD (Ecx, TraceTransportSubsystem);
  PRINT_BIT_FIELD (Ecx, LIP);

  CpuidIntelProcessorTraceSubLeaf (Eax);
}

/**
  Display CPUID_TIME_STAMP_COUNTER leaf.

**/
VOID
CpuidTimeStampCounter (
  VOID
  )
{
  UINT32  Eax;
  UINT32  Ebx;
  UINT32  Ecx;

  if (CPUID_TIME_STAMP_COUNTER > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_TIME_STAMP_COUNTER, &Eax, &Ebx, &Ecx, NULL);
  Print (L"CPUID_TIME_STAMP_COUNTER (Leaf %08x)\n", CPUID_TIME_STAMP_COUNTER);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx, Ecx, 0);
}

/**
  Display CPUID_PROCESSOR_FREQUENCY leaf.

**/
VOID
CpuidProcessorFrequency (
  VOID
  )
{
  CPUID_PROCESSOR_FREQUENCY_EAX  Eax;
  CPUID_PROCESSOR_FREQUENCY_EBX  Ebx;
  CPUID_PROCESSOR_FREQUENCY_ECX  Ecx;

  if (CPUID_PROCESSOR_FREQUENCY > gMaximumBasicFunction) {
    return;
  }

  AsmCpuid (CPUID_PROCESSOR_FREQUENCY, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, NULL);
  Print (L"CPUID_PROCESSOR_FREQUENCY (Leaf %08x)\n", CPUID_PROCESSOR_FREQUENCY);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, 0);
  PRINT_BIT_FIELD (Eax, ProcessorBaseFrequency);
  PRINT_BIT_FIELD (Ebx, MaximumFrequency);
  PRINT_BIT_FIELD (Ecx, BusFrequency);
}

/**
  Display CPUID_SOC_VENDOR sub-leafs that contain the SoC Vendor Brand String.
  Also display these sub-leafs as a single SoC Vendor Brand String.

**/
VOID
CpuidSocVendorBrandString (
  VOID
  )
{
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Eax;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ebx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ecx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Edx;
  //
  // Array to store brand string from 3 brand string leafs with
  // 4 32-bit brand string values per leaf and an extra value to
  // null terminate the string.
  //
  UINT32  BrandString[3 * 4 + 1];

  AsmCpuidEx (
    CPUID_SOC_VENDOR,
    CPUID_SOC_VENDOR_BRAND_STRING1,
    &Eax.Uint32,
    &Ebx.Uint32,
    &Ecx.Uint32,
    &Edx.Uint32
    );
  Print (L"CPUID_SOC_VENDOR (Leaf %08x, Sub-Leaf %08x)\n", CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_BRAND_STRING1);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
  BrandString[0] = Eax.Uint32;
  BrandString[1] = Ebx.Uint32;
  BrandString[2] = Ecx.Uint32;
  BrandString[3] = Edx.Uint32;

  AsmCpuidEx (
    CPUID_SOC_VENDOR,
    CPUID_SOC_VENDOR_BRAND_STRING2,
    &Eax.Uint32,
    &Ebx.Uint32,
    &Ecx.Uint32,
    &Edx.Uint32
    );
  Print (L"CPUID_SOC_VENDOR (Leaf %08x, Sub-Leaf %08x)\n", CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_BRAND_STRING2);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
  BrandString[4] = Eax.Uint32;
  BrandString[5] = Ebx.Uint32;
  BrandString[6] = Ecx.Uint32;
  BrandString[7] = Edx.Uint32;

  AsmCpuidEx (
    CPUID_SOC_VENDOR,
    CPUID_SOC_VENDOR_BRAND_STRING3,
    &Eax.Uint32,
    &Ebx.Uint32,
    &Ecx.Uint32,
    &Edx.Uint32
    );
  Print (L"CPUID_SOC_VENDOR (Leaf %08x, Sub-Leaf %08x)\n", CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_BRAND_STRING3);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
  BrandString[8]  = Eax.Uint32;
  BrandString[9]  = Ebx.Uint32;
  BrandString[10] = Ecx.Uint32;
  BrandString[11] = Edx.Uint32;

  BrandString[12] = 0;

  Print (L"Vendor Brand String = %a\n", (CHAR8 *)BrandString);
}

/**
  Display CPUID_SOC_VENDOR main leaf and sub-leafs.

**/
VOID
CpuidSocVendor (
  VOID
  )
{
  UINT32                          Eax;
  CPUID_SOC_VENDOR_MAIN_LEAF_EBX  Ebx;
  UINT32                          Ecx;
  UINT32                          Edx;

  if (CPUID_SOC_VENDOR > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_SOC_VENDOR,
    CPUID_SOC_VENDOR_MAIN_LEAF,
    &Eax,
    &Ebx.Uint32,
    &Ecx,
    &Edx
    );
  Print (L"CPUID_SOC_VENDOR (Leaf %08x, Sub-Leaf %08x)\n", CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_MAIN_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx.Uint32, Ecx, Edx);
  if (Eax < 3) {
    Print (L"  Not Supported\n");
    return;
  }

  PRINT_VALUE (Eax, MaxSOCID_Index);
  PRINT_BIT_FIELD (Ebx, SocVendorId);
  PRINT_BIT_FIELD (Ebx, IsVendorScheme);
  PRINT_VALUE (Ecx, ProjectID);
  PRINT_VALUE (Edx, SteppingID);
  CpuidSocVendorBrandString ();
}

/**
  Display CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS main leaf and sub-leafs.

**/
VOID
CpuidDeterministicAddressTranslationParameters (
  VOID
  )
{
  UINT32                                                  Eax;
  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EBX  Ebx;
  UINT32                                                  Ecx;
  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EDX  Edx;

  if (CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS > gMaximumBasicFunction) {
    return;
  }

  AsmCpuidEx (
    CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS,
    CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_MAIN_LEAF,
    &Eax,
    &Ebx.Uint32,
    &Ecx,
    &Edx.Uint32
    );
  Print (L"CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS (Leaf %08x, Sub-Leaf %08x)\n", CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS, CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_MAIN_LEAF);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, Ebx.Uint32, Ecx, Edx.Uint32);

  PRINT_VALUE (Eax, MaxID_Index);
  PRINT_BIT_FIELD (Ebx, Page4K);
  PRINT_BIT_FIELD (Ebx, Page2M);
  PRINT_BIT_FIELD (Ebx, Page4M);
  PRINT_BIT_FIELD (Ebx, Page1G);
  PRINT_BIT_FIELD (Ebx, Partitioning);
  PRINT_BIT_FIELD (Ebx, Way);

  PRINT_VALUE (Ecx, NumberOfSets);

  PRINT_BIT_FIELD (Edx, TranslationCacheType);
  PRINT_BIT_FIELD (Edx, TranslationCacheLevel);
  PRINT_BIT_FIELD (Edx, FullyAssociative);
  PRINT_BIT_FIELD (Edx, MaximumNum);
}

/**
  Display CPUID_EXTENDED_FUNCTION leaf.

**/
VOID
CpuidExtendedFunction (
  VOID
  )
{
  UINT32  Eax;

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &Eax, NULL, NULL, NULL);
  Print (L"CPUID_EXTENDED_FUNCTION (Leaf %08x)\n", CPUID_EXTENDED_FUNCTION);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, 0, 0, 0);
  PRINT_VALUE (Eax, MaximumExtendedFunction);

  gMaximumExtendedFunction = Eax;
}

/**
  Display CPUID_EXTENDED_CPU_SIG leaf.

**/
VOID
CpuidExtendedCpuSig (
  VOID
  )
{
  UINT32                      Eax;
  CPUID_EXTENDED_CPU_SIG_ECX  Ecx;
  CPUID_EXTENDED_CPU_SIG_EDX  Edx;

  if (CPUID_EXTENDED_CPU_SIG > gMaximumExtendedFunction) {
    return;
  }

  AsmCpuid (CPUID_EXTENDED_CPU_SIG, &Eax, NULL, &Ecx.Uint32, &Edx.Uint32);
  Print (L"CPUID_EXTENDED_CPU_SIG (Leaf %08x)\n", CPUID_EXTENDED_CPU_SIG);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax, 0, Ecx.Uint32, Edx.Uint32);
  PRINT_BIT_FIELD (Ecx, LAHF_SAHF);
  PRINT_BIT_FIELD (Ecx, LZCNT);
  PRINT_BIT_FIELD (Ecx, PREFETCHW);
  PRINT_BIT_FIELD (Edx, SYSCALL_SYSRET);
  PRINT_BIT_FIELD (Edx, NX);
  PRINT_BIT_FIELD (Edx, Page1GB);
  PRINT_BIT_FIELD (Edx, RDTSCP);
  PRINT_BIT_FIELD (Edx, LM);
}

/**
  Display CPUID_BRAND_STRING1, CPUID_BRAND_STRING2 and  CPUID_BRAND_STRING3
  leafs.  Also display these three leafs as a single brand string.

**/
VOID
CpuidProcessorBrandString (
  VOID
  )
{
  CPUID_BRAND_STRING_DATA  Eax;
  CPUID_BRAND_STRING_DATA  Ebx;
  CPUID_BRAND_STRING_DATA  Ecx;
  CPUID_BRAND_STRING_DATA  Edx;
  //
  // Array to store brand string from 3 brand string leafs with
  // 4 32-bit brand string values per leaf and an extra value to
  // null terminate the string.
  //
  UINT32  BrandString[3 * 4 + 1];

  if (CPUID_BRAND_STRING1 <= gMaximumExtendedFunction) {
    AsmCpuid (CPUID_BRAND_STRING1, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
    Print (L"CPUID_BRAND_STRING1 (Leaf %08x)\n", CPUID_BRAND_STRING1);
    Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
    BrandString[0] = Eax.Uint32;
    BrandString[1] = Ebx.Uint32;
    BrandString[2] = Ecx.Uint32;
    BrandString[3] = Edx.Uint32;
  }

  if (CPUID_BRAND_STRING2 <= gMaximumExtendedFunction) {
    AsmCpuid (CPUID_BRAND_STRING2, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
    Print (L"CPUID_BRAND_STRING2 (Leaf %08x)\n", CPUID_BRAND_STRING2);
    Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
    BrandString[4] = Eax.Uint32;
    BrandString[5] = Ebx.Uint32;
    BrandString[6] = Ecx.Uint32;
    BrandString[7] = Edx.Uint32;
  }

  if (CPUID_BRAND_STRING3 <= gMaximumExtendedFunction) {
    AsmCpuid (CPUID_BRAND_STRING3, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
    Print (L"CPUID_BRAND_STRING3 (Leaf %08x)\n", CPUID_BRAND_STRING3);
    Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, Ebx.Uint32, Ecx.Uint32, Edx.Uint32);
    BrandString[8]  = Eax.Uint32;
    BrandString[9]  = Ebx.Uint32;
    BrandString[10] = Ecx.Uint32;
    BrandString[11] = Edx.Uint32;
  }

  BrandString[12] = 0;

  Print (L"Brand String = %a\n", (CHAR8 *)BrandString);
}

/**
  Display CPUID_EXTENDED_CACHE_INFO leaf.

**/
VOID
CpuidExtendedCacheInfo (
  VOID
  )
{
  CPUID_EXTENDED_CACHE_INFO_ECX  Ecx;

  if (CPUID_EXTENDED_CACHE_INFO > gMaximumExtendedFunction) {
    return;
  }

  AsmCpuid (CPUID_EXTENDED_CACHE_INFO, NULL, NULL, &Ecx.Uint32, NULL);
  Print (L"CPUID_EXTENDED_CACHE_INFO (Leaf %08x)\n", CPUID_EXTENDED_CACHE_INFO);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", 0, 0, Ecx.Uint32, 0);
  PRINT_BIT_FIELD (Ecx, CacheLineSize);
  PRINT_BIT_FIELD (Ecx, L2Associativity);
  PRINT_BIT_FIELD (Ecx, CacheSize);
}

/**
  Display CPUID_EXTENDED_TIME_STAMP_COUNTER leaf.

**/
VOID
CpuidExtendedTimeStampCounter (
  VOID
  )
{
  CPUID_EXTENDED_TIME_STAMP_COUNTER_EDX  Edx;

  if (CPUID_EXTENDED_TIME_STAMP_COUNTER > gMaximumExtendedFunction) {
    return;
  }

  AsmCpuid (CPUID_EXTENDED_TIME_STAMP_COUNTER, NULL, NULL, NULL, &Edx.Uint32);
  Print (L"CPUID_EXTENDED_TIME_STAMP_COUNTER (Leaf %08x)\n", CPUID_EXTENDED_TIME_STAMP_COUNTER);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", 0, 0, 0, Edx.Uint32);
  PRINT_BIT_FIELD (Edx, InvariantTsc);
}

/**
  Display CPUID_VIR_PHY_ADDRESS_SIZE leaf.

**/
VOID
CpuidVirPhyAddressSize (
  VOID
  )
{
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  Eax;

  if (CPUID_VIR_PHY_ADDRESS_SIZE > gMaximumExtendedFunction) {
    return;
  }

  AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &Eax.Uint32, NULL, NULL, NULL);
  Print (L"CPUID_VIR_PHY_ADDRESS_SIZE (Leaf %08x)\n", CPUID_VIR_PHY_ADDRESS_SIZE);
  Print (L"  EAX:%08x  EBX:%08x  ECX:%08x  EDX:%08x\n", Eax.Uint32, 0, 0, 0);
  PRINT_BIT_FIELD (Eax, PhysicalAddressBits);
  PRINT_BIT_FIELD (Eax, LinearAddressBits);
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print (L"UEFI CPUID Version 0.5\n");

  CpuidSignature ();
  CpuidVersionInfo ();
  CpuidCacheInfo ();
  CpuidSerialNumber ();
  CpuidCacheParams ();
  CpuidMonitorMwait ();
  CpuidThermalPowerManagement ();
  CpuidStructuredExtendedFeatureFlags ();
  CpuidDirectCacheAccessInfo ();
  CpuidArchitecturalPerformanceMonitoring ();
  CpuidExtendedTopology (CPUID_EXTENDED_TOPOLOGY);
  CpuidExtendedStateMainLeaf ();
  CpuidIntelRdtMonitoringEnumerationSubLeaf ();
  CpuidIntelRdtMonitoringL3CacheCapabilitySubLeaf ();
  CpuidIntelRdtAllocationMainLeaf ();
  CpuidEnumerationOfIntelSgx ();
  CpuidIntelProcessorTraceMainLeaf ();
  CpuidTimeStampCounter ();
  CpuidProcessorFrequency ();
  CpuidSocVendor ();
  CpuidDeterministicAddressTranslationParameters ();
  CpuidExtendedTopology (CPUID_V2_EXTENDED_TOPOLOGY);
  CpuidExtendedFunction ();
  CpuidExtendedCpuSig ();
  CpuidProcessorBrandString ();
  CpuidExtendedCacheInfo ();
  CpuidExtendedTimeStampCounter ();
  CpuidVirPhyAddressSize ();

  return EFI_SUCCESS;
}
