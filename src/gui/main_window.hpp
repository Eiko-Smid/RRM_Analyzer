
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
    explicit MainWindow(QWidget* parent = nullptr);

private:
    /**
     * @brief Method that reads the log file and checks if it is empty -> status bar msg or not.
     */
    void browseLogFile();

    /**
     * @brief Method that reads the log file measurements and analyzes them. Reports to msg box if successful or not.
     * Catches exception if something went wrong during log file read or analysis. 
     */
    void analyzeLogFile();


    /**
     * @brief Displays the analysis results in the visu.
     * @param Result of the analysis result that needs to be displayed.
     */
    void displayAnalysis(const rrm::MeasurementAnalysis& analysis);

    void resetAnalysisDisplay();

    // Input controls
    QLineEdit* file_path_edit_{nullptr};
    QSpinBox* step_spinbox_{nullptr};
    QPushButton* analyze_button_{nullptr};

    // Results
    QLabel* duration_label_{nullptr};

    MeasurementWidget* radial_force_widget_{nullptr};
    MeasurementWidget* torque_widget_{nullptr};
};
