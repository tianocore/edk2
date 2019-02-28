# **EDK II firmware for Minnowboard Max/Turbot which is based on Intel Valleyview2 SoC (Byatrail platform)**

----------


## **How to Create a Full Source Tree for Minnowboard Max/Turbot under Windows**
### Pre-requisites

* GIT client: Available from https://git-scm.com/downloads

### Download Source Code
* Create a new directory C:\WORKSPACE as an EDK II work space.

* GIT clone operations required to pull the EDK II source tree and the edk2-non-osi repository. Run below command in git bash or windows command line.

  - cd C:\WORKSPACE
  - git clone https://github.com/tianocore/edk2.git
  - git clone https://github.com/tianocore/edk2-non-osi.git
  
    Note: The EDK II [Multiple Workspace](https://github.com/tianocore/tianocore.github.io/wiki/Multiple_Workspace)
feature is used by this project.
  
* Follow the instructions found in the file "OpenSSL-HOWTO.txt" in your work space (e.g. "C:\WORKSPACE\edk2\CryptoPkg\Library\OpensslLib\OpenSSL-HOWTO.txt") to install the Openssl source code.

## **Windows Build Instructions**

### Pre-requisites Tools

* Microsoft Visual Studio.
  - Visual Studio 2015 recommended and is used in the examples below. Visual Studio 2013 is also supported.
  
* WINDDK
  - Download Microsoft Windows Driver Development Kit 3790.1830 and install it to C:\WINDDK\3790.1830.
  
* Python 2.7
  - Available from http://www.python.org. Install Python to C:\Python27, and add the path "C:\Python27" to system environment variable **PYTHON_HOME**.

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



### Build Commands
  * cd C:\WORKSPACE\edk2\Vlv2TbltDevicePkg

  * To build 64-bit release version image: Build_IFWI.bat MNW2 Release

  * To build 64-bit debug version image: Build_IFWI.bat MNW2 Debug
  
  * To build 32-bit release version image: Build_IFWI.bat /IA32 MNW2 Release

  * To build 32-bit debug version image: Build_IFWI.bat /IA32 MNW2 Debug

### Output
* After the build process successfully completes, the 8MB firmware binary image will
be located in the following location on your local hard drive:
"C:\WORKSPACE\edk2\Vlv2TbltDevicePkg\Stitch\"


