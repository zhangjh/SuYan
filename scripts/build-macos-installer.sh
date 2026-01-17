#!/bin/bash
# IME Pinyin macOS Full Build Script
#
# This script builds the complete macOS installer including:
#   1. Download prebuilt librime (with plugins)
#   2. Build Squirrel frontend
#   3. Build PKG installer package
#
# Usage:
#   ./build-macos-installer.sh [options]
#
# Options:
#   all       - Build everything (default)
#   deps      - Download dependencies only
#   app       - Build app only
#   package   - Build package only
#   clean     - Clean build artifacts
#   sign      - Sign and notarize (requires DEV_ID env var)
#
# Environment Variables:
#   IME_VERSION   - Version number (default: 1.0.0)
#   DEV_ID        - Apple Developer ID for signing
#
# Prerequisites:
#   - Xcode Command Line Tools
#   - curl

set -e

echo "============================================"
echo "IME Pinyin macOS Full Build"
echo "============================================"
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Set default version
IME_VERSION="${IME_VERSION:-1.0.0}"

# librime prebuilt version (from official releases)
RIME_VERSION="1.16.0"
RIME_GIT_HASH="a251145"
SPARKLE_VERSION="2.6.2"

echo "Project Root: ${PROJECT_ROOT}"
echo "Version: ${IME_VERSION}"
echo "librime Version: ${RIME_VERSION}"
echo ""

# Parse command line
BUILD_DEPS=0
BUILD_APP=0
BUILD_PACKAGE=0
BUILD_CLEAN=0
BUILD_SIGN=0

if [ $# -eq 0 ]; then
    BUILD_DEPS=1
    BUILD_APP=1
    BUILD_PACKAGE=1
fi

for arg in "$@"; do
    case $arg in
        all)
            BUILD_DEPS=1
            BUILD_APP=1
            BUILD_PACKAGE=1
            ;;
        deps)
            BUILD_DEPS=1
            ;;
        app)
            BUILD_APP=1
            ;;
        package)
            BUILD_PACKAGE=1
            ;;
        clean)
            BUILD_CLEAN=1
            ;;
        sign)
            BUILD_SIGN=1
            ;;
    esac
done

# Clean if requested
if [ $BUILD_CLEAN -eq 1 ]; then
    echo "Cleaning build artifacts..."
    rm -rf "${PROJECT_ROOT}/output/macos"
    rm -rf "${PROJECT_ROOT}/deps/squirrel/build"
    rm -rf "${PROJECT_ROOT}/deps/squirrel/lib"
    rm -rf "${PROJECT_ROOT}/deps/squirrel/bin"
    rm -rf "${PROJECT_ROOT}/deps/squirrel/Frameworks"
    rm -rf "${PROJECT_ROOT}/deps/squirrel/download"
    # Remove librime directory (but not if it's a symlink to preserve submodule)
    if [ -d "${PROJECT_ROOT}/deps/squirrel/librime" ] && [ ! -L "${PROJECT_ROOT}/deps/squirrel/librime" ]; then
        rm -rf "${PROJECT_ROOT}/deps/squirrel/librime"
    fi
    echo "Clean completed."
    
    if [ $BUILD_DEPS -eq 0 ] && [ $BUILD_APP -eq 0 ] && [ $BUILD_PACKAGE -eq 0 ]; then
        exit 0
    fi
fi

# Check for Xcode
if ! command -v xcodebuild &> /dev/null; then
    echo "ERROR: Xcode Command Line Tools not found!"
    echo "Please install with: xcode-select --install"
    exit 1
fi

# Create output directory
mkdir -p "${PROJECT_ROOT}/output/macos"

# Download dependencies (prebuilt librime)
if [ $BUILD_DEPS -eq 1 ]; then
    echo ""
    echo "============================================"
    echo "Downloading Dependencies"
    echo "============================================"
    echo ""
    
    cd "${PROJECT_ROOT}/deps/squirrel"
    
    # Define download URLs
    RIME_ARCHIVE="rime-${RIME_GIT_HASH}-macOS-universal.tar.bz2"
    RIME_DOWNLOAD_URL="https://github.com/rime/librime/releases/download/${RIME_VERSION}/${RIME_ARCHIVE}"
    
    RIME_DEPS_ARCHIVE="rime-deps-${RIME_GIT_HASH}-macOS-universal.tar.bz2"
    RIME_DEPS_DOWNLOAD_URL="https://github.com/rime/librime/releases/download/${RIME_VERSION}/${RIME_DEPS_ARCHIVE}"
    
    SPARKLE_ARCHIVE="Sparkle-${SPARKLE_VERSION}.tar.xz"
    SPARKLE_DOWNLOAD_URL="https://github.com/sparkle-project/Sparkle/releases/download/${SPARKLE_VERSION}/${SPARKLE_ARCHIVE}"
    
    # Create download directory
    mkdir -p download
    cd download
    
    # Download librime
    if [ ! -f "${RIME_ARCHIVE}" ]; then
        echo "Downloading librime ${RIME_VERSION}..."
        curl -LO "${RIME_DOWNLOAD_URL}"
    else
        echo "Using cached librime archive."
    fi
    echo "Extracting librime..."
    tar --bzip2 -xf "${RIME_ARCHIVE}"
    
    # Download librime deps (opencc data)
    if [ ! -f "${RIME_DEPS_ARCHIVE}" ]; then
        echo "Downloading librime deps..."
        curl -LO "${RIME_DEPS_DOWNLOAD_URL}"
    else
        echo "Using cached librime deps archive."
    fi
    echo "Extracting librime deps..."
    tar --bzip2 -xf "${RIME_DEPS_ARCHIVE}"
    
    # Download Sparkle
    if [ ! -f "${SPARKLE_ARCHIVE}" ]; then
        echo "Downloading Sparkle ${SPARKLE_VERSION}..."
        curl -LO "${SPARKLE_DOWNLOAD_URL}"
    else
        echo "Using cached Sparkle archive."
    fi
    echo "Extracting Sparkle..."
    tar -xJf "${SPARKLE_ARCHIVE}"
    
    cd ..
    
    # Setup librime directory
    # Squirrel needs librime source headers (src/rime/*.h) for compilation
    # but uses prebuilt library files
    if [ -L "librime" ]; then
        echo "Removing old librime symlink..."
        rm -f librime
    fi
    if [ -d "librime" ] && [ ! -L "librime" ]; then
        echo "Removing old librime directory..."
        rm -rf librime
    fi
    
    # Create librime directory structure
    mkdir -p librime/dist
    mkdir -p librime/share
    mkdir -p librime/include
    mkdir -p librime/src
    
    # Copy prebuilt files to dist
    cp -R download/dist/* librime/dist/
    cp -R download/share/opencc librime/share/
    
    # Copy headers for Xcode
    cp -R download/dist/include/* librime/include/
    
    # Link to project's librime source for internal headers (rime/key_table.h etc)
    # These are needed for compilation but not included in prebuilt package
    if [ -d "${PROJECT_ROOT}/deps/librime/src/rime" ]; then
        cp -R "${PROJECT_ROOT}/deps/librime/src/rime" librime/src/
        echo "Copied librime source headers from deps/librime"
    else
        echo "WARNING: deps/librime/src/rime not found!"
        echo "Please run: git submodule update --init deps/librime"
        exit 1
    fi
    
    # Setup other directories
    mkdir -p Frameworks
    mkdir -p lib
    mkdir -p bin
    
    # Copy Sparkle framework
    cp -R download/Sparkle.framework Frameworks/
    
    # Copy rime binaries and plugins
    echo "Copying rime binaries..."
    cp -L librime/dist/lib/librime.1.dylib lib/
    cp -pR librime/dist/lib/rime-plugins lib/
    cp librime/dist/bin/rime_deployer bin/
    cp librime/dist/bin/rime_dict_manager bin/
    cp plum/rime-install bin/
    
    # Fix install names
    install_name_tool -change @rpath/librime.1.dylib @loader_path/../Frameworks/librime.1.dylib bin/rime_deployer 2>/dev/null || true
    install_name_tool -change @rpath/librime.1.dylib @loader_path/../Frameworks/librime.1.dylib bin/rime_dict_manager 2>/dev/null || true
    
    # Setup plum for input schemas
    echo "Setting up plum..."
    git submodule update --init plum
    
    # Install default Rime recipes
    echo "Installing Rime recipes..."
    rime_dir=plum/output bash plum/rime-install
    
    # Copy plum data
    mkdir -p SharedSupport
    cp -R plum/output/* SharedSupport/ 2>/dev/null || true
    
    # Copy opencc data
    cp -R librime/share/opencc SharedSupport/
    
    echo ""
    echo "Dependencies downloaded and configured successfully."
    echo "  - librime ${RIME_VERSION} (with lua, octagram, predict plugins)"
    echo "  - Sparkle ${SPARKLE_VERSION}"
    echo "  - OpenCC data"
    echo "  - Default Rime schemas"
fi

# Build app
if [ $BUILD_APP -eq 1 ]; then
    echo ""
    echo "============================================"
    echo "Building Squirrel App"
    echo "============================================"
    echo ""
    
    cd "${PROJECT_ROOT}/deps/squirrel"
    
    # Ensure Sparkle framework is available
    if [ ! -d "Frameworks/Sparkle.framework" ]; then
        echo "ERROR: Sparkle framework not found!"
        echo "Please run with 'deps' option first."
        exit 1
    fi
    
    # Build Squirrel
    mkdir -p build
    bash package/add_data_files
    xcodebuild -project Squirrel.xcodeproj \
        -configuration Release \
        -scheme Squirrel \
        -derivedDataPath build \
        COMPILER_INDEX_STORE_ENABLE=YES \
        build
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build app!"
        exit 1
    fi
    
    echo "App built successfully."
fi

# Build package
if [ $BUILD_PACKAGE -eq 1 ]; then
    echo ""
    echo "============================================"
    echo "Building Package"
    echo "============================================"
    echo ""
    
    cd "${PROJECT_ROOT}/installer/macos/package"
    chmod +x make_package.sh
    
    ./make_package.sh "${PROJECT_ROOT}/deps/squirrel/build"
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build package!"
        exit 1
    fi
    
    echo "Package built successfully."
fi

# Sign and notarize
if [ $BUILD_SIGN -eq 1 ]; then
    echo ""
    echo "============================================"
    echo "Signing and Notarizing"
    echo "============================================"
    echo ""
    
    if [ -z "${DEV_ID}" ]; then
        echo "ERROR: DEV_ID environment variable not set!"
        echo "Please set DEV_ID to your Apple Developer ID."
        exit 1
    fi
    
    cd "${PROJECT_ROOT}/installer/macos/package"
    chmod +x sign_and_notarize.sh
    
    ./sign_and_notarize.sh "${DEV_ID}" "${PROJECT_ROOT}/deps/squirrel/build"
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to sign and notarize!"
        exit 1
    fi
    
    echo "Signing and notarization completed."
fi

echo ""
echo "============================================"
echo "Build completed successfully!"
echo "============================================"
echo ""
echo "Output files:"
ls -la "${PROJECT_ROOT}/output/macos/"

cd "${PROJECT_ROOT}"
