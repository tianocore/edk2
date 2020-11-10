#!/bin/bash

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
TMPPATH="/tmp/coreboot.rom"

QEMUARGS=
QEMUARGS+="-M q35 "
QEMUARGS+="-m 1024 "
QEMUARGS+="-bios ${TMPPATH} "
QEMUARGS+="-smp $(nproc) "
QEMUARGS+="-display none "

QEMU=qemu-system-x86_64
CBFSTOOL="${SCRIPTPATH}/cbfstool"
XZ=xz

KEYWORDS=
KEYWORDS+="ASSERT "
KEYWORDS+="ASSERT_EFI_ERROR "

${XZ} -k -d "${SCRIPTPATH}/coreboot.rom.xz" -c > ${TMPPATH}
${CBFSTOOL} ${TMPPATH} add-payload -f $1 -n "fallback/payload" -c LZMA
${QEMU} ${QEMUARGS} -serial stdio > /tmp/qemu.log &
pid=$!
i=0
while [ /bin/true ]; do
	if [ "$(grep '\[Bds\]Booting\sUEFI\sShell' /tmp/qemu.log)" != "" ]; then
		break
	fi
	if [ "$(grep 'UEFI\sInteractive\sShell\sv' /tmp/qemu.log)" != "" ]; then
		break
	fi
	for k in ${KEYWORDS}; do
		if [ "$(grep ${k} /tmp/qemu.log)" != "" ]; then
			i=10000
			break
		fi
	done
	i=$((i+1))
	if [ ${i} -gt 60 ]; then
		break
	fi
	sleep 1
	echo "sleeping...."
done
kill -9 ${pid}
for k in ${KEYWORDS}; do
	if [ "$(grep ${k} /tmp/qemu.log)" != "" ]; then
		echo "Found bad string ${k}"
		cat /tmp/qemu.log
		printf "\n\nErrors found:\n"
		grep ${k} /tmp/qemu.log
		rm /tmp/qemu.log
		exit 1
	fi
done
cat /tmp/qemu.log
printf "\n\nNo errors found\n"
rm /tmp/qemu.log
exit 0
