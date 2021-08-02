## @file
# FDF include file which allows to embed Secure Boot keys
#
#  Copyright (c) 2021, ARM Limited. All rights reserved.
#  Copyright (c) 2021, Semihalf. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

!if $(DEFAULT_KEYS) == TRUE
  FILE FREEFORM = 85254ea7-4759-4fc4-82d4-5eed5fb0a4a0 {
  !ifdef $(PK_DEFAULT_FILE)
    SECTION RAW = $(PK_DEFAULT_FILE)
  !endif
    SECTION UI = "PK Default"
  }

  FILE FREEFORM = 6f64916e-9f7a-4c35-b952-cd041efb05a3 {
  !ifdef $(KEK_DEFAULT_FILE1)
    SECTION RAW = $(KEK_DEFAULT_FILE1)
  !endif
  !ifdef $(KEK_DEFAULT_FILE2)
    SECTION RAW = $(KEK_DEFAULT_FILE2)
  !endif
  !ifdef $(KEK_DEFAULT_FILE3)
    SECTION RAW = $(KEK_DEFAULT_FILE3)
  !endif
    SECTION UI = "KEK Default"
  }

  FILE FREEFORM = c491d352-7623-4843-accc-2791a7574421 {
  !ifdef $(DB_DEFAULT_FILE1)
    SECTION RAW = $(DB_DEFAULT_FILE1)
  !endif
  !ifdef $(DB_DEFAULT_FILE2)
    SECTION RAW = $(DB_DEFAULT_FILE2)
  !endif
  !ifdef $(DB_DEFAULT_FILE3)
    SECTION RAW = $(DB_DEFAULT_FILE3)
  !endif
    SECTION UI = "DB Default"
  }

  FILE FREEFORM = 36c513ee-a338-4976-a0fb-6ddba3dafe87 {
  !ifdef $(DBT_DEFAULT_FILE1)
    SECTION RAW = $(DBT_DEFAULT_FILE1)
  !endif
  !ifdef $(DBT_DEFAULT_FILE2)
    SECTION RAW = $(DBT_DEFAULT_FILE2)
  !endif
  !ifdef $(DBT_DEFAULT_FILE3)
    SECTION RAW = $(DBT_DEFAULT_FILE3)
  !endif
    SECTION UI = "DBT Default"
  }

  FILE FREEFORM = 5740766a-718e-4dc0-9935-c36f7d3f884f {
  !ifdef $(DBX_DEFAULT_FILE1)
    SECTION RAW = $(DBX_DEFAULT_FILE1)
  !endif
  !ifdef $(DBX_DEFAULT_FILE2)
    SECTION RAW = $(DBX_DEFAULT_FILE2)
  !endif
  !ifdef $(DBX_DEFAULT_FILE3)
    SECTION RAW = $(DBX_DEFAULT_FILE3)
  !endif
    SECTION UI = "DBX Default"
  }

!endif
