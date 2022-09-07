#include <Uefi.h>
#include  <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/Acpi.h>

/***
    Print a welcoming message.

    Establishes the main structure of the application.

    @retval  0         The application exited normally.
    @retval  Other     An error occurred.
***/
#define  count 10
void WaitKey(){
    EFI_INPUT_KEY   Key;
    EFI_STATUS     Status;
    UINTN          Index;
    
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (EFI_ERROR(Status))
    {
        Print(L"ReadKeyStroke fail\n");
    }
    Print(L"ReadKeyStroke sucess\n");
    
    
}


VOID ListAcpiTable(VOID)
{
    UINTN     i,j,EntryCount;
    CHAR8 strBuff[20];
    UINT64    *EntryPtr;
    EFI_GUID  AcpiTableGuid  = ACPI_TABLE_GUID;
    EFI_GUID  Acpi2TableGuid = EFI_ACPI_TABLE_GUID;
    EFI_CONFIGURATION_TABLE   *configTab=NULL;  
    EFI_ACPI_DESCRIPTION_HEADER           *XSDT,*Entry,*DSDT;
    EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE   *FADT;
    EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Root;

    Print(L"List ACPI Table:\n");
    configTab=gST->ConfigurationTable;
    Print(L"NumberOfTableEntrie = 0x%x\n", gST->NumberOfTableEntries);
    for (i=0;i < gST->NumberOfTableEntries;i++)
    {   
        //Step1. Find the table for ACPI
        if ((CompareGuid(&configTab->VendorGuid,&AcpiTableGuid) == 0) ||
        (CompareGuid(&configTab->VendorGuid,&Acpi2TableGuid) == 0))
        { 
            Print(L"Found table: %g\n",&configTab->VendorGuid); 
            Print(L"Address: @[0x%p]\n",configTab);
            
            Root=configTab->VendorTable;
            Print(L"ROOT SYSTEM DESCRIPTION @[0x%p]\n",Root);
            ZeroMem(strBuff,sizeof(strBuff));
            CopyMem(strBuff,&(Root->Signature),sizeof(UINT64));
            Print(L"RSDP-Signature [%a] (",strBuff);
            for(j=0;j<8;j++)  
            Print(L"0x%x ",strBuff[j]);
            Print(L")\n");
            Print(L"RSDP-Revision [%d]\n",Root->Revision);
            ZeroMem(strBuff,sizeof(strBuff));
            for (j=0;j<6;j++) { strBuff[j]= (Root->OemId[j] & 0xFF); }
            Print(L"RSDP-OEMID [%a]\n",strBuff);
            Print(L"RSDT address= [0x%p], Length=[0x%X]\n",Root->RsdtAddress,Root->Length);
            Print(L"XSDT address= [0x%LX]\n",Root->XsdtAddress);
            WaitKey();
            // Step2. Check the Revision, we olny accept Revision >= 2
            if (Root->Revision >= EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION)
            {
            // Step3. Get XSDT address
            XSDT=(EFI_ACPI_DESCRIPTION_HEADER *)(UINTN) Root->XsdtAddress;
            EntryCount = (XSDT->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) 
                        / sizeof(UINT64);
            ZeroMem(strBuff,sizeof(strBuff));
            CopyMem(strBuff,&(XSDT->Signature),sizeof(UINT32));
            Print(L"XSDT-Sign [%a]\n",strBuff);           
            Print(L"XSDT-length [%d]\n",XSDT->Length);            
            Print(L"XSDT-Counter [%d]\n",EntryCount); 
                    
            // Step4. Check the signature of every entry
            EntryPtr=(UINT64 *)(XSDT+1);
            for (j=0;j<EntryCount; j++,EntryPtr++)
                {
                    
                    Entry=(EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(*EntryPtr));
                    
                    // Step5. Find the FADT table
                    if (Entry->Signature==0x50434146) { //'FACP'
                    FADT = (EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE *)(UINTN) Entry;
                    Print(L"FADT->Dsdt = 0x%X\n",FADT->Dsdt);
                    Print(L"FADT->xDsdt = 0x%LX\n",FADT->XDsdt);
                    
                    // Step6. Get DSDT address
                    DSDT = (EFI_ACPI_DESCRIPTION_HEADER *) (FADT->Dsdt);
                    Print(L"DSDT table @[%X]\n",DSDT);
                    Print(L"DSDT-Length = 0x%x\n",DSDT->Length);
                    Print(L"DSDT-Checksum = 0x%x\n",DSDT->Checksum);
                    }
                }           
            }
        }
        configTab++;
    }
}

EFI_STATUS
EFIAPI
UefiMain (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
)
{

    ListAcpiTable();

    return 0;
}