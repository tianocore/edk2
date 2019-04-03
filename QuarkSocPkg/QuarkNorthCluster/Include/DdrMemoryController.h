/** @file
Memory controller configuration.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __DDR_MEMORY_CONTROLLER_H__
#define __DDR_MEMORY_CONTROLLER_H__

//
// DDR timing data definitions.
// These are used to create bitmaps of valid timing configurations.
//

#define DUAL_CHANNEL_DDR_TIMING_DATA_FREQUENCY_UNKNOWN    0xFF
#define DUAL_CHANNEL_DDR_TIMING_DATA_REFRESH_RATE_UNKNOWN 0xFF

#define DUAL_CHANNEL_DDR_TIMING_DATA_TCL_20    0x01
#define DUAL_CHANNEL_DDR_TIMING_DATA_TCL_25    0x00
#define DUAL_CHANNEL_DDR_TIMING_DATA_TCL_30    0x02
#define DUAL_CHANNEL_DDR_TIMING_DATA_TCL_ALL   0x03


#define DUAL_CHANNEL_DDR_TIMING_DATA_TRCD_02   0x02
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRCD_03   0x01
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRCD_04   0x00
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRCD_ALL  0x03

#define DUAL_CHANNEL_DDR_TIMING_DATA_TRP_02    0x02
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRP_03    0x01
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRP_04    0x00
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRP_ALL   0x03

#define DUAL_CHANNEL_DDR_TIMING_DATA_TRAS_05   0x05
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRAS_06   0x04
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRAS_07   0x03
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRAS_08   0x02
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRAS_09   0x01
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRAS_10   0x00
#define DUAL_CHANNEL_DDR_TIMING_DATA_TRAS_ALL  0x07

#define DUAL_CHANNEL_DDR_DATA_TYPE_REGISTERED    0x01
#define DUAL_CHANNEL_DDR_DATA_TYPE_UNREGISTERED  0x02
#define DUAL_CHANNEL_DDR_DATA_TYPE_BUFFERED      0x04
#define DUAL_CHANNEL_DDR_DATA_TYPE_UNBUFFERED    0x08
#define DUAL_CHANNEL_DDR_DATA_TYPE_SDR           0x10
#define DUAL_CHANNEL_DDR_DATA_TYPE_DDR           0x20


//
// Maximum number of SDRAM channels supported by the memory controller
//
#define MAX_CHANNELS 1

//
// Maximum number of DIMM sockets supported by the memory controller
//
#define MAX_SOCKETS 1

//
// Maximum number of sides supported per DIMM
//
#define   MAX_SIDES                         2

//
// Maximum number of "Socket Sets", where a "Socket Set is a set of matching
// DIMM's from the various channels
//
#define   MAX_SOCKET_SETS                   2

//
// Maximum number of rows supported by the memory controller
//
#define MAX_ROWS (MAX_SIDES * MAX_SOCKETS)

//
// Maximum number of memory ranges supported by the memory controller
//
#define MAX_RANGES (MAX_ROWS + 5)

//
// Maximum Number of Log entries
//
#define   MEMORY_LOG_MAX_INDEX          16


typedef struct _MEMORY_LOG_ENTRY {
  EFI_STATUS_CODE_VALUE                     Event;
  EFI_STATUS_CODE_TYPE                      Severity;
  UINT8                                     Data;
} MEMORY_LOG_ENTRY;

typedef struct _MEMORY_LOG {
  UINT8                                     Index;
  MEMORY_LOG_ENTRY                      Entry[MEMORY_LOG_MAX_INDEX];
} MEMORY_LOG;



//
// Defined ECC types
//
#define DUAL_CHANNEL_DDR_ECC_TYPE_NONE             0x01   // No error checking
#define DUAL_CHANNEL_DDR_ECC_TYPE_EC               0x02   // Error checking only
#define DUAL_CHANNEL_DDR_ECC_TYPE_SECC             0x04   // Software Scrubbing ECC
#define DUAL_CHANNEL_DDR_ECC_TYPE_HECC             0x08   // Hardware Scrubbing ECC
#define DUAL_CHANNEL_DDR_ECC_TYPE_CKECC            0x10   // Chip Kill ECC

//
// Row configuration status values
//
#define DUAL_CHANNEL_DDR_ROW_CONFIG_SUCCESS        0x00  // No error
#define DUAL_CHANNEL_DDR_ROW_CONFIG_UNKNOWN        0x01  // Pattern mismatch, no memory
#define DUAL_CHANNEL_DDR_ROW_CONFIG_UNSUPPORTED    0x02  // Memory type not supported
#define DUAL_CHANNEL_DDR_ROW_CONFIG_ADDRESS_ERROR  0x03  // Row/Col/Bnk mismatch
#define DUAL_CHANNEL_DDR_ROW_CONFIG_ECC_ERROR      0x04  // Received ECC error
#define DUAL_CHANNEL_DDR_ROW_CONFIG_NOT_PRESENT    0x05  // Row is not present
#define DUAL_CHANNEL_DDR_ROW_CONFIG_DISABLED       0x06  // Row is disabled


//
// Memory range types
//
typedef enum {
  DualChannelDdrMainMemory,
  DualChannelDdrSmramCacheable,
  DualChannelDdrSmramNonCacheable,
  DualChannelDdrGraphicsMemoryCacheable,
  DualChannelDdrGraphicsMemoryNonCacheable,
  DualChannelDdrReservedMemory,
  DualChannelDdrMaxMemoryRangeType
} DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE;

//
// Memory map range information
//
typedef struct {
  EFI_PHYSICAL_ADDRESS                          PhysicalAddress;
  EFI_PHYSICAL_ADDRESS                          CpuAddress;
  EFI_PHYSICAL_ADDRESS                          RangeLength;
  DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE                 Type;
} DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE;
typedef struct {
    unsigned    dramType        :1;                 /**< Type: 0 = RESERVED; 1 = DDR2 */
    unsigned    dramWidth       :1;                 /**< Width: 0 = x8; 1 = x16 */
    unsigned    dramDensity     :2;                 /**< Density: 00b = 2Gb; 01b = 1Gb; 10b = 512Mb; 11b = 256Mb */
    unsigned    dramSpeed       :1;                 /**< Speed Grade: 0 = RESERVED; 1 = 800MT/s;*/
    unsigned    dramTimings     :3;                 /**< Timings: 4-4-4, 5-5-5, 6-6-6 */
    unsigned    dramRanks       :1;                 /**< Ranks: 0 = Single Rank; 1 = Dual Rank */
} DramGeometry;                                     /**< DRAM Geometry Descriptor */

typedef union _RegDRP {
    UINT32    raw;
    struct {
        unsigned rank0Enabled       :1;     /**< Rank 0 Enable */
        unsigned rank0DevWidth      :2;     /**< DRAM Device Width (x8,x16) */
        unsigned rank0DevDensity    :2;     /**< DRAM Device Density (256Mb,512Mb,1Gb,2Gb) */
        unsigned reserved2          :1;
        unsigned rank1Enabled       :1;     /**< Rank 1 Enable */
        unsigned reserved3          :5;
        unsigned dramType           :1;     /**< DRAM Type (0=DDR2) */
        unsigned reserved4          :5;
        unsigned reserved5          :14;
      } field;
} RegDRP;                                   /**< DRAM Rank Population and Interface Register */


typedef union {
    UINT32    raw;
    struct {
        unsigned dramFrequency      :3;     /**< DRAM Frequency (000=RESERVED,010=667,011=800) */
        unsigned tRP                :2;     /**< Precharge to Activate Delay (3,4,5,6) */
        unsigned reserved1          :1;
        unsigned tRCD               :2;     /**< Activate to CAS Delay (3,4,5,6) */
        unsigned reserved2          :1;
        unsigned tCL                :2;     /**< CAS Latency (3,4,5,6) */
        unsigned reserved3          :21;
      } field;
} RegDTR0;                                  /**< DRAM Timing Register 0 */

typedef union {
    UINT32    raw;
    struct {
        unsigned tWRRD_dly          :2;     /**< Additional Write to Read Delay (0,1,2,3) */
        unsigned reserved1          :1;
        unsigned tRDWR_dly          :2;     /**< Additional Read to Write Delay (0,1,2,3) */
        unsigned reserved2          :1;
        unsigned tRDRD_dr_dly       :1;     /**< Additional Read to Read Delay (1,2) */
        unsigned reserved3          :1;
        unsigned tRD_dly            :3;     /**< Additional Read Data Sampling Delay (0-7) */
        unsigned reserved4          :1;
        unsigned tRCVEN_halfclk_dly :4;     /**< Additional RCVEN Half Clock Delay Control */
        unsigned reserved5          :1;
        unsigned readDqDelay        :2;     /**< Read DQ Delay */
        unsigned reserved6          :13;
      } field;
} RegDTR1;                                  /**< DRAM Timing Register 1 */

typedef union {
    UINT32    raw;
    struct {
        unsigned ckStaticDisable    :1;     /**< CK/CK# Static Disable */
        unsigned reserved1          :3;
        unsigned ckeStaticDisable   :2;     /**< CKE Static Disable */
        unsigned reserved2          :8;
        unsigned refreshPeriod      :2;     /**< Refresh Period (disabled,128clks,3.9us,7.8us) */
        unsigned refreshQueueDepth  :2;     /**< Refresh Queue Depth (1,2,4,8) */
        unsigned reserved5          :13;
        unsigned initComplete       :1;     /**< Initialization Complete */
      } field;
} RegDCO;

//
// MRC Data Structure
//
typedef struct {
    RegDRP          drp;
    RegDTR0         dtr0;
    RegDTR1         dtr1;
    RegDCO          dco;
    UINT32          reg0104;
    UINT32          reg0120;
    UINT32          reg0121;
    UINT32          reg0123;
    UINT32          reg0111;
    UINT32          reg0130;
    UINT8           refreshPeriod;      /**< Placeholder for the chosen refresh
                                         *   period.  This value will NOT be
                                         *   programmed into DCO until all
                                         *   initialization is done.
                                         */
    UINT8           ddr2Odt;            /**< 0 = Disabled, 1 = 75 ohm, 2 = 150ohm, 3 = 50ohm */
    UINT8           sku;                /**< Detected QuarkNcSocId SKU */
    UINT8           capabilities;       /**< Capabilities Available on this part */
    UINT8           state;              /**< NORMAL_BOOT, S3_RESUME */
    UINT32          memSize;            /**< Memory size */
    UINT16          pmBase;             /**< PM Base */
    UINT16          mrcVersion;         /**< MRC Version */
    UINT32          hecbase;            /**< HECBASE shifted left 16 bits */
    DramGeometry    geometry;          /**< DRAM Geometry */
} MRC_DATA_STRUCTURE;             /**< QuarkNcSocId Memory Parameters for MRC */

typedef struct _EFI_MEMINIT_CONFIG_DATA {
  MRC_DATA_STRUCTURE                        MrcData;
} EFI_MEMINIT_CONFIG_DATA;



#endif
