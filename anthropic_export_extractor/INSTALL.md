# Installation Guide

## Anthropic Export Extractor v1.0.0

**Author:** Richard Tune <rich@quantumencoding.io>
**Company:** QUANTUM ENCODING LTD
**License:** MIT

---

## Quick Start

### 1. Build from Source

```bash
make all
```

This will create:
- `anthropic_export_extractor` - The main tool
- `json_parser` - Test suite
- `libjson_parser.a` - JSON parser library

### 2. Run the Tool

```bash
./anthropic_export_extractor conversations.json
```

### 3. Get Help

```bash
./anthropic_export_extractor --help
```

---

## System Requirements

### Minimum Requirements
- **Operating System:** Linux, macOS, or Windows (with MinGW/Cygwin)
- **Compiler:** GCC 4.9+ or Clang 3.5+
- **C Standard:** C11
- **Build System:** GNU Make 3.81+
- **Memory:** 512MB RAM minimum (recommended 1GB+ for large exports)
- **Disk Space:** Variable (depends on export size; allocate 2-3x your JSON file size)

### Tested Platforms
- ✅ Ubuntu 20.04+ (GCC 9.x, 11.x, 13.x)
- ✅ Debian 11+ (GCC 10.x+)
- ✅ macOS 11+ (Clang/Apple LLVM)
- ✅ Fedora 35+ (GCC 11.x+)
- ✅ Arch Linux (GCC latest)

---

## Detailed Build Instructions

### On Linux (Debian/Ubuntu)

```bash
# Install build tools (if not already installed)
sudo apt update
sudo apt install build-essential

# Build the project
cd anthropic_export_extractor
make all

# Verify the build
./anthropic_export_extractor --help
```

### On Linux (Fedora/RHEL)

```bash
# Install build tools
sudo dnf install gcc make

# Build the project
cd anthropic_export_extractor
make all
```

### On macOS

```bash
# Install Xcode Command Line Tools (if not installed)
xcode-select --install

# Build the project
cd anthropic_export_extractor
make all
```

### On Windows (MinGW)

```bash
# Using MinGW or MSYS2
cd anthropic_export_extractor
make all
```

---

## Build Options

```bash
make all          # Build all binaries
make rebuild      # Clean and rebuild from scratch
make clean        # Remove all build artifacts
make test         # Run the JSON parser test suite
make help         # Display tool usage information
```

---

## Installation (Optional)

To install system-wide (Linux/macOS):

```bash
sudo cp anthropic_export_extractor /usr/local/bin/
sudo chmod +x /usr/local/bin/anthropic_export_extractor
```

Now you can run it from anywhere:

```bash
anthropic_export_extractor ~/Downloads/conversations.json
```

---

## Troubleshooting

### Build Errors

**Error: `gcc: command not found`**
- **Solution:** Install GCC using your package manager (see platform-specific instructions above)

**Error: `make: command not found`**
- **Solution:** Install GNU Make using your package manager

**Error: Compiler warnings about truncation**
- **Status:** These are informational warnings and safe to ignore. The code handles buffer sizes correctly.

### Runtime Errors

**Error: `Cannot open file: conversations.json`**
- **Solution:** Verify the file path is correct and the file exists
- **Solution:** Check file permissions (must be readable)

**Error: `Failed to parse JSON`**
- **Solution:** Verify the JSON file is a valid Anthropic export
- **Solution:** Check the file isn't corrupted (try opening in a text editor)

**Error: `Out of memory`**
- **Solution:** The JSON file may be too large for available RAM
- **Solution:** Close other applications or upgrade system memory

---

## Getting Your Anthropic Export

1. Visit: https://claude.ai/settings/export
2. Click "Request data export"
3. Wait for email notification (usually within 24 hours)
4. Download the `conversations.json` file
5. Run the extractor:

```bash
./anthropic_export_extractor ~/Downloads/conversations.json
```

---

## Verifying the Installation

Run the test suite to verify everything is working:

```bash
make test
```

Expected output:
```
JSON Parser Test Suite
======================

Test 1: null
Result: null
...
```

---

## Uninstalling

```bash
# Remove system-wide installation
sudo rm /usr/local/bin/anthropic_export_extractor

# Remove build artifacts
cd anthropic_export_extractor
make clean
```

---

## Getting Help

- **Documentation:** See `README.md` for detailed usage instructions
- **Bug Reports:** rich@quantumencoding.io
- **License:** See `LICENSE` for terms

---

**Built with precision. Built for production. Built by QUANTUM ENCODING LTD.**