## FMU Storage Component (C++/Cross-Platform)

Three public APIs implemented with POSIX system calls:

- RestoreData: Replace on-disk store with provided records
- DeleteOldData: Remove records older than a timestamp
- RetrieveData: Read records in a timestamp range

### Data Models

See `include/fmu/data_models.hpp`.

- LocationData: `timestampMs, latitude, longitude, accurate, valid, fixType`
- DeviceStatus: `powerStage`
- VehicleStatus: `vehicleSpeed, acceleration, fuelLevelPct, cargoWeight`
- CompositeData: aggregates the three above

### Prerequisites

- **C++17 compiler** (g++, clang++, or MSVC)
- **CMake 3.15+**
- **POSIX-compatible system** (Linux, macOS, Windows with MinGW)

### Build

#### Linux/macOS:

```bash
mkdir build && cd build
cmake ..
cmake --build . -j
```

#### Windows (MinGW):

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build . -j
```

#### Windows (Visual Studio):

```bash
mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

### Run example

```bash
# optional: set custom storage dir (default is ./data)
export FMU_STORAGE_DIR=/var/lib/fmu
./fmu_example
```

### Public API

Include headers:

```cpp
#include "fmu/api.hpp"
```

Functions:

```cpp
bool RestoreData(const std::vector<fmu::CompositeData>& records, std::string* errorMessage = nullptr);
bool DeleteOldData(int64_t olderThanTimestampMs, std::string* errorMessage = nullptr);
std::vector<fmu::CompositeData> RetrieveData(int64_t fromTsMs, int64_t toTsMs, std::string* errorMessage = nullptr);
```

### Storage format

- File: `${FMU_STORAGE_DIR:-./data}/store.ndjson`
- Each line: simple CSV written via `open/write/fsync`:
  `timestampMs,latitude,longitude,accurate,valid,fixType,powerStage,vehicleSpeed,acceleration,fuelLevelPct,cargoWeight`

### Platform Support

- ✅ **Linux** - Full support with POSIX system calls
- ✅ **macOS** - Full support with POSIX system calls
- ✅ **Windows** - Support with MinGW or Visual Studio
- ⚠️ **Other systems** - May require additional testing

### Quick Test

```bash
# Clone and test
git clone <your-repo>
cd FMU
mkdir build && cd build
cmake ..
cmake --build . -j
./fmu_example
```
