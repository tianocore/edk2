/** @file
  This file is copied from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/TpmConfiguration/TpmConfiguration/TpmProfile_Misc.h
  to set build option for TPM reference library.

  This file redefines below defines:
      - MAX_CONTEXT_SIZE
      - NV_CLOCK_UPDATE_INTERVAL
      - MAX_NV_INDEX_SIZE
  to coherent with edk2's MdePkg/Include/IndustryStandard/Tpm20.h

**/

#pragma once

// YES & NO defined by TpmBuildSwitches.h
#if(YES != 1 || NO != 0)
#  error YES or NO incorrectly set
#endif

// clang-format off
// clang-format off to preserve horizontal spacing
#define IMPLEMENTATION_PCR         24
#define PLATFORM_PCR               24
#define DRTM_PCR                   17
#define HCRTM_PCR                  0
#define NUM_LOCALITIES             5
#define MAX_HANDLE_NUM             3
#define MAX_ACTIVE_SESSIONS        64
#define MAX_LOADED_SESSIONS        3
#define MAX_SESSION_NUM            3
#define MAX_LOADED_OBJECTS         3
#define MIN_EVICT_OBJECTS          2
#define NUM_POLICY_PCR_GROUP       1
#define NUM_AUTHVALUE_PCR_GROUP    1
#define MAX_CONTEXT_SIZE           4000
#define MAX_DIGEST_BUFFER          1024
#define MAX_NV_INDEX_SIZE          1024
#define MAX_NV_BUFFER_SIZE         1024
#define MAX_CAP_BUFFER             1024
#define NV_MEMORY_SIZE             16384
#define MIN_COUNTER_INDICES        8
#define NUM_STATIC_PCR             16
#define MAX_ALG_LIST_SIZE          64
#define PRIMARY_SEED_SIZE          32
#define CONTEXT_ENCRYPT_ALGORITHM  AES
#define NV_CLOCK_UPDATE_INTERVAL   12
#define NUM_POLICY_PCR             1

#define ORDERLY_BITS               8
#define MAX_SYM_DATA               128
#define MAX_RNG_ENTROPY_SIZE       64
#define RAM_INDEX_SPACE            512
#define ENABLE_PCR_NO_INCREMENT    YES

#define SIZE_OF_X509_SERIAL_NUMBER 20

// amount of space the platform can provide in PERSISTENT_DATA during
// manufacture
#define PERSISTENT_DATA_PLATFORM_SPACE  16

// structure padding space for these structures.  Used if a
// particular configuration needs them to be aligned to a
// specific size
#define ORDERLY_DATA_PADDING            0
#define STATE_CLEAR_DATA_PADDING        0
#define STATE_RESET_DATA_PADDING        0

// configuration values that may vary by SIMULATION/DEBUG
#if SIMULATION && DEBUG
// This forces the use of a smaller context slot size. This reduction reduces the
// range of the epoch allowing the tester to force the epoch to occur faster than
// the normal production size
#  define CONTEXT_SLOT UINT8
#else
#  define CONTEXT_SLOT UINT16
#endif
