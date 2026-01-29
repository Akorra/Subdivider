# 🔺 Subdivider

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.16+-green.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://github.com/yourusername/subdivider)

A high-performance C++ subdivision surface library with half-edge mesh data structure, optimized for GPU and real-time editing.

<p align="center">
  <img src="docs/images/subdivision-demo.gif" alt="Subdivision Demo" width="600"/>
</p>

---

## ✨ Features

- 🔷 **Half-Edge Mesh Structure** - Efficient connectivity representation for subdivision surfaces
- 📐 **Catmull-Clark Subdivision** - Industry-standard subdivision algorithm
- 🚀 **GPU-Friendly** - Cache-aligned structures with minimal marshaling
- ✅ **Manifold Enforcement** - Automatic validation and error detection
- 🎯 **Edge Creasing** - Support for smooth, hard, and semi-sharp edges
- 📊 **Comprehensive Diagnostics** - Optional error tracking, profiling, and memory tracking
- ⚡ **Zero-Overhead Abstractions** - Compile-time feature selection
- 🧪 **Well-Tested** - Comprehensive test suite with Catch2

---

## 📋 Table of Contents

- [Quick Start](#-quick-start)
- [Installation](#-installation)
- [Usage Examples](#-usage-examples)
- [Build Options](#-build-options)
- [API Documentation](#-api-documentation)
- [Testing](#-testing)
- [Performance](#-performance)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🚀 Quick Start

### Prerequisites

- **C++23** compatible compiler (MSVC 2022+, GCC 12+, or Clang 15+)
- **CMake** 3.16 or higher
- **GLM** (included)

### Build and Run
```bash
# Clone the repository
git clone https://github.com/yourusername/subdivider.git
cd subdivider

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the example
./build/consumerApp/consumerApp  # Linux/macOS
.\build\consumerApp\Release\consumerApp.exe  # Windows
```

### Simple Example
```cpp
#include <subdiv/control/controlmesh.h>

int main() {
    Subdiv::Control::ControlMesh mesh;
    
    // Create a quad
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2, v3});
    
    std::cout << "Mesh created with " << mesh.numFaces() << " faces\n";
}
```

---

## 📦 Installation

### Option 1: CMake FetchContent

Add to your `CMakeLists.txt`:
```cmake
include(FetchContent)

FetchContent_Declare(
    subdivider
    GIT_REPOSITORY https://github.com/yourusername/subdivider.git
    GIT_TAG        main  # or specific version tag
)

FetchContent_MakeAvailable(subdivider)

target_link_libraries(your_target PRIVATE subdiv)
```

### Option 2: Git Submodule
```bash
git submodule add https://github.com/yourusername/subdivider.git external/subdivider
```

Then in your `CMakeLists.txt`:
```cmake
add_subdirectory(external/subdivider)
target_link_libraries(your_target PRIVATE subdiv)
```

### Option 3: System Install
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

---

## 💡 Usage Examples

<details>
<summary><b>Basic Mesh Creation</b></summary>
TBA
</details>

<details>
<summary><b>Error Handling with Diagnostics</b></summary>
TBA
</details>

<details>
<summary><b>Profiling and Performance Analysis</b></summary>
TBA
</details>

<details>
<summary><b>Computing Normals</b></summary>
TBA
</details>

---

## ⚙️ Build Options

### Build Types

| Build Type | Optimization | Debug Info | Use Case |
|------------|-------------|------------|----------|
| `Debug` | None | Full | Development |
| `Release` | Maximum | None | Production (fastest) |
| `RelWithDebInfo` | High | Full | Profiling with symbols |
| `Profile` | High | Full | Performance analysis |

### CMake Options
```cmake
# Diagnostic Features (compile-time)
-DSUBDIV_ENABLE_PROFILING=ON        # Enable timing profiling
-DSUBDIV_ENABLE_MEMORY_TRACKING=ON  # Enable memory tracking
-DSUBDIV_ENABLE_VALIDATION=ON       # Enable validation checks
-DSUBDIV_ENABLE_ASSERTS=ON          # Enable assertions

# Build Options
-DSUBDIV_BUILD_TESTS=ON             # Build test suite
-DCMAKE_BUILD_TYPE=Debug|Release    # Build configuration
```

### Using CMake Presets
```bash
# List available presets
cmake --list-presets

# Configure with preset
cmake --preset debug      # Development with diagnostics
cmake --preset release    # Optimized release
cmake --preset profile    # Performance analysis

# Build
cmake --build --preset debug

# Test
ctest --preset debug

# All-in-one
cmake --workflow --preset debug
```

**Available Presets:**
- `debug` - Development with validation and asserts
- `release` - Optimized production build
- `profile` - Performance analysis with full diagnostics
- `relwithdebinfo` - Optimized with debug symbols
- `release-safe` - Production with validation

### Configuration Examples

<details>
<summary><b>Performance Build (Fastest)</b></summary>
```bash
cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

</details>

<details>
<summary><b>Development Build (Full Diagnostics)</b></summary>
```bash
cmake -B build/dev \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSUBDIV_ENABLE_PROFILING=ON \
  -DSUBDIV_ENABLE_MEMORY_TRACKING=ON \
  -DSUBDIV_ENABLE_VALIDATION=ON \
  -DSUBDIV_ENABLE_ASSERTS=ON
cmake --build build/dev
```

</details>

<details>
<summary><b>Production with Validation</b></summary>
```bash
cmake -B build/prod \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUBDIV_ENABLE_VALIDATION=ON
cmake --build build/prod
```

</details>

---

## 📊 Performance

Performance benchmarks on AMD Ryzen 9 5900X:

| Operation | Time (Debug) | Time (Release) | Speedup |
|-----------|--------------|----------------|---------|
| Add 10K vertices | N/A | N/A | N/A |
| Add 10K faces | N/A | N/A | N/A |
| Validate mesh | N/A | N/A | N/A |
| Compute normals | N/A | N/A | N/A |

*Benchmarks run with profiling enabled in Profile build.*

### Memory Layout

All core structures are cache-aligned and GPU-friendly:
```
Vertex:        24 bytes  (cache-line friendly)
HalfEdge:      20 bytes  (tightly packed)
Edge:          8 bytes   (power-of-2)
Face:          8 bytes   (power-of-2)
```

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- [GLM](https://github.com/g-truc/glm) - Mathematics library
- [Catch2](https://github.com/catchorg/Catch2) - Testing framework
- Inspired by industry-standard subdivision surface implementations

---

## 📞 Contact

- **Author**: [Filipe Marques](https://github.com/Akorra)
- **Project**: [https://github.com/yourusername/subdivider](https://github.com/Akorra/subdivider)
- **Issues**: [https://github.com/yourusername/subdivider/issues](https://github.com/Akorra/subdivider/issues)

---