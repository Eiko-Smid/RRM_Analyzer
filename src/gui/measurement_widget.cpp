
#include "measurement_widget.hpp"
#include "rrm/analysis_result.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

// -----------------------------------------------------------------------------------
// Define constants
// -----------------------------------------------------------------------------------
const double MS_TO_MIN = 1 / 60000.0;
const QString DEFAULT_VALUE_TEXT = "-- ";
const QString DEFAULT_TIME_TEXT = "-- min";


// ------------------------------------------------------------
// Implement class MeasurementWidget
// ------------------------------------------------------------
MeasurementWidget::MeasurementWidget(
    const QString& title,
    const QString& unit,
    QWidget* parent
)
    : QGroupBox(title, parent), unit_(unit)

    
{
    // ------------------------------------------------------------
    // LAYOUT CONFIGURATION
    // ------------------------------------------------------------

    // A: Gap between labels and their values/units
    constexpr int label_value_gap_px = 40;

    // B: Gap between left and right statistics blocks
    constexpr int block_gap_px = 180;

    // Vertical distance between statistics rows
    constexpr int row_gap_px = 8;

    // Define main layout
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(12);

    // ------------------------------------------------------------
    // CREATE VALUE LABELS
    // ------------------------------------------------------------
    mean_value_label_ = new QLabel(DEFAULT_VALUE_TEXT + unit, this);
    min_value_label_ = new QLabel(DEFAULT_VALUE_TEXT + unit, this);
    max_value_label_ = new QLabel(DEFAULT_VALUE_TEXT + unit, this);

    min_time_label_ = new QLabel(DEFAULT_TIME_TEXT, this);
    max_time_label_ = new QLabel(DEFAULT_TIME_TEXT, this);

    // ------------------------------------------------------------
    // Define LEFT STATISTICS BLOCK
    // ------------------------------------------------------------
    auto* left_stats = new QGridLayout();

    left_stats->setHorizontalSpacing(label_value_gap_px);
    left_stats->setVerticalSpacing(row_gap_px);

    // Mean
    left_stats->addWidget(new QLabel("Mean:", this), 0, 0);
    left_stats->addWidget(mean_value_label_, 0, 1);

    // Minimum
    left_stats->addWidget(new QLabel("Minimum:", this), 1, 0);
    left_stats->addWidget(min_value_label_, 1, 1);

    // Minimum time
    left_stats->addWidget(new QLabel("Time:", this), 2, 0);
    left_stats->addWidget(min_time_label_, 2, 1);

    // ------------------------------------------------------------
    // Define RIGHT STATISTICS BLOCK
    // ------------------------------------------------------------
    auto* right_stats = new QGridLayout();

    right_stats->setHorizontalSpacing(label_value_gap_px);
    right_stats->setVerticalSpacing(row_gap_px);

    // Empty first row to align Maximum with Minimum
    right_stats->addWidget(new QLabel(" ", this), 0, 0);

    // Maximum
    right_stats->addWidget(new QLabel("Maximum:", this), 1, 0);
    right_stats->addWidget(max_value_label_, 1, 1);

    // Maximum time
    right_stats->addWidget(new QLabel("Time:", this), 2, 0);
    right_stats->addWidget(max_time_label_, 2, 1);

    // ------------------------------------------------------------
    // COMBINE STATISTICS BLOCKS
    // ------------------------------------------------------------
    auto* stats_layout = new QHBoxLayout();

    stats_layout->setSpacing(0);

    // Left block
    stats_layout->addLayout(left_stats);

    // Fixed distance between both blocks
    stats_layout->addSpacing(block_gap_px);

    // Right block
    stats_layout->addLayout(right_stats);

    // Remaining empty space AFTER both blocks
    stats_layout->addStretch(1);

    main_layout->addLayout(stats_layout);

    // ------------------------------------------------------------
    // TIME-SERIES CHART PLACEHOLDER
    // ------------------------------------------------------------
    auto* chart_placeholder = new QFrame(this);

    chart_placeholder->setFrameShape(QFrame::StyledPanel);
    chart_placeholder->setMinimumHeight(280);

    auto* chart_layout = new QVBoxLayout(chart_placeholder);

    auto* chart_label = new QLabel(
        "Time-series chart",
        chart_placeholder
    );

    chart_label->setAlignment(Qt::AlignCenter);

    chart_layout->addWidget(chart_label);

    main_layout->addWidget(chart_placeholder, 1);
}


void MeasurementWidget::setStatistics(
    const rrm::QuantityStats& stats,
    std::int64_t start_time_ms
)
{
    // Convert tmin and max time to min
    const double min_time_min = static_cast<double>(stats.min.time_ms) * MS_TO_MIN;
    const double max_time_min = static_cast<double>(stats.max.time_ms) * MS_TO_MIN;

    // Display values 
    // Display mean, min, max
    mean_value_label_->setText(
        QString("%1 %2").arg(stats.mean, 0, 'f', 2).arg(unit_)
    );

    min_value_label_->setText(
        QString("%1 %2").arg(stats.min.value, 0, 'f', 2).arg(unit_)
    );

    max_value_label_->setText(
        QString("%1 %2").arg(stats.max.value, 0, 'f', 2).arg(unit_)
    );

    // Display time
    min_time_label_->setText(
        QString("%1 min").arg(min_time_min, 0, 'f', 2)
    );

    max_time_label_->setText(
        QString("%1 min").arg(max_time_min, 0, 'f', 2)
    );
}



void MeasurementWidget::resetStatistics()
{
    // Reset statistic of widget
    mean_value_label_->setText(DEFAULT_VALUE_TEXT + unit_);
    min_value_label_->setText(DEFAULT_VALUE_TEXT + unit_);
    max_value_label_->setText(DEFAULT_VALUE_TEXT + unit_);

    min_time_label_->setText(DEFAULT_TIME_TEXT);
    max_time_label_->setText(DEFAULT_TIME_TEXT);
}
