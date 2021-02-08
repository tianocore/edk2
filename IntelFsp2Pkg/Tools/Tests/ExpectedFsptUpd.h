#ifndef __FSPTUPD_H__
#define __FSPTUPD_H__

#include <FspUpd.h>

#pragma pack(1)


/** Fsp T Common UPD
**/
typedef struct {

/** Offset 0x0040
**/
  UINT8                       Revision;

/** Offset 0x0041
**/
  UINT8                       Reserved[3];

/** Offset 0x0044
**/
  UINT32                      MicrocodeRegionBase;

/** Offset 0x0048
**/
  UINT32                      MicrocodeRegionLength;

/** Offset 0x004C
**/
  UINT32                      CodeRegionBase;

/** Offset 0x0050
**/
  UINT32                      CodeRegionLength;

/** Offset 0x0054
**/
  UINT8                       Reserved1[12];
} FSPT_COMMON_UPD;

/** Fsp T Configuration
**/
typedef struct {

/** Offset 0x0060 - Chicken bytes to test Hex config
  This option shows how to present option for 4 bytes data
**/
  UINT32                      ChickenBytes;

/** Offset 0x0064
**/
  UINT8                       ReservedFsptUpd1[28];
} FSP_T_CONFIG;

/** Fsp T UPD Configuration
**/
typedef struct {

/** Offset 0x0000
**/
  FSP_UPD_HEADER              FspUpdHeader;

/** Offset 0x0020
**/
  FSPT_ARCH_UPD               FsptArchUpd;

/** Offset 0x0040
**/
  FSPT_COMMON_UPD             FsptCommonUpd;

/** Offset 0x0060
**/
  FSP_T_CONFIG                FsptConfig;

/** Offset 0x0080
**/
  UINT8                       UnusedUpdSpace0[6];

/** Offset 0x0086
**/
  UINT16                      UpdTerminator;
} FSPT_UPD;

#pragma pack()

#endif
