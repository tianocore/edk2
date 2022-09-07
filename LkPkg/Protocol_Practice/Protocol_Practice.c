#include <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>

/***
    Print a welcoming message.

    Establishes the main structure of the application.

    @retval  0         The application exited normally.
    @retval  Other     An error occurred.
***/
#define  count 10
EFI_STATUS
EFIAPI
UefiMain (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_STATUS Status;
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

    UINT32                                ModeNumber = 0 ;
    UINTN                                 SizeOfInfo = 0 ;
    UINTN                                 i;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
    // EFI_GRAPHICS_OUTPUT_BLT_PIXEL red = {0, 0, 255, 0};
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL blue = {255, 0, 0, 0};
    EFI_PHYSICAL_ADDRESS  Bluestart;


    Status = gBS->LocateHandleBuffer(
                ByProtocol,
                &gEfiGraphicsOutputProtocolGuid,
                NULL,
                &HandleCount,
                &HandleBuffer
    );
    Print(L"lk Status = %r\n", Status);
    
    if (EFI_ERROR(Status))
    {
        Print(L"LocateHandleBuffer failed !\n");
        return Status;
    }
    Print(L" HandleCount = %d \n", HandleCount);

    Status = gBS->OpenProtocol(
                    HandleBuffer[0],
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID **)&Gop,
                    ImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    Print(L"lk OpenProtocol Status = %r\n", Status);

    for ( i = 0; i < Gop->Mode->MaxMode; i++)
    {
        Status = Gop->QueryMode(
        Gop,
        ModeNumber,
        &SizeOfInfo,
        &Info
        );
        if (!EFI_ERROR(Status)) {
            Print(L"ModeNumber = 0x%x ,SizeOfInfo = %d \n",ModeNumber, SizeOfInfo );
            Print(L"VerticalResolution = %d ,HorizontalResolution = %d  \n",Info->VerticalResolution, Info->HorizontalResolution);
            Print(L"Gop->Mode->Info->PixelsPerScanLine  = %d \n", Info->PixelsPerScanLine);
        }
    }
    // //»­¿ò
    // Status = Gop->Blt(
    //     Gop,
    //     &red,
    //     EfiBltVideoFill,
    //     0,0,
    //     0,0,
    //     100,100,
    //     0
    //     );
    // if (EFI_ERROR(Status))
    // {
    //     Print(L"blt failed !\n");
    //     return Status;
    // }
    // Status =  Gop->Blt(
    //         Gop,
    //         NULL,
    //         EfiBltVideoToVideo,
    //         0,0,
    //         200,200,
    //         100,100,
    //         0
    //         );

    Bluestart = Gop->Mode->FrameBufferBase + Gop->Mode->Info->PixelsPerScanLine * 20 * 4;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL * Videohandle;
    Videohandle = ( EFI_GRAPHICS_OUTPUT_BLT_PIXEL * )(UINTN)Bluestart;
    UINTN NoPixels = Gop->Mode->Info->PixelsPerScanLine * 20;
    Print(L"NoPixels = %d  !\n", NoPixels);
    for ( i = 0; i < NoPixels; i++)
    {
        *Videohandle = blue;
        Videohandle++;
        Print(L"i = %d  !\n", i);
    }
    



    return Status;
}

// VOID  Drawdialog(){

// }