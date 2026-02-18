#!/usr/bin/env bash

set -euo pipefail

VERSION="2025.12"

URL="http://mscc-ent-open-source.s3-eu-west-1.amazonaws.com/public_root/bsp"
TARBALL="mchp-brsdk-source-${VERSION}.tar.gz"
SRC_DIR="mchp-brsdk-source-${VERSION}"
PATCH_FILE="prebuild_lan8651_customizations_clean.patch"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="${PWD}"

echo "== BSP Build Script with LAN8651 Patch =="
echo "Using Version: ${VERSION}"
echo ""

echo "== Step 1: Download BSP Tarball =="
if [ ! -f "${WORK_DIR}/${TARBALL}" ]; then
    if command -v curl >/dev/null 2>&1; then
        curl -fL "${URL}/${TARBALL}" -o "${WORK_DIR}/${TARBALL}"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "${WORK_DIR}/${TARBALL}" "${URL}/${TARBALL}"
    else
        echo "ERROR: Neither curl nor wget is installed."
        exit 1
    fi
else
    echo "Tarball already exists: ${WORK_DIR}/${TARBALL}"
fi

echo "== Step 2: Extract Tarball =="
if [ ! -d "${WORK_DIR}/${SRC_DIR}" ]; then
    tar xf "${WORK_DIR}/${TARBALL}" -C "${WORK_DIR}"
else
    echo "Source directory already exists: ${WORK_DIR}/${SRC_DIR}"
fi

if [ ! -f "${SCRIPT_DIR}/${PATCH_FILE}" ]; then
    echo "ERROR: Patch file not found: ${SCRIPT_DIR}/${PATCH_FILE}"
    exit 1
fi

echo "== Step 3: Copy Patch to Source Directory =="
cp -f "${SCRIPT_DIR}/${PATCH_FILE}" "${WORK_DIR}/${SRC_DIR}/"

echo "== Step 4: Change to Source Directory =="
cd "${WORK_DIR}/${SRC_DIR}"

echo "== Step 5: Check and Apply Patch =="
# Try git apply first, fallback to patch if not a git repository
if [ -d .git ]; then
    echo "Git repository detected - using git apply"
    git apply --check --whitespace=fix "${PATCH_FILE}"
    git apply --whitespace=fix "${PATCH_FILE}"
else
    echo "No git repository - using patch command"
    patch --dry-run -p1 < "${PATCH_FILE}"
    patch -p1 < "${PATCH_FILE}"
fi

echo "== Step 6: Generate Buildroot Defconfig =="
make BR2_EXTERNAL=./external O=./output/mybuild arm_standalone_defconfig

echo "== Step 7: Change to output/mybuild =="
cd output/mybuild

echo "== Step 8: Start Build (make) =="
make

echo "== Finished =="
echo "Current directory: $(pwd)"