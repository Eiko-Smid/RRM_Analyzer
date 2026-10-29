
#pragma once

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class MeasurementWidget;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void browseLogFile();

    // Input controls
    QLineEdit* file_path_edit_{nullptr};
    QSpinBox* step_spinbox_{nullptr};
    QPushButton* analyze_button_{nullptr};

    // Results
    QLabel* duration_label_{nullptr};

    MeasurementWidget* radial_force_widget_{nullptr};
    MeasurementWidget* torque_widget_{nullptr};
};
