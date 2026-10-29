
#include "measurement_widget.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>



MeasurementWidget::MeasurementWidget(
    const QString& title,
    const QString& unit,
    QWidget* parent
)
    : QGroupBox(title, parent)
{
    // Define column positions
    const int meas_col_one = 0;
    const int unit_col_one = 2;

    const int meas_col_two = 3;
    const int unit_col_two = 5;

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(12);

    // --------------------------------------------------
    // Define stats layout
    // --------------------------------------------------

    auto* stats_layout = new QGridLayout();

    stats_layout->setHorizontalSpacing(10);
    stats_layout->setVerticalSpacing(8);

    // Create value labels
    mean_value_label_ = new QLabel("-- " + unit, this);
    min_value_label_ = new QLabel("-- " + unit, this);
    max_value_label_ = new QLabel("-- " + unit, this);

    min_time_label_ = new QLabel("-- min", this);
    max_time_label_ = new QLabel("-- min", this);

    // Mean
    stats_layout->addWidget(new QLabel("Mean:", this), 0, meas_col_one);
    stats_layout->addWidget(mean_value_label_, 0, unit_col_one);

    // Minimum
    stats_layout->addWidget(new QLabel("Minimum:", this), 1, meas_col_one);
    stats_layout->addWidget(min_value_label_, 1, unit_col_one);

    // Minimum time
    stats_layout->addWidget(new QLabel("Time:", this), 2, meas_col_one);
    stats_layout->addWidget(min_time_label_, 2, unit_col_one);

    // // Maximum
    stats_layout->addWidget(new QLabel("Maximum:", this), 1, meas_col_two);
    stats_layout->addWidget(max_value_label_, 1, unit_col_two);

    // Maximum time
    stats_layout->addWidget(new QLabel("Time:", this), 2, meas_col_two);
    stats_layout->addWidget(max_time_label_, 2, unit_col_two);

    // Distribute remaining horizontal space
    stats_layout->setColumnStretch(1, 1);
    stats_layout->setColumnStretch(3, 1);

    main_layout->addLayout(stats_layout);

    // --------------------------------------------------
    // Mean
    // --------------------------------------------------
    // auto* mean_layout = new QHBoxLayout();

    // mean_value_label_ = new QLabel("-- " + unit, this);

    // mean_layout->addWidget(new QLabel("Mean:", this));
    // mean_layout->addWidget(mean_value_label_);
    // mean_layout->addStretch();

    // main_layout->addLayout(mean_layout);

    // // --------------------------------------------------
    // // Minimum and maximum
    // // --------------------------------------------------
    // auto* extrema_layout = new QGridLayout();

    // min_value_label_ = new QLabel("-- " + unit, this);
    // max_value_label_ = new QLabel("-- " + unit, this);

    // min_time_label_ = new QLabel("Time: -- min", this);
    // max_time_label_ = new QLabel("Time: -- min", this);

    // extrema_layout->addWidget(new QLabel("Minimum:", this), 0, 0);
    // extrema_layout->addWidget(min_value_label_, 0, 1);

    // extrema_layout->addWidget(new QLabel("Maximum:", this), 0, 2);
    // extrema_layout->addWidget(max_value_label_, 0, 3);

    // extrema_layout->addWidget(min_time_label_, 1, 0);
    // extrema_layout->addWidget(max_time_label_, 1, 2);

    // extrema_layout->setColumnStretch(1, 1);
    // extrema_layout->setColumnStretch(3, 1);

    // main_layout->addLayout(extrema_layout);

    // --------------------------------------------------
    // Placeholder for the future time-series chart
    // --------------------------------------------------
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
