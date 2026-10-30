#pragma once

#include <QGroupBox>
#include <QList>
#include <QPointF>
#include <QString>

#include <cstdint>

namespace rrm
{
    struct QuantityStats;
}

class QEvent;
class QLabel;
class QChartView;
class QLineSeries;
class QValueAxis;

class MeasurementWidget : public QGroupBox
{
public:
    /**
     * @brief Creates a measurement panel with grouped minimum, maximum, and mean statistics
     * and a zoomable time-series chart.
     * @param title Title of the measurement panel.
     * @param unit Unit displayed with the measurement values.
     * @param parent Optional parent widget.
     */
    explicit MeasurementWidget(
        const QString& title,
        const QString& unit,
        QWidget* parent = nullptr
    );

    /**
     * @brief Displays the mean, minimum, and maximum values and the timestamps of the extrema.
     * Times are converted from raw milliseconds to total elapsed minutes without subtracting 
     * the step start time.
     * @param stats Computed statistics and timestamps for the measurement.
     * @param start_time_ms Step start timestamp; currently unused.
     */
    void setStatistics(
        const rrm::QuantityStats& stats,
        std::int64_t start_time_ms
    );

    /**
     * @brief Displays measurement samples in the chart and adjusts its axes and horizontal
     * statistic reference lines to the data range.
     * @param points Samples containing total time in minutes on the x-axis and the measured
     * quantity on the y-axis.
     */
    void setTimeSeries(const QList<QPointF>& points);

     /**
     * @brief Resets the displayed mean, extrema, and extremum times and clears the horizontal
     * statistic reference lines.
     */
    void resetStatistics();

     /**
     * @brief Clears all chart series, resets the zoom, and restores the default axis ranges.
     */
    void resetTimeSeries();

protected:
    /**
     * @brief Updates chart colors when the Qt palette or style changes.
     * @param event Event delivered to the measurement widget.
     */
    void changeEvent(QEvent* event) override;
    
private:
     /**
     * @brief Applies theme-aware colors to the chart, axes, reference lines, and displayed
     * statistic values.
     */
    void updateChartColors();

     /**
     * @brief Updates horizontal mean, minimum, and maximum reference lines across the current
     * time-axis range when data is available.
     */
    void updateStatisticLines();

    // ------------------------------------------------------------
    // Member variables
    // ------------------------------------------------------------

    /**
     * @brief Label displaying the mean measurement value.
     */
    QLabel* mean_value_label_{nullptr};

    /**
     * @brief Label displaying the minimum measurement value.
     */
    QLabel* min_value_label_{nullptr};

    /**
     * @brief Label displaying the maximum measurement value.
     */
    QLabel* max_value_label_{nullptr};

    /**
     * @brief Label displaying the timestamp of the minimum value in minutes.
     */
    QLabel* min_time_label_{nullptr};

    /**
     * @brief Label displaying the timestamp of the maximum value in minutes.
     */
    QLabel* max_time_label_{nullptr};

    /**
     * @brief Chart widget displaying the measurement time series with zoom support.
     */
    QChartView* chart_view_{nullptr};

    /**
     * @brief Line series containing the measurement samples.
     */
    QLineSeries* series_{nullptr};

    /**
     * @brief Horizontal reference line representing the mean value.
     */
    QLineSeries* mean_series_{nullptr};

    /**
     * @brief Horizontal reference line representing the minimum value.
     */
    QLineSeries* min_series_{nullptr};

    /**
     * @brief Horizontal reference line representing the maximum value.
     */
    QLineSeries* max_series_{nullptr};

    /**
     * @brief Horizontal chart axis representing total measurement time in minutes.
     */
    QValueAxis* time_axis_{nullptr};

    /**
     * @brief Vertical chart axis representing the measured quantity.
     */
    QValueAxis* value_axis_{nullptr};

    /**
     * @brief Stored mean value used to update the horizontal reference line.
     */
    double mean_value_{0.0};

    /**
     * @brief Stored minimum value used to update the horizontal reference line.
     */
    double min_value_{0.0};

    /**
     * @brief Stored maximum value used to update the horizontal reference line.
     */
    double max_value_{0.0};

    /**
     * @brief Indicates whether valid statistics are available for displaying
     *        the horizontal reference lines.
     */
    bool has_statistics_{false};

    /**
     * @brief Physical unit of the measured quantity (e.g., N or Nm).
     */
    QString unit_;
};
