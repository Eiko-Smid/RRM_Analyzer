
#include "main_window.hpp"
#include "measurement_widget.hpp"

#include "rrm/rrm_log_parser.hpp"
#include "rrm/measurement_analyzer.hpp"

#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QStatusBar>

#include <exception>



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
    // Define widget for defining the step to read and button to start analysis of loaded data
    // --------------------------------------------------
    auto* control_layout = new QHBoxLayout();

    //Define widget for setting step 
    step_spinbox_ = new QSpinBox(central_widget);
    step_spinbox_->setRange(0, 9999);
    step_spinbox_->setValue(5);

    // Define analyze button
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
    // Define widget for displaying the duration
    // --------------------------------------------------
    duration_label_ = new QLabel(
        "Duration: -- min",
        central_widget
    );

    main_layout->addWidget(duration_label_);

    // --------------------------------------------------
    // Measurement panels
    // --------------------------------------------------
    auto* panels_layout = new QHBoxLayout();
    panels_layout->setSpacing(12);

    // Define measurement widget that displays radial force metrics and graph.
    radial_force_widget_ = new MeasurementWidget(
        "Radial Force",
        "N",
        central_widget
    );

    // Define measurement widget that displays torque metrics and graph.
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
    
    // Define connection for file browsing.
    connect(
        browse_button,
        &QPushButton::clicked,
        this,
        &MainWindow::browseLogFile
    );

    // Define connection for analyze button.
    connect(
        analyze_button_,
        &QPushButton::clicked,
        this, 
        &MainWindow::analyzeLogFile
    );
}

// --------------------------------------------------
// File selection dialog
// --------------------------------------------------
void MainWindow::browseLogFile()
{
    // Stores filename.
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "Select RRM log file",
        QString(),
        "Log files (*.log);;All files (*)"
    );

    // Check if file exists -> enables analyze button.
    if (!file_path.isEmpty())
    {
        file_path_edit_->setText(file_path);
        analyze_button_->setEnabled(true);

        statusBar()->showMessage("Ready to analyze.");
    }
}


void MainWindow::analyzeLogFile()
{
    // Get analysis config from gui.
    const std::string file_path = file_path_edit_->text().toStdString();
    const int step = step_spinbox_->value();

    // Output msg if file is empty.
    if (file_path.empty())
    {
        QMessageBox::warning(
            this,
            "No log file",
            "Please select a log file first."
        );

        return;
    }

    // Disable analyze button during analysis.
    analyze_button_->setEnabled(false);
    statusBar()->showMessage("Analyzing measurements...");

    try
    {
        // Init objects.
        rrm::RrmLogParser parser;
        const rrm::MeasurementAnalyzer analyzer;

        // Read data.
        parser.load(file_path, step);

        // Analyze loaded data
        const rrm::MeasurementAnalysis analysis = analyzer.analyzeMeasurements(
            parser.getData(),
            step
        );

        // Report successful analysis.
        statusBar()->showMessage(
            QString("Analysis completed. Step %1, samples: %2")
            .arg(analysis.step)
            .arg(static_cast<qulonglong>(analysis.sample_count))
        );
    }
    catch (const std::exception& e)
    {
        // Report that analysis failed
        statusBar()->showMessage("Analysis failed.");

        QMessageBox::critical(
            this,
            "Analysis failed",
            QString::fromUtf8(e.what())
        );
    }

    // Enable analysis button again for next analysis
    analyze_button_->setEnabled(true);

}
