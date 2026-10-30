
#include "measurement_widget.hpp"
#include "rrm/analysis_result.hpp"

#include <QChart>
#include <QChartView>
#include <QLegend>
#include <QLineSeries>
#include <QValueAxis>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QVBoxLayout>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QPalette>
#include <QGroupBox>

#include <algorithm>
#include <cmath>

// -----------------------------------------------------------------------------------
// Define constants
// -----------------------------------------------------------------------------------
const double MS_TO_MIN = 1 / 60000.0;
const QString DEFAULT_VALUE_TEXT = "-- ";
const QString DEFAULT_TIME_TEXT = "-- min";
const double REFERENCE_LINE_WIDTH = 1.5;


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

    // Set bold title for the measurement panel
    setObjectName("measurementPanel");
    setStyleSheet(
        "QGroupBox#measurementPanel { font-weight: bold; }"
    );

    // A: Gap between labels and their values/units
    constexpr int label_value_gap_px = 30;

    // B: Gap between Maximum and Mean boxes
    constexpr int max_mean_gap_px = 30;

    // C: Gap between Minimum and Maximum boxes
    constexpr int min_max_gap_px = 40;

    // D: Vertical distance between statistics rows
    constexpr int row_gap_px = 8;

    // E: Horizontal offset of the statistics section
    constexpr int stats_left_offset_px = 80;

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
    // DEFINE MEAN STATISTICS GROUP
    // ------------------------------------------------------------
    auto* mean_group = new QGroupBox("Mean", this);
    auto* mean_layout = new QGridLayout(mean_group);

    mean_layout->setHorizontalSpacing(label_value_gap_px);

    // Mean value
    mean_layout->addWidget(new QLabel("Value:", mean_group), 0, 0);
    mean_layout->addWidget(mean_value_label_, 0, 1);

    // ------------------------------------------------------------
    // DEFINE MINIMUM STATISTICS GROUP
    // ------------------------------------------------------------
    auto* min_group = new QGroupBox("Minimum", this);
    auto* min_layout = new QGridLayout(min_group);

    min_layout->setHorizontalSpacing(label_value_gap_px);
    min_layout->setVerticalSpacing(row_gap_px);

    // Minimum value
    min_layout->addWidget(new QLabel("Value:", min_group), 0, 0);
    min_layout->addWidget(min_value_label_, 0, 1);

    // Minimum time
    min_layout->addWidget(new QLabel("Time:", min_group), 1, 0);
    min_layout->addWidget(min_time_label_, 1, 1);


    // ------------------------------------------------------------
    // DEFINE MAXIMUM STATISTICS GROUP
    // ------------------------------------------------------------
    auto* max_group = new QGroupBox("Maximum", this);
    auto* max_layout = new QGridLayout(max_group);

    max_layout->setHorizontalSpacing(label_value_gap_px);
    max_layout->setVerticalSpacing(row_gap_px);

    // Maximum value
    max_layout->addWidget(new QLabel("Value:", max_group), 0, 0);
    max_layout->addWidget(max_value_label_, 0, 1);

    // Maximum time
    max_layout->addWidget(new QLabel("Time:", max_group), 1, 0);
    max_layout->addWidget(max_time_label_, 1, 1);


    // ------------------------------------------------------------
    // COMBINE STATISTICS GROUPS
    // ------------------------------------------------------------
    auto* stats_layout = new QHBoxLayout();

    stats_layout->setSpacing(0);

    // Horizontal offset of the entire statistics section
    stats_layout->addSpacing(stats_left_offset_px);

    // Minimum group
    stats_layout->addWidget(min_group, 0, Qt::AlignTop);

    // Gap between Minimum and Maximum
    stats_layout->addSpacing(min_max_gap_px);

    // Maximum group
    stats_layout->addWidget(max_group, 0, Qt::AlignTop);

    // Gap between Maximum and Mean
    stats_layout->addSpacing(max_mean_gap_px);

    // Mean group
    stats_layout->addWidget(mean_group, 0, Qt::AlignTop);

    // Keep all groups aligned to the left
    stats_layout->addStretch(1);

    main_layout->addLayout(stats_layout);


    // ------------------------------------------------------------
    // TIME-SERIES CHART
    // ------------------------------------------------------------

    // Define a line series for the measured values.
    series_ = new QLineSeries();

    // Define horizontal reference series for the computed metrics.
    mean_series_ = new QLineSeries();
    min_series_ = new QLineSeries();
    max_series_ = new QLineSeries();

    // Create a chart and transfer ownership of the series to it.
    auto* chart = new QChart();
    chart->addSeries(series_);
    chart->addSeries(mean_series_);
    chart->addSeries(min_series_);
    chart->addSeries(max_series_);

    chart->legend()->hide();
    chart->setBackgroundVisible(false);

    // Set the line width while keeping Qt's default series color.
    QPen series_pen = series_->pen();
    series_pen.setWidthF(1.5);
    series_->setPen(series_pen);

    // ------------------------------------------------------------
    // DEFINE TIME AXIS
    // ------------------------------------------------------------
    time_axis_ = new QValueAxis();
    time_axis_->setTitleText("Total time [min]");
    time_axis_->setLabelFormat("%.2f");
    time_axis_->setTickCount(6);
    time_axis_->setRange(0.0, 1.0);

    chart->addAxis(time_axis_, Qt::AlignBottom);
    series_->attachAxis(time_axis_);
    mean_series_->attachAxis(time_axis_);
    min_series_->attachAxis(time_axis_);
    max_series_->attachAxis(time_axis_);

    // ------------------------------------------------------------
    // DEFINE MEASUREMENT VALUE AXIS
    // ------------------------------------------------------------
    value_axis_ = new QValueAxis();
    value_axis_->setTitleText(QString("%1 [%2]").arg(title).arg(unit_));
    value_axis_->setLabelFormat("%.2f");
    value_axis_->setTickCount(6);
    value_axis_->setRange(0.0, 1.0);

    chart->addAxis(value_axis_, Qt::AlignLeft);
    series_->attachAxis(value_axis_);
    mean_series_->attachAxis(value_axis_);
    min_series_->attachAxis(value_axis_);
    max_series_->attachAxis(value_axis_);

    // ------------------------------------------------------------
    // CREATE CHART VIEW
    // ------------------------------------------------------------
    chart_view_ = new QChartView(chart, this);
    chart_view_->setMinimumHeight(280);
    chart_view_->setRenderHint(QPainter::Antialiasing);

    // Draw a rectangle with the mouse to zoom into the chart.
    chart_view_->setRubberBand(QChartView::RectangleRubberBand);

    main_layout->addWidget(chart_view_, 1);

    // Add a button to restore the full chart after zooming.
    auto* chart_controls = new QHBoxLayout();
    chart_controls->addStretch();

    auto* reset_zoom_button = new QPushButton("Reset zoom", this);
    chart_controls->addWidget(reset_zoom_button);
    main_layout->addLayout(chart_controls);

    connect(
        reset_zoom_button,
        &QPushButton::clicked,
        this,
        [this]()
        {
            chart_view_->chart()->zoomReset();
        }
    );

    // Apply chart colors according to the current Qt palette.
    updateChartColors();
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

    // Save the metrics and update their horizontal lines.
    mean_value_ = stats.mean;
    min_value_ = stats.min.value;
    max_value_ = stats.max.value;
    has_statistics_ = true;
    updateStatisticLines();
}


void MeasurementWidget::resetStatistics()
{
    // Reset statistic of widget
    mean_value_label_->setText(DEFAULT_VALUE_TEXT + unit_);
    min_value_label_->setText(DEFAULT_VALUE_TEXT + unit_);
    max_value_label_->setText(DEFAULT_VALUE_TEXT + unit_);

    min_time_label_->setText(DEFAULT_TIME_TEXT);
    max_time_label_->setText(DEFAULT_TIME_TEXT);

    // Clear the reference lines when statistics are reset.
    has_statistics_ = false;
    mean_series_->clear();
    min_series_->clear();
    max_series_->clear();
}


void MeasurementWidget::setTimeSeries(const QList<QPointF>& points)
{
    // Handle empty datasets by clearing the current chart.
    if (points.isEmpty())
    {
        resetTimeSeries();
        return;
    }

    // Restore the unzoomed axes before loading different measurements.
    chart_view_->chart()->zoomReset();

    // Replace all points at once rather than appending them individually.
    series_->replace(points);

    // Find the actual overall time range and value range in the data.
    double min_x = points.first().x();
    double max_x = min_x;
    double min_y = points.first().y();
    double max_y = min_y;

    for (const QPointF& point : points)
    {
        min_x = std::min(min_x, point.x());
        max_x = std::max(max_x, point.x());
        min_y = std::min(min_y, point.y());
        max_y = std::max(max_y, point.y());
    }

    // Use the original total measurement times on the X-axis.
    // For a single timestamp, add a small range to keep the axis valid.
    if (min_x == max_x)
    {
        time_axis_->setRange(min_x - 0.1, max_x + 0.1);
    }
    else
    {
        time_axis_->setRange(min_x, max_x);
    }

    // Add 5% padding so the curve does not touch the Y-axis limits.
    const double y_range = max_y - min_y;
    const double y_padding = (y_range > 0.0)
        ? y_range * 0.05
        : std::max(0.1, std::abs(min_y) * 0.05);

    value_axis_->setRange(min_y - y_padding, max_y + y_padding);

    // Extend metric lines across the newly loaded time range.
    updateStatisticLines();
}


void MeasurementWidget::resetTimeSeries()
{
    // Remove the zoom before returning the chart to its default state.
    chart_view_->chart()->zoomReset();

    series_->clear();
    mean_series_->clear();
    min_series_->clear();
    max_series_->clear();

    // Restore the axes until new data is loaded.
    time_axis_->setRange(0.0, 1.0);
    value_axis_->setRange(0.0, 1.0);
}


void MeasurementWidget::updateStatisticLines()
{
    // Wait until statistics and measurement points are available.
    if (!has_statistics_ || series_->count() == 0)
        return;

    const double min_time = time_axis_->min();
    const double max_time = time_axis_->max();

    // Two points define each horizontal line.
    mean_series_->replace(QList<QPointF>{
        QPointF(min_time, mean_value_),
        QPointF(max_time, mean_value_)
    });

    min_series_->replace(QList<QPointF>{
        QPointF(min_time, min_value_),
        QPointF(max_time, min_value_)
    });

    max_series_->replace(QList<QPointF>{
        QPointF(min_time, max_value_),
        QPointF(max_time, max_value_)
    });
}


void MeasurementWidget::changeEvent(QEvent* event)
{
    // Let QGroupBox process the event normally.
    QGroupBox::changeEvent(event);

    // Reapply chart colors if the active Qt palette or style changes.
    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ApplicationPaletteChange ||
        event->type() == QEvent::StyleChange)
    {
        updateChartColors();
    }
}


void MeasurementWidget::updateChartColors()
{
    // Ensure all chart components have been initialized.
    if (!chart_view_ || !series_ || !mean_series_ || !min_series_ ||
        !max_series_ || !time_axis_ || !value_axis_)
        return;

    // Retrieve colors from the current Qt widget palette.
    const QPalette& current_palette = palette();

    const QColor background_color =
        current_palette.color(QPalette::Window);

    const QColor text_color =
        current_palette.color(QPalette::WindowText);

    // Determine whether the current palette is dark or light.
    const bool dark_mode = background_color.lightness() < 128;

    // Select a blue curve color with good contrast against
    // the current background.
    const QColor curve_color = dark_mode
        ? QColor("#38BDF8")
        : QColor("#0369A1");

    // Use distinct, theme-aware colors for mean, minimum and maximum.
    const QColor mean_color = dark_mode
        ? QColor("#FBBF24")
        : QColor("#A16207");

    const QColor min_color = dark_mode
        ? QColor("#4ADE80")
        : QColor("#15803D");

    const QColor max_color = dark_mode
        ? QColor("#FB7185")
        : QColor("#BE123C");

    // Use less prominent colors for grid lines and axis lines.
    QColor grid_color = text_color;
    grid_color.setAlpha(55);

    QColor axis_color = text_color;
    axis_color.setAlpha(140);

    // Match the chart background to the surrounding widget.
    chart_view_->setBackgroundBrush(QBrush(background_color));

    // ------------------------------------------------------------
    // Apply colors to both axes
    // ------------------------------------------------------------
    
    // Define identical fonts for both axes.
    QFont labels_font = font();
    labels_font.setPointSize(9);
    labels_font.setBold(false);

    QFont title_font = font();
    title_font.setPointSize(10);
    title_font.setBold(false);

    const auto update_axis_colors =
    [&](QValueAxis* axis)
    {
        // Configure the numerical tick labels.
        axis->setLabelsFont(labels_font);
        axis->setLabelsColor(text_color);

        // Configure the axis title.
        axis->setTitleFont(title_font);
        axis->setTitleBrush(QBrush(text_color));

        // Color of the grid lines.
        axis->setGridLinePen(QPen(grid_color, 1.0));

        // Color of the main axis line and tick marks.
        axis->setLinePen(QPen(axis_color, 1.0));
    };

    update_axis_colors(time_axis_);
    update_axis_colors(value_axis_);

    // ------------------------------------------------------------
    // Apply color to the measurement curve
    // ------------------------------------------------------------
    QPen series_pen = series_->pen();

    series_pen.setColor(curve_color);
    series_pen.setWidthF(1.5);

    series_->setPen(series_pen);

    // Draw statistic lines dashed to distinguish them from measurements.
    const auto set_reference_pen = [](QLineSeries* line, const QColor& color)
    {
        QPen reference_pen(color);
        reference_pen.setWidthF(REFERENCE_LINE_WIDTH);
        reference_pen.setStyle(Qt::DashLine);
        line->setPen(reference_pen);
    };

    set_reference_pen(mean_series_, mean_color);
    set_reference_pen(min_series_, min_color);
    set_reference_pen(max_series_, max_color);

    // Match displayed metric values to the chart line colors.
    const auto set_value_label_color = [](QLabel* label, const QColor& color)
    {
        QPalette label_palette = label->palette();
        label_palette.setColor(QPalette::WindowText, color);
        label->setPalette(label_palette);
    };

    set_value_label_color(mean_value_label_, mean_color);
    set_value_label_color(min_value_label_, min_color);
    set_value_label_color(max_value_label_, max_color);
}
