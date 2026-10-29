
#include "main_window.hpp"
#include "measurement_widget.hpp"

#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>


// --------------------------------------------------
// Constructor
// --------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("RRM Measurement Analysis");
    resize(1150, 700);

    // Central widget and main layout
    auto* central_widget = new QWidget(this);
    auto* main_layout = new QVBoxLayout(central_widget);

    main_layout->setContentsMargins(16, 16, 16, 16);
    main_layout->setSpacing(14);

    // --------------------------------------------------
    // Window heading
    // --------------------------------------------------
    auto* title_label = new QLabel(
        "RRM Measurement Analysis",
        central_widget
    );

    QFont title_font = title_label->font();
    title_font.setPointSize(title_font.pointSize() + 6);
    title_font.setBold(true);

    title_label->setFont(title_font);
    main_layout->addWidget(title_label);

    // --------------------------------------------------
    // File selection
    // --------------------------------------------------
    auto* file_layout = new QHBoxLayout();

    file_path_edit_ = new QLineEdit(central_widget);
    file_path_edit_->setReadOnly(true);
    file_path_edit_->setPlaceholderText(
        "Select a .log file..."
    );

    auto* browse_button = new QPushButton(
        "Browse...",
        central_widget
    );

    file_layout->addWidget(new QLabel("Log file:", central_widget));
    file_layout->addWidget(file_path_edit_, 1);
    file_layout->addWidget(browse_button);

    main_layout->addLayout(file_layout);

    // --------------------------------------------------
    // Step selection and Analyze button
    // --------------------------------------------------
    auto* control_layout = new QHBoxLayout();

    step_spinbox_ = new QSpinBox(central_widget);
    step_spinbox_->setRange(0, 9999);
    step_spinbox_->setValue(5);

    analyze_button_ = new QPushButton(
        "Analyze",
        central_widget
    );

    // Analysis will be implemented in the next step
    analyze_button_->setEnabled(false);

    control_layout->addWidget(
        new QLabel("Measurement step:", central_widget)
    );

    control_layout->addWidget(step_spinbox_);
    control_layout->addStretch();
    control_layout->addWidget(analyze_button_);

    main_layout->addLayout(control_layout);

    // --------------------------------------------------
    // Measurement duration
    // --------------------------------------------------
    duration_label_ = new QLabel(
        "Warm-up duration: -- min",
        central_widget
    );

    main_layout->addWidget(duration_label_);

    // --------------------------------------------------
    // Measurement panels
    // --------------------------------------------------
    auto* panels_layout = new QHBoxLayout();
    panels_layout->setSpacing(12);

    radial_force_widget_ = new MeasurementWidget(
        "Radial Force",
        "N",
        central_widget
    );

    torque_widget_ = new MeasurementWidget(
        "Torque",
        "Nm",
        central_widget
    );

    panels_layout->addWidget(radial_force_widget_, 1);
    panels_layout->addWidget(torque_widget_, 1);

    main_layout->addLayout(panels_layout, 1);

    // Assign central widget
    setCentralWidget(central_widget);

    // --------------------------------------------------
    // Signal / slot connections
    // --------------------------------------------------
    connect(
        browse_button,
        &QPushButton::clicked,
        this,
        &MainWindow::browseLogFile
    );
}

// --------------------------------------------------
// File selection dialog
// --------------------------------------------------
void MainWindow::browseLogFile()
{
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "Select RRM log file",
        QString(),
        "Log files (*.log);;All files (*)"
    );

    if (!file_path.isEmpty())
    {
        file_path_edit_->setText(file_path);
    }
}
