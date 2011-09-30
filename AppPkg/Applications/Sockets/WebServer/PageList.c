/*++
  This file contains an 'Intel UEFI Application' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/*++

Copyright (c)  2011 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

--*/

/** @file
  List of pages to display

**/

#include <WebServer.h>


/**
  List of pages in the system
**/
CONST DT_PAGE mPageList[] = {

  //
  //  The index page must be first
  //
  { L"/", IndexPage, L"Index of pages" },   ///<  List the pages

  //
  //  All other pages follow in alphabetical order
  //
  { PAGE_BOOT_SERVICES_TABLE, BootServicesTablePage, L"Boot Services Table" },          ///<  Format boot services table
  { PAGE_CONFIGURATION_TABLE, ConfigurationTablePage, L"Configuration Table" },         ///<  Format configuration table
  { L"/DhcpOptions", DhcpOptionsPage, L"DHCP Options" },                                ///<  Display the DHCP options
  { PAGE_ACPI_DSDT, AcpiDsdtPage, L"DSDT - Differentiated System Description Table" },  ///<  Format DSDT
  { PAGE_DXE_SERVICES_TABLE, DxeServicesTablePage, L"DXE Services Table" },             ///<  Format DXE services table
  { PAGE_ACPI_FADT, AcpiFadtPage, L"FADT - Fixed ACPI Description Table" },             ///<  Format FADT
  { L"/Firmware", FirmwarePage, L"Firmware" },          ///<  Firmware status
  { L"/Handles", HandlePage, L"Display handles and associated protocol GUIDs" },        ///<  Handle database page
  { L"/Hello", HelloPage, L"Hello World" },             ///<  Hello world page
  { L"/Reboot", RebootPage, L"Reboot the sytem" },      ///<  Reboot page
  { PAGE_ACPI_RSDP_10B, AcpiRsdp10Page, L"RSDP 1.0b - ACPI Root System Description Pointer" },  ///<  Format RSDP 1.0b table
  { PAGE_ACPI_RSDP_30, AcpiRsdp30Page, L"RSDP 3.0 - ACPI Root System Description Pointer" },    ///<  Format RSDP 3.0 table
  { PAGE_ACPI_RSDT, AcpiRsdtPage, L"RSDT - ACPI Root System Description Table" },       ///<  Format RSDT
  { PAGE_RUNTIME_SERVICES_TABLE, RuntimeSservicesTablePage, L"Runtime Services Table" },///<  Format runtime services table
  { L"/SystemTable", SystemTablePage, L"System Table" } ///<  Format system table
};

CONST UINTN mPageCount = DIM ( mPageList );
