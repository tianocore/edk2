/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

    BoardFeatures.h

Abstract:

    EFI Platform Board Features



--*/

#ifndef BoardFeatures_h_included
#define BoardFeatures_h_included

#include <Base.h>

#pragma pack(1)

//
// Board Features
//
#if defined LEGACY_BOARD_FEATURES && LEGACY_BOARD_FEATURES
#define B_BOARD_FEATURES_CHIPSET_LAN           BIT0
#define B_BOARD_FEATURES_LAN_MARVELL           BIT1
#define B_BOARD_FEATURES_AA_NOT_FOUND           BIT2
#define B_BOARD_FEATURES_SIO_NO_COM1           BIT3
#define B_BOARD_FEATURES_SIO_COM2              BIT4
#define B_BOARD_FEATURES_SIO_NO_PARALLEL       BIT5
#define B_BOARD_FEATURES_CHIPSET_VIDEO         BIT6
#define B_BOARD_FEATURES_CHIPSET_VIDEO_OPTION0 BIT7
#define B_BOARD_FEATURES_VIDEO_SLOT            BIT8
#define B_BOARD_FEATURES_MINI_CARD             BIT9
#define B_BOARD_FEATURES_DISCRETE_1394         BIT10
#define B_BOARD_FEATURES_LEGACY_FREE           BIT11
#define B_BOARD_FEATURES_USB_HUB               BIT12
#define B_BOARD_FEATURES_TPM                   BIT13
#define B_BOARD_FEATURES_VIIV                   BIT14
#define B_BOARD_FEATURES_FORM_FACTOR_MASK      (BIT15|BIT16|BIT17|BIT18|BIT19)
#define B_BOARD_FEATURES_FORM_FACTOR_PBTX      BIT15
#define B_BOARD_FEATURES_FORM_FACTOR_ATX       BIT16
#define B_BOARD_FEATURES_FORM_FACTOR_BTX       BIT17
#define B_BOARD_FEATURES_FORM_FACTOR_MICRO_ATX BIT18
#define B_BOARD_FEATURES_FORM_FACTOR_MICRO_BTX BIT19
#define B_BOARD_FEATURES_MEMORY_TYPE_DDR1      BIT20
#define B_BOARD_FEATURES_MEMORY_TYPE_DDR2      BIT21
#define B_BOARD_FEATURES_MEMORY_SLOT_MASK      BIT23 | BIT22
#define   V_BOARD_FEATURES_1_MEMORY_SLOT         0              // BIT22=0, BIT23=0
#define   V_BOARD_FEATURES_2_MEMORY_SLOT         BIT22          // BIT22=1, BIT23=0
#define   V_BOARD_FEATURES_3_MEMORY_SLOT         BIT23          // BIT22=0, BIT23=1
#define   V_BOARD_FEATURES_4_MEMORY_SLOT         BIT23 | BIT22  // BIT22=1, BIT23=1
#define B_BOARD_FEATURES_ALT_MEM_CLK_RT         BIT24
#define B_BOARD_FEATURES_SLEEP_MASK            BIT25
#define   V_BOARD_FEATURES_SLEEP_S1              0              // BIT25=0
#define   V_BOARD_FEATURES_SLEEP_S3              BIT25          // BIT25=1
#define B_BOARD_FEATURES_3JACK_AUDIO_SOLUTION    BIT26          // 0/1= 5/3 Rear Jacks
#define B_BOARD_FEATURES_DISCRETE_SATA         BIT27
#define B_BOARD_FEATURES_2_SATA               BIT28    // 2SATA instead of 4(pre Ich8) or 4 SATA instead of 6(Ich8)
#define B_BOARD_FEATURES_RVP                  BIT29    // Board is an RVP board
#define B_BOARD_FEATURES_PORT80_LPC           BIT30    // Port80 PCI(0) or LPC(1)
#define B_BOARD_FEATURES_LIMITED_CPU_SUPPORT  BIT31    // Limited CPU support
#define B_BOARD_FEATURES_PMP_SUPPORT          BIT32    // Support for over-voltaging memory
#define B_BOARD_FEATURES_HW_WATCHDOG_TIMER    BIT33    // Support for the HW-based 555 Watchdog Timer feature
#define B_BOARD_FEATURES_NO_QRT               BIT34    // disable QRT
#define B_BOARD_FEATURES_VERB_TABLE1          BIT35    // Verb table 1
#define B_BOARD_FEATURES_VERB_TABLE2          BIT36    // Verb table 2
#define B_BOARD_FEATURES_VERB_TABLE3          BIT37    // Verb table 3
#define B_BOARD_FEATURES_VERB_TABLE4          BIT38    // Verb table 4
#define B_BOARD_FEATURES_VERB_TABLE5          BIT39    // Reserved for Verb table 5
#define B_BOARD_FEATURES_VERB_TABLE_MASK      BIT35 | BIT36 | BIT37 | BIT38 | BIT39
#define B_BOARD_FEATURES_KENTSFIELD_BLOCK     BIT40    // Kentsfield not supported
#define B_BOARD_FEATURES_KENTSFIELD_WARNING   BIT41    // Kentsfield warning
#define B_BOARD_FEATURES_ESATA_PORT0          BIT42    // E-SATA on Port0
#define B_BOARD_FEATURES_ESATA_PORT1          BIT43    // E-SATA on Port1
#define B_BOARD_FEATURES_ESATA_PORT2          BIT44    // E-SATA on Port2
#define B_BOARD_FEATURES_ESATA_PORT3          BIT45    // E-SATA on Port3
#define B_BOARD_FEATURES_ESATA_PORT4          BIT46    // E-SATA on Port4
#define B_BOARD_FEATURES_ESATA_PORT5          BIT47    // E-SATA on Port5
#define B_BOARD_FEATURES_ECIR                 BIT48    // Enhanced Consumer IR
#define B_BOARD_FEATURES_PS2WAKEFROMS5        BIT49    // Wake from S5 via PS2 keyboard
#define B_BOARD_FEATURES_HDAUDIOLINK          BIT50    // HD audio link support
#define B_BOARD_FEATURES_1_PATA               BIT51
#define B_BOARD_FEATURES_MOBILE               BIT52
#define B_BOARD_FEATURES_NO_FLOPPY            BIT53
#define B_BOARD_FEATURES_DISABLE_UNUSED_FSB   BIT54

//
// Bit 55-58 reserved by PSID support.  CPU power requirement below are preliminary.
// They might be changed.
// This is not same as 8.6.1 products so be careful.
//
#define B_BOARD_FEATURES_CPU_POWER_BITNUM     55
#define B_BOARD_FEATURES_CPU_POWER_MASK       (BIT55 | BIT56 | BIT57 | BIT58)
#define B_BOARD_FEATURES_CPU_POWER_35W        0     // Theoretically doesn't exist.
#define B_BOARD_FEATURES_CPU_POWER_40W        BIT55 // 0001
#define B_BOARD_FEATURES_CPU_POWER_45W        BIT56 // 0010
#define B_BOARD_FEATURES_CPU_POWER_50W        (BIT55 | BIT56) // 0011
#define B_BOARD_FEATURES_CPU_POWER_65W        BIT57 // 0100 Wolfdale-H/-M
#define B_BOARD_FEATURES_CPU_POWER_70W        (BIT55 | BIT57) // 0101
#define B_BOARD_FEATURES_CPU_POWER_75W        (BIT56 | BIT57) // 0110
#define B_BOARD_FEATURES_CPU_POWER_80W        (BIT55 | BIT56 | BIT57) // 0111
#define B_BOARD_FEATURES_CPU_POWER_95W        BIT58 // 1000 Yorkfield
#define B_BOARD_FEATURES_CPU_POWER_100W       (BIT55 | BIT58) // 1001
#define B_BOARD_FEATURES_CPU_POWER_105W       (BIT56 | BIT58) // 1010
#define B_BOARD_FEATURES_CPU_POWER_110W       (BIT55 | BIT56 | BIT58) // 1011
#define B_BOARD_FEATURES_CPU_POWER_130W       (BIT57 | BIT58) // 1100 XE Yorkfield
#define B_BOARD_FEATURES_CPU_POWER_135W       (BIT55 | BIT57 | BIT58) // 1101
#define B_BOARD_FEATURES_CPU_POWER_Over135W   (BIT56 | BIT57 | BIT58) // 1110 Reserved
#define B_BOARD_FEATURES_CPU_POWER_140W       (BIT55 | BIT56 | BIT57 | BIT58) // 1111 Reserved
#define B_VV_BOARD_FEATURES                    BIT59
#define B_BOARD_FEATURES_IDCC2_SUPPORT         BIT60 // Include IDCC2 support
#define B_BOARD_FEATURES_NO_SATA_PORT2_3       BIT61 // No SATA Port2&3 Connector, used with B_BOARD_FEATURES_2_SATA flag
#define B_BOARD_FEATURES_FORM_FACTOR_MINI_ITX  BIT62
#define B_BOARD_FEATURES_NPI_QPI_VOLTAGE       BIT63

#else

#define B_BOARD_FEATURES_CHIPSET_LAN              BIT0
#define B_BOARD_FEATURES_CHIPSET_VIDEO            BIT1
#define B_BOARD_FEATURES_VIDEO_SLOT               BIT2
#define B_BOARD_FEATURES_AA_NOT_FOUND             BIT3
#define B_BOARD_FEATURES_SIO_NO_COM1              BIT4
#define B_BOARD_FEATURES_SIO_COM2                 BIT5
#define B_BOARD_FEATURES_SIO_NO_PARALLEL          BIT6
#define B_BOARD_FEATURES_NO_FLOPPY                BIT7
#define B_BOARD_FEATURES_PS2WAKEFROMS5            BIT8             // Wake from S5 via PS2 keyboard
#define B_BOARD_FEATURES_ECIR                     BIT9             // Enhanced Consumer IR
#define B_BOARD_FEATURES_LEGACY_FREE              BIT10
#define B_BOARD_FEATURES_MINI_CARD                BIT11
#define B_BOARD_FEATURES_DISCRETE_1394            BIT12
#define B_BOARD_FEATURES_USB_HUB                  BIT13
#define B_BOARD_FEATURES_TPM                      BIT14
#define B_BOARD_FEATURES_FORM_FACTOR_MASK         (BIT15|BIT16|BIT17|BIT18|BIT19|BIT20)
#define B_BOARD_FEATURES_FORM_FACTOR_PBTX         BIT15
#define B_BOARD_FEATURES_FORM_FACTOR_ATX          BIT16
#define B_BOARD_FEATURES_FORM_FACTOR_BTX          BIT17
#define B_BOARD_FEATURES_FORM_FACTOR_MICRO_ATX    BIT18
#define B_BOARD_FEATURES_FORM_FACTOR_MICRO_BTX    BIT19
#define B_BOARD_FEATURES_FORM_FACTOR_MINI_ITX     BIT20
#define B_BOARD_FEATURES_MEMORY_TYPE_DDR2         BIT21
#define B_BOARD_FEATURES_MEMORY_TYPE_DDR3         BIT22
#define B_BOARD_FEATURES_MEMORY_SLOT_MASK         (BIT24 | BIT23)
#define   V_BOARD_FEATURES_1_MEMORY_SLOT            0              // BIT23=0, BIT24=0
#define   V_BOARD_FEATURES_2_MEMORY_SLOT          BIT23            // BIT23=1, BIT24=0
#define   V_BOARD_FEATURES_3_MEMORY_SLOT          BIT24            // BIT23=0, BIT24=1
#define   V_BOARD_FEATURES_4_MEMORY_SLOT          (BIT24 | BIT23)  // BIT23=1, BIT24=1
#define B_BOARD_FEATURES_2_C0_MEMORY_SLOT         BIT25            // 2 Channel 0 memory slot
#define B_BOARD_FEATURES_SLEEP_MASK               BIT26
#define   V_BOARD_FEATURES_SLEEP_S1                 0              // BIT26=0
#define   V_BOARD_FEATURES_SLEEP_S3               BIT26            // BIT26=1
#define B_BOARD_FEATURES_3JACK_AUDIO_SOLUTION     BIT27            // 0/1= 5/3 Rear Jacks
#define B_BOARD_FEATURES_HDAUDIOLINK              BIT28            // HD audio link support
#define B_BOARD_FEATURES_DISCRETE_SATA            BIT29
#define B_BOARD_FEATURES_2_SATA                   BIT30            // 2SATA instead of 4(pre Ich8) or 4 SATA instead of 6(Ich8)
#define B_BOARD_FEATURES_NO_SATA_PORT2_3          BIT31            // No SATA Port2&3 Connector, used with B_BOARD_FEATURES_2_SATA flag
#define B_BOARD_FEATURES_RVP                      BIT32            // Board is an RVP board
#define B_BOARD_FEATURES_ESATA_PORT0              BIT33            // E-SATA on Port0
#define B_BOARD_FEATURES_ESATA_PORT1              BIT34            // E-SATA on Port1
#define B_BOARD_FEATURES_ESATA_PORT2              BIT35            // E-SATA on Port2
#define B_BOARD_FEATURES_ESATA_PORT3              BIT36            // E-SATA on Port3
#define B_BOARD_FEATURES_ESATA_PORT4              BIT37            // E-SATA on Port4
#define B_BOARD_FEATURES_ESATA_PORT5              BIT38            // E-SATA on Port5
#define B_BOARD_FEATURES_IDCC2_SUPPORT            BIT39            // Include IDCC2 support
#define B_BOARD_FEATURES_NPI_QPI_VOLTAGE          BIT40
#define B_BOARD_FEATURES_LIMITED_CPU_SUPPORT      BIT41            // Limited CPU support
#define B_BOARD_FEATURES_PMP_SUPPORT              BIT42            // Support for over-voltaging memory
#define B_BOARD_FEATURES_HW_WATCHDOG_TIMER        BIT43            // Support for the HW-based 555 Watchdog Timer feature
#define B_BOARD_FEATURES_LVDS                     BIT44            // Support for LVDS
#define B_BOARD_FEATURES_VERB_TABLE_MASK          (BIT45|BIT46|BIT47|BIT48)    // Verb table
#define B_BOARD_FEATURES_VERB_TABLE1              BIT45            // Verb table 1
#define B_BOARD_FEATURES_VERB_TABLE2              BIT46            // Verb table 2
#define B_BOARD_FEATURES_VERB_TABLE3              BIT47            // Verb table 3
#define B_BOARD_FEATURES_VERB_TABLE4              BIT48            // Verb table 4
#define B_BOARD_FEATURES_NO_MINIPCIE              BIT49            // Mini PCIe slot
#define B_BOARD_FEATURES_HDMI_SLOT                BIT50            // HDMI slot
#define B_BOARD_FEATURES_PS2_HIDE                 BIT51            // PS2 hide
#define B_BOARD_FEATURES_DVID_SLOT                BIT52            // DVID slot

#define B_BOARD_FEATURES_SIO_COM3                 BIT53
#define B_BOARD_FEATURES_SIO_COM4                 BIT54

#define B_BOARD_FEATURES_LAN2                     BIT55
#define B_BOARD_FEATURES_PCIe_SLOT                BIT56
#endif

typedef UINT64 EFI_BOARD_FEATURES;

#pragma pack()

//
// Global ID for the Platform Boot Mode Protocol.
//
#define EFI_BOARD_FEATURES_GUID \
  { 0x94b9e8ae, 0x8877, 0x479a, 0x98, 0x42, 0xf5, 0x97, 0x4b, 0x82, 0xce, 0xd3 }

extern EFI_GUID gEfiBoardFeaturesGuid;

#define BOARD_FEATURES_NAME   L"BoardFeatures"

#define EFI_BOARD_ID_GUID \
  { 0x6b2dd245, 0x3f2, 0x414a, 0x8c, 0x2, 0x9f, 0xfc, 0x23, 0x52, 0xe3, 0x1e }
#define EFI_BOARD_ID_NAME (L"BoardId")

#endif

