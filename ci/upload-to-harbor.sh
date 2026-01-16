#!/usr/bin/env bash
set -euo pipefail

############################################
# Usage:
# upload-to-harbor.sh <repo> <tag> <file1> [file2 ...]
#
# repo: <harbor-project>/<repository>
# tag : artifact tag
############################################

REPO="$1"        # e.g. ovmf-build/ovmf
TAG="$2"         # e.g. edk2-202308-secure
shift 2
FILES=("$@")

REGISTRY="${ARTIFACTORY_URL#https://}"

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No files provided for Harbor upload"
  exit 0
fi

echo "Pushing OCI artifact to Harbor"
echo "Registry : $REGISTRY"
echo "Repo     : $REPO"
echo "Tag      : $TAG"

ARGS=()
for f in "${FILES[@]}"; do
  if [[ ! -f "$f" ]]; then
    echo "WARNING: File not found: $f (skipping)"
    continue
  fi
  ARGS+=("$f:application/octet-stream")
done

if [[ ${#ARGS[@]} -eq 0 ]]; then
  echo "No valid files to upload"
  exit 0
fi

oras push "$REGISTRY/$REPO:$TAG" "${ARGS[@]}"
