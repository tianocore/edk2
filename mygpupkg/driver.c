#include <Uefi.h>

#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/DriverBinding.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#define MYGPU_VENDOR_ID 0x1234
#define MYGPU_DEVICE_ID 0x5678

#define MYGPU_WIDTH 1024
#define MYGPU_HEIGHT 768
#define MYGPU_BPP 4
#define MYGPU_FB_SIZE (MYGPU_WIDTH * MYGPU_HEIGHT * MYGPU_BPP)

#define REG_WIDTH 0x00
#define REG_HEIGHT 0x04
#define REG_ENABLE 0x08
#define REG_FLUSH 0x0C

typedef struct
{
    UINT32 VendorId;
    UINT32 DeviceId;
    UINT16 Command;
    UINT16 Status;
    UINT8 RevisionId;
    UINT8 ProgIf;
    UINT8 SubClass;
    UINT8 BaseClass;
} PCI_TYPE0_HEAD_MIN;

typedef struct
{
    EFI_HANDLE Handle;
    EFI_PCI_IO_PROTOCOL *PciIo;

    EFI_PHYSICAL_ADDRESS MmioBase;
    UINT64 MmioSize;

    EFI_PHYSICAL_ADDRESS FramebufferBase;
    UINT64 FrameBufferSize;

    EFI_GRAPHICS_OUTPUT_PROTOCOL Gop;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE GopMode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION GopModeInfo;
} MYGPU_PRIVATE;

EFI_STATUS
EFIAPI
MyGpuGopQueryMode(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber,
    OUT UINTN *SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info)
{
    MYGPU_PRIVATE *Private = NULL;

    if (This == NULL || SizeOfInfo == NULL || Info == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    if (ModeNumber != 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    Private = BASE_CR(This, MYGPU_PRIVATE, Gop);

    *SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    *Info = AllocateCopyPool(*SizeOfInfo, &Private->GopModeInfo);
    if (*Info == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MyGpuGopSetMode(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber)
{
    MYGPU_PRIVATE *Private = NULL;
    UINT32 Enable;

    if (This == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }
    if (ModeNumber != 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    Private = BASE_CR(This, MYGPU_PRIVATE, Gop);

    Enable = 1;
    Private->PciIo->Mem.Write(Private->PciIo,
                              EfiPciIoWidthUint32,
                              0,
                              REG_ENABLE,
                              1,
                              &Enable);

    Private->GopMode.Mode = ModeNumber;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MyGpuGopBlt(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX,
    IN UINTN SourceY,
    IN UINTN DestinationX,
    IN UINTN DestinationY,
    IN UINTN Width,
    IN UINTN Height,
    IN UINTN Delta OPTIONAL)
{
    MYGPU_PRIVATE *Private = NULL;
    UINT8 *Fb;
    UINTN X, Y;
    UINTN SrcStride;
    UINT32 Flush;

    if (This == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    Private = BASE_CR(This, MYGPU_PRIVATE, Gop);
    Fb = (UINT8 *)(UINTN)Private->FramebufferBase;

    if (DestinationX + Width > Private->GopModeInfo.HorizontalResolution ||
        DestinationY + Height > Private->GopModeInfo.VerticalResolution)
    {
        return EFI_INVALID_PARAMETER;
    }

    if (Delta == 0)
    {
        Delta = Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    }

    switch (BltOperation)
    {
    case EfiBltVideoFill:
        if (BltBuffer == NULL)
        {
            return EFI_INVALID_PARAMETER;
        }
        break;

        for (Y = 0; Y < Height; Y++)
        {
            for (X = 0; X < Width; X++)
            {
                UINT8 *Pixel = Fb + ((DestinationY + Y) * Private->GopModeInfo.PixelsPerScanLine + (DestinationX + X)) * MYGPU_BPP;
                Pixel[0] = BltBuffer->Blue;
                Pixel[1] = BltBuffer->Green;
                Pixel[2] = BltBuffer->Red;
                Pixel[3] = BltBuffer->Reserved;
            }
        }
        break;

    case EfiBltBufferToVideo:
        if (BltBuffer == NULL)
        {
            return EFI_INVALID_PARAMETER;
        }

        for (Y = 0; Y < Height; Y++)
        {
            for (X = 0; X < Width; X++)
            {
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Src;
                UINT8 *Dst;

                Src = &BltBuffer[(SourceY + Y) * SrcStride + (SourceX + X)];
                Dst = Fb + ((DestinationY + Y) * Private->GopModeInfo.PixelsPerScanLine + (DestinationX + X)) * MYGPU_BPP;

                Dst[0] = Src->Blue;
                Dst[1] = Src->Green;
                Dst[2] = Src->Red;
                Dst[3] = Src->Reserved;
            }
        }
        break;

    case EfiBltVideoToBltBuffer:
        if (BltBuffer == NULL)
        {
            return EFI_INVALID_PARAMETER;
        }

        for (Y = 0; Y < Height; Y++)
        {
            for (X = 0; X < Width; X++)
            {
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Dst;
                UINT8 *Src;

                Src = Fb + ((SourceY + Y) * Private->GopModeInfo.PixelsPerScanLine + (SourceX + X)) * MYGPU_BPP;
                Dst = &BltBuffer[(DestinationY + Y) * SrcStride + (DestinationX + X)];

                Dst->Blue = Src[0];
                Dst->Green = Src[1];
                Dst->Red = Src[2];
                Dst->Reserved = Src[3];
            }
        }
        break;

    case EfiBltVideoToVideo:
        for (Y = 0; Y < Height; Y++)
        {
            CopyMem(Fb + ((DestinationY + Y) * Private->GopModeInfo.PixelsPerScanLine + DestinationX) * MYGPU_BPP,
                    Fb + ((SourceY + Y) * Private->GopModeInfo.PixelsPerScanLine + SourceX) * MYGPU_BPP,
                    Width * MYGPU_BPP);
        }
        break;
    default:
        return EFI_UNSUPPORTED;
    }

    Flush = 1;
    Private->PciIo->Mem.Write(Private->PciIo,
                              EfiPciIoWidthUint32,
                              0,
                              REG_FLUSH,
                              1,
                              &Flush);

    return EFI_SUCCESS;
}