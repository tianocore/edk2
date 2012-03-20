/**
  @file
  List of pages to display

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  { PAGE_ACPI_APIC, AcpiApicPage, L"APIC" },            ///<  Format APIC
  { PAGE_ACPI_BGRT, AcpiBgrtPage, L"BGRT" },            ///<  Format BGRT
  { PAGE_ACPI_DSDT, AcpiDsdtPage, L"DSDT - Differentiated System Description Table" },  ///<  Format DSDT
  { PAGE_DXE_SERVICES_TABLE, DxeServicesTablePage, L"DXE Services Table" },             ///<  Format DXE services table
  { L"/Exit", ExitPage, L"Exit the web server" },       ///<  Exit the web server application
  { PAGE_ACPI_FADT, AcpiFadtPage, L"FADT - Fixed ACPI Description Table" },             ///<  Format FADT
  { L"/Firmware", FirmwarePage, L"Firmware" },          ///<  Firmware status
  { L"/Handles", HandlePage, L"Display handles and associated protocol GUIDs" },        ///<  Handle database page
  { L"/Hello", HelloPage, L"Hello World" },             ///<  Hello world page
  { PAGE_ACPI_HPET, AcpiHpetPage, L"HPET" },            ///<  Format HPET
  { PAGE_ACPI_MCFG, AcpiMcfgPage, L"MCFG" },            ///<  Format MCFG
  { L"/MemoryMap", MemoryMapPage, L"Memory Map" },      ///<  Memory list
#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
  { L"/MTRRs", MemoryTypeRegistersPage, L"Memory Type Range Registers" }, ///<  Memory type range register table
#endif  //  Intel
  { L"/Ports", PortsPage, L"Display web-server ports" },///<  Web-server ports page
  { L"/Reboot", RebootPage, L"Reboot the sytem" },      ///<  Reboot page
  { PAGE_ACPI_RSDP_10B, AcpiRsdp10Page, L"RSDP 1.0b - ACPI Root System Description Pointer" },  ///<  Format RSDP 1.0b table
  { PAGE_ACPI_RSDP_30, AcpiRsdp30Page, L"RSDP 3.0 - ACPI Root System Description Pointer" },    ///<  Format RSDP 3.0 table
  { PAGE_ACPI_RSDT, AcpiRsdtPage, L"RSDT - ACPI Root System Description Table" },       ///<  Format RSDT
  { PAGE_RUNTIME_SERVICES_TABLE, RuntimeSservicesTablePage, L"Runtime Services Table" },///<  Format runtime services table
  { PAGE_ACPI_SSDT, AcpiSsdtPage, L"SSDT" },            ///<  Format SSDT
  { L"/SystemTable", SystemTablePage, L"System Table" },///<  Format system table
  { PAGE_ACPI_TCPA, AcpiTcpaPage, L"TCPA" },            ///<  Format TCPA
  { PAGE_ACPI_UEFI, AcpiUefiPage, L"UEFI" }             ///<  Format UEFI
};

CONST UINTN mPageCount = DIM ( mPageList );
