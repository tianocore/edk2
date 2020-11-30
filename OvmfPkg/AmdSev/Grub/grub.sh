##  @file
#  Build a version of grub capable of decrypting a luks volume with a SEV
#  Supplied secret
#
#  Copyright (C) 2020 James Bottomley, IBM Corporation.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

set -e
remove_efi=1

cleanup()  {
    # remove the intermediates
    for f in disk.fat grub-bootstrap.cfg; do
        rm -f -- "${basedir}/$f"
    done
    if [ $remove_efi -eq 1 ]; then
        rm -f -- "${basedir}/grub.efi"
    fi
}

trap cleanup EXIT

GRUB_MODULES="
            part_msdos
            part_gpt
            cryptodisk
            luks
            gcry_rijndael
            gcry_sha256
            ext2
            btrfs
            xfs
            fat
            configfile
            memdisk
            sleep
            normal
            echo
            test
            regexp
            linux
            linuxefi
            reboot
            sevsecret
            "
basedir=$(dirname -- "$0")

# don't run a build if grub.efi exists and is newer than the config files
if [ -e "${basedir}/grub.efi" ] && \
     [ "${basedir}/grub.efi" -nt "${basedir}/grub.cfg" ] && \
     [ "${basedir}/grub.efi" -nt "${basedir}/grub.sh" ]; then
    remove_efi=0
    echo "preserving existing grub.efi" >&2
    exit 0
fi

##
# different distributions have different names for grub-mkimage, so
# search all the known ones
##
mkimage=
for b in grub2-mkimage grub-mkimage; do
    if which "$b" > /dev/null 2>&1; then
        mkimage="$b"
        break
    fi
done
if [ -z "$mkimage" ]; then
    echo "Can't find grub mkimage" >&2
    exit 1
fi

# GRUB's rescue parser doesn't understand 'if'.
echo 'normal (memdisk)/grub.cfg' > "${basedir}/grub-bootstrap.cfg"

# Now build a memdisk with the correct grub.cfg
rm -f -- "${basedir}/disk.fat"
mkfs.msdos -C -- "${basedir}/disk.fat" 64
mcopy -i "${basedir}/disk.fat" -- "${basedir}/grub.cfg" ::grub.cfg


${mkimage} -O x86_64-efi \
           -p '(crypto0)' \
           -c "${basedir}/grub-bootstrap.cfg" \
           -m "${basedir}/disk.fat" \
           -o "${basedir}/grub.efi" \
           ${GRUB_MODULES}

remove_efi=0
echo "grub.efi generated in ${basedir}"
