#!/usr/bin/env bash
set -euo pipefail

############################################
# Usage:
# upload-to-artifactory.sh <file> <target_url> <user> <apikey>
############################################

FILE="$1"
TARGET_URL="$2"
USER="${3:-}"
API_KEY="${4:-}"

if [[ -z "$FILE" || -z "$TARGET_URL" ]]; then
  echo "ERROR: Missing arguments"
  echo "Usage: $0 <file> <target_url> <user> <apikey>"
  exit 2
fi

if [[ ! -f "$FILE" ]]; then
  echo "ERROR: File not found: $FILE"
  exit 3
fi

# If credentials are missing, skip upload gracefully
if [[ -z "$USER" || -z "$API_KEY" ]]; then
  echo "INFO: Artifactory credentials not set, skipping upload for $FILE"
  exit 0
fi

echo "Uploading $FILE → $TARGET_URL"

curl -f \
  -u "$USER:$API_KEY" \
  -T "$FILE" \
  "$TARGET_URL"

echo "Upload successful: $(basename "$FILE")"
