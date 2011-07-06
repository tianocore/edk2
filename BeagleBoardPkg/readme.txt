On Ubuntu 10.04, in your $(WORKROOT) directory (eg: ~/dev/)

Build UEFI for the BeagleBoard :
================================
# Requirements
sudo apt-get install uuid-dev

# Get the arm-none-eabi Toolchain:
cd $(WORKROOT)
wget http://www.codesourcery.com/sgpp/lite/arm/portal/package7813/public/arm-none-eabi/arm-2010.09-51-arm-none-eabi-i686-pc-linux-gnu.tar.bz2
tar xjf arm-2010.09-51-arm-none-eabi-i686-pc-linux-gnu.tar.bz2
Add the arm-none-eabi toolchain to your path

# Build UEFI
svn co https://edk2.svn.sourceforge.net/svnroot/edk2/trunk/edk2 edk2 --username guest
cd $(WORKROOT)/edk2
svn co https://edk2-fatdriver2.svn.sourceforge.net/svnroot/edk2-fatdriver2/trunk/FatPkg FatPkg --username guest
patch -p1 < ArmPlatformPkg/Documentation/patches/BaseTools-Pending-Patches.patch
cd BeagleBoardPkg/
./build.sh

# To Build a Release verion of UEFI
./build.sh RELEASE


Test UEFI on qEmu :
===================

Installing Linaro qEmu:
-----------------------
cd $(WORKROOT)
git clone git://git.linaro.org/qemu/qemu-linaro.git
cd $(WORKROOT)/qemu-linaro
./configure --target-list=arm-softmmu,arm-linux-user,armeb-linux-user
make

Installing Linaro image Creator:
--------------------------------
wget http://launchpad.net/linaro-image-tools/trunk/0.4.8/+download/linaro-image-tools-0.4.8.tar.gz
tar xzf linaro-image-tools-0.4.8.tar.gz
cd $(WORKROOT)/linaro-image-tools-0.4.8/
sudo apt-get install parted dosfstools uboot-mkimage python-argparse python-dbus python-debian python-parted qemu-arm-static btrfs-tools command-not-found

Creating u-boot + Linux Linaro image:
-------------------------------------
mkdir $(WORKROOT)/beagle_image && cd $(WORKROOT)/beagle_image
wget http://releases.linaro.org/platform/linaro-m/hwpacks/final/hwpack_linaro-omap3_20101109-1_armel_supported.tar.gz
wget http://releases.linaro.org/platform/linaro-m/headless/release-candidate/linaro-m-headless-tar-20101101-0.tar.gz
sudo $(WORKROOT)/linaro-image-tools-0.4.8/linaro-media-create --image_file beagle_sd.img --dev beagle --binary linaro-m-headless-tar-20101101-0.tar.gz --hwpack hwpack_linaro-omap3_20101109-1_armel_supported.tar.gz
sudo chmod a+rw beagle_sd.img

Test u-boot + Linux Linaro image on qEmu:
-----------------------------------------
$(WORKROOT)/qemu-linaro/arm-softmmu/qemu-system-arm -M beagle -sd $(WORKROOT)/beagle_image/beagle_sd.img -serial stdio -clock unix
# in u-boot:
boot

Start UEFI from NOR Flash :
---------------------------
# Adding zImage to beagle_sd.img
mkdir /tmp/beagle_img1
sudo mount -o loop,offset=$[63*512] $(WORKROOT)/beagle_image/beagle_sd.img /tmp/beagle_img1
cp zImage /tmp/beagle_img1
sudo umount /tmp/beagle_img1

./qemu-system-arm -M beagle -mtdblock /work/tianocore/Build/BeagleBoard/DEBUG_ARMGCC/FV/BeagleBoard_EFI_flashboot.fd -serial stdio -sd /work/linaro-image-tools-0.4.8/beagle_sd.img

Start UEFI from SD card :
-------------------------
# To replace u-boot by uefi in the SD card
1) Build the BeagleBoard UEFI firmware without the OMAP353x header
cd $(WORKROOT)/edk2/BeagleBoardPkg/
./build.sh -D EDK2_SECOND_STAGE_BOOTOLADER=1

2) Replace u-boot by UEFI
sudo mount -o loop,offset=$[63*512] $(WORKROOT)/beagle_image/beagle_sd.img /tmp/beagle_img1
sudo cp ../Build/BeagleBoard/DEBUG_ARMGCC/FV/BEAGLEBOARD_EFI.fd /tmp/beagle_img1/u-boot.bin
sudo umount /tmp/beagle_img1
