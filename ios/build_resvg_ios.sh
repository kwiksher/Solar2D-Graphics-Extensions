#!/bin/bash

# Build script for resvg on iOS
set -e

# Setup
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
# RUST_DIR="$PROJECT_DIR/rust_template"
RUST_DIR="$PROJECT_DIR/dependencies/resvg-0.40.0"
OUTPUT_DIR="$SCRIPT_DIR/lib"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Build for iOS
echo "Building resvg for iOS..."

# For arm64
rustup target add aarch64-apple-ios
cargo build --release --target aarch64-apple-ios --manifest-path "$RUST_DIR/crates/c-api/Cargo.toml"

# For simulator (x86_64)
rustup target add x86_64-apple-ios
cargo build --release --target x86_64-apple-ios --manifest-path "$RUST_DIR/crates/c-api/Cargo.toml"

# Create universal binary
lipo -create \
  "$RUST_DIR/target/aarch64-apple-ios/release/libresvg_capi.a" \
  "$RUST_DIR/target/x86_64-apple-ios/release/libresvg_capi.a" \
  -output "$OUTPUT_DIR/libresvg_universal.a"

echo "Done building resvg for iOS."
