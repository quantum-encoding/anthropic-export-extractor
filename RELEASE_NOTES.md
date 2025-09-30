# Release Notes - Anthropic Export Extractor

## Version 1.0.0 (2025-09-30)

**Author:** Richard Tune <rich@quantumencoding.io>
**Company:** QUANTUM ENCODING LTD
**License:** MIT

---

### Initial Public Release

This is the first public release of the Anthropic Export Extractor, a production-grade C tool for extracting and organizing conversations from Anthropic Claude JSON exports.

---

### Features

✅ **Core Functionality**
- Extracts conversations from official Anthropic Claude JSON exports
- Generates human-readable markdown files for each conversation
- Creates JSON manifests with structured metadata
- Extracts embedded artifacts (code, images, attachments)
- Handles 300MB+ JSON files efficiently
- Timestamped output directories for versioning

✅ **Architecture**
- Library-first design pattern
- RFC 8259 compliant JSON parser
- Clean separation: `libjson_parser.a` (reusable library) + thin application wrapper
- Zero external dependencies
- Production-grade error handling

✅ **User Experience**
- Comprehensive `--help` menus for both tools
- Clear extraction progress indicators
- Detailed statistics reporting
- Professional output formatting
- JSON validation and pretty-print utility included

✅ **Documentation**
- Complete README.md with usage examples
- INSTALL.md with platform-specific build instructions
- Inline code documentation
- MIT License

---

### Technical Specifications

- **Language:** C11
- **Build System:** GNU Make
- **Dependencies:** Standard C library only
- **Tested Platforms:** Linux (Ubuntu, Debian, Fedora, Arch), macOS 11+
- **Binary Size:** ~94KB (statically linked)
- **Library Size:** ~72KB (libjson_parser.a)

---

### Performance

- Successfully tested with 300MB+ JSON exports
- Handles 600+ conversations with minimal memory footprint
- Fast execution on commodity hardware
- Streaming parser architecture for memory efficiency

---

### Distribution Formats

This release is available in two formats:

1. **TAR.GZ** (Unix/Linux/macOS preferred)
   - `anthropic_export_extractor-v1.0.0.tar.gz`
   - SHA256: [To be generated]

2. **ZIP** (Windows/cross-platform)
   - `anthropic_export_extractor-v1.0.0.zip`
   - SHA256: [To be generated]

---

### Installation

```bash
# Extract the archive
tar -xzf anthropic_export_extractor-v1.0.0.tar.gz
# OR
unzip anthropic_export_extractor-v1.0.0.zip

# Build
cd anthropic_export_extractor
make all

# Run
./anthropic_export_extractor conversations.json
```

See `INSTALL.md` for detailed platform-specific instructions.

---

### Getting Your Anthropic Export

1. Visit: https://claude.ai/settings/export
2. Request your data export
3. Download `conversations.json`
4. Run the extractor

---

### Known Issues

**None at this time.**

Compiler warnings about buffer truncation (snprintf) are informational only and do not affect functionality. These warnings are from GCC's static analysis being overly conservative with maximum theoretical path lengths.

---

### Roadmap (Future Versions)

Potential features for consideration:
- JSON output filtering options
- Parallel processing for multi-core systems
- Incremental extraction (skip already-processed conversations)
- Custom output templates
- Search/indexing capabilities

Feedback welcome at: rich@quantumencoding.io

---

### Credits

**Developed by:** Richard Tune
**Company:** QUANTUM ENCODING LTD
**Email:** rich@quantumencoding.io

---

### License

MIT License - See LICENSE file for full terms.

Copyright (c) 2025 Richard Tune / QUANTUM ENCODING LTD

---

**Built with precision. Built for production. Built by QUANTUM ENCODING LTD.**