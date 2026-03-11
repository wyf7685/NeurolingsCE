#!/bin/sh
# Generate metainfo.xml from template
# Usage: generate-metainfo.sh <template> <output>

TEMPLATE="$1"
OUTPUT="$2"

if [ -z "$TEMPLATE" ] || [ -z "$OUTPUT" ]; then
    echo "Usage: $0 <template> <output>"
    exit 1
fi

# Read version from VERSION.txt
VERSION=$(grep "^VERSION=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "0.2.0")
RELEASE_DATE=$(grep "^RELEASE_DATE=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "2026-03-08")

# Substitute variables in template
sed -e "s/@VERSION@/${VERSION}/g" \
    -e "s/@RELEASE_DATE@/${RELEASE_DATE}/g" \
    "$TEMPLATE" > "$OUTPUT"

echo "Generated $OUTPUT with version ${VERSION}"
