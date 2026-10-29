
#pragma once

#include <QGroupBox>
#include <QString>

#include <cstdint>


namespace rrm
{
    struct QuantityStats;
}

class QLabel;

class MeasurementWidget : public QGroupBox
{
public:
    explicit MeasurementWidget(
        const QString& title,
        const QString& unit,
        QWidget* parent = nullptr
    );

    void setStatistics(
        const rrm::QuantityStats& stats,
        std::int64_t start_time_ms
    );

    void resetStatistics();

private:
    QLabel* mean_value_label_{nullptr};
    QLabel* min_value_label_{nullptr};
    QLabel* max_value_label_{nullptr};

    QLabel* min_time_label_{nullptr};
    QLabel* max_time_label_{nullptr};

    QString unit_;
};
