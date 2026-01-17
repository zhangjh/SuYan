#!/bin/bash
# IME Pinyin macOS Package Builder
#
# This script creates a macOS installer package (.pkg) for IME Pinyin.
#
# Usage:
#   ./make_package.sh [derived_data_path]
#
# Arguments:
#   derived_data_path: Path to Xcode derived data (default: build)
#
# Prerequisites:
#   - Xcode Command Line Tools
#   - Built IMEPinyin.app in derived data path

set -e

DERIVED_DATA_PATH="${1:-build}"
BUNDLE_IDENTIFIER='com.example.inputmethod.IMEPinyin'
INSTALL_LOCATION='/Library/Input Methods'

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

# Source common functions
source "${SCRIPT_DIR}/common.sh"

echo "============================================"
echo "IME Pinyin macOS Package Builder"
echo "============================================"
echo ""
echo "Project Root: ${PROJECT_ROOT}"
echo "Derived Data: ${DERIVED_DATA_PATH}"
echo ""

# Get version
APP_VERSION=$(get_app_version)
echo "Version: ${APP_VERSION}"
echo ""

# Handle absolute vs relative path
if [[ "${DERIVED_DATA_PATH}" = /* ]]; then
    BUILD_ROOT="${DERIVED_DATA_PATH}"
else
    BUILD_ROOT="${PROJECT_ROOT}/${DERIVED_DATA_PATH}"
fi

# Check for built app
APP_PATH="${BUILD_ROOT}/Build/Products/Release/IMEPinyin.app"
if [ ! -d "${APP_PATH}" ]; then
    # Try Squirrel.app as fallback (for development)
    APP_PATH="${BUILD_ROOT}/Build/Products/Release/Squirrel.app"
    if [ ! -d "${APP_PATH}" ]; then
        echo "ERROR: Built app not found at expected location."
        echo "Expected: ${BUILD_ROOT}/Build/Products/Release/IMEPinyin.app"
        echo ""
        echo "Please build the app first using:"
        echo "  make release"
        exit 1
    fi
fi

# Determine actual app name
APP_NAME=$(basename "${APP_PATH}")
echo "App Path: ${APP_PATH}"
echo "App Name: ${APP_NAME}"
echo ""

# Create output directory
OUTPUT_DIR="${PROJECT_ROOT}/output/macos"
mkdir -p "${OUTPUT_DIR}"

# Generate component plist dynamically based on actual app name
TEMP_COMPONENT_PLIST="${OUTPUT_DIR}/component.plist"
cat > "${TEMP_COMPONENT_PLIST}" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<array>
	<dict>
		<key>RootRelativeBundlePath</key>
		<string>${APP_NAME}</string>
		<key>BundleHasStrictIdentifier</key>
		<false/>
		<key>BundleIsRelocatable</key>
		<false/>
		<key>BundleIsVersionChecked</key>
		<true/>
		<key>BundleOverwriteAction</key>
		<string>upgrade</string>
		<key>ChildBundles</key>
		<array>
			<dict>
				<key>BundleOverwriteAction</key>
				<string>upgrade</string>
				<key>RootRelativeBundlePath</key>
				<string>${APP_NAME}/Contents/Frameworks/Sparkle.framework</string>
			</dict>
		</array>
	</dict>
</array>
</plist>
EOF

echo "Generated component plist for: ${APP_NAME}"
echo ""

# Build the package
echo "Building package..."

pkgbuild \
    --info "${SCRIPT_DIR}/PackageInfo" \
    --root "${BUILD_ROOT}/Build/Products/Release" \
    --filter '.*\.swiftmodule' \
    --filter '.*\.dSYM' \
    --component-plist "${TEMP_COMPONENT_PLIST}" \
    --identifier "${BUNDLE_IDENTIFIER}" \
    --version "${APP_VERSION}" \
    --install-location "${INSTALL_LOCATION}" \
    --scripts "${SCRIPT_DIR}/../scripts" \
    "${OUTPUT_DIR}/IMEPinyin-${APP_VERSION}.pkg"

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to build package!"
    exit 1
fi

echo ""
echo "============================================"
echo "Package built successfully!"
echo "Output: ${OUTPUT_DIR}/IMEPinyin-${APP_VERSION}.pkg"
echo "============================================"
