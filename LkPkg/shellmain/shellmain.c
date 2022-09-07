#include <Uefi.h>
#include  <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <AMI/AmiKeycode.h>

#define     DELAY_1_S       1000000
#define     PRINT_COUNT     5


#define     DEFAULT_YEAR    2021
#define     DEFAULT_MOUTH   1
#define     DEFAULT_DAY     1
#define     DEFAULT_HOUR    0
#define     DEFAULT_MINUTE  0
#define     DEFAULT_SECOND  0


AMI_EFI_KEY_DATA AMIKeyData;


EFI_STATUS QuryGraphicInfo(){
    EFI_STATUS Status;
    UINTN       Index;
    UINT32      ModeIndex;
    UINTN       HandlesCount;
    EFI_HANDLE  *Handles;
    UINTN       SizeOfInfo;
    EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info; 

    Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiGraphicsOutputProtocolGuid,
                NULL,
                &HandlesCount,
                &Handles
                );

    Print(L"HandlesCount = %d\n",HandlesCount);
    for (Index = 0; Index < HandlesCount; Index++) {
        GraphicsOutput = NULL;

        Status = gBS->HandleProtocol(Handles[Index], &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
        if (EFI_ERROR (Status)) {
        continue;
        }
        for (ModeIndex = 0; ModeIndex < GraphicsOutput->Mode->MaxMode; ModeIndex++) {

            Status = GraphicsOutput->QueryMode(
                        GraphicsOutput, ModeIndex, &SizeOfInfo, &Info);

            Print(L"modeindex = %d SizeOfInfo = %d\n", ModeIndex, SizeOfInfo);
            Print(L"modeindex = %d Info->Version = 0x%04x\n", ModeIndex, Info->Version);
            Print(L"modeindex = %d Info->HorizontalResolution = %d\n", ModeIndex, Info->HorizontalResolution);
            Print(L"modeindex = %d Info->VerticalResolution = %d\n", ModeIndex, Info->VerticalResolution);
            Print(L"modeindex = %d Info->PixelsPerScanLine = %d\n", ModeIndex, Info->PixelsPerScanLine);
        }
        Status = GraphicsOutput->SetMode(
            GraphicsOutput, 3);
        Print(L"Status = %r\n", Status);

    }
    return Status;
    // Status = gBS->HandleProtocol (gST->ConsoleOutHandle,
    //                 &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);

    // if (EFI_ERROR(Status)){
    //     return Status;
    // }

    
}
VOID SetDefaultTime(EFI_TIME * Time) {
    Time->Year  = DEFAULT_YEAR;
    Time->Month = DEFAULT_MOUTH;
    Time->Day   = DEFAULT_DAY;
    Time->Hour  = DEFAULT_HOUR;
    Time->Minute= DEFAULT_MINUTE;
    Time->Second= DEFAULT_SECOND;
    gRT->SetTime(Time);
    
}
VOID PrintTIme(EFI_TIME curTime) {
    Print(L"CurrentTime : %d-%d-%d  %02d:%02d:%02d\n", \
    curTime.Year, curTime.Month, curTime.Day, \
    curTime.Hour, curTime.Minute, curTime.Second);
}

INTN
EFIAPI
ShellAppMain (
    IN UINTN Argc,
    IN CHAR16 **Argv)
{
    EFI_TIME curTime;
    INTN  Index;
    EFI_STATUS Status;
    Print(L"Please input a key:\n");
    gST->BootServices->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &AMIKeyData.Key);
    Print(L"key's Unicode is 0x%04x\n", AMIKeyData.Key.ScanCode);

    Print(L"hello ,this is Entry of shellMain!\n");
    gBS->Stall(DELAY_1_S);
    gRT->GetTime(&curTime,NULL);
    PrintTIme(curTime);
    SetDefaultTime(&curTime);
    for (Index = 0; Index < PRINT_COUNT; Index++) {
        gRT->GetTime(&curTime,NULL);
        PrintTIme(curTime);
        gBS->Stall(DELAY_1_S);
    }
    Status =  QuryGraphicInfo();
    Print(L"Status = %r\n", Status);

    gST->ConOut->OutputString(gST->ConOut, L"Test Systemtable ....\n\r");
    return 0;

}

