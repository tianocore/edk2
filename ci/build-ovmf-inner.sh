#!/usr/bin/env bash
set -euo pipefail

log() {
  echo -e "\n==== $1 ====\n"
}

# --------------------------------------------------
# Paths and variables
EDK2_DIR="/workspace"
BUILD_DIR="$EDK2_DIR/Build"
OUT_DIR="$BUILD_DIR/OvmfX64/RELEASE_GCC5/FV"

GIT_TAG="${GIT_TAG:-}"

# Harbor configuration
HARBOR_PROJECT="${HARBOR_PROJECT:-ovmf-build}"
REPO_NAME="ovmf"

cd "$EDK2_DIR"

# --------------------------------------------------
log "Git preparation"

git config --global --add safe.directory /workspace

if [[ -n "$GIT_TAG" ]]; then
  echo "Using tag: $GIT_TAG"
  git fetch --tags --force
  git checkout "tags/$GIT_TAG"
  git submodule update --init --recursive || \
    echo "WARNING: submodule update failed for this tag"
else
  echo "No tag provided → using Top Of Trunk"
  git submodule update --init --recursive
fi

# --------------------------------------------------
log "Building BaseTools"
make -C BaseTools

# --------------------------------------------------
log "Setting up EDK2 environment"

export WORKSPACE=/workspace
export PYTHON_COMMAND=python3

# EDK2 setup scripts are not nounset-safe
set +u
. edksetup.sh
set -u

# --------------------------------------------------
log "Secure Boot build"

build -a X64 -t GCC5 -b RELEASE \
  -p OvmfPkg/OvmfPkgX64.dsc \
  -D SECURE_BOOT_ENABLE=TRUE \
  -D TPM2_ENABLE=TRUE \
  -D HTTP_BOOT_ENABLE=TRUE \
  -D PXE_BOOT_ENABLE=TRUE \
  -D NETWORK_ENABLE=TRUE \
  -D SMM_REQUIRE=TRUE \
  -D FD_SIZE_4MB=TRUE \
  -D DEBUG_ON_SERIAL_PORT=FALSE

# --------------------------------------------------
log "Renaming Secure Boot outputs"

for f in OVMF.fd OVMF_CODE.fd OVMF_VARS.fd; do
  src="$OUT_DIR/$f"
  dst="$OUT_DIR/sec_$f"

  if [[ -f "$src" ]]; then
    mv -f "$src" "$dst"
    echo "Renamed $f → sec_$f"
  else
    echo "WARNING: $src not found"
  fi
done

# --------------------------------------------------
log "Uploading Secure Boot artifacts to Harbor"

ID="${GIT_TAG:-$(git rev-parse --short HEAD)}"

./ci/upload-to-harbor.sh \
  "$HARBOR_PROJECT/$REPO_NAME" \
  "${ID}-secure" \
  "$OUT_DIR/sec_OVMF.fd" \
  "$OUT_DIR/sec_OVMF_CODE.fd" \
  "$OUT_DIR/sec_OVMF_VARS.fd"

# --------------------------------------------------
log "Cleaning Build directory"
rm -rf "$BUILD_DIR"

# --------------------------------------------------
log "Standard Release build"

build -a X64 -t GCC5 -b RELEASE \
  -p OvmfPkg/OvmfPkgX64.dsc

# --------------------------------------------------
log "Uploading Standard Release artifacts to Harbor"

./ci/upload-to-harbor.sh \
  "$HARBOR_PROJECT/$REPO_NAME" \
  "${ID}-standard" \
  "$OUT_DIR/OVMF.fd" \
  "$OUT_DIR/OVMF_CODE.fd" \
  "$OUT_DIR/OVMF_VARS.fd"

# --------------------------------------------------
log "OVMF build completed successfully"
