# RRM Measurement Analyzer

## 1. Project Overview

The task was to develop a C++ application with a Qt graphical interface to load an RRM log file, analyze a selected measurement step, and display the results. The main focus is the analysis of step measurements.

We divided the solution into three components:

1. **Log Parser:** Reads the metadata, measurement table, and summary from a `.log` file. It loads the selected step and validates the file content. 
2. **Measurement Analyzer:** Calculates the measurement duration and the mean, minimum, and maximum of radial force and torque. It also determines when the minimum and maximum occurred. 
3. **Qt Visualization:** Displays the calculated metrics and the measurements over time in two interactive charts. 

Error handling is described in the "Validation and error handling" section.

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

The selected step must also exist in the log file. Otherwise, an error message is displayed in the GUI.
To analyze another step, simply select a different step. To choose another log file, use the file browser again. The metrics and charts will be reset automatically. Pressing **Analyze** button gain will compute the new ones and display the data.

### Results and charts

The application displays the measurement duration and the mean, minimum, and maximum values for **radial force** and **torque**, including the times of the extrema.

Each chart shows the measured values over time. Dashed horizontal lines indicate the **mean**, **minimum**, and **maximum**. The chart colors adapt to the current light or dark theme.

The original timestamps from the log file are preserved. They are **not shifted to start at zero** for the selected step, because this was not specified in the task. Milliseconds are converted to minutes only for display.

To inspect a smaller area, drag a rectangle across a chart to **zoom in**. Click **Reset zoom** to return to the full view.



### Validation and error handling

The application checks the file and measurement data before displaying results. Errors are caught and shown in a dialog, so they do not crash the application.

**File and format validation**

- The Analyze button remains disabled until a log file is selected. The GUI also checks for an empty file path. The parser checks whether the file can be opened and whether the configured delimiter is not empty.
- The parser requires a measurement header and the necessary columns for time, step, radial force, and torque. It also checks whether the number of fields in each measurement row matches the expected number of columns from the header, allowing one missing trailing empty field ("message"). An error is reported if the validation fails.
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

After building the project, launch the application from the project root:

```powershell
.\build\rrm_analyzer.exe
```

Make sure the required Qt libraries are available in your environment.


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

The project includes two test executable test: 
- test_log_parser: Test the log parser
- test_measurement_analyzer: Test

1. **Analyzer test:** Checks calculated metrics against expected results for predefined measurement samples.
2. **Integration test:** Loads an RRM log file through the parser and analyzes the selected measurement step.

After building the project, run the test executable from the project root:

```powershell
.\build\test_log_parser.exe
.\build\test_measurement_analyzer.exe
```

**Both tests use data/202211302134.log by default. The file must be available at the expected path**. A different log file can be provided as a command-line argument.


## 5. Testing
1. **Log Parser Test (test_log_parser)**
    - Loads an RRM log file for a selected measurement step or all steps.
    - Checks whether the parser successfully loaded the data.
    - Displays the metadata, the first 10 measurement samples, and the summary.
    - Reports errors or incomplete parsing results. 

2. **Measurement Analyzer Test (test_measurement_analyzer)**
    - Analyzer unit test: Uses predefined measurement samples with known expected results. Verifies the step filtering, sample count, measurement duration, mean, minimum, maximum, and timestamps of the extrema. Numerical results are compared against expected values within a defined tolerance.

    - Parser and analyzer integration test: Loads step 5 from a real RRM log file using the parser and passes the data to the analyzer. Checks that parsing completed successfully, only the requested step was loaded, the sample counts match, and the calculated duration is positive. The computed metrics are displayed but are not compared against predefined reference values.


## 6. Future Work
The current application analyzes RRM log files offline. In a production machine, the **PLC would remain responsible for machine control**, while a separate PC would handle data processing, analysis, and visualization. 

Possible extensions include:

- **PLC communication:** Receive measurement data and machine status from the PLC through a suitable communication interface. Machine control and safety functions remain with the PLC.
- **Database integration:** Store measurement data, metadata, and analysis results in a database. Support efficient searches and loading of historical measurements without reading all records at once.
- **Customer system integration:** Exchange relevant measurement results or status information with the customer's higher-level supervisory system or other IT systems, if required.
- **Extended GUI:** Add pages for live measurements, historical results, diagnostics, data export, and visualization settings.
- **Background processing:** Run data reception, database queries, and time-consuming analysis in worker threads so the Qt interface remains responsive.
- **Visualization performance:** Improve the display of large datasets, for example by plotting a reduced number of points while keeping the full data for analysis.
- **Reliability and security:** Add connection monitoring, error logging, reconnection handling, and appropriate access protection for external interfaces.
- **Cross-platform support:** Extend the software to run reliably on both Linux and Windows. Avoid operating-system-specific dependencies where possible, adapt the build and deployment process, and test the application on both platforms.
- **Integration testing and deployment:** Test data exchange with the PLC and any connected customer systems, including communication failures. Verify the application on the target visualization PC, such as an Ubuntu system or windows.
