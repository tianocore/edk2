#!/bin/bash

# config
venv="$(dirname $0)/Build/python-venv-ci"
opts="TOOL_CHAIN_TAG=GCC5"

########################################################################

function msg() {
    local txt="$1"

    echo ""
    echo "###"
    echo "### $txt"
    echo "###"
    # window title
    echo -ne "\x1b]2;${txt}\007"
}

########################################################################

if test -f ${venv}/bin/activate; then
    source ${venv}/bin/activate
else
    msg "python setup"
    python -m venv ${venv}
    source ${venv}/bin/activate
    python -m pip install --upgrade pip
    python -m pip install --upgrade -r pip-requirements.txt
fi

msg "build basetools"
python BaseTools/Edk2ToolsBuild.py -t GCC5                      || exit 1


msg "pytool"
(set -x; stuart_setup -c .pytool/CISettings.py -t DEBUG -a x64 TOOL_CHAIN_TAG=GCC5 $build)          || exit 1
(set -x; stuart_update -c .pytool/CISettings.py -t DEBUG -a x64 TOOL_CHAIN_TAG=GCC5 $build)         || exit 1

build="OvmfPkg/PlatformCI/PlatformBuild.py"
for arch in X64 IA32,X64 IA32; do
    msg "stuart setup ($build,$arch)"
    (set -x; stuart_setup $opts -c $build -a $arch)             || exit 1

    msg "stuart update ($build,$arch)"
    (set -x; stuart_update $opts -c $build -a $arch)            || exit 1

    msg "stuart build ($build,$arch)"
    (set -x; stuart_build $opts -c $build -a $arch)             || exit 1
done

# run qemu
stuart_build -c OvmfPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=GCC5 -a X64 MAKE_STARTUP_NSH=TRUE QEMU_HEADLESS=TRUE --FlashOnly

arch=X64
for build in \
    OvmfPkg/PlatformCI/AmdSevBuild.py \
    OvmfPkg/PlatformCI/BhyveBuild.py \
    OvmfPkg/PlatformCI/MicrovmBuild.py \
    ; do
    msg "stuart setup ($build,$arch)"
    (set -x; stuart_setup $opts -c $build -a $arch)             || exit 1

    msg "stuart update ($build,$arch)"
    (set -x; stuart_update $opts -c $build -a $arch)            || exit 1

    msg "stuart build ($build,$arch)"
    (set -x; stuart_build $opts -c $build -a $arch)             || exit 1
    exit
done

