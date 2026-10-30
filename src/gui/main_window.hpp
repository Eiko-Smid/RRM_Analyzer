
#pragma once

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class MeasurementWidget;


namespace rrm
{
    struct MeasurementAnalysis;
}


class MainWindow : public QMainWindow
{
public:
    /**
     * @brief Creates the main window with file selection, step selection,
     *        analysis controls, statistics, and measurement charts.
     * @param parent Optional parent widget.
     */
    explicit MainWindow(QWidget* parent = nullptr);

private:
     /**
     * @brief Opens a file dialog to select a log file, displays its path, enables analysis,
     * and resets previously displayed results.
     */
    void browseLogFile();

    /**
     * @brief Loads and analyzes the selected measurement step, then displays statistics
     * and time-series charts. Reports success in the status bar and shows an error dialog 
     * if loading or analysis fails.
     */
    void analyzeLogFile();


    /**
     * @brief Displays the measurement duration and the radial force and torque statistics
     * for the completed analysis.
     * @param analysis Computed measurement analysis results to display.
     */
    void displayAnalysis(const rrm::MeasurementAnalysis& analysis);

    /**
     * @brief Resets the displayed duration, statistics, and measurement charts.
     */
    void resetAnalysisDisplay();


    /**
     * @brief Read-only text field displaying the selected log file path.
     */
    QLineEdit* file_path_edit_{nullptr};

    /**
     * @brief Spin box for selecting the measurement step to analyze.
     */
    QSpinBox* step_spinbox_{nullptr};

    /**
     * @brief Button used to start the log file analysis.
     */
    QPushButton* analyze_button_{nullptr};

    /**
     * @brief Label displaying the duration of the selected measurement step.
     */
    QLabel* duration_label_{nullptr};

    /**
     * @brief Widget displaying radial force statistics and the time-series chart.
     */
    MeasurementWidget* radial_force_widget_{nullptr};

    /**
     * @brief Widget displaying torque statistics and the time-series chart.
     */
    MeasurementWidget* torque_widget_{nullptr};
    };
