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

if [ "$(which swtpm)" != "" ]; then
  i=0
  while [ -d /tmp/mytpm$i ]; do
    let i=i+1
  done
  tpm=/tmp/mytpm$i

  mkdir $tpm
  trap "rm -rf $tpm" EXIT

  if [ -z "${XDG_CONFIG_HOME:-}" ]; then
    export XDG_CONFIG_HOME=~/.config
  fi

  if [ ! -f "${XDG_CONFIG_HOME}/swtpm-localca.conf" ]; then
    cat <<EOF > "${XDG_CONFIG_HOME}/swtpm-localca.conf"
statedir = ${XDG_CONFIG_HOME}/var/lib/swtpm-localca
signingkey = ${XDG_CONFIG_HOME}/var/lib/swtpm-localca/signkey.pem
issuercert = ${XDG_CONFIG_HOME}/var/lib/swtpm-localca/issuercert.pem
certserial = ${XDG_CONFIG_HOME}/var/lib/swtpm-localca/certserial
EOF
  fi

  if [ ! -f "${XDG_CONFIG_HOME}/swtpm-localca.options" ]; then
    cat <<EOF > "${XDG_CONFIG_HOME}/swtpm-localca.options"
--platform-manufacturer EDK2
--platform-version 2.12
--platform-model QEMU
EOF
  fi

  if [ ! -f "${XDG_CONFIG_HOME}/swtpm_setup.conf" ]; then
    cat <<EOF > "${XDG_CONFIG_HOME}/swtpm_setup.conf"
# Program invoked for creating certificates
create_certs_tool= /usr/share/swtpm/swtpm-localca
create_certs_tool_config = ${XDG_CONFIG_HOME}/swtpm-localca.conf
create_certs_tool_options = ${XDG_CONFIG_HOME}/swtpm-localca.options
EOF
  fi

  # Note: TPM1 needs to access tcsd as root..
  swtpm_setup --tpmstate $tpm --tpm2 \
    --create-ek-cert --create-platform-cert --lock-nvram

  echo "Starting $tpm"
  swtpm socket --tpmstate dir=$tpm --tpm2 --ctrl type=unixio,path=/$tpm/swtpm-sock &

  QEMUARGS+="-chardev socket,id=chrtpm,path=$tpm/swtpm-sock "
  QEMUARGS+="-tpmdev emulator,id=tpm0,chardev=chrtpm "
  QEMUARGS+="-device tpm-tis,tpmdev=tpm0 "
fi

${XZ} -k -d "${SCRIPTPATH}/coreboot.rom.xz" -c > ${TMPPATH}
${CBFSTOOL} ${TMPPATH} add-payload -f $1 -n "fallback/payload" -c LZMA

trap "rm -f /tmp/qemu.log" EXIT
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
