/** @file
  DiskIo driver that lays on every BlockIo protocol in the system.
  DiskIo converts a block oriented device to a byte oriented device.

  Disk access may have to handle unaligned request about sector boundaries.
  There are three cases:
    UnderRun - The first byte is not on a sector boundary or the read request is
               less than a sector in length.
    Aligned  - A read of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary.

Copyright (c) 2006 - 2009, Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "DriverHealthDxe.h"

#undef STRING_TOKEN
#define STRING_TOKEN(x) 0

extern EFI_GUID gEfiCallerIdGuid;

CHAR16 VariableName[]    = L"Config";
UINTN  mNumNotHealthy = 0;
UINT8  ControllerIndex = 0;

//
// Link used to store the controller health status
//
LIST_ENTRY  mControllerList = {NULL, NULL};

//
// 0           - Healthy -> {0}
// 1           - Health with warning messages -> {1}
// 2           - Failed -> {2}
// 3           - Failed with error messages -> {3}
// 4           - RebootRequired -> {4}
// 5           - RebootRequired with messages -> {5}
// 6           - ReconnectRequired -> {6}
// 7           - ReconnectRequired with messages -> {7}
// 100..103    - RepairRequired -> {0..3}
// 104..107    - RepairRequired with error messages -> {0..3}
// 108..111    - RepairRequired with progress notifications -> {0..3}
// 112..115    - RepairRequired with error messages and progress notifications -> {0..3}
// 132..163    - RepairRequired -> {300..331}
// 164..195    - RepairRequired with error messages -> {300..331}
// 196..227    - RepairRequired with progress notifications -> {300..331}
// 228..259    - RepairRequired with error messages and progress notifications -> {300..331}
// 300..307    - ConfigRequired -> {0..7}
// 308..315    - ConfigRequired with error messages -> {0..7}
// 316..323    - ConfigRequired with forms -> {0..7}
// 324..331    - ConfigRequired with forms and error messages -> {0..7}
// 332..347    - ConfigRequired -> {100..115}
// 348..363    - ConfigRequired with error messages -> {100..115}
// 364..379    - ConfigRequired with forms -> {100.115}
// 380..395    - ConfigRequired with forms and error messages -> {100..115}

DEVICE_STATE  mDeviceState[] = {
  { TRUE,  308, 000, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  309, 001, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  310, 002, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  311, 003, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  312, 004, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  313, 005, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  314, 006, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  315, 007, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  000, 000, 0,                                       FALSE, EfiDriverHealthStatusHealthy },
  { TRUE,  001, 001, STRING_TOKEN (STR_HEALTHY_WARNING),      FALSE, EfiDriverHealthStatusHealthy },

  { TRUE,  002, 002, 0,                                       FALSE, EfiDriverHealthStatusFailed },
  { TRUE,  003, 003, STRING_TOKEN (STR_FAILED_ERROR),         FALSE, EfiDriverHealthStatusFailed },

  { FALSE, 004, 004, 0,                                       FALSE, EfiDriverHealthStatusRebootRequired },
  { FALSE, 005, 005, STRING_TOKEN (STR_REBOOT_REQUIRED),      FALSE, EfiDriverHealthStatusRebootRequired },

  { FALSE, 006, 006, 0,                                       FALSE, EfiDriverHealthStatusReconnectRequired },
  { FALSE, 007, 007, STRING_TOKEN (STR_RECONNECT_REQUIRED),   FALSE, EfiDriverHealthStatusReconnectRequired },

  { TRUE,  100, 000, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired },
  { TRUE,  101, 001, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired },
  { TRUE,  102, 002, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired },
  { TRUE,  103, 003, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired },

  { TRUE,  104, 000, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  105, 001, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  106, 002, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  107, 003, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },

  { TRUE,  108, 000, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired   },
  { TRUE,  109, 001, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired   },
  { TRUE,  110, 002, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired   },
  { TRUE,  111, 003, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired   },

  { TRUE,  112, 000, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired   },
  { TRUE,  113, 001, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired   },
  { TRUE,  114, 002, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired   },
  { TRUE,  115, 003, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired   },

  { TRUE,  132, 300, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  133, 301, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  134, 302, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  135, 303, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  136, 304, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  137, 305, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  138, 306, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  139, 307, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  140, 308, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  141, 309, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  142, 310, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  143, 311, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  144, 312, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  145, 313, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  146, 314, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  147, 315, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  148, 316, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  149, 317, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  150, 318, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  151, 319, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  152, 320, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  153, 321, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  154, 322, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  155, 323, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  156, 324, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  157, 325, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  158, 326, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  159, 327, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  160, 328, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  161, 329, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  162, 330, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  163, 331, 0,                                       FALSE, EfiDriverHealthStatusRepairRequired  },

  { TRUE,  164, 300, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  165, 301, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  166, 302, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  167, 303, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  168, 304, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  169, 305, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  170, 306, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  171, 307, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  172, 308, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  173, 309, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  174, 310, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  175, 311, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  176, 312, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  177, 313, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  178, 314, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  179, 315, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  180, 316, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  181, 317, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  182, 318, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  183, 319, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  184, 320, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  185, 321, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  186, 322, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  187, 323, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  188, 324, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  189, 325, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  190, 326, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  191, 327, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  192, 328, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  193, 329, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  194, 330, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  195, 331, STRING_TOKEN (STR_REPAIR_REQUIRED),      FALSE, EfiDriverHealthStatusRepairRequired  },

  { TRUE,  196, 300, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  197, 301, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  198, 302, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  199, 303, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  200, 304, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  201, 305, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  202, 306, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  203, 307, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  204, 308, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  205, 309, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  206, 310, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  207, 311, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  208, 312, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  209, 313, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  210, 314, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  211, 315, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  212, 316, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  213, 317, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  214, 318, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  215, 319, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  216, 320, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  217, 321, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  218, 322, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  219, 323, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  220, 324, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  221, 325, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  222, 326, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  223, 327, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  224, 328, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  225, 329, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  226, 330, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  227, 331, 0,                                       TRUE, EfiDriverHealthStatusRepairRequired  },

  { TRUE,  228, 300, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  229, 301, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  230, 302, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  231, 303, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  232, 304, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  233, 305, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  234, 306, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  235, 307, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  236, 308, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  237, 309, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  238, 310, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  239, 311, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  240, 312, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  241, 313, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  242, 314, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  243, 315, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  244, 316, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  245, 317, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  246, 318, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  247, 319, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  248, 320, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  249, 321, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  250, 322, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  251, 323, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  252, 324, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  253, 325, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  254, 326, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  255, 327, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  256, 328, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  257, 329, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  258, 330, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },
  { TRUE,  259, 331, STRING_TOKEN (STR_REPAIR_REQUIRED),      TRUE, EfiDriverHealthStatusRepairRequired  },

  { TRUE,  300, 000, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  301, 001, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  302, 002, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  303, 003, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  304, 004, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  305, 005, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  306, 006, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  307, 007, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  308, 000, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  309, 001, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  310, 002, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  311, 003, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  312, 004, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  313, 005, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  314, 006, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  315, 007, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  316, 000, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  317, 001, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  318, 002, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  319, 003, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  320, 004, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  321, 005, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  322, 006, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  323, 007, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  324, 000, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  325, 001, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  326, 002, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  327, 003, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  328, 004, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  329, 005, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  330, 006, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  331, 007, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  332, 100, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  333, 101, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  334, 102, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  335, 103, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  336, 104, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  337, 105, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  338, 106, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  339, 107, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  340, 108, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  341, 109, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  342, 110, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  343, 111, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  344, 112, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  345, 113, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  346, 114, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  347, 115, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  348, 100, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  349, 101, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  350, 102, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  351, 103, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  352, 104, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  353, 105, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  354, 106, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  355, 107, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  356, 108, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  357, 109, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  358, 110, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  359, 111, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  360, 112, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  361, 113, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  362, 114, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  363, 115, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  364, 100, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  365, 101, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  366, 102, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  367, 103, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  368, 104, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  369, 105, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  370, 106, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  371, 107, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  372, 108, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  373, 109, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  374, 110, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  375, 111, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  376, 112, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  377, 113, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  378, 114, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  379, 115, 0,                                       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  380, 100, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  381, 101, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  382, 102, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  383, 103, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  384, 104, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  385, 105, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  386, 106, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  387, 107, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  388, 108, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  389, 109, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  390, 110, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  391, 111, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  392, 112, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  393, 113, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  394, 114, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },
  { TRUE,  395, 115, STRING_TOKEN (STR_CONFIG_WARNING),       FALSE, EfiDriverHealthStatusConfigurationRequired },

  { TRUE,  999, 999, 0,                                       FALSE }
};

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePathDiskIoDummy = {
  {
    {
      HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
          (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
      //
      // {C153B68E-EBFC-488e-B110-662867745BBE}
      //
    { 0xc153b68e, 0xebfc, 0x488e, { 0xb1, 0x10, 0x66, 0x28, 0x67, 0x74, 0x5b, 0xbe} }
  },
  {
    END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
        (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

EFI_HII_HANDLE     mHiiHandle = NULL;

EFI_DRIVER_BINDING_PROTOCOL gDiskIoDriverBinding = {
  DiskIoDriverBindingSupported,
  DiskIoDriverBindingStart,
  DiskIoDriverBindingStop,
  0xaa,
  NULL,
  NULL
};

EFI_DRIVER_HEALTH_PROTOCOL gDiskIoDriverHealth = {
  DiskIoDriverHealthGetHealthStatus,
  DiskIoDriverHealthRepair
};
//
// Template for DiskIo private data structure.
// The pointer to BlockIo protocol interface is assigned dynamically.
//
DISK_IO_PRIVATE_DATA gDiskIoPrivateDataTemplate = {
  DISK_IO_PRIVATE_DATA_SIGNATURE,
  {
    EFI_DISK_IO_PROTOCOL_REVISION,
    DiskIoReadDisk,
    DiskIoWriteDisk
  },
  NULL,
  NULL, // Handle
  NULL, // Consumed Protocol
  NULL,
  //
  // Produced Protocol
  //
  {
    DummyExtractConfig,
    DummyRouteConfig,
    DummyDriverCallback
  },
  //
  // NVdata
  //
  { 0x0 },
  //
  // Controller Name
  //
  NULL,
  //
  // Controller Index
  //
  0
};

DEVICE_STATE *
GetDeviceState (
  UINTN  DeviceStateNumber
  )
{
  UINTN  Index;

  for (Index = 0; mDeviceState[Index].CurrentState != 999 && mDeviceState[Index].CurrentState != DeviceStateNumber; Index++);
  ASSERT (mDeviceState[Index].CurrentState != 999);

  return &mDeviceState[Index];
}


/**
  Test to see if this driver supports ControllerHandle. 

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test.
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiBlockIoProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );
  return EFI_SUCCESS;
}


/**
  Start this driver on ControllerHandle by opening a Block IO protocol and
  installing a Disk IO protocol on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS            Status;
  DISK_IO_PRIVATE_DATA  *Private;
  DEVICE_STATE          *DeviceState;
  UINTN                 DataSize;
  UINT32                StartCount;
  CONTROLLER_STATE      *ControllerState;

  Private          = NULL;
  ControllerState  = NULL;

  //
  // Connect to the Block IO interface on ControllerHandle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &gDiskIoPrivateDataTemplate.BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Initialize the Disk IO device instance.
  //
  Private = AllocateCopyPool (sizeof (DISK_IO_PRIVATE_DATA), &gDiskIoPrivateDataTemplate);
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  
  //
  // Begin Driver Health Protocol Support
  //
  DataSize = sizeof (StartCount);
  Status = gRT->GetVariable (
                  L"StartCount",
                  &gEfiCallerIdGuid,
                  NULL,
                  &DataSize,
                  &StartCount
                  );
  if (EFI_ERROR (Status)) {
    //
    // If the configuration can not be read, then set the default config value of 0
    //
    StartCount = 0;
  }

  ControllerIndex++;

  DeviceState = GetDeviceState (mDeviceState[StartCount].CurrentState);
  ASSERT (DeviceState != NULL);

  ControllerState = AllocateZeroPool (sizeof (CONTROLLER_STATE));
  if (ControllerState == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  ControllerState->ControllerHandle  = ControllerHandle;
  ControllerState->Signature         = DISK_IO_CONTROLLER_STATE_SIGNATURE;
  ControllerState->DeviceStateNum    = DeviceState->CurrentState;
  ControllerState->ChildHandle       = NULL;
  ControllerState->ControllerIndex   = ControllerIndex; 

  InsertTailList (&mControllerList, &ControllerState->Link);

  if (DeviceState->HealthStatus != EfiDriverHealthStatusHealthy || DeviceState->StringId != 0) {
    mNumNotHealthy++;
  }

  StartCount++;
  while (!mDeviceState[StartCount].StartState) {
    if (mDeviceState[StartCount].CurrentState == 999) {
      StartCount = 0;
    } else {
      StartCount++;
    }
  }
  if (mDeviceState[StartCount].CurrentState == 999) {
    StartCount = 0;
  }

  Status = gRT->SetVariable (
                  L"StartCount",
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (StartCount),
                  &StartCount
                  );
  ASSERT_EFI_ERROR (Status);

  if (DeviceState->HealthStatus == EfiDriverHealthStatusConfigurationRequired) {
    Private->NVdata.ConfigGood = 0;
  } else {
    Private->NVdata.ConfigGood = 1;
  }
  Status = gRT->SetVariable (
                  L"Config",
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (Private->NVdata.ConfigGood),
                  &Private->NVdata.ConfigGood
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // End Driver Health Protocol Support
  //

  //
  // Install protocol interfaces for the Disk IO device.
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->DiskIo
                  );
 
  Private->ControllerIndex = ControllerIndex;
  AddName (Private);

ErrorExit:
  if (EFI_ERROR (Status)) {

    if (Private != NULL) {
      FreeUnicodeStringTable (Private->ControllerNameTable);
      FreePool (Private);
    }

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiBlockIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
  }

  return Status;
}


/**
  Stop this driver on ControllerHandle by removing Disk IO protocol and closing
  the Block IO protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  DISK_IO_PRIVATE_DATA  *Private;
  DEVICE_STATE          *DeviceState;
  CONTROLLER_STATE      *ControllerState;
  LIST_ENTRY            *Link;

  ControllerState   = NULL;
  DeviceState       = NULL;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **) &DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = DISK_IO_PRIVATE_DATA_FROM_THIS (DiskIo);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  &Private->DiskIo
                  );
  if (!EFI_ERROR (Status)) {

    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiBlockIoProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );
    //
    // Get the Controller State from global list
    //
    Link = GetFirstNode (&mControllerList);

    while (!IsNull (&mControllerList, Link)) {
      ControllerState = DISK_IO_CONTROLLER_STATE_FROM_LINK (Link);

      if (ControllerState->ControllerHandle == ControllerHandle) {
        DeviceState = GetDeviceState (ControllerState->DeviceStateNum);
        break;
      }
      Link = GetNextNode (&mControllerList, Link);
    }

    ASSERT (DeviceState != NULL);

    if (DeviceState->HealthStatus != EfiDriverHealthStatusHealthy || DeviceState->StringId != 0) {
      mNumNotHealthy--;
    }
     
    RemoveEntryList (Link);

    if (ControllerState != NULL) {
      FreePool (ControllerState);
    }    
  }

  if (!EFI_ERROR (Status)) {
    FreeUnicodeStringTable (Private->ControllerNameTable);
    FreePool (Private);
  }

  ControllerIndex = 0;
  return Status;
}



/**
  Read BufferSize bytes from Offset into Buffer.
  Reads may support reads that are not aligned on
  sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the read request is
               less than a sector in length.
    Aligned  - A read of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary.

  @param  This                  Protocol instance pointer.
  @param  MediaId               Id of the media, changes every time the media is replaced.
  @param  Offset                The starting byte offset to read from
  @param  BufferSize            Size of Buffer
  @param  Buffer                Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  OUT VOID                 *Buffer
  )
{
  EFI_STATUS            Status;
  DISK_IO_PRIVATE_DATA  *Private;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  EFI_BLOCK_IO_MEDIA    *Media;
  UINT32                BlockSize;
  UINT64                Lba;
  UINT64                OverRunLba;
  UINT32                UnderRun;
  UINT32                OverRun;
  BOOLEAN               TransactionComplete;
  UINTN                 WorkingBufferSize;
  UINT8                 *WorkingBuffer;
  UINTN                 Length;
  UINT8                 *Data;
  UINT8                 *PreData;
  UINTN                 IsBufferAligned;
  UINTN                 DataBufferSize;
  BOOLEAN               LastRead;

  Private   = DISK_IO_PRIVATE_DATA_FROM_THIS (This);

  BlockIo   = Private->BlockIo;
  Media     = BlockIo->Media;
  BlockSize = Media->BlockSize;

  if (Media->MediaId != MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  WorkingBuffer     = Buffer;
  WorkingBufferSize = BufferSize;

  //
  // Allocate a temporary buffer for operation
  //
  DataBufferSize = BlockSize * DATA_BUFFER_BLOCK_NUM;

  if (Media->IoAlign > 1) {
    PreData = AllocatePool (DataBufferSize + Media->IoAlign);
    Data    = PreData - ((UINTN) PreData & (Media->IoAlign - 1)) + Media->IoAlign;
  } else {
    PreData = AllocatePool (DataBufferSize);
    Data    = PreData;
  }

  if (PreData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Lba                 = DivU64x32Remainder (Offset, BlockSize, &UnderRun);

  Length              = BlockSize - UnderRun;
  TransactionComplete = FALSE;

  Status              = EFI_SUCCESS;
  if (UnderRun != 0) {
    //
    // Offset starts in the middle of an Lba, so read the entire block.
    //
    Status = BlockIo->ReadBlocks (
                        BlockIo,
                        MediaId,
                        Lba,
                        BlockSize,
                        Data
                        );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (Length > BufferSize) {
      Length              = BufferSize;
      TransactionComplete = TRUE;
    }

    CopyMem (WorkingBuffer, Data + UnderRun, Length);

    WorkingBuffer += Length;

    WorkingBufferSize -= Length;
    if (WorkingBufferSize == 0) {
      goto Done;
    }

    Lba += 1;
  }

  OverRunLba = Lba + DivU64x32Remainder (WorkingBufferSize, BlockSize, &OverRun);

  if (!TransactionComplete && WorkingBufferSize >= BlockSize) {
    //
    // If the DiskIo maps directly to a BlockIo device do the read.
    //
    if (OverRun != 0) {
      WorkingBufferSize -= OverRun;
    }
    //
    // Check buffer alignment
    //
    IsBufferAligned = (UINTN) WorkingBuffer & (UINTN) (Media->IoAlign - 1);

    if (Media->IoAlign <= 1 || IsBufferAligned == 0) {
      //
      // Alignment is satisfied, so read them together
      //
      Status = BlockIo->ReadBlocks (
                          BlockIo,
                          MediaId,
                          Lba,
                          WorkingBufferSize,
                          WorkingBuffer
                          );

      if (EFI_ERROR (Status)) {
        goto Done;
      }

      WorkingBuffer += WorkingBufferSize;

    } else {
      //
      // Use the allocated buffer instead of the original buffer
      // to avoid alignment issue.
      // Here, the allocated buffer (8-byte align) can satisfy the alignment
      //
      LastRead = FALSE;
      do {
        if (WorkingBufferSize <= DataBufferSize) {
          //
          // It is the last calling to readblocks in this loop
          //
          DataBufferSize  = WorkingBufferSize;
          LastRead        = TRUE;
        }

        Status = BlockIo->ReadBlocks (
                            BlockIo,
                            MediaId,
                            Lba,
                            DataBufferSize,
                            Data
                            );
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        CopyMem (WorkingBuffer, Data, DataBufferSize);
        WorkingBufferSize -= DataBufferSize;
        WorkingBuffer += DataBufferSize;
        Lba += DATA_BUFFER_BLOCK_NUM;
      } while (!LastRead);
    }
  }

  if (!TransactionComplete && OverRun != 0) {
    //
    // Last read is not a complete block.
    //
    Status = BlockIo->ReadBlocks (
                        BlockIo,
                        MediaId,
                        OverRunLba,
                        BlockSize,
                        Data
                        );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    CopyMem (WorkingBuffer, Data, OverRun);
  }

Done:
  if (PreData != NULL) {
    FreePool (PreData);
  }

  return Status;
}


/**
  Writes BufferSize bytes from Buffer into Offset.
  Writes may require a read modify write to support writes that are not
  aligned on sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the write request
               is less than a sector in length. Read modify write is required.
    Aligned  - A write of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary. Read modified write
               required.

  @param  This       Protocol instance pointer.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Offset     The starting byte offset to read from
  @param  BufferSize Size of Buffer
  @param  Buffer     Buffer containing read data

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not
                                 valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  )
{
  EFI_STATUS            Status;
  DISK_IO_PRIVATE_DATA  *Private;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  EFI_BLOCK_IO_MEDIA    *Media;
  UINT32                BlockSize;
  UINT64                Lba;
  UINT64                OverRunLba;
  UINT32                UnderRun;
  UINT32                OverRun;
  BOOLEAN               TransactionComplete;
  UINTN                 WorkingBufferSize;
  UINT8                 *WorkingBuffer;
  UINTN                 Length;
  UINT8                 *Data;
  UINT8                 *PreData;
  UINTN                 IsBufferAligned;
  UINTN                 DataBufferSize;
  BOOLEAN               LastWrite;

  Private   = DISK_IO_PRIVATE_DATA_FROM_THIS (This);

  BlockIo   = Private->BlockIo;
  Media     = BlockIo->Media;
  BlockSize = Media->BlockSize;

  if (Media->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  if (Media->MediaId != MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  DataBufferSize = BlockSize * DATA_BUFFER_BLOCK_NUM;

  if (Media->IoAlign > 1) {
    PreData = AllocatePool (DataBufferSize + Media->IoAlign);
    Data    = PreData - ((UINTN) PreData & (Media->IoAlign - 1)) + Media->IoAlign;
  } else {
    PreData = AllocatePool (DataBufferSize);
    Data    = PreData;
  }

  if (PreData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  WorkingBuffer       = Buffer;
  WorkingBufferSize   = BufferSize;

  Lba                 = DivU64x32Remainder (Offset, BlockSize, &UnderRun);

  Length              = BlockSize - UnderRun;
  TransactionComplete = FALSE;

  Status              = EFI_SUCCESS;
  if (UnderRun != 0) {
    //
    // Offset starts in the middle of an Lba, so do read modify write.
    //
    Status = BlockIo->ReadBlocks (
                        BlockIo,
                        MediaId,
                        Lba,
                        BlockSize,
                        Data
                        );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (Length > BufferSize) {
      Length              = BufferSize;
      TransactionComplete = TRUE;
    }

    CopyMem (Data + UnderRun, WorkingBuffer, Length);

    Status = BlockIo->WriteBlocks (
                        BlockIo,
                        MediaId,
                        Lba,
                        BlockSize,
                        Data
                        );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    WorkingBuffer += Length;
    WorkingBufferSize -= Length;
    if (WorkingBufferSize == 0) {
      goto Done;
    }

    Lba += 1;
  }

  OverRunLba = Lba + DivU64x32Remainder (WorkingBufferSize, BlockSize, &OverRun);

  if (!TransactionComplete && WorkingBufferSize >= BlockSize) {
    //
    // If the DiskIo maps directly to a BlockIo device do the write.
    //
    if (OverRun != 0) {
      WorkingBufferSize -= OverRun;
    }
    //
    // Check buffer alignment
    //
    IsBufferAligned = (UINTN) WorkingBuffer & (UINTN) (Media->IoAlign - 1);

    if (Media->IoAlign <= 1 || IsBufferAligned == 0) {
      //
      // Alignment is satisfied, so write them together
      //
      Status = BlockIo->WriteBlocks (
                          BlockIo,
                          MediaId,
                          Lba,
                          WorkingBufferSize,
                          WorkingBuffer
                          );

      if (EFI_ERROR (Status)) {
        goto Done;
      }

      WorkingBuffer += WorkingBufferSize;

    } else {
      //
      // The buffer parameter is not aligned with the request
      // So use the allocated instead.
      // It can fit almost all the cases.
      //
      LastWrite = FALSE;
      do {
        if (WorkingBufferSize <= DataBufferSize) {
          //
          // It is the last calling to writeblocks in this loop
          //
          DataBufferSize  = WorkingBufferSize;
          LastWrite       = TRUE;
        }

        CopyMem (Data, WorkingBuffer, DataBufferSize);
        Status = BlockIo->WriteBlocks (
                            BlockIo,
                            MediaId,
                            Lba,
                            DataBufferSize,
                            Data
                            );
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        WorkingBufferSize -= DataBufferSize;
        WorkingBuffer += DataBufferSize;
        Lba += DATA_BUFFER_BLOCK_NUM;
      } while (!LastWrite);
    }
  }

  if (!TransactionComplete && OverRun != 0) {
    //
    // Last bit is not a complete block, so do a read modify write.
    //
    Status = BlockIo->ReadBlocks (
                        BlockIo,
                        MediaId,
                        OverRunLba,
                        BlockSize,
                        Data
                        );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    CopyMem (Data, WorkingBuffer, OverRun);

    Status = BlockIo->WriteBlocks (
                        BlockIo,
                        MediaId,
                        OverRunLba,
                        BlockSize,
                        Data
                        );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

Done:
  if (PreData != NULL) {
    FreePool (PreData);
  }

  return Status;
}


/**
  Retrieves the health status of a controller in the platform.  This function can also 
  optionally return warning messages, error messages, and a set of HII Forms that may 
  be repair a controller that is not proper configured. 
  
  @param  This             A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.

  @param  ControllerHandle The handle of the controller to retrieve the health status 
                           on.  This is an optional parameter that may be NULL.  If 
                           this parameter is NULL, then the value of ChildHandle is 
                           ignored, and the combined health status of all the devices 
                           that the driver is managing is returned.

  @param  ChildHandle      The handle of the child controller to retrieve the health 
                           status on.  This is an optional parameter that may be NULL.  
                           This parameter is ignored of ControllerHandle is NULL.  It 
                           will be NULL for device drivers.  It will also be NULL for 
                           bus drivers when an attempt is made to collect the health 
                           status of the bus controller.  If will not be NULL when an 
                           attempt is made to collect the health status for a child 
                           controller produced by the driver.

  @param  HealthStatus     A pointer to the health status that is returned by this 
                           function.  This is an optional parameter that may be NULL.  
                           This parameter is ignored of ControllerHandle is NULL.  
                           The health status for the controller specified by 
                           ControllerHandle and ChildHandle is returned. 

  @param  MessageList      A pointer to an array of warning or error messages associated 
                           with the controller specified by ControllerHandle and 
                           ChildHandle.  This is an optional parameter that may be NULL.  
                           MessageList is allocated by this function with the EFI Boot 
                           Service AllocatePool(), and it is the caller's responsibility 
                           to free MessageList with the EFI Boot Service FreePool().  
                           Each message is specified by tuple of an EFI_HII_HANDLE and 
                           an EFI_STRING_ID.  The array of messages is terminated by tuple 
                           containing a EFI_HII_HANDLE with a value of NULL.  The 
                           EFI_HII_STRING_PROTOCOL.GetString() function can be used to 
                           retrieve the warning or error message as a Null-terminated 
                           Unicode string in a specific language.  Messages may be 
                           returned for any of the HealthStatus values except 
                           EfiDriverHealthStatusReconnectRequired and 
                           EfiDriverHealthStatusRebootRequired.

  @param  FormHiiHandle    A pointer to the HII handle for an HII form associated with the 
                           controller specified by ControllerHandle and ChildHandle.  
                           This is an optional parameter that may be NULL.  An HII form 
                           is specified by a combination of an EFI_HII_HANDLE and an 
                           EFI_GUID that identifies the Form Set GUID.  The 
                           EFI_FORM_BROWSER2_PROTOCOL.SendForm() function can be used 
                           to display and allow the user to make configuration changes 
                           to the HII Form.  An HII form may only be returned with a 
                           HealthStatus value of EfiDriverHealthStatusConfigurationRequired.

  @retval EFI_SUCCESS           ControllerHandle is NULL, and all the controllers 
                                managed by this driver specified by This have a health 
                                status of EfiDriverHealthStatusHealthy with no warning 
                                messages to be returned.  The ChildHandle, HealthStatus, 
                                MessageList, and FormList parameters are ignored.

  @retval EFI_DEVICE_ERROR      ControllerHandle is NULL, and one or more of the 
                                controllers managed by this driver specified by This 
                                do not have a health status of EfiDriverHealthStatusHealthy.  
                                The ChildHandle, HealthStatus, MessageList, and 
                                FormList parameters are ignored.

  @retval EFI_DEVICE_ERROR      ControllerHandle is NULL, and one or more of the 
                                controllers managed by this driver specified by This 
                                have one or more warning and/or error messages.  
                                The ChildHandle, HealthStatus, MessageList, and 
                                FormList parameters are ignored.

  @retval EFI_SUCCESS           ControllerHandle is not NULL and the health status 
                                of the controller specified by ControllerHandle and 
                                ChildHandle was returned in HealthStatus.  A list 
                                of warning and error messages may be optionally 
                                returned in MessageList, and a list of HII Forms 
                                may be optionally returned in FormList.

  @retval EFI_UNSUPPORTED	      ControllerHandle is not NULL, and the controller 
                                specified by ControllerHandle and ChildHandle is not 
                                currently being managed by the driver specified by This.

  @retval EFI_INVALID_PARAMETER	HealthStatus is NULL.

  @retval EFI_OUT_OF_RESOURCES	MessageList is not NULL, and there are not enough 
                                resource available to allocate memory for MessageList.

**/
EFI_STATUS
DiskIoDriverHealthGetHealthStatus (
  IN  EFI_DRIVER_HEALTH_PROTOCOL       *This,
  IN  EFI_HANDLE                       ControllerHandle  OPTIONAL,
  IN  EFI_HANDLE                       ChildHandle       OPTIONAL,
  OUT EFI_DRIVER_HEALTH_STATUS         *HealthStatus,
  OUT EFI_DRIVER_HEALTH_HII_MESSAGE    **MessageList     OPTIONAL,
  OUT EFI_HII_HANDLE                   *FormHiiHandle    OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  DISK_IO_PRIVATE_DATA  *Private;
  DEVICE_STATE          *DeviceState;
  CONTROLLER_STATE      *ControllerState;
  LIST_ENTRY            *Link;
  UINTN                 BufferSize;

  ControllerState   = NULL;
  DeviceState       = NULL;

  if (HealthStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ControllerHandle == NULL) {
    *HealthStatus = EfiDriverHealthStatusHealthy;
    if (mNumNotHealthy != 0) {
      *HealthStatus = EfiDriverHealthStatusFailed;
    }
    return EFI_SUCCESS;
  }

  //
  // This is a device driver, so ChildHandle must be NULL.
  //
  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }
  //
  // Make sure this driver is currently managing ControllerHandle
  //
  Status = EfiTestManagedDevice (
             ControllerHandle,
             gDiskIoDriverBinding.DriverBindingHandle,
             &gEfiBlockIoProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (HealthStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = gBS->HandleProtocol (ControllerHandle, &gEfiDiskIoProtocolGuid, (VOID **) &DiskIo);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (HealthStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = DISK_IO_PRIVATE_DATA_FROM_THIS (DiskIo);


  //
  // Get the Controller State from global list
  //
  Link = GetFirstNode (&mControllerList);

  while (!IsNull (&mControllerList, Link)) {
    ControllerState = DISK_IO_CONTROLLER_STATE_FROM_LINK (Link);

    if (ControllerState->ControllerHandle == ControllerHandle) {
      DeviceState = GetDeviceState (ControllerState->DeviceStateNum);
      break;
    }
    Link = GetNextNode (&mControllerList, Link);
  }

  ASSERT (DeviceState != NULL);

  if (DeviceState->HealthStatus == EfiDriverHealthStatusConfigurationRequired) {
    
    //
    // Read the configuration for this device
    //
    BufferSize = sizeof (Private->NVdata.ConfigGood);
    Status = gRT->GetVariable (
                    L"Config",
                    &gEfiCallerIdGuid,
                    NULL,
                    &BufferSize,
                    &Private->NVdata.ConfigGood
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // If the config value is 1, then the configuration is valid and the state machine can move to the next state
    // Otherwise, the state machine returns ConfigurationRequired again
    //
    if (Private->NVdata.ConfigGood == ControllerState->ControllerIndex) {
      if (DeviceState->HealthStatus != EfiDriverHealthStatusHealthy || DeviceState->StringId != 0) {
        mNumNotHealthy--;
      }

      ControllerState->DeviceStateNum = DeviceState->NextState;

      DeviceState = GetDeviceState (ControllerState->DeviceStateNum);
      ASSERT (DeviceState != NULL);

      if (DeviceState->HealthStatus != EfiDriverHealthStatusHealthy) {
        mNumNotHealthy++;
      }
    }
  }    
  
  *HealthStatus = DeviceState->HealthStatus;

  if (MessageList != NULL) {
    *MessageList = NULL;
    if (DeviceState->StringId != 0) {
      *MessageList = AllocateZeroPool (sizeof(EFI_DRIVER_HEALTH_HII_MESSAGE) * 2);
      if (*MessageList == NULL) {
        return EFI_UNSUPPORTED;
      }
      (*MessageList)[0].HiiHandle = mHiiHandle;
      (*MessageList)[0].StringId  = DeviceState->StringId;
    } else {
      *MessageList = AllocateZeroPool (sizeof(EFI_DRIVER_HEALTH_HII_MESSAGE) * 1);
      if (*MessageList == NULL) {
        return EFI_UNSUPPORTED;
      }
    }
  }
  if (FormHiiHandle != NULL) {
    *FormHiiHandle = mHiiHandle;
  }

  if (DeviceState->HealthStatus == EfiDriverHealthStatusConfigurationRequired) {
    Private->NVdata.ConfigGood = 0;
    Status = gRT->SetVariable (
                    L"Config",
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (Private->NVdata.ConfigGood),
                    &Private->NVdata.ConfigGood
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Performs a repair operation on a controller in the platform.  This function can 
  optionally report repair progress information back to the platform. 
  
  @param  This             A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param  ControllerHandle The handle of the controller to repair.
  @param  ChildHandle      The handle of the child controller to repair.  This is 
                           an optional parameter that may be NULL.  It will be NULL 
                           for device drivers.  It will also be NULL for bus 
                           drivers when an attempt is made to repair a bus controller.
                           If will not be NULL when an attempt is made to repair a 
                           child controller produced by the driver.
  @param  RepairNotify     A notification function that may be used by a driver to 
                           report the progress of the repair operation.  This is 
                           an optional parameter that may be NULL.  


  @retval EFI_SUCCESS	          An attempt to repair the controller specified by 
                                ControllerHandle and ChildHandle was performed.  
                                The result of the repair operation can be 
                                determined by calling GetHealthStatus().
  @retval EFI_UNSUPPORTED	      The driver specified by This is not currently 
                                managing the controller specified by ControllerHandle 
                                and ChildHandle.
  @retval EFI_OUT_OF_RESOURCES	There are not enough resources to perform the 
                                repair operation.

*/
EFI_STATUS
DiskIoDriverHealthRepair (
  IN  EFI_DRIVER_HEALTH_PROTOCOL                *This,
  IN  EFI_HANDLE                                ControllerHandle,
  IN  EFI_HANDLE                                ChildHandle       OPTIONAL,
  IN  EFI_DRIVER_HEALTH_REPAIR_PROGRESS_NOTIFY  RepairNotify      OPTIONAL
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  DEVICE_STATE          *DeviceState;
  CONTROLLER_STATE      *ControllerState;
  LIST_ENTRY            *Link;
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  DISK_IO_PRIVATE_DATA  *Private;

  Index           = 0;
  ControllerState = NULL;
  DeviceState     = NULL;
  //
  // This is a device driver, so ChildHandle must be NULL.
  //
  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }
  //
  // Make sure this driver is currently managing ControllerHandle
  //
  Status = EfiTestManagedDevice (
             ControllerHandle,
             gDiskIoDriverBinding.DriverBindingHandle,
             &gEfiBlockIoProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (ControllerHandle, &gEfiDiskIoProtocolGuid, (VOID **) &DiskIo);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = DISK_IO_PRIVATE_DATA_FROM_THIS (DiskIo);

  Link = GetFirstNode (&mControllerList);

  while (!IsNull (&mControllerList, Link)) {
    ControllerState = DISK_IO_CONTROLLER_STATE_FROM_LINK (Link);

    if (ControllerState->ControllerHandle == ControllerHandle) {
      DeviceState = GetDeviceState (ControllerState->DeviceStateNum);
      break;
    }
    Link = GetNextNode (&mControllerList, Link);
  }

  ASSERT (DeviceState != NULL);
  //
  // Check to see if the controller has already been repaired
  //
  if (DeviceState->HealthStatus != EfiDriverHealthStatusRepairRequired) {
    return EFI_SUCCESS;
  }

  if (DeviceState->RepairNotify) {
    do {
      RepairNotify(Index, 10);
      Index++;
    } while ((gBS->Stall(100000) == EFI_SUCCESS) && (Index < 10));
  }

  if (DeviceState->HealthStatus != EfiDriverHealthStatusHealthy || DeviceState->StringId != 0) {
    mNumNotHealthy--;
  }
  
  //
  // Repair success, go to next state
  //
  ControllerState->DeviceStateNum = DeviceState->NextState;

  DeviceState = GetDeviceState (ControllerState->DeviceStateNum);
  ASSERT (DeviceState != NULL);

  if (DeviceState->HealthStatus != EfiDriverHealthStatusHealthy || DeviceState->StringId != 0) {
    mNumNotHealthy++;
  }

  if (DeviceState->HealthStatus == EfiDriverHealthStatusConfigurationRequired) {
    Private->NVdata.ConfigGood = 0;
    Status = gRT->SetVariable (
                    L"Config",
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (Private->NVdata.ConfigGood),
                    &Private->NVdata.ConfigGood
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for module DiskIo. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeDiskIo (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS        Status;
  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDiskIoDriverBinding,
           ImageHandle,
           &gDiskIoComponentName,
           &gDiskIoComponentName2
           );

  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiDriverHealthProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gDiskIoDriverHealth
                  );
  ASSERT_EFI_ERROR (Status);

  InitializeListHead (&mControllerList);

  gDiskIoPrivateDataTemplate.Handle = ImageHandle;
  Status = DiskIoConfigFormInit ();

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Initialize the serial configuration form.

  @retval EFI_SUCCESS              The serial configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
**/
EFI_STATUS
DiskIoConfigFormInit (
  VOID
  )
{
  EFI_STATUS                       Status;

  //
  // Locate Hii Database protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&gDiskIoPrivateDataTemplate.HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&gDiskIoPrivateDataTemplate.HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
    &gDiskIoPrivateDataTemplate.Handle,
    &gEfiDevicePathProtocolGuid, &mHiiVendorDevicePathDiskIoDummy,
    &gEfiHiiConfigAccessProtocolGuid, &gDiskIoPrivateDataTemplate.ConfigAccess,
    NULL
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Publish our HII data
  //
  mHiiHandle = HiiAddPackages (
    &gEfiCallerIdGuid,
    gDiskIoPrivateDataTemplate.Handle,
    DriverHealthDxeStrings,
    DriverHealthVfrBin,
    NULL
    );
  if (mHiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
DummyDriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{

  DISK_IO_NV_DATA  *IfrNvData;

  //
  // Retrieve uncommitted data from Browser
  //

  IfrNvData = AllocateZeroPool (sizeof (DISK_IO_NV_DATA));
  ASSERT (IfrNvData != NULL);

  if (!HiiGetBrowserData (&gEfiCallerIdGuid, VariableName, sizeof (DISK_IO_NV_DATA), (UINT8 *) IfrNvData)) {
    FreePool (IfrNvData);
    return EFI_NOT_FOUND;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;


  return EFI_SUCCESS;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
DummyExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  DISK_IO_PRIVATE_DATA             *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       ConfigRequestHdr;
  UINTN                            Size;
  
  if (Progress == NULL || Results == NULL || Request == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize the local variables.
  //
  ConfigRequestHdr  = NULL;
  ConfigRequest     = NULL;
  Size              = 0;
  *Progress         = Request;

  PrivateData = DISK_IO_PRIVATE_DATA_FROM_CONFIG_ACCESS(This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (DISK_IO_NV_DATA);
  Status = gRT->GetVariable (
            VariableName,
            &gEfiCallerIdGuid,
            NULL,
            &BufferSize,
            &PrivateData->NVdata
            );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  
  if (Request == NULL) {
    //
    // Request is set to NULL, construct full request string.
    //

    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template 
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gEfiCallerIdGuid, VariableName, PrivateData->Handle);
    Size = (StrLen (ConfigRequest) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  } else {
    //
    // Check routing data in <ConfigHdr>.
    // Note: if only one Storage is used, then this checking could be skipped.
    //
    if (!HiiIsConfigHdrMatch (Request, &gEfiCallerIdGuid, VariableName)) {
      return EFI_NOT_FOUND;
    }
    ConfigRequest = Request;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                                HiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &PrivateData->NVdata,
                                BufferSize,
                                Results,
                                Progress
                                );
  
  if (Request == NULL) {
    FreePool (ConfigRequest);
    *Progress = NULL;
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
DummyRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  DISK_IO_PRIVATE_DATA             *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  PrivateData = DISK_IO_PRIVATE_DATA_FROM_CONFIG_ACCESS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gEfiCallerIdGuid, VariableName)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DISK_IO_NV_DATA);
  Status = gRT->GetVariable (
            VariableName,
            &gEfiCallerIdGuid,
            NULL,
            &BufferSize,
            &PrivateData->NVdata
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (DISK_IO_NV_DATA);
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               Configuration,
                               (UINT8 *) &PrivateData->NVdata,
                               &BufferSize,
                               Progress
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Store Buffer Storage back to EFI variable
  //
  Status = gRT->SetVariable(
                  VariableName,
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (DISK_IO_NV_DATA),
                  &PrivateData->NVdata
                  );

  return Status;
}
