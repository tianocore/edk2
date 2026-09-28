#!/bin/bash
## @file
# Build ChainloadApp with embedded payload
#
# Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Under PACKAGES_PATH or any multi-repo workspace -- the normal
# edk2-platforms arrangement -- UefiPayloadPkg's parent directory is not
# the edk2 root and holds no edksetup.sh.  Prefer $WORKSPACE when the
# caller has already sourced edksetup.sh, and only fall back to guessing
# from the script location otherwise.
if [ -n "$WORKSPACE" ]; then
    EDK2_DIR="$WORKSPACE"
else
    EDK2_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
    if [ ! -f "$EDK2_DIR/edksetup.sh" ]; then
        echo "Error: WORKSPACE is not set and $EDK2_DIR/edksetup.sh does not exist." >&2
        echo "Source edksetup.sh from your edk2 workspace before running this script." >&2
        exit 1
    fi

    cd "$EDK2_DIR"

    # edksetup.sh returns non-zero on several paths, including some it
    # recovers from, which under set -e would abort here before anything
    # useful is printed.  Check for the variable it is meant to export
    # instead of trusting its exit status.
    set +e
    # shellcheck disable=SC1091
    source edksetup.sh
    set -e

    if [ -z "$WORKSPACE" ]; then
        echo "Error: sourcing $EDK2_DIR/edksetup.sh did not set WORKSPACE." >&2
        exit 1
    fi
fi

# Build configuration
BUILD_TARGET="${BUILD_TARGET:-RELEASE}"
ARCH="${ARCH:-X64}"

# Select toolchain based on architecture
if [ "$ARCH" = "AARCH64" ]; then
    TOOL_CHAIN="${TOOL_CHAIN_TAG:-GCC5}"
    export GCC5_AARCH64_PREFIX="${GCC5_AARCH64_PREFIX:-aarch64-unknown-linux-gnu-}"
else
    TOOL_CHAIN="${TOOL_CHAIN_TAG:-GCC5}"
fi

cd "$EDK2_DIR"

# Both build passes below share one Build/ output tree, so any
# BUILD_DEFINES entry that changes library selection must be applied
# to both.  BUILD_ARCH gives each ISA its own OUTPUT_DIRECTORY so
# concurrent X64 and AArch64 builds do not overwrite each other's
# UEFIPAYLOAD.fd or intermediate objects.
BUILD_DEFINES=(-D BOOTLOADER=SBL
               -D TIMER_SUPPORT=LAPIC
               -D CHAINLOAD_DEFAULTS=TRUE
               -D VIRTIO_ENABLE=TRUE
               -D BUILD_ARCH="Legacy${ARCH}")

OUT_DIR="$EDK2_DIR/Build/UefiPayloadPkgLegacy${ARCH}/${BUILD_TARGET}_${TOOL_CHAIN}"

echo "=== Building UniversalPayload ($BUILD_TARGET, $ARCH) ==="
build -p UefiPayloadPkg/UefiPayloadPkg.dsc \
      -b "$BUILD_TARGET" \
      -t "$TOOL_CHAIN" \
      -a "$ARCH" \
      "${BUILD_DEFINES[@]}"

PAYLOAD_FD="$OUT_DIR/FV/UEFIPAYLOAD.fd"

if [ ! -f "$PAYLOAD_FD" ]; then
    echo "Error: Payload not found at $PAYLOAD_FD"
    exit 1
fi

echo "=== Generating embedded payload header ==="
BUILD_DIR="$OUT_DIR/${ARCH}/UefiPayloadPkg/ChainloadApp/ChainloadApp/DEBUG"
mkdir -p "$BUILD_DIR"
PAYLOAD_HDR="$BUILD_DIR/EmbeddedPayload.h"
python3 "$SCRIPT_DIR/ChainloadApp/GenPayloadHdr.py" "$PAYLOAD_FD" "$PAYLOAD_HDR"

# The pass-1 build compiled ChainloadApp.c against the stub header
# before EmbeddedPayload.h existed, and its .deps file does not name
# the header.  Remove the pass-1 object so pass 2 recompiles it.  This
# is per-arch and per-target, so a concurrent build for another ISA is
# not disturbed (touching the shared source file would be).
rm -f "$OUT_DIR/${ARCH}/UefiPayloadPkg/ChainloadApp/ChainloadApp/OUTPUT/ChainloadApp.obj"

echo "=== Building ChainloadApp ==="
build -p UefiPayloadPkg/UefiPayloadPkg.dsc \
      -b "$BUILD_TARGET" \
      -t "$TOOL_CHAIN" \
      -a "$ARCH" \
      "${BUILD_DEFINES[@]}" \
      -m UefiPayloadPkg/ChainloadApp/ChainloadApp.inf

CHAINLOAD_EFI="$OUT_DIR/${ARCH}/ChainloadApp.efi"

echo ""
echo "=== Build complete ==="
echo "Payload:      $PAYLOAD_FD"
echo "ChainloadApp: $CHAINLOAD_EFI"
echo ""
if [ "$ARCH" = "X64" ]; then
    echo "Test with QEMU (X64):"
    echo "  qemu-system-x86_64 -bios /path/to/OVMF_CODE.fd -m 1G -nographic -enable-kvm \\"
    echo "    -net none -netdev user,tftp=Build/UefiPayloadPkgLegacy${ARCH}/${BUILD_TARGET}_${TOOL_CHAIN}/${ARCH}/,bootfile=ChainloadApp.efi,id=nd \\"
    echo "    -device virtio-net-pci,netdev=nd"
else
    echo "Test with QEMU (AArch64):"
    echo "  qemu-system-aarch64 -M virt -cpu cortex-a57 -m 1G -nographic \\"
    echo "    -bios /path/to/AAVMF_CODE.fd \\"
    echo "    -net none -netdev user,tftp=Build/UefiPayloadPkgLegacy${ARCH}/${BUILD_TARGET}_${TOOL_CHAIN}/${ARCH}/,bootfile=ChainloadApp.efi,id=nd \\"
    echo "    -device virtio-net-pci,netdev=nd"
fi
