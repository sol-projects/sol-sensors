# SOL Sensors

SOL Sensors is a collection of programs and libraries for Linux and Windows meant for fetching information from hardware sensors and displaying them in real time, storing them or using them in your code.

The website for this project can be found on sol-projects.github.io/sensors/home.html. It hosts tutorials and more information about the project, including how to compile it and include it into your projects.

## Functionality
### Linux
- [x] CPU usage
- [x] CPU temperature
- [x] CPU Threads usage
- [x] RAM usage
- [ ] RAM temperature
- [x] Nvidia GPU usage
- [x] Nvidia GPU temperature
- [x] Nvidia GPU VRAM usage
- [x] Nvidia GPU VRAM temperature
- [x] AMD GPU usage
- [x] AMD GPU temperature
- [x] AMD GPU VRAM usage
- [x] AMD GPU VRAM temperature

AMD readings use the Linux amdgpu sysfs/hwmon interfaces. GPU and VRAM temperatures are available only when exposed by the hardware and driver.

### Windows 10/11
- [x] CPU usage
- [x] CPU Threads usage
- [x] RAM usage
- [x] AMD and Nvidia GPU usage
- [x] AMD and Nvidia GPU VRAM usage
- [x] GPU temperature, when reported by WDDM and built with the Windows SDK
- [ ] CPU, RAM and VRAM temperature

Windows uses native PDH performance counters and DXGI, without extra sensor libraries or configuration. GPU counters require WDDM 2.0 or newer. GPU temperature requires WDDM 2.4 or newer and a current Windows SDK (`d3dkmthk.h`); MinGW builds without that header return an unsupported reading. CPU, GPU and thread usage are percentages; RAM and VRAM usage are decimal GB. The existing integer precision and unsupported error code stay the same.

### MacOS
- [x] CPU usage
- [x] CPU temperature
- [x] CPU Threads usage
- [x] RAM usage
- [ ] RAM temperature
- [ ] Nvidia GPU usage
- [ ] Nvidia GPU temperature
- [ ] Nvidia GPU VRAM usage
- [ ] Nvidia GPU VRAM temperature
- [ ] AMD GPU usage
- [ ] AMD GPU temperature
- [ ] AMD GPU VRAM usage
- [ ] AMD GPU VRAM temperature

## QT Program
The GUI version of the program displays realtime graphs for hardware components.
To compile and run the QT GUI version of the program, you must have Qt5 or Qt6 installed. 
You can find more information on the [website](https://sol-projects.github.io/sensors/tutorial/gui.html)

## Console Program
You can find more information on the [website](https://sol-projects.github.io/sensors/tutorial/nogui.html)
## Sensors Library
You can find more information on the [website](https://sol-projects.github.io/sensors/tutorial/sensors.html)

## Build

The existing CMake options and `config.conf` format are unchanged. Install a C++20 compiler, CMake, Git and Qt 5 or Qt 6 Widgets/PrintSupport. CMake downloads the existing project dependencies on first configuration.

```sh
cmake -S . -B build
cmake --build build --parallel
./build/SOL_SENSORS
ctest --test-dir build --output-on-failure
```

On Windows, run these commands in a Visual Studio developer terminal, using a Qt build matching your compiler. Set `Qt6_ROOT` (or `Qt5_ROOT`) to your Qt installation if CMake does not find it automatically.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
windeployqt --release build\Release\SOL_SENSORS.exe
.\build\Release\SOL_SENSORS.exe
```

`windeployqt` must be available from the matching Qt installation. For a console-only build, use `-DBUILD_GUI=OFF`; for the sensors library without Qt or console dependencies, also use `-DBUILD_NOGUI=OFF -DBUILD_TESTS=OFF`.

The dashboard keeps the 300 ms refresh, shows CPU, RAM, GPU, VRAM and CPU thread graphs, and retains 60 seconds of history. Graph controls hide whole sections. Missing sensor readings are shown as unavailable. Narrow windows use one column.

Tests cover AMD sysfs fixtures, unsupported measurements, native Windows enumeration (on Windows), graph controls, narrow layouts and bounded history. The legacy console test calls `exit()`, so CTest checks CLI help in a separate process and runs the other existing tests together. Tests write their config and CSV files inside the build directory.
