# **EDK II firmware for Minnowboard Max/Turbot which is based on Intel Valleyview2 SoC (Byatrail platform)**

----------
# Windows Pre-requisites

* GIT client: Available from https://git-scm.com/downloads

* Microsoft Visual Studio.
  - Visual Studio 2015 recommended and is used in the examples below. Visual Studio 2013 is also supported.

* WINDDK
  - Download Microsoft Windows Driver Development Kit 3790.1830 and install it to C:\WINDDK\3790.1830.

* Python 3
  - https://www.python.org/downloads/

* Install iASL
   - Install the iasl compiler by downloading iasl-win-20160527.zip from the following
   location: "https://acpica.org/downloads/" and place the unzipped
   content ("iasl.exe") into the directory "C:\ASL" on your local hard drive
   (create the folder "C:\ASL" if it does not exist).

* Install the NASM* assembly language compiler
   - Download NASM* 2.12.02 binaries from
   http://www.nasm.us/pub/nasm/releasebuilds/2.12.02/win64/nasm-2.12.02-win64.zip and place the
   unzipped content ("nasm.exe") into the directory "C:\NASM" on your local hard drive
   (create the folder "C:\NASM" if it does not exist). Add the path "C:\NASM\" to system environment variable **NASM_PREFIX**.

* Install Openssl
   - Download a pre-compiled Openssl Windows binary from
   https://wiki.openssl.org/index.php/Binaries. Search for a Windows binary in the list
   of "Third Party OpenSSL Related Binary Distributions". Go to the third party site to
   download the latest version. Download and extract to C:\Openssl, add the path of openssl.exe
   ("C:\openssl") to system environment variable **OPENSSL_PATH**.

# Download and Build MinnowMax using Windows/Visual Studio

Run the script below from an empty directory.  The script clones the EDK II
repository from GitHub and downloads and unzips the binary support files for the
MinnowBoard MAX.  It then sets up the environment for EDK II builds and builds
the MinnowBoard MAX firmware and generates UEFI Capsules that can be used to
update the MinnowBoard MAX firmware and three sample devices.

```
git clone --recurse-submodules https://github.com/tianocore/edk2.git

powershell "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri "https://indy.fulgan.com/SSL/openssl-1.0.2r-x64_86-win64.zip -OutFile openssl-1.0.2r-x64_86-win64.zip"}"
powershell Expand-Archive openssl-1.0.2r-x64_86-win64.zip

powershell "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri "https://firmware.intel.com/sites/default/files/MinnowBoardMax-Development190216.zip -OutFile MinnowBoardMax-Development190216.zip"}"
powershell Expand-Archive MinnowBoardMax-Development190216.zip
sleep 1
rename MinnowBoardMax-Development190216 Vlv2Binaries
cd Vlv2Binaries
powershell Expand-Archive Vlv2SocBinPkg.zip .
sleep 1
cd ..

powershell "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri "https://www.nasm.us/pub/nasm/releasebuilds/2.13.03/win64/nasm-2.13.03-win64.zip -OutFile nasm-2.13.03-win64.zip"}"
powershell Expand-Archive nasm-2.13.03-win64.zip .

mkdir Conf

set WORKSPACE=%CD%
set EDK_TOOLS_PATH=%WORKSPACE%\edk2\BaseTools
set EDK_TOOLS_BIN=%EDK_TOOLS_PATH%\BinWrappers\WindowsLike
set PACKAGES_PATH=%WORKSPACE%\edk2;%WORKSPACE%\Vlv2Binaries
path=%path%;%EDK_TOOLS_PATH%\Bin\Win32;%WORKSPACE%\openssl-1.0.2r-x64_86-win64
set NASM_PREFIX=%WORKSPACE%\nasm-2.13.03\

cd %WORKSPACE%\edk2

call edkSetup.bat Rebuild

cd Vlv2TbltDevicePkg

Build_IFWI.bat /m /y MNW2 Debug
```

Once all the code and tools are downloaded and installed, only the following
commands are required to setup the environment.  Run these from the same
directory used to install the source and binaries.

```
set WORKSPACE=%CD%
set EDK_TOOLS_PATH=%WORKSPACE%\edk2\BaseTools
set EDK_TOOLS_BIN=%EDK_TOOLS_PATH%\BinWrappers\WindowsLike
set PACKAGES_PATH=%WORKSPACE%\edk2;%WORKSPACE%\Vlv2Binaries
path=%path%;%EDK_TOOLS_PATH%\Bin\Win32;%WORKSPACE%\openssl-1.0.2r-x64_86-win64
set NASM_PREFIX=%WORKSPACE%\nasm-2.13.03\

cd %WORKSPACE%\edk2

call edkSetup.bat Rebuild
```

Once the environment is setup, the MinnowBoard MAX firmware and capsules can be
rebuilt using the following commands.

* Build Debug Image

```
cd Vlv2TbltDevicePkg
Build_IFWI.bat /m /y MNW2 Debug
```

* Build Release Image

```
cd Vlv2TbltDevicePkg
Build_IFWI.bat /m /y MNW2 Release
```

The generated firmware image is the newest `.bin` file in `edk2/Vlv2TbltDevicePkg/Stitch`.
The file is in the form `MNW2MAX1.X64.0084.D01.<DATE>.bin`.

The CapsuleApp and generated UEFI Capsules are in `Build/Vlv2TbltDevicePkg/Capsules`

# Linux Pre-requisites

* The tool GenBiosId has a dependency on libc.so.6.  Make sure it is installed.
  Here are a few example installation commands:

    sudo dnf install libc.so.6

    apt-get install libc:i386

# Download and Build MinnowMax using Linux/GCC

Run the script below from an empty directory.  The script clones the EDK II
repository from GitHub and downloads and unzips the binary support files for the
MinnowBoard MAX.  It then sets up the environment for EDK II builds and builds
the MinnowBoard MAX firmware and generates UEFI Capsules that can be used to
update the MinnowBoard MAX firmware and three sample devices.

```
git clone --recurse-submodules https://github.com/tianocore/edk2.git

mkdir Vlv2Binaries
cd Vlv2Binaries
wget https://firmware.intel.com/sites/default/files/MinnowBoardMax-Development190216.zip
unzip MinnowBoardMax-Development190216.zip
unzip Vlv2SocBinPkg.zip

cd ..
mkdir Conf

export WORKSPACE=$PWD/edk2
export PACKAGES_PATH=$PWD/Vlv2Binaries
export EDK_TOOLS_PATH=$WORKSPACE/BaseTools

cd edk2
cd Vlv2TbltDevicePkg
. Build_IFWI.sh MNW2 Debug
```

Once all the code is downloaded and installed, only the following commands are
required to setup the environment.  Run these from the same directory used to
install the source and binaries.

```
export WORKSPACE=$PWD/edk2
export PACKAGES_PATH=$PWD/Vlv2Binaries
export EDK_TOOLS_PATH=$WORKSPACE/BaseTools

cd edk2
cd Vlv2TbltDevicePkg
```

Once the environment is setup, the MinnowBoard MAX firmware and capsules can be
rebuilt using the following commands.


* Build Debug Image

```
cd Vlv2TbltDevicePkg
./Build_IFWI.sh MNW2 Debug
```

* Build Release Image

```
cd Vlv2TbltDevicePkg
./Build_IFWI.sh MNW2 Release
```

The generated firmware image is the `MNW2MAX_X64_D_0084_01_GCC.bin` file in
`edk2\Vlv2TbltDevicePkg\Stitch`

The CapsuleApp and generated UEFI Capsules are in `Build\Vlv2TbltDevicePkg\Capsules`

# Use DediProg to update FLASH image on a MinnowBoard MAX Target

# Update MinnowBoard MAX Firmware from UEFI Capsules

* Copy the `Build/Vlv2TbltDevicePkg/Capsules` directory to a USB FLASH drive
* Connect USB FLASH Drive to MinnowBoard MAX
* Boot MinnowBoard MAX to the Boot Manager
* Boot the `EFI Internal Shell` boot option
* Mount the USB FLASH Drive (usually `FS1`)
* Use `cd` command to go to `Capsules/TestCert` directory
* Run the following command to apply all four capsules

```
CapsuleApp.efi Red.cap Green.cap Blue.cap MinnowMax.cap
```

* The MinnowBoard MAX should reboot and the four capsules are applied in the
  order listed.  The progress bar matches the color name of the capsule.
  MinnowMax.cap uses the color purple.  Once all capsules are processed, the
  MinnowBoard MAX should reboot again using the new firmware images.

# Generate and Test a UX BitMap Capsule

* Use bitmap editor to generate a BMP file.  Recommend resolution of 600 wide
  by 100 tell and either 24 or 32 bits per pixel.
* Save BMP file to USB FLASH drive
* Use CapsuleApp.efi to convert BMP file to a UX Capsule

```
CapsuleApp.efi -G MyImage.bmp -O MyImage.cap
```

* When updating firmware using capsules, add UX capsule to the list of capsules
  passed into CapsuleApp.efi.

```
CapsuleApp.efi MyImage.cap Red.cap Green.cap Blue.cap MinnowMax.cap
```

* When the capsules are processed the UX bitmap image should be displayed at the
  bottom of the screen.
