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
    explicit MeasurementWidget(
        const QString& title,
        const QString& unit,
        QWidget* parent = nullptr
    );

    /**
     * @brief Displays statistical results for one measurement quantity.
     * @param stats Computed mean, minimum and maximum.
     * @param start_time_ms Raw timestamp at the start of the selected step.
     */
    void setStatistics(
        const rrm::QuantityStats& stats,
        std::int64_t start_time_ms
    );

    /**
     * @brief Displays the time-series data.
     * @param points List of (time in minutes, measurement value).
     */
    void setTimeSeries(const QList<QPointF>& points);

    /**
     * @brief Resets the displayed statistics.
     */
    void resetStatistics();

    /**
     * @brief Clears the chart and resets its axes.
     */
    void resetTimeSeries();

protected:
    // Update chart appearance when the Qt theme or palette changes.
    void changeEvent(QEvent* event) override;
    
private:
    // Apply colors from the current Qt palette to the chart.
    void updateChartColors();

    // Draw horizontal statistic lines across the measurement time range.
    void updateStatisticLines();

    // Statistical value labels
    QLabel* mean_value_label_{nullptr};
    QLabel* min_value_label_{nullptr};
    QLabel* max_value_label_{nullptr};

    QLabel* min_time_label_{nullptr};
    QLabel* max_time_label_{nullptr};

    // Time-series chart components
    QChartView* chart_view_{nullptr};
    QLineSeries* series_{nullptr};
    QLineSeries* mean_series_{nullptr};
    QLineSeries* min_series_{nullptr};
    QLineSeries* max_series_{nullptr};
    QValueAxis* time_axis_{nullptr};
    QValueAxis* value_axis_{nullptr};

    // Statistical values for the horizontal reference lines
    double mean_value_{0.0};
    double min_value_{0.0};
    double max_value_{0.0};
    bool has_statistics_{false};

    // Unit of the displayed measurement quantity
    QString unit_;
};
