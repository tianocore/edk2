/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 ************************************************************************/
#ifndef _MRC_H_
#define _MRC_H_

#include "core_types.h"

// define the MRC Version
#define MRC_VERSION 0x0112


// architectural definitions
#define NUM_CHANNELS   1 // number of channels
#define NUM_RANKS      2 // number of ranks per channel
#define NUM_BYTE_LANES 4 // number of byte lanes per channel

// software limitations
#define MAX_CHANNELS   1
#define MAX_RANKS      2
#define MAX_BYTE_LANES 4

// only to mock MrcWrapper
#define MAX_SOCKETS    1
#define MAX_SIDES      1
#define MAX_ROWS       (MAX_SIDES * MAX_SOCKETS)
// end


// Specify DRAM of nenory channel width
enum {
  x8,   // DRAM width
  x16,  // DRAM width & Channel Width
  x32   // Channel Width
};

// Specify DRAM speed
enum {
  DDRFREQ_800,
  DDRFREQ_1066
};

// Specify DRAM type
enum {
  DDR3,
  DDR3L
};

// Delay configuration for individual signals
// Vref setting
// Scrambler seed
typedef struct MrcTimings_s
{
  uint32_t rcvn[NUM_CHANNELS][NUM_RANKS][NUM_BYTE_LANES];
  uint32_t rdqs[NUM_CHANNELS][NUM_RANKS][NUM_BYTE_LANES];
  uint32_t wdqs[NUM_CHANNELS][NUM_RANKS][NUM_BYTE_LANES];
  uint32_t wdq [NUM_CHANNELS][NUM_RANKS][NUM_BYTE_LANES];
  uint32_t vref[NUM_CHANNELS][NUM_BYTE_LANES];
  uint32_t wctl[NUM_CHANNELS][NUM_RANKS];
  uint32_t wcmd[NUM_CHANNELS];

  uint32_t scrambler_seed;
  uint8_t  ddr_speed;            // need to save for the case of frequency change
} MrcTimings_t;


// DENSITY: 0=512Mb, 1=Gb, 2=2Gb, 3=4Gb
// tCL is DRAM CAS Latency in clocks.
// All other timings are in picoseconds.
// Refer to JEDEC spec (or DRAM datasheet) when changing these values.
typedef struct DRAMParams_s {
  uint8_t  DENSITY;
  uint8_t  tCL;   // CAS latency in clocks
  uint32_t tRAS;  // ACT to PRE command period
  uint32_t tWTR;  // Delay from start of internal write transaction to internal read command
  uint32_t tRRD;  // ACT to ACT command period (JESD79 specific to page size 1K/2K)
  uint32_t tFAW;  // Four activate window (JESD79 specific to page size 1K/2K)
} DRAMParams_t;


// Boot mode defined as bit mask (1<<n)
#define bmCold     1    // full training
#define bmFast     2    // restore timing parameters
#define bmS3       4    // resume from S3
#define bmWarm     8
#define bmUnknown  0


// MRC execution status
#define MRC_SUCCESS     0      // initialization ok
#define MRC_E_MEMTEST   1      // memtest failed


//
// Input/output/context parameters for Memory Reference Code
//
typedef struct MRCParams_s
{
  //
  // Global settings
  //

  uint32_t boot_mode;           // bmCold, bmFast, bmWarm, bmS3
  uint32_t uart_mmio_base;      // pcie serial port base address (force 0 to disable debug)

  uint8_t  dram_width;          // x8, x16
  uint8_t  ddr_speed;           // DDRFREQ_800, DDRFREQ_1066
  uint8_t  ddr_type;            // DDR3, DDR3L
  uint8_t  ecc_enables;         // 0, 1 (memory size reduced to 7/8)
  uint8_t  scrambling_enables;  // 0, 1
  uint32_t rank_enables;        // 1, 3 (1'st rank has to be populated if 2'nd rank present)
  uint32_t channel_enables;     // 1 only
  uint32_t channel_width;       // x16 only
  uint32_t address_mode;        // 0, 1, 2 (mode 2 forced if ecc enabled)

  // memConfig_t begin
  uint8_t refresh_rate;         // REFRESH_RATE       : 1=1.95us, 2=3.9us, 3=7.8us, others=RESERVED
  uint8_t sr_temp_range;        // SR_TEMP_RANGE      : 0=normal, 1=extended, others=RESERVED
  uint8_t ron_value;            // RON_VALUE          : 0=34ohm, 1=40ohm, others=RESERVED (select MRS1.DIC driver impedance control)
  uint8_t rtt_nom_value;        // RTT_NOM_VALUE      : 0=40ohm, 1=60ohm, 2=120ohm, others=RESERVED
  uint8_t rd_odt_value;         // RD_ODT_VALUE       : 0=off, 1=60ohm, 2=120ohm, 3=180ohm, others=RESERVED
  // memConfig_t end

  DRAMParams_t params;

  //
  // Internally used
  //

  uint32_t board_id;            // internally used for board layout (use x8 or x16 memory)
  uint32_t hte_setup : 1;       // when set hte reconfiguration requested
  uint32_t menu_after_mrc : 1;
  uint32_t power_down_disable :1;
  uint32_t tune_rcvn :1;

  uint32_t channel_size[NUM_CHANNELS];
  uint32_t column_bits[NUM_CHANNELS];
  uint32_t row_bits[NUM_CHANNELS];

  uint32_t mrs1;                // register content saved during training

  //
  // Output
  //

  uint32_t status;              // initialization result (non zero specifies error code)
  uint32_t mem_size;            // total memory size in bytes (excludes ECC banks)

  MrcTimings_t timings;         // training results (also used on input)

} MRCParams_t;

// Alternative type name for consistent naming convention
#define MRC_PARAMS    MRCParams_t

#endif // _MRC_H_
