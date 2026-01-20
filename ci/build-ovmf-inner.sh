#!/usr/bin/env bash
set -euo pipefail

###############################################################################
# Usage:
#   ci/build-ovmf-inner.sh secure
#   ci/build-ovmf-inner.sh standard
###############################################################################

MODE="${1:-}"

if [[ "$MODE" != "secure" && "$MODE" != "standard" ]]; then
  echo "ERROR: Invalid build mode"
  echo "Usage: $0 <secure|standard>"
  exit 1
fi

log() {
  echo -e "\n==== $1 ====\n"
}

# --------------------------------------------------
# Resolve workspace safely (GitHub Actions / local / container)
EDK2_DIR="${GITHUB_WORKSPACE:-$(pwd)}"
BUILD_DIR="$EDK2_DIR/Build"
OUT_DIR="$BUILD_DIR/OvmfX64/RELEASE_GCC5/FV"

GIT_TAG="${GIT_TAG:-}"

# --------------------------------------------------
# Sanity check
if [[ ! -d "$EDK2_DIR" ]]; then
  echo "ERROR: Workspace directory does not exist: $EDK2_DIR"
  exit 1
fi

cd "$EDK2_DIR"

# --------------------------------------------------
log "Git preparation"

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

export WORKSPACE="$EDK2_DIR"
export PYTHON_COMMAND=python3

# EDK2 setup scripts are not nounset-safe
set +u
. edksetup.sh --reconfig
set -u

# --------------------------------------------------
if [[ "$MODE" == "secure" ]]; then
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

  log "Secure Boot build completed successfully"
  echo "Artifacts available under:"
  echo "  $OUT_DIR"

fi

# --------------------------------------------------
if [[ "$MODE" == "standard" ]]; then
  log "Standard Release build"

  build -a X64 -t GCC5 -b RELEASE \
    -p OvmfPkg/OvmfPkgX64.dsc

  log "Standard Release build completed successfully"
  echo "Artifacts available under:"
  echo "  $OUT_DIR"
fi
