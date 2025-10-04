# Anthropic Export Extractor

A production-grade C tool for extracting and organizing conversations from Anthropic Claude JSON exports into human-readable markdown files with structured artifact management.

**Author:** Richard Tune <rich@quantumencoding.io>
**Company:** QUANTUM ENCODING LTD
**License:** MIT

## Overview

The Anthropic Export Extractor transforms massive, monolithic `conversations.json` exports from Anthropic Claude into a clean, organized directory structure with:

- **Markdown files** for each conversation (human-readable format)
- **Artifact extraction** (embedded files, code snippets, attachments)
- **JSON manifests** for each conversation (machine-readable metadata)
- **Timestamped output** directories for versioning and audit trails

## Features

- ✅ **Production-Grade Architecture**: Built with the library-first pattern (core logic in `libjson_parser.a`)
- ✅ **RFC 8259 Compliant**: Full JSON parsing with proper error handling
- ✅ **Large File Support**: Handles 300MB+ JSON exports efficiently
- ✅ **Artifact Management**: Extracts embedded content to organized `artifacts/` directories
- ✅ **Metadata Preservation**: Captures UUIDs, timestamps, and conversation structure
- ✅ **Cross-Platform**: Builds on Linux, macOS, and Windows (with minor adjustments)

## Build Instructions

### Prerequisites

- GCC or Clang compiler
- GNU Make
- Standard C11 support

### Building

```bash
make all          # Build all binaries
make rebuild      # Clean and rebuild from scratch
make help         # Display usage information
```

This produces:
- `anthropic_export_extractor` - The main extraction tool
- `json_parser` - JSON validation and pretty-print utility
- `libjson_parser.a` - The reusable JSON parser library

## Usage

### Getting Help

```bash
./anthropic_export_extractor --help    # Display full help menu
./anthropic_export_extractor -h        # Short version
make help                              # Via makefile
```

### Basic Extraction

```bash
./anthropic_export_extractor conversations.json
```

### Output Structure

The tool creates a timestamped directory with the following structure:

```
extracted_conversations_2025-09-30_17-08-37/
├── Conversation_Name_abc12345/
│   ├── Conversation_Name.md        # Human-readable markdown
│   ├── manifest.json               # Machine-readable metadata
│   └── artifacts/                  # Extracted files/attachments
│       ├── code_snippet.py
│       ├── diagram.svg
│       └── data.csv
├── Another_Conversation_def67890/
│   ├── Another_Conversation.md
│   ├── manifest.json
│   └── artifacts/
└── ...
```

### Output Format

Each conversation directory contains:

1. **Markdown File** (`*.md`): Complete conversation history with:
   - Conversation metadata (UUID, timestamps)
   - Message-by-message transcripts
   - Sender identification (human/assistant)
   - Links to extracted artifacts

2. **Manifest File** (`manifest.json`): Structured metadata including:
   - Conversation metadata
   - List of artifacts and their types
   - Statistics (message count, artifact count)
   - External file references

3. **Artifacts Directory**: Extracted embedded content:
   - Code files
   - Attachments
   - Images
   - Data files

## Example Output

```
═══════════════════════════════════════════════════════
   ANTHROPIC EXPORT EXTRACTOR
═══════════════════════════════════════════════════════

Input: conversations.json (335544320 bytes)

Parsing JSON...
Found 615 conversations

Created root output directory: extracted_conversations_2025-09-30_17-08-37/

Extracting conversations:
───────────────────────────────────────────────────────
  [23] Build_quantum_library (msg:23 art:5 ext:0)
  [18] Implement_neural_network (msg:18 art:12 ext:2)
  [45] Debugging_session (msg:45 art:8 ext:1)
  ...
───────────────────────────────────────────────────────

✓ Extraction complete: 615/615 conversations processed
✓ Output directory: extracted_conversations_2025-09-30_17-08-37/
```

## Architecture

This project follows the **Library-First Pattern**:

1. **Core Library** (`libjson_parser.a`): Self-contained, reusable JSON parsing engine
   - Zero external dependencies
   - Clean public API via `json_parser.h`
   - Full memory management and error handling

2. **Application Binary** (`anthropic_export_extractor`): Thin wrapper that:
   - Handles command-line arguments
   - Manages file I/O and directory creation
   - Links against the core library

This architecture enables:
- Code reuse across multiple projects
- Easy testing and validation
- Clean separation of concerns
- Maintainable, production-grade code

## Development

### Using the JSON Parser Tool

The included `json_parser` utility can validate and format any JSON file:

```bash
./json_parser --help                  # Show help
./json_parser data.json               # Parse and display JSON
./json_parser --validate data.json    # Validate only
./json_parser --pretty data.json      # Pretty-print JSON
./json_parser --compact data.json     # Minify JSON
```

### Cleaning Build Artifacts

```bash
make clean        # Remove all build artifacts
```

### Code Structure

- `json_parser.c/h` - Core JSON parsing library
- `json_extractor.c` - Main extraction logic
- `main.c` - JSON parser test suite
- `Makefile` - Build system

## Use Cases

- **Knowledge Management**: Archive and organize AI conversation history
- **Documentation**: Extract project documentation from development conversations
- **Research**: Analyze conversation patterns and artifact usage
- **Backup**: Create searchable, human-readable backups of Claude conversations
- **Migration**: Transform Anthropic exports into other formats

## Technical Specifications

- **Language**: C11
- **Build System**: GNU Make
- **Dependencies**: Standard C library only
- **Memory Management**: Manual allocation with comprehensive cleanup
- **Error Handling**: POSIX errno + custom error reporting
- **File Format**: UTF-8 text (Markdown, JSON)

## Performance

- Tested with 300MB+ JSON exports
- Handles 600+ conversations efficiently
- Minimal memory footprint (streaming parser architecture)
- Fast execution on commodity hardware

## Contributing

This is a production tool from QUANTUM ENCODING LTD. For inquiries, contact:

**Richard Tune**
Email: rich@quantumencoding.io
Company: QUANTUM ENCODING LTD

## License

MIT License - See LICENSE file for details

---

**Built with precision. Built for production. Built by QUANTUM ENCODING LTD.**