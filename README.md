# RRM Measurement Analyzer

## 1. Project Overview

The task was to develop a C++ application with a Qt graphical interface to load an RRM log file, analyze a selected measurement step, and display the results. The main focus is the analysis of step measurements.

We divided the solution into three components:

1. **Log Parser:** Reads the metadata, measurement table, and summary from a `.log` file. It loads the selected step and validates the file content. 
2. **Measurement Analyzer:** Calculates the measurement duration and the mean, minimum, and maximum of radial force and torque. It also determines when the minimum and maximum occurred. 
3. **Qt Visualization:** Displays the calculated metrics and the measurements over time in two interactive charts. 

Exception handling included, see section "Validation and error handling".

## 2. Using the Application

### Select and analyze a measurement

1. Click **Browse...** to select a `.log` file.
2. Choose the measurement step using the step selector.
3. Click **Analyze** to load the data and display the results.

The step selector uses these configuration constants:

| Parameter | Value | Description |
| --- | --- | --- |
| `MIN_STEP` | `1` | Lowest selectable step; step `0` is excluded. |
| `MAX_STEP` | `9999` | Highest selectable step. |
| `DEFAULT_STEP` | `5` | Step selected when the application starts. |

The selected step must also exist in the log file, otherwise an exception occurs and a error message will be shown in the visualization. 
To analyze another step, simply enter the step. For choosing a different logfile, simply use the File browser again. The metrics and charts will be resetted automatically. Pressing **Analyze** again will compute the new ones and display the data.

### Results and charts

The application displays the measurement duration and the mean, minimum, and maximum values for **radial force** and **torque**, including the times of the extrema.

Each chart shows the measured values over time. Dashed horizontal lines indicate the **mean**, **minimum**, and **maximum**. The chart colors adapt to the current light or dark theme.

The original timestamps from the log file are preserved. They are **not shifted to start at zero** for the selected step, because this was not specified in the task. Milliseconds are converted to minutes only for display.

To inspect a smaller area, drag a rectangle across a chart to **zoom in**. Click **Reset zoom** to return to the full view.



### Validation and error handling

The application checks the file and measurement data before displaying results. Errors are caught and shown in a dialog, so they do not crash the application.

**File and format validation**

- The Parser warns if no log file is selected. The parser checks whether the file can be opened and if the configured delimiter exists.
- The parser requires a measurement header and the necessary columns for time, step, radial force, and torque. It also checks if the current number of measurements equals the expected number of measurements from the first read of the measurement column names and throws error if this is not the case.
- File reading errors, unexpected end-of-file conditions, and a missing final `log stopped:` marker are detected. Incomplete or invalid logs are rejected by the analyzer.

**Step validation**

- The GUI limits the selected step to the configured range (`MIN_STEP = 1` to `MAX_STEP = 9999`). Step `0` cannot be selected.
- If the selected step is not found in a completely parsed log, the parser reports: `Step 13 not part of file. Please choose another step.` The number reflects the selected step.
- The analyzer rejects a requested step that differs from the step loaded by the parser.

**Measurement validation**

- Measurement timestamps must be non-negative and must not decrease within the selected step. Radial force and torque values must be finite numbers.
- At least two samples and a measurement duration greater than zero are required for analysis.

**Error handling**

- Loading and analysis errors are caught by the GUI and displayed in an error dialog. Once the user has confirmed the error, interaction with the GUI resumes.


## 3. Requirements and Build Instructions

The application was developed on **Windows 11** using the **MSYS2 UCRT64** toolchain.

**Requirements**

- C++17-compatible compiler (GCC via MSYS2 UCRT64 was used).
- Qt 6 with **Qt Widgets** and **Qt Charts**.
- CMake and Ninja.

**Build**

Open a terminal configured to use the MSYS2 UCRT64 compiler and Qt installation. From the project root, run:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

**Run**

Launch the GUI executable produced in the `build` directory. In PowerShell, you can list the generated executables with:

```powershell
Get-ChildItem .\build\*.exe
```

Run the application executable (not the test executable) from the same configured environment so the required Qt libraries can be found. The GUI executable's exact name is defined in `CMakeLists.txt`.

## 4. Project Structure

The application is divided into separate components:

| File | Purpose |
| --- | --- |
| `rrm_log_parser.hpp/.cpp` | Reads and validates RRM log files and selects measurement samples. |
| `measurement_analyzer.hpp/.cpp` | Calculates measurement statistics for the selected step. |
| `main_window.hpp/.cpp` | Handles file selection, step selection, analysis, and errors in the GUI. |
| `measurement_widget.hpp/.cpp` | Displays statistics and interactive measurement charts. |
| `log_data.hpp`, `analysis_result.hpp` | Define the data structures exchanged between components. |

The parser also reads **metadata and summary information** from the log. The current GUI focuses on measurement statistics and time-series charts and does not display those other sections.

## 5. Testing

The project includes an analyzer test executable with two tests:

1. **Analyzer test:** Checks calculated metrics against expected results for predefined measurement samples.
2. **Integration test:** Loads an RRM log file through the parser and analyzes the selected measurement step.

After building the project, run the test executable from the project root:

```powershell
.\build\test_measurement_analyzer.exe
```

The integration test uses the sample log file `202211302134.log`, which must be available at the path expected by the test. A successful run reports that the tests passed.
