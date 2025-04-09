# Rust Components for Graphics Extensions

This directory contains the Rust code for the resvg wrapper used by the Graphics Extensions plugin.

## Structure
- `src/` - Rust source code
- `Cargo.toml` - Rust package manifest
- `build.rs` - Rust build script

## Building
The Rust components are built using the platform-specific build scripts:
- iOS: `ios/build_resvg_ios.sh`
- macOS: `mac/build_resvg_mac.sh`

## Development
To modify the Rust bindings:
1. Edit the source files in `src/`
2. Run the appropriate build script
3. The compiled static libraries will be placed in the respective platform directories
