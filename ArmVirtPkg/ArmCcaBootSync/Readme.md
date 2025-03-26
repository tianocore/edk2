# Arm CCA Boot Sync

Attestation is an important part of establishing trust before any secrets
can be released to the Realm VM. This requires performing remote attestation
during the early boot stages, i.e. by the Realm Guest firmware.

The Realm Guest firmware does not support networking and instead utilises the
Realm Host Call interface provided by the Realm Service interface defined by
the [1] DEN0137 - Realm Management Monitor specification.

The Realm Host calls allow the requests from the Realm Code to be delivered
to the Host for processing the request and for returning a response.

The following diagram depicts the high-level architecture.

```
            Host                     |          Realm World
           (Normal World)            |
                                     |      ##################
                                     |      #     Realm VM   #
                                     |      #  +----------+  #
                                     |      #  |    OS    |  #
  +---------+           +-----+      |      #  |          |  #
  | User    |           |     |      |      #  +----------+  #
  | Context |<--(net)-->| VMM |      |      #                #
  |         |           |     |      |      #  +----------+  #
  +---------+           +-----+      |      #  | Firmware |  #
                           ^         |      #  +----------+  #
                           |         |      #        ^       #
                           |         |      #        |       #
                    (SMCCC filter    |      #########|########
                    and forward RHI  |               |
                    to user space)   |             (RSI)
                           |         |               |
                           |         |               |
                           v         |               v
                        +-----+      |            +-----+
                        |     |      |            |     |
                        | KVM |<-----|---(RHI)--->| RMM |
                        |     |      |            |     |
                        +-----+      |            +-----+
                                     |
```

Arm CCA Boot Sync is used to enable the following use-cases:
  - UEFI Secure Boot
  - Encrypted Disk Boot

## Introduction

Boot Sync is a mechanism for synchronising Realm boot with communications
required to obtain parameters or secret blocks that can be injected into the
boot sequence.

The [2] DEN0148 - Realm Host Interface specification, describes the ‘Boot
Sync Blocks’ Protocol (BSB Protocol) that uses the RHI_SESSION_* calls as a
transport for exchanging messages between Realm code and a User Context.

The main use case for this BSB protocol is to inject values used in the early
boot sequence of Realm firmware. Such values could be UEFI parameters or
secrets used to protect user data.

Use of the BSB protocol may be required because:
 - there is no network stack available in the early stages of boot
 - in order to maintain confidential computing guarantees, these values
   need to be provided by the User Context of the Realm initiator, which
   is known by the host service context, but difficult to establish for
   the Realm code at this early stage of boot

The Appendix A, in the Realm Host Interface specification describes in
detail how the BSB protocol can be implemented.

### Boot Sync Blocks (BSB protocol)

The BSB protocol can be further divided logically into
the following sub protocols:
 - Key Exchange protocol
 - Attestation and verification protocol
 - Boot information Blocks (BIB) protocol


### Boot Sync Key Exchange protocol

The BSB protocol involves establishing a secure session between the Realm
guest firmware and a User Context.

This is achieved by performing a public key exchange sequence (ECDH in
this implementation) and computing a common key.

Two keys are further derived from the common key, the encryption key (Ke)
and the binding Key (Kb).

The encryption key is used for encrypting the protocol data units being
exchanged between the realm guest firmware and the User Context.

The binding key is used as the challenge data for obtaining an Attestation
Report (AR) from the Realm Management Monitor (RMM). The RMM includes the
challenge data (in this case the binding key) as an attestation claim (in
the Attestation Report) that can be verified by the User Context.

```
           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                |<---------Key Xchg Request-----------|
                |                                     |
                |                                     |
                |---------Key Xchg Response---------->|
  Compute       |                                     |     Compute
  Common        |                                     |     Common
  Key (Kcomm)   |                                     |     Key (Kcomm)
                |                                     |
  Derive        |                                     |     Derive
  Keys (Ke, Kb) |                                     |     Keys (Ke, Kb)
                |                                     |
                /            Other Protocol           \
                \            Communication            /
                |                                     |
                |<----------------FIN-----------------|
  End of        |                                     |     End of
  Communication |                                     |     Communication

                                  OR

                .                                     .
                .                                     .
                .                                     .
                /                                     \
                \                                     /
                |                                     |
                |----------------NACK---------------->|
  End of        |                                     |     End of
  Communication |                                     |     Communication

```

### Attestation protocol

Once a secure session has been established the Realm guest firmware obtains
an attestation report from the RMM passing in the binding key as the challenge
data.

This Attestation Report is then encrypted using the encryption key and sent
to the User Context. The User context can then verify the Attestation Report
(or delegate the verification to another service). Once the verification is
complete the User Context will encrypt and send the result of the attestation
report verification.

```
           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                /            Secure session           \
                \            Established              /
                |                                     |
                |<---------Attestation Request--------|
                |          (Attestation Report)       |
                |                                     |
                |---------Attestation Response------->|
                |         (Attestation Result)        |
                |                                     |
                /            Other Protocol           \
                \            Communication            /
                |                                     |
```


### Boot Information Blocks protocol

Once the attestation report verification is successful, the realm guest
firmware can request the User Context for the Boot information Blocks (which
may include UEFI parameters or secrets used to protect user data) to be
released.

The User Context encrypts the Boot Information Blocks and sends it back to
the Realm guest firmware, who can then proceed to use this information for
initialising the configuration information or secret data to be used for
booting the realm software stack.

```
           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                /            Secure Session           \
                \             Established             /
                |                                     |
                /      Attestation & Verification     \
                \             Completed               /
                |                                     |
                |<--------------BIB-REQ---------------|
                |                                     |
                |---------------BIB-RESP------------->|
                |                                     |
                /                                     \
                \                                     /
                |                                     |
```

### Cryptographic algorithms

The following algorithms are used by this implementation:
 - ECDH key using the ECC Curve-P384 for key exchange
 - AEAD AES-GCM for encryption
 - SHA512 HMAC-based Extract-and-Expand Key Derivation Function (HKDF) for
   key Derivation
 - SHA256 for rolling hash

## Building the components for Arm Boot Sync

Arm Boot Sync Components:
  - FVP Host UEFI firmware
  - Realm Guest UEFI firmware
  - Test User Context Service
  - Provisioning Data script
  - RMM firmware
  - TF-A firmware
  - Kvmtool
  - Linux Kernel
  - Grub

## Build prerequisites

Set up the development environment by following the steps as described in:
https://github.com/tianocore/edk2-platforms/blob/master/Platform/ARM/Readme.md

## Certificates for signing images

### Secure boot signing certificates

1. Create a folder called SecBootCert and change directory to the folder.
   ```
   mkdir SecBootCert
   cd SecBootCert
   ```
2. Generate the certificates <BR>

   ```
   # Choose a PEM pass phrase and remember it as it would be needed later.
   # e.g. The PEM pass phrase used in this example is: secboot <BR>

   # Run the following command to generate the certificates.

   openssl req -new -x509 -newkey rsa:2048 -keyout PK.key -out PK.crt -days 3605 -subj "/CN=My Secure PK/"
   openssl req -new -x509 -newkey rsa:2048 -keyout KEK.key -out KEK.crt -days 3650 -subj "/CN=My Secure KEK/"
   openssl req -new -x509 -newkey rsa:2048 -keyout db.key -out db.crt -days 3650 -subj "/CN=My Secure db/"
   openssl req -new -x509 -newkey rsa:2048 -keyout dbx.key -out dbx.crt -days 3650 -subj "/CN=My Secure dbx/"
   openssl req -new -x509 -newkey rsa:2048 -keyout dbt.key -out dbt.crt -days 3650 -subj "/CN=My Secure dbt/"
   openssl req -new -x509 -newkey rsa:2048 -keyout dbr.key -out dbr.crt -days 3650 -subj "/CN=My Secure dbr/"

   #
   openssl x509 -in PK.crt -out PK.cer -outform DER
   openssl x509 -in KEK.crt -out KEK.cer -outform DER
   openssl x509 -in db.crt -out db.cer -outform DER
   openssl x509 -in dbx.crt -out dbx.cer -outform DER
   openssl x509 -in dbt.crt -out dbt.cer -outform DER
   openssl x509 -in dbr.crt -out dbr.cer -outform DER
   ```

### Grub (detached) signing using GPG keys

Grub needs detached signing (file_name and file_name.sig), which can be achieved
using GPG keys

1. Create a new folder to store the GPG key and change directory to that folder.
   ```
   #
   mkdir GpgKey
   cd GpgKey
   ```

2. Generate a GPG key <BR>
   ```
   # Generate the key
   gpg --full-generate-key --homedir $PWD
   ```

2. Export the public key to be passed to grub build command
   ```
   gpg --homedir GpgKey/ --export > boot.key
   gpg --homedir GpgKey/ --export-secret-keys > private.key
   ```


### FVP Host UEFI firmware

1. Setup the development environment for building edk2 firmware.
2. Run the following command to build the Realm Guest firmware with support
   for Arm Boot Sync.

   ```
   build -a AARCH64 -p Platform/ARM/VExpressPkg/ArmVExpress-FVP-AArch64.dsc \
     -t GCC -b DEBUG \
     -D EDK2_OUT_DIR=Build/ArmVExpress-FVP-AArch64-DynamicTables \
     --pcd PcdUefiShellDefaultBootEnable=1 \
     --pcd PcdShellDefaultDelay=0 \
     --pcd PcdPlatformBootTimeOut=0x0
   ```

### Realm Guest UEFI firmware

1. Setup the development environment for building edk2 firmware.
2. Run the following command to build the Realm Guest firmware with support
   for Arm Boot Sync.

   ```
   build -a AARCH64 -p ArmVirtPkg/ArmVirtKvmTool.dsc \
     -t GCC -b DEBUG \
     -D EDK2_OUT_DIR=Build/ArmVirtKvmTool \
     --pcd PcdUefiShellDefaultBootEnable=1 \
     -DARMCCA_SECURE_BOOT_ENABLE=1
   ```


### Test User Context Service

1. Setup the development environment for building edk2 firmware
2. If the development/build PC is an Arm64 machine, the Arm GCC toolchain
   from https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
   does not appear to build the Host Based unit test correctly. Therefore,
   relaunch the development environment in a new console and use the distro
   supplied gcc toolchain and setup the edk2 build environment.
3. Run the following command to build the Test User Context Service.

   ```
   build -a AARCH64 -p ArmVirtPkg/ArmCcaHostUnitTest.dsc -t GCC -b NOOPT
   ```

### Provisioning Data script

Follow the steps below to generate the provisioning data file.

1. Set path to Basetools Python folder
   ```
   set PYTHONPATH=edk2/BaseTools/Source/Python

   # Alternatively, setup the environment using:
   pip install -r pip-requirements.txt
   ```

2. Create the Provisioning Data File
   ```
   ArmVirtPkg/ArmCcaBootSync/Scripts/GenPd.py

   python ArmVirtPkg/ArmCcaBootSync/Scripts/GenPd.py
          --pk SecBootCert\PK.cer
          --kek SecBootCert\KEK.cer
          --db SecBootCert\db.cer
          --dbx SecBootCert\dbx.cer
          --dbt SecBootCert\dbt.cer
          --dbr SecBootCert\dbr.cer
          --out SecBootCert\pd.bin
   ```


### RMM firmware

To build the RMM firmware run the following commands

  ```
  pushd rmm
  CROSS_COMPILE=${GCC_AARCH64_PREFIX} \
  cmake -DRMM_CONFIG="fvp_defcfg"  -S . -B build -DCMAKE_BUILD_TYPE=Debug -DLOG_LEVEL=50
  cmake --build build
  popd
  ```

### TF-A firmware

To build the TF-A firmware run the following commands

  ```
  pushd tf-a
  make CROSS_COMPILE=${GCC_AARCH64_PREFIX} PLAT=fvp DEBUG=1 \
     ARM_DISABLE_TRUSTED_WDOG=1 \
     FVP_HW_CONFIG_DTS=fdts/fvp-base-gicv3-psci-1t.dts \
     all fip \
     BL33=Build/ArmVExpress-FVP-AArch64-DynamicTables/DEBUG_GCC/FV/FVP_AARCH64_EFI.fd \
     ENABLE_RME=1 \
     RMM=rmm/build/Debug/rmm.img
  popd
  ```

### Kvmtool

To build the kvmtool run the following commands

  ```
  pushd kvmtool
  make lkvm-static
  popd
  ```


### Linux Kernel

To build the Linux Kernel run the following commands

1. Configure the default kernel config.

  ```
  make ARCH=arm64 defconfig <CROSS_COMPILE=path to gcc>
  ```

2. Enable the following Kernel configuration for crypto
    ```
    CRYPTO_USER_API_HASH=y
    BLK_DEV_DM=y
    CRYPTO_XTS=y
    CONFIG_DM_CRYPT=y
    CONFIG_CRYPTO_USER_API_SKCIPHER=y
    BR2_PACKAGE_CRYPTSETUP=y
    ```

3. Build the kernel image
   ```
   make ARCH=arm64 Image dtbs <CROSS_COMPILE=path to gcc>
   ```

4. Signing the Linux Kernel image
   ```
   # To sign the kernel image run the following command
   sbsign --key SecBootCert/db.key
          --cert SecBootCert/db.pem
          linux/arch/arm64/boot/Image
          --output Image_db_signed
   ```

5. Grub requires detached signing. So, sign the Linux kernel image with the GPG keys.
   ```
   # Note: Grub does not work with SHA512
   # Image_db_signed is the Linux Kernel image which is already signed using
   # the certificates/db.key
   gpg --homedir gpg_key/ --digest-algo SHA256
       --detach-sign Image_db_signed
   ```

### Grub

#### Prerequisites
  ```
  sudo apt-get install autoconf autopoint automake gettext
  sudo apt-get install gcc-aarch64-linux-gnu
  sudo apt install python-is-python3
  sudo apt-get install gawk
  ```

Building GRUB

1. Create a grub config file called grub_coco.cfg and include the following
   content
   ```
   echo 'Mounting encrypted disk...'
   cryptomount -s efisecret (hd0,gpt2)

   echo 'Loading Kernel...'
   linux (crypto0)/Image_db_signed "root=/dev/vda2"

   echo 'Loading InitRD...'
   initrd (crypto0)/rootfs.cpio

   echo 'Booting Linux...'
   boot
   ```

2. Enable encrypted disk support for grub
   ```
   sudo grub-mkconfig GRUB_ENABLE_CRYPTODISK=y
   ```

3. Run bootstrap script
   ```
   ./bootstrap
   ```
4. Run Configure script
   ```
   ./configure --prefix=`pwd`/install --target=aarch64-linux-gnu
   ```

5. Build grub
   ```
   make -j`nproc` && make install

   Note: The binaries would be generated in $PWD/install
   ```

6. Run the grub mkimage command

   ```
   ./grub-mkimage \
   --verbose \
   -m Image_db_signed.sig \
   --pubkey=GpgKey/boot.key \
   -d grub-core \
   -p /EFI/BOOT \
   -O arm64-efi \
   -o grub2.efi \
   boot \
   chain \
   configfile \
   echo \
   efisecret \
   pgp \
   efifwsetup \
   ext2 \
   fat \
   gcry_dsa \
   gcry_sha256 \
   gcry_sha512 \
   gcry_rijndael \
   gcry_rsa \
   gzio \
   iso9660 \
   loadenv \
   luks \
   luks2 \
   lvm \
   minicmd \
   normal \
   part_msdos \
   part_gpt \
   password_pbkdf2 \
   reboot \
   search \
   search_fs_uuid \
   search_fs_file \
   search_label \
   syslinuxcfg \
   xfs \
   linux \
   --config=../grub_coco.cf
   ```

7. Sbsign the grub image
   ```
   sbsign --key SecBootCert/db.key
          --cert SecBootCert/db.pem
          grub2.efi
          --output grub2_db_signed.efi
   ```

### Building Buildroot

1. Create a buildroot config file
    ```
    Create a config file buildroot/config/armcca_defconfig with the following content

    BR2_aarch64=y
    BR2_neoverse_n1=y
    BR2_SYSTEM_BIN_SH_BASH=y
    BR2_PACKAGE_BUSYBOX_SHOW_OTHERS=y
    BR2_PACKAGE_BUSYBOX_INDIVIDUAL_BINARIES=y
    BR2_PACKAGE_NFS_UTILS=y
    BR2_PACKAGE_DTC=y
    BR2_PACKAGE_DTC_PROGRAMS=y
    BR2_PACKAGE_DROPBEAR=y
    BR2_PACKAGE_UTIL_LINUX_SCHEDUTILS=y
    BR2_PACKAGE_VIM=y
    BR2_TARGET_ROOTFS_CPIO=y
    BR2_TARGET_ROOTFS_EXT2=y
    BR2_TARGET_ROOTFS_EXT2_4=y
    BR2_TARGET_ROOTFS_EXT2_SIZE="512M"
    BR2_PACKAGE_HOST_DOSFSTOOLS=y
    BR2_PACKAGE_HOST_MKPASSWD=y
    BR2_PACKAGE_HOST_MTOOLS=y

    # Following options are required for encrypted disk boot.
    BR2_PACKAGE_HOST_CRYPTSETUP=y
    ```

2. Build buildroot
   ```
   make armcca_defconfig
   make BR_JLEVEL=4
   ```

3. GPG signing the buildroot image.
   ```
   gpg --homedir GpgKey/ --digest-algo SHA256
       --detach-sign buildroot/output/images/rootfs.cpio
   ```

### Creating a Realm VM Diskimage

    ```
    #!/bin/bash -xv

    #disk_uefi: is the output
    disk_uefi="realm-disk-enc.img"
    #bootimg is the intermediate file
    bootimg="boot.img"
    #efi_path is the folder where input grub binary is placed
    efi_path="realm-uefi-boot/"
    #rootfs_img is the intermediate file
    rootfs_img=rootfs.img"
    #kernel_img is the input kernel image binary
    kernel_img="linux/arch/arm64/boot/Image"
    #rootfs_tar is the input root file system
    rootfs_tar="buildroot/output/images/rootfs.tar"
    #initramfs
    init_ramfs="buildroot/output/images/rootfs.cpio"
    #sec_part_size is the partition size for the second partition
    sec_part_size=256
    # BOOT_DISK_SIZE is the partition size for the boot partition
    BOOT_DISK_SIZE=64
    # Pre-define the UUID for the encrypted partition
    #// {49E6CC22-6D96-41DB-B497-C06DE9D57AD3}
    #static const GUID <<name>> =
    #{ 0x49e6cc22, 0x6d96, 0x41db, { 0xb4, 0x97, 0xc0, 0x6d, 0xe9, 0xd5, 0x7a, 0xd3 } };
    ENC_UUID="49E6CC22-6D96-41DB-B497-C06DE9D57AD3"
    #grub_img
    grub_img="grub/grub2.efi"

    # Copy the kernel image to the efi path
    cp ${kernel_img} $efi_path/Image
    sbsign --key SecBootCert/db.key --cert SecBootCert/db.pem ${kernel_img} --output $efi_path/Image_db_signed

    # Sign the Kernel image that was already signed using sbsign again with the PGP key, this generates $efi_path/Image_signed.sig
    gpg --homedir GpgKey/ --digest-algo SHA256 --detach-sign $efi_path/Image_db_signed


    cp ${grub_img} $efi_path/grub2.efi
    sbsign --key SecBootCert/db.key --cert SecBootCert/db.pem $efi_path/grub2.efi --output $efi_path/grub2_signed.efi

    set -e

    # Create a directory for the partition
    mkdir part2


    # Use following command to pack a rootfs in a tar
    # tar cf ../rootfs2img.tar * .[^.]*
    #
    dd if=/dev/urandom of=${rootfs_img} bs=1M count=${sec_part_size}

    # Setting --iter-time 50 to reduce boot time on FVP.
    # Note: This may not be suitable for production builds.
    sudo cryptsetup --verbose -y luksFormat --type luks2 --pbkdf=pbkdf2 ${rootfs_img} --iter-time 50

    sudo cryptsetup luksOpen ${rootfs_img} volume2
    sudo mke2fs -t ext4 /dev/mapper/volume2 -U ${ENC_UUID}
    sudo mount /dev/mapper/volume2 part2/
    sudo blkid /dev/mapper/volume2

    # Copy the required files to the partition2

    (cd part2; sudo tar xf ${rootfs_tar})
    (cd part2; sudo cp $efi_path/Image_signed .)
    (cd part2; sudo cp $efi_path/Image_signed.sig .)
    (cd part2; sudo cp $efi_path/rootfs.cpio .)
    (cd part2; sudo cp $efi_path/rootfs.cpio.sig .)

    # Add the key generated for the initramfs to slot 1 of the rootfs
    (sudo cryptsetup luksAddKey ${rootfs_img} ./cpio/etc/keys/root.key)

    (sudo cryptsetup luksDump ${rootfs_img})

    sync -f
    sync -f
    sync -f
    #this is required for ubuntu machine for the disk to be not-busy
    sleep 2s
    sudo umount -v -f /dev/mapper/volume2
    #sudo umount part2
    #sudo umount part1
    sudo cryptsetup luksClose volume2
    echo "UUID for encrypted partition"
    sudo cryptsetup luksUUID ${rootfs_img}

    echo "Creating boot partition"
    # create fat filesystem image of 64MB size
    # if the size of boot partition is changed then the parted command needs to be changed accordingly
    dd if=/dev/zero of=${bootimg} bs=1M count=${BOOT_DISK_SIZE}
    mkfs.vfat -F16 -n boot ${bootimg}

    mcopy -spmv -i ${bootimg} ${efi_path}/grub2_signed.efi ::
    # mcopy -spmv -i ${bootimg} ${efi_path}/* ::


    # combine everything and create partitions
    echo "Creating disk image"
    # Add 1MB first block for GPT partition
    dd if=/dev/zero bs=512 count=2048 > ${disk_uefi}
    dd if=${bootimg} >> ${disk_uefi}
    dd if=${rootfs_img} >> ${disk_uefi}
    # Add 1MB end block for GPT partition
    dd if=/dev/zero bs=512 count=2048 >> ${disk_uefi}

    BOOT_DISK_END=$((BOOT_DISK_SIZE+1))
    echo Boot Disk End = ${BOOT_DISK_END}

    parted ${disk_uefi} mktable gpt mkpart boot fat16 1MiB ${BOOT_DISK_END}MiB mkpart root ext2 ${BOOT_DISK_END}MiB 100%


    # cleanup
    rm ${bootimg}

    echo "Successfully created disk image: ${disk_uefi}"

    ```

## Running the setup

### Launch the unit test User Context service

1. Determine the IP Address of the machine. This will be useful later when
   launching the Realm Guest.
   ```
   # run ifconfig and note the IP Address of the machine.
   ifconfig
   ```
2. Create a directory for storing the user data.
   ```
   mkdir user_data
   ```

3. Save the user data files. <BR>
   #### Note: Prefix the user data files with the Realm Personalisation Value (RPV).

   ```
   # Let the RPV be "ARMCCA01"
   #
   # Save the provisioning data file ARMCCA01_VAR.dat
   # Run the provisioning data script to generate the output file SecBootCert\pd.bin
   cp SecBootCert/pd.bin user_data/ARMCCA01_VAR.dat

   # Save the disk decryption password in file ARMCCA01_SEC.dat
   # e.g. let the disk decryption password be 'realm01'
   echo -n realm01 > user_data/ARMCCA01_SEC.dat
   ```

4. Launch the test User Context service
   ```
   cd user_data
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH: \
   Build/ArmVirtPkg/HostTest/NOOPT_GCC/AARCH64/ArmVirtPkg/ArmCcaBootSync/QcborLib/QcborLib/OUTPUT/ \
   Build/ArmVirtPkg/HostTest/NOOPT_GCC/AARCH64/TestUserContextService \
   <port no for the User Context Service>

   # Note the port number for the User Context Service as this will be used when we launch the Realm VM.

   ```

### Launch the Realm VM

1. Boot the Host Linux OS.

2. Copy the Realm Disk image, Kvmtool, Guest firmware binary to a folder on the Host Linux OS and change to that folder.

3. Launch the Realm Guest VM with the following parameters.
   ```
   ./lkvm run -c 1 --realm --restricted_mem --disable-sve \
          --irqchip=gicv3-its \
          --firmware KVMTOOL_EFI.fd \
          --console=serial \
          --nodefaults \
          -m 512 \
          --no-pvtime \
          --force-pci \
          --service-ip <IP Address of the User Context Service> \
          --service-port <User Context Service Port> \
          --realm-pv <The Realm Personalisation Value, e.g. ARMCCA01>
   ```


# Tools

 - [FVP Base RevC AEM Model (available on x86_64 / Arm64 Linux)](https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms)

# Terms and abbreviations

This document uses the following terms and abbreviations.

  | Term     |  Meaning                                                   |
  | :---     |  :---                                                      |
  | AEAD     |  Authenticated Encryption with Associated Data             |
  | Arm CCA  |  Arm Confidential Compute Architecture                     |
  | ECC      |  Elliptic-curve cryptography                               |
  | ECDH     |  Elliptic-curve Diffie-Hellman                             |
  | GCM      |  Galois/Counter Mode                                       |
  | HKDF     |  HMAC based key derivation function                        |
  | HMAC     |  Message authentication code (MAC) using a hash function   |
  | KVM      |  Kernel-based Virtual Machine                              |
  | RHI      |  Realm Host Interface                                      |
  | RSI      |  Realm Service Interface                                   |
  | RMM      |  Realm Management Monitor                                  |
  | SHA      |  Secure Hash Algorithm                                     |
  | SMC      |  Secure Monitor Call                                       |
  | VMM      |  Virtual Machine Manager                                   |

# References

 - [1] [DEN0137 - Realm Management Monitor specification, v1.0-REL0](https://developer.arm.com/documentation/den0137/latest/)
 - [2] [DEN0148 - Realm Host Interface specification, v1.0-apl2](https://developer.arm.com/documentation/den0148/latest/)
 - [3] [Instructions for Building Firmware components and running the model, see
       section 4.19.2 "Building and running TF-A with RME"](https://trustedfirmware-a.readthedocs.io/en/latest/components/realm-management-extension.html#building-and-running-tf-a-with-rme)

