# Subdivider

A high-performance C++ subdivision surface library with half-edge mesh data structure, optimized for GPU and real-time editing.

## Features

- **Half-Edge Mesh Structure**: Efficient connectivity representation for subdivision surfaces
- **Catmull-Clark Subdivision**: Industry-standard subdivision algorithm (base mesh implementation complete)
- **GPU-Friendly Data Layout**: Cache-aligned structures with minimal marshaling
- **Manifold Enforcement**: Automatic validation and error detection
- **Edge Creasing**: Support for smooth, hard, and semi-sharp edges
- **Corner Vertices**: Dart vertex support for subdivision surfaces
- **Comprehensive Diagnostics**: Optional error tracking, profiling, and memory tracking
- **Modern C++23**: Clean, type-safe API with zero-overhead abstractions

## Table of Contents

- [Requirements](#requirements)
- [Building](#building)
  - [Quick Start](#quick-start)
  - [Using CMake Presets](#using-cmake-presets)
  - [Manual CMake Configuration](#manual-cmake-configuration)
- [Build Options](#build-options)
- [Project Structure](#project-structure)
- [Usage](#usage)
  - [Basic Example](#basic-example)
  - [With Diagnostics](#with-diagnostics)
  - [With Profiling](#with-profiling)
- [API Overview](#api-overview)
- [Testing](#testing)
- [Documentation](#documentation)
- [License](#license)

## Requirements

- **C++ Compiler**: C++23 support required
  - MSVC 2022+ (Windows)
  - GCC 12+ (Linux)
  - Clang 15+ (macOS/Linux)
- **CMake**: Version 3.16 or higher
- **GLM**: Mathematics library (included in `subdiv/external/glm`)
- **Catch2**: Testing framework (automatically fetched)

## Building

### Quick Start

#### Windows (PowerShell)
```powershell
# Debug build
cmake -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --config Debug

# Release build
cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --config Release

# Run the consumer app
.\build\debug\consumerApp\Debug\consumerApp.exe
```

#### Linux/macOS
```bash
# Debug build
cmake -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug

# Release build
cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release

# Run the consumer app
./build/debug/consumerApp/consumerApp
```

### Using CMake Presets

CMake presets provide pre-configured build types with optimized settings.
```powershell
# List available presets
cmake --list-presets

# Configure with preset
cmake --preset debug
cmake --preset release
cmake --preset profile

# Build with preset
cmake --build --preset debug
cmake --build --preset release
cmake --build --preset profile

# Test with preset
ctest --preset debug
ctest --preset release

# All-in-one workflow (configure + build + test)
cmake --workflow --preset debug
```

**Available Presets:**

| Preset | Description | Diagnostics |
|--------|-------------|-------------|
| `debug` | Debug build with validation and asserts | Validation + Asserts |
| `release` | Optimized release build | None |
| `profile` | Optimized with profiling enabled | Profiling + Memory + Validation |
| `relwithdebinfo` | Optimized with debug symbols | Validation |
| `release-safe` | Release with validation | Validation |
| `debug-no-tests` | Debug without building tests | Validation + Asserts |

### Manual CMake Configuration

Full control over build options:
```powershell
# Configure with custom options
cmake -B build/custom \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSUBDIV_ENABLE_PROFILING=ON \
  -DSUBDIV_ENABLE_MEMORY_TRACKING=ON \
  -DSUBDIV_ENABLE_VALIDATION=ON \
  -DSUBDIV_ENABLE_ASSERTS=ON \
  -DSUBDIV_BUILD_TESTS=ON

# Build
cmake --build build/custom --config Debug

# Build specific target
cmake --build build/custom --config Debug --target subdiv
cmake --build build/custom --config Debug --target consumerApp
cmake --build build/custom --config Debug --target subdiv_tests

# Verbose build (see compiler commands)
cmake --build build/custom --config Debug --verbose
```

## Build Options

### Build Types

| Build Type | Optimization | Debug Info | Default Diagnostics |
|------------|-------------|------------|---------------------|
| `Debug` | None (`-O0`) | Full | Validation + Asserts |
| `Release` | Max (`-O3`) | None | None (fastest) |
| `RelWithDebInfo` | High (`-O2`) | Full | Validation |
| `Profile` | High (`-O2`) | Full | All diagnostics |

### Diagnostic Options

Control diagnostic features at compile time for zero overhead when disabled:
```cmake
-DSUBDIV_ENABLE_PROFILING=ON|OFF        # Enable timing profiling
-DSUBDIV_ENABLE_MEMORY_TRACKING=ON|OFF  # Enable memory allocation tracking
-DSUBDIV_ENABLE_VALIDATION=ON|OFF       # Enable runtime validation checks
-DSUBDIV_ENABLE_ASSERTS=ON|OFF          # Enable assertion checks
```

**Diagnostic Features:**

- **Profiling** (`SUBDIV_ENABLE_PROFILING`):
  - Function-level timing with `SUBDIV_PROFILE()` macro
  - Automatic call counting and statistics
  - Min/max/average timing per operation
  - Zero overhead when disabled (macros compile to nothing)

- **Memory Tracking** (`SUBDIV_ENABLE_MEMORY_TRACKING`):
  - Per-category allocation tracking
  - Peak memory usage monitoring
  - Allocation count statistics
  - Requires profiling to be enabled

- **Validation** (`SUBDIV_ENABLE_VALIDATION`):
  - Mesh integrity checks
  - Manifold edge validation
  - Connectivity verification
  - Detailed error messages with context

- **Asserts** (`SUBDIV_ENABLE_ASSERTS`):
  - Internal consistency checks
  - Precondition/postcondition validation
  - Disabled in Release builds by default

### Other Options
```cmake
-DSUBDIV_BUILD_TESTS=ON|OFF             # Build test suite (default: ON)
-DCMAKE_INSTALL_PREFIX=path/to/install  # Installation directory
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON      # Generate compile_commands.json for IDEs
```

### Build Configuration Examples

**Performance (fastest):**
```bash
cmake -B build/perf -DCMAKE_BUILD_TYPE=Release
```

**Development (full diagnostics):**
```bash
cmake -B build/dev \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSUBDIV_ENABLE_PROFILING=ON \
  -DSUBDIV_ENABLE_MEMORY_TRACKING=ON \
  -DSUBDIV_ENABLE_VALIDATION=ON \
  -DSUBDIV_ENABLE_ASSERTS=ON
```

**Production with validation:**
```bash
cmake -B build/prod \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUBDIV_ENABLE_VALIDATION=ON
```

**Profiling/benchmarking:**
```bash
cmake -B build/benchmark \
  -DCMAKE_BUILD_TYPE=Profile \
  -DSUBDIV_ENABLE_PROFILING=ON \
  -DSUBDIV_ENABLE_MEMORY_TRACKING=ON
```

## Project Structure
```
Subdivider/
├── CMakeLists.txt              # Root CMake configuration
├── CMakePresets.json           # CMake preset definitions
├── README.md                   # This file
│
├── subdiv/                     # Subdivision library
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── subdiv/
│   │       ├── config.h                      # Build configuration
│   │       ├── subdiv.h                      # Main header
│   │       ├── control/
│   │       │   └── controlmesh.h             # Control mesh API
│   │       └── diagnostics/
│   │           └── diagnosticscontext.h      # Diagnostics system
│   ├── src/
│   │   ├── subdiv.cpp
│   │   ├── control/
│   │   │   └── controlmesh.cpp
│   │   └── diagnostics/
│   │       └── diagnosticscontext.cpp
│   ├── external/
│   │   └── glm/                              # GLM math library
│   └── tests/
│       ├── CMakeLists.txt
│       ├── test_controlmesh.cpp
│       ├── test_diagnostics.cpp
│       └── test_halfedge.cpp
│
├── consumerApp/                # Example application
│   ├── CMakeLists.txt
│   └── src/
│       └── main.cpp
│
└── build/                      # Build output (generated)
    ├── debug/
    ├── release/
    └── profile/
```

## Usage

### Basic Example
```cpp
#include <subdiv/control/controlmesh.h>

using namespace Subdiv::Control;

int main()
{
    ControlMesh mesh;
    
    // Add vertices
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    // Add quad face
    FaceIndex face = mesh.addFace({v0, v1, v2, v3});
    
    if (face != INVALID_INDEX) {
        std::cout << "Face created successfully!\n";
        std::cout << "Vertices: " << mesh.numVertices() << "\n";
        std::cout << "Faces: " << mesh.numFaces() << "\n";
        std::cout << "Edges: " << mesh.numEdges() << "\n";
    }
    
    return 0;
}
```

### With Diagnostics
```cpp
#include <subdiv/control/controlmesh.h>
#include <subdiv/diagnostics/diagnosticscontext.h>

using namespace Subdiv;
using namespace Subdiv::Control;

int main()
{
    // Enable error tracking
    Diagnostics::enable(Diagnostics::Mode::ERRORS_ONLY);
    
    ControlMesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    
    // Try to create invalid face (too few vertices)
    mesh.addFace({v0, v1});
    
    // Check for errors
    if (Diagnostics::hasErrors()) {
        std::cerr << Diagnostics::getErrorSummary();
        // Output:
        // === Error Summary ===
        // Errors: 1
        // 
        // [ERROR] FACE_TOO_FEW_VERTICES: Face must have at least 3 vertices (vertex count: 2)
    }
    
    // Validate mesh
    if (mesh.validate()) {
        std::cout << "Mesh is valid!\n";
    }
    
    Diagnostics::disable();
    return 0;
}
```

### With Profiling
```cpp
#include <subdiv/control/controlmesh.h>
#include <subdiv/diagnostics/diagnosticscontext.h>
#include <subdiv/config.h>

using namespace Subdiv;
using namespace Subdiv::Control;

int main()
{
    // Enable profiling (only works if built with SUBDIV_ENABLE_PROFILING=ON)
    if constexpr (BuildInfo::profilingEnabled) {
        Diagnostics::enable(Diagnostics::Mode::ERRORS_AND_PROFILING);
    } else {
        Diagnostics::enable(Diagnostics::Mode::ERRORS_ONLY);
    }
    
    {
        SUBDIV_PROFILE("BuildMesh");
        
        ControlMesh mesh;
        
        // Create 100x100 grid
        for (int y = 0; y < 100; ++y) {
            for (int x = 0; x < 100; ++x) {
                mesh.addVertex({float(x), float(y), 0.0f});
            }
        }
        
        // Create faces
        for (int y = 0; y < 99; ++y) {
            for (int x = 0; x < 99; ++x) {
                VertexIndex v0 = y * 100 + x;
                VertexIndex v1 = y * 100 + (x + 1);
                VertexIndex v2 = (y + 1) * 100 + (x + 1);
                VertexIndex v3 = (y + 1) * 100 + x;
                
                mesh.addFace({v0, v1, v2, v3});
            }
        }
        
        mesh.computeVertexNormals();
    }
    
    // Print profiling results
    if constexpr (BuildInfo::profilingEnabled) {
        std::cout << Diagnostics::getProfilingSummary();
        // Output:
        // === Profiling Summary ===
        // Operation                      Total (ms)   Avg (ms)   Min (ms)   Max (ms)     Calls
        // --------------------------------------------------------------------------------
        // BuildMesh                         123.456    123.456    123.456    123.456        1
        // ControlMesh::addFace               98.765      0.010      0.001      1.234     9801
        // ControlMesh::computeVertexNormals  12.345     12.345     12.345     12.345        1
    }
    
    Diagnostics::disable();
    return 0;
}
```

### Result-Based Error Handling
```cpp
#include <subdiv/control/controlmesh.h>

using namespace Subdiv;
using namespace Subdiv::Control;

int main()
{
    ControlMesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    // Use Result type for explicit error handling
    auto result = mesh.tryAddFace({v0, v1, v2});
    
    if (result.isOk()) {
        FaceIndex face = result.value();
        std::cout << "Face created: " << face << "\n";
    } else {
        const auto& error = result.error();
        std::cerr << "Error: " << error.message << "\n";
        std::cerr << "Code: " << error.code << "\n";
        if (!error.context.empty()) {
            std::cerr << "Context: " << error.context << "\n";
        }
    }
    
    return 0;
}
```

## API Overview

### ControlMesh

Main class for creating and manipulating subdivision control meshes.

**Creation:**
```cpp
VertexIndex addVertex(const glm::vec3& pos);
FaceIndex addFace(const std::vector<VertexIndex>& verts);
Result<FaceIndex> tryAddFace(const std::vector<VertexIndex>& verts);
```

**Queries:**
```cpp
VertexIndex getFromVertex(HalfEdgeIndex heIdx) const;
int getVertexValence(VertexIndex vIdx) const;
bool isBoundaryVertex(VertexIndex vIdx) const;
HalfEdgeIndex findHalfEdge(VertexIndex v0, VertexIndex v1) const;
EdgeIndex findEdge(VertexIndex v0, VertexIndex v1) const;
```

**Validation:**
```cpp
bool validate() const;
```

**Attributes:**
```cpp
void computeVertexNormals();
void computeFaceNormals();
```

**Statistics:**
```cpp
size_t numVertices() const;
size_t numHalfEdges() const;
size_t numEdges() const;
size_t numFaces() const;
```

### Diagnostics

Global singleton for error tracking, profiling, and memory tracking.

**Configuration:**
```cpp
Diagnostics::enable(Diagnostics::Mode::ERRORS_ONLY);
Diagnostics::enable(Diagnostics::Mode::ERRORS_AND_PROFILING);
Diagnostics::enable(Diagnostics::Mode::FULL_DIAGNOSTICS);
Diagnostics::disable();
```

**Error Tracking:**
```cpp
bool Diagnostics::hasErrors();
bool Diagnostics::hasWarnings();
bool Diagnostics::hasFatalErrors();
const std::vector<ErrorInfo>& Diagnostics::getErrors();
std::string Diagnostics::getErrorSummary();
```

**Profiling:**
```cpp
SUBDIV_PROFILE("OperationName");           // Profile a scope
SUBDIV_PROFILE_FUNCTION();                 // Profile current function
std::string Diagnostics::getProfilingSummary();
```

**Full Report:**
```cpp
std::string Diagnostics::getFullReport();  // Errors + Profiling + Memory
```

### Macros

Diagnostic macros compile to zero overhead when disabled:
```cpp
SUBDIV_PROFILE(name)                       // Profile scope
SUBDIV_PROFILE_FUNCTION()                  // Profile function
SUBDIV_TRACK_ALLOC(category, bytes)        // Track allocation
SUBDIV_TRACK_DEALLOC(category, bytes)      // Track deallocation
SUBDIV_ASSERT(condition, message)          // Debug assertion
```

## Testing

### Run All Tests
```bash
# Using CTest
ctest --test-dir build/debug --output-on-failure

# Or run directly
./build/debug/subdiv/tests/subdiv_tests

# Run with verbose output
./build/debug/subdiv/tests/subdiv_tests -s

# Run specific test suite
./build/debug/subdiv/tests/subdiv_tests "[controlmesh]"
./build/debug/subdiv/tests/subdiv_tests "[diagnostics]"
./build/debug/subdiv/tests/subdiv_tests "[halfedge]"
```

### Test Organization

Tests are organized by tags:
- `[controlmesh]` - Core mesh functionality
- `[face]` - Face creation and validation
- `[manifold]` - Manifold mesh tests
- `[boundary]` - Boundary detection
- `[queries]` - Topology queries
- `[halfedge]` - Half-edge connectivity
- `[diagnostics]` - Error tracking
- `[profiling]` - Profiling tests (only when enabled)
- `[memory]` - Memory tracking tests (only when enabled)

## Documentation

### Build Information

Check build configuration at runtime:
```cpp
#include <subdiv/config.h>

std::cout << "Version: " << Subdiv::BuildInfo::getVersionString() << "\n";
std::cout << "Build Type: " << Subdiv::BuildInfo::getBuildType() << "\n";
std::cout << "Configuration: " << Subdiv::BuildInfo::getConfigString() << "\n";

if constexpr (Subdiv::BuildInfo::profilingEnabled) {
    std::cout << "Profiling is available\n";
}
```

### Error Codes

Common error codes returned by the library:

| Code | Description |
|------|-------------|
| `FACE_TOO_FEW_VERTICES` | Face must have at least 3 vertices |
| `INVALID_VERTEX_INDEX` | Vertex index is out of bounds |
| `DUPLICATE_VERTEX_IN_FACE` | Face contains duplicate vertices |
| `NON_MANIFOLD_EDGE` | Edge already has two faces (non-manifold geometry) |
| `INVALID_HALFEDGE_*` | Half-edge connectivity error |
| `FACE_LOOP_*` | Face loop integrity error |
| `*_MISMATCH` | Data structure size mismatch |

## License

[Your License Here]

## Contributing

[Contributing guidelines]

## Authors

[Your name/team]

## Acknowledgments

- GLM for mathematics
- Catch2 for testing
- [Other acknowledgments]