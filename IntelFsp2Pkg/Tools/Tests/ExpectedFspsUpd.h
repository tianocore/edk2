#ifndef __FSPSUPD_H__
#define __FSPSUPD_H__

#include <FspUpd.h>

#pragma pack(1)

/** Fsp S Configuration
**/
typedef struct {
  /** Offset 0x0118 - BMP Logo Data Size
    BMP logo data buffer size. 0x00000000(Default).
  **/
  UINT32    LogoSize;

  /** Offset 0x011C - BMP Logo Data Pointer
    BMP logo data pointer to a BMP format buffer. 0x00000000(Default).
  **/
  UINT32    LogoPtr;

  /** Offset 0x0120 - Graphics Configuration Data Pointer
    Graphics configuration data used for initialization. 0x00000000(Default).
  **/
  UINT32    GraphicsConfigPtr;

  /** Offset 0x0124 - PCI GFX Temporary MMIO Base
    PCI Temporary PCI GFX Base used before full PCI enumeration. 0x80000000(Default).
  **/
  UINT32    PciTempResourceBase;

  /** Offset 0x0128
  **/
  UINT8     UnusedUpdSpace1[3];

  /** Offset 0x012B
  **/
  UINT8     ReservedFspsUpd;
} FSP_S_CONFIG;

/** Fsp S UPD Configuration
**/
typedef struct {
  /** Offset 0x0000
  **/
  FSP_UPD_HEADER    FspUpdHeader;

  /** Offset 0x00F8
  **/
  FSPS_ARCH_UPD     FspsArchUpd;

  /** Offset 0x0118
  **/
  FSP_S_CONFIG      FspsConfig;

  /** Offset 0x012C
  **/
  UINT8             UnusedUpdSpace2[2];

  /** Offset 0x012E
  **/
  UINT16            UpdTerminator;
} FSPS_UPD;

#pragma pack()

#endif
