# Android ADB Toolbox

A Qt-based Android utility for device inspection, app management, display adjustments, battery controls, scripting, and command-line ADB interaction.


### Linux build

From the project root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j$(nproc)
```

Then run:

```bash
./toolbox
```

