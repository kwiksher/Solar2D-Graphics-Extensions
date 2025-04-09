# iOS Platform Build Instructions

## Prerequisites
- Xcode 12.0+
- Rust (for resvg compilation)
- Solar2D Enterprise

## Setup
1. Install Rust and Cargo:
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

2. Add iOS targets to Rust:
```bash
rustup target add aarch64-apple-ios x86_64-apple-ios
```

3. Open the Xcode project and build

## Building resvg for iOS
The script `build_resvg_ios.sh` handles the compilation of resvg for iOS:
```bash
./build_resvg_ios.sh
```

## Plugin Structure
- `plugin/` - Corona/Solar2D plugin interface
- `src/` - C++ implementation of the plugin
- `../rust/` - Rust bindings for resvg
- `../common/` - Shared code between platforms
