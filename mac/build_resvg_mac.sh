#!/bin/bash

# Build script for resvg on macOS
set -e

# Setup
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
# RUST_DIR="$PROJECT_DIR/rust_template"
RUST_DIR="$PROJECT_DIR/dependencies/resvg-0.40.0"
OUTPUT_DIR="$SCRIPT_DIR/lib"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Build for macOS
echo "Building resvg for macOS..."

# For x86_64 (Intel)
rustup target add x86_64-apple-darwin
cargo build --release --target x86_64-apple-darwin --manifest-path "$RUST_DIR/crates/c-api/Cargo.toml"

# For arm64 (Apple Silicon)
rustup target add aarch64-apple-darwin
cargo build --release --target aarch64-apple-darwin --manifest-path "$RUST_DIR/crates/c-api/Cargo.toml"

# Create universal binary
lipo -create \
  "$RUST_DIR/target/aarch64-apple-darwin/release/libresvg_capi.a" \
  "$RUST_DIR/target/x86_64-apple-darwin/release/libresvg_capi.a" \
  -output "$OUTPUT_DIR/libresvg_universal.a"

echo "Done building resvg for macOS."

# Make the Mac plugin build script executable
chmod +x "$SCRIPT_DIR/build.sh"
