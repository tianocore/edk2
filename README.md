# VgaShim
## Who is this for
Anyone who wants to run Windows 7 natively on Apple MacBook Air (Early 2015) models. Could also work for other Apple and non-Apple machines that do away with legacy loaders and legacy VGA ROM. Was originally devised for a MacBookAir7,2 13" (Early 2015).

## Why is this needed
Windows 7, even the x64 version booting in EFI mode, insists on using old VGA/VESA graphics configuration routines with the 10h interrupt. This used to work on older Apple machines because they had a legacy loader and/or an apropriate VGA ROM that provided that support. In newer computers they decided to remove that legacy compatibility layer and go pure EFI.

## How does it work
This program attempts to:

1. Unlock the (unused but locked for writing) `C0000:CFFFF` memory area where VGA ROM would normally reside using `EFI_LEGACY_REGION_PROTOCOL`, `EFI_LEGACY_REGION2_PROTOCOL` or Memory Type Range Registers depending on which method is available.
2. Install an int10h handler in the `C0000:CFFFF` memory area which handles the most important calls Windows 7 makes and announces lack of support for others. 
3. Fill in and make available to Windows VESA video mode information compatible with the display adapter present that should enable Windows to write directly to the framebuffer before a more robust display driver takes over from vgapnp.sys.
4. Lock the `C0000:CFFFF` memory area to prevent further possibly unauthorized writes.
4. Adjust the Interrupt Vector Table entry for the 10h interrupt handler to point to the shim entry point.
5. Display the Windows wavy flag (or any other animation of choice).
6. Chainload `\efi\microsoft\boot\bootmgfw.efi` if present on the same volume so that Windows can continue loading.

## How to use
The shim needs to run before the Windows Boot Manager. For more information on how to get a dual booting setup working please see [this MacRumors thread](http://forums.macrumors.com/threads/guide-install-windows-7-on-air-2015.1961618/).

## How to build
I used a virtual Lubuntu 15.04 install with GCC 4.9. Edit `Conf/target.txt` after `make`-ing BaseTools if you have a different version of GCC.

    sudo apt-get install uuid-dev nasm tofrodos
    git clone https://github.com/davidcie/VgaShim
    cd VgaShim
    source edksetup.sh BaseTools
    make -C BaseTools/Source/C
    build
You will find the binary in `Build/MdeModule/RELEASE_GCC49/X64/VgaShim.efi` (or a similar folder if using GCC != 4.9).

## Show me teh code
The actual source lives under [`MdeModulePkg/Applications/VgaShim`](https://github.com/davidcie/VgaShim/tree/master/MdeModulePkg/Application/VgaShim).

## Credits
* This solution is based on a VBE shim prepared by the OVMF project (`QemuVideoDxe/VbeShim`).
* My wife for her patience over many coding evenings.
* Several people at MacRumors Forums who helped me understand graphics configuration.
* ReactOS for sharing code that helped me understand what Windows is trying to do.
* The EDK II project (and wider Tianocore) for making EFI app development possible at all.
* Microsoft and Apple for for making this enjoyable journey necessary.
