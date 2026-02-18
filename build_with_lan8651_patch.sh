#!/usr/bin/env bash

set -euo pipefail

URL="http://mscc-ent-open-source.s3-eu-west-1.amazonaws.com/public_root/bsp/mscc-brsdk-source-2025.12.tar.gz"
TARBALL="mscc-brsdk-source-2025.12.tar.gz"
SRC_DIR="mchp-brsdk-source-2025.12"
PATCH_FILE="prebuild_lan8651_customizations_clean.patch"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="${PWD}"

echo "== Schritt 1: BSP-Tarball herunterladen =="
if [ ! -f "${WORK_DIR}/${TARBALL}" ]; then
    if command -v curl >/dev/null 2>&1; then
        curl -fL "${URL}" -o "${WORK_DIR}/${TARBALL}"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "${WORK_DIR}/${TARBALL}" "${URL}"
    else
        echo "ERROR: Weder curl noch wget ist installiert."
        exit 1
    fi
else
    echo "Tarball bereits vorhanden: ${WORK_DIR}/${TARBALL}"
fi

echo "== Schritt 2: Tarball entpacken =="
if [ ! -d "${WORK_DIR}/${SRC_DIR}" ]; then
    tar xf "${WORK_DIR}/${TARBALL}" -C "${WORK_DIR}"
else
    echo "Quellverzeichnis bereits vorhanden: ${WORK_DIR}/${SRC_DIR}"
fi

if [ ! -f "${SCRIPT_DIR}/${PATCH_FILE}" ]; then
    echo "ERROR: Patch-Datei nicht gefunden: ${SCRIPT_DIR}/${PATCH_FILE}"
    exit 1
fi

echo "== Schritt 3: Patch in Quellverzeichnis kopieren =="
cp -f "${SCRIPT_DIR}/${PATCH_FILE}" "${WORK_DIR}/${SRC_DIR}/"

echo "== Schritt 4: In Quellverzeichnis wechseln =="
cd "${WORK_DIR}/${SRC_DIR}"

echo "== Schritt 5: Patch prüfen und anwenden =="
git apply --check --whitespace=fix "${PATCH_FILE}"
git apply --whitespace=fix "${PATCH_FILE}"

echo "== Schritt 6: Buildroot-Defconfig erzeugen =="
make BR2_EXTERNAL=./external O=./output/mybuild arm_standalone_defconfig

echo "== Schritt 7: In output/mybuild wechseln =="
cd output/mybuild

echo "== Schritt 8: Build starten (make) =="
make

echo "== Fertig =="
echo "Aktuelles Verzeichnis: $(pwd)"