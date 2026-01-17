#!/bin/bash
# IME Pinyin macOS Code Signing and Notarization Script
#
# This script signs the app and package, then submits for notarization.
#
# Usage:
#   ./sign_and_notarize.sh "Developer ID Name" [derived_data_path]
#
# Arguments:
#   Developer ID Name: Your Apple Developer ID (e.g., "John Doe (TEAMID)")
#   derived_data_path: Path to Xcode derived data (default: build)
#
# Prerequisites:
#   - Valid Apple Developer ID certificate installed
#   - Keychain profile configured for notarytool
#   - Built app in derived data path
#
# To set up keychain profile:
#   xcrun notarytool store-credentials "Developer ID Name" \
#       --apple-id "your@email.com" \
#       --team-id "TEAMID" \
#       --password "app-specific-password"

set -e

DEV_ID="$1"
DERIVED_DATA_PATH="${2:-build}"

if [ -z "${DEV_ID}" ]; then
    echo "Usage: $0 \"Developer ID Name\" [derived_data_path]"
    echo ""
    echo "Example:"
    echo "  $0 \"John Doe (ABC123XYZ)\""
    exit 1
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

# Source common functions
source "${SCRIPT_DIR}/common.sh"

echo "============================================"
echo "IME Pinyin Code Signing and Notarization"
echo "============================================"
echo ""
echo "Developer ID: ${DEV_ID}"
echo "Derived Data: ${DERIVED_DATA_PATH}"
echo ""

# Get version
APP_VERSION=$(get_app_version)
echo "Version: ${APP_VERSION}"
echo ""

# Define paths
APP_PATH="${PROJECT_ROOT}/${DERIVED_DATA_PATH}/Build/Products/Release/IMEPinyin.app"
OUTPUT_DIR="${PROJECT_ROOT}/output/macos"
PKG_FILE="${OUTPUT_DIR}/IMEPinyin-${APP_VERSION}.pkg"
ENTITLEMENTS="${PROJECT_ROOT}/installer/macos/resources/IMEPinyin.entitlements"

# Check for app
if [ ! -d "${APP_PATH}" ]; then
    # Try Squirrel.app as fallback
    APP_PATH="${PROJECT_ROOT}/${DERIVED_DATA_PATH}/Build/Products/Release/Squirrel.app"
    if [ ! -d "${APP_PATH}" ]; then
        echo "ERROR: Built app not found!"
        exit 1
    fi
fi

# Check for entitlements
if [ ! -f "${ENTITLEMENTS}" ]; then
    ENTITLEMENTS="${PROJECT_ROOT}/deps/squirrel/resources/Squirrel.entitlements"
fi

# Step 1: Sign the app
echo "Step 1: Signing the app..."
echo ""

codesign --deep --force --options runtime --timestamp \
    --sign "Developer ID Application: ${DEV_ID}" \
    --entitlements "${ENTITLEMENTS}" \
    --verbose "${APP_PATH}"

echo ""
echo "Verifying signature..."
spctl -a -vv "${APP_PATH}"

echo ""
echo "App signed successfully!"
echo ""

# Step 2: Build the package
echo "Step 2: Building the package..."
echo ""

"${SCRIPT_DIR}/make_package.sh" "${DERIVED_DATA_PATH}"

# Step 3: Sign the package
echo ""
echo "Step 3: Signing the package..."
echo ""

SIGNED_PKG="${OUTPUT_DIR}/IMEPinyin-${APP_VERSION}-signed.pkg"

productsign --sign "Developer ID Installer: ${DEV_ID}" \
    "${PKG_FILE}" "${SIGNED_PKG}"

# Replace unsigned with signed
rm "${PKG_FILE}"
mv "${SIGNED_PKG}" "${PKG_FILE}"

echo "Package signed successfully!"
echo ""

# Step 4: Notarize the package
echo "Step 4: Notarizing the package..."
echo ""
echo "This may take several minutes..."
echo ""

xcrun notarytool submit "${PKG_FILE}" \
    --keychain-profile "${DEV_ID}" \
    --wait

# Step 5: Staple the notarization ticket
echo ""
echo "Step 5: Stapling notarization ticket..."
echo ""

xcrun stapler staple "${PKG_FILE}"

echo ""
echo "============================================"
echo "Signing and notarization completed!"
echo "============================================"
echo ""
echo "Output: ${PKG_FILE}"
echo ""

# Verify notarization
echo "Verifying notarization..."
spctl -a -vv --type install "${PKG_FILE}"

echo ""
echo "Package is ready for distribution!"
