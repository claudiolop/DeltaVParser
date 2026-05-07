# DeltaV FHX Parser

A C++ parser that converts **Emerson DeltaV** `.fhx` files into structured, human-readable CSV tables.

This tool is designed for automation, data analysis, and documentation of large DeltaV control systems by extracting objects, attributes, hierarchies, and relationships from the proprietary FHX format.

## Features

- **Full hierarchical parsing** of DeltaV FHX files (objects, nested children, attributes)
- **Configurable output** via CSV configuration files:
  - Skip specific object types
  - Combine or individual tables per object type
  - Custom column ordering and selection
- **Robust handling** of:
  - Quoted strings with embedded commas and quotes
  - Multi-line comments (`/* */`)
  - User & timestamp information
  - Complex nested structures using stack-based parsing
- **Progress tracking** with ETA
- **Clean CSV output** with proper escaping
- **Modular design** with separate configuration for types and tables

## Project Structure
```plaintext
DeltaV-FHX-Parser/
├── main3.cpp                  # Main application logic and parsing workflow
├── DeltaVObject.cpp           # Core DeltaVObject class (tree structure & traversal)
├── DeltaVObject.h             # Header for DeltaVObject class
├── StringOperations.cpp       # Utility functions (CSV handling, file operations, progress bar, etc.)
├── StringOperations.h         # Header for utility functions
│
├── Config/                    # Configuration files
│   ├── TypeConfig.csv         # Defines behavior for each object type (SKIP / INDIVIDUAL / COMBINE)
│   ├── TableConfig.csv        # Defines output table columns and order
│   ├── AvoidTypes.csv         # Object types to completely ignore
│   ├── SkipTypes.csv          # Object types to skip during hierarchy building
│   └── SkipAttributes.csv     # Attributes to exclude from generated tables
│
├── OutputTables/              # Output folder (created automatically at runtime)
│   └── *.csv                  # Generated CSV files (one per configured table)
│
├── fhx/                       # Recommended folder for your DeltaV .fhx files
│   └── YourDatabase.fhx   # Example source file
│
└── README.md                  # This documentation file
---
```
## How It Works

1. **Configuration Loading**
   - Reads `TypeConfig.csv` to decide how to handle each DeltaV object type (SKIP, INDIVIDUAL, COMBINE)
   - Reads `TableConfig.csv` to define which attributes become columns in output tables

2. **Parsing**
   - Uses a stack-based approach to handle nested `{ }` structure of FHX files
   - Builds a tree of `DeltaVObject` nodes with dynamic attributes

3. **Output Generation**
   - Creates one or more CSV files in `OutputTables/`
   - Supports both flat first-level data and full hierarchical (pre-order) traversal

## Configuration Files

### TypeConfig.csv
Controls processing behavior per object type:

| Type          | FirstLevelAction | FirstLevelTable | DataAction | DataTable     | Headers... |
|---------------|------------------|------------------|------------|---------------|------------|
| CONTROLLER    | INDIVIDUAL       | Controllers      | COMBINE    | AllData       | Name,Description,... |
| MODULE        | COMBINE          | Modules          | SKIP       | -             | -          |

### TableConfig.csv
Defines the columns for each output table.

## Building & Running

### Prerequisites
- Windows (uses `<windows.h>`, `Beep()`, etc.)
- Visual Studio or MinGW with C++17 support

### Build
```bash
# Using Visual Studio Developer Command Prompt
cl /EHsc /std:c++17 main3.cpp DeltaVObject.cpp StringOperations.cpp
```
## License

This project is licensed under the MIT License.  
See the [`LICENSE`](LICENSE) file for details.

## Contact

Claudio Lopez  
GitHub: https://github.com/claudiolop
