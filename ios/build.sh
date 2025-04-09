#!/bin/bash

# Build the Rust components
./build_resvg_ios.sh

# Set environment variables
PLUGIN_NAME=plugin_gfxe
BUILD_DIR=build
FRAMEWORK_DIR=plugin
FRAMEWORK
_NAME="${PLUGIN_NAME}.framework"
OUTPUT_DIR="${FRAMEWORK_DIR}/${BUILD_DIR}/${PLUGIN_NAME}"

# Clean and create output directories
rm -rf "${FRAMEWORK_DIR}/${BUILD_DIR}"
mkdir -p "${OUTPUT_DIR}"

# Build the plugin
xcodebuild -project Plugin.xcodeproj -target plugin_gfxe -configuration Release clean build

# Copy the plugin library
cp -R "${BUILD_DIR}/Release-iphoneos/lib${PLUGIN_NAME}.a" "${OUTPUT_DIR}/"

# Copy the metadata.lua
cp "${FRAMEWORK_DIR}/${PLUGIN_NAME}.lua" "${OUTPUT_DIR}/metadata.lua"

echo "Plugin built successfully at ${OUTPUT_DIR}"
