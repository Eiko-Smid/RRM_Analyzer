
#pragma once

#include <QGroupBox>
#include <QString>

class QLabel;

class MeasurementWidget : public QGroupBox
{
public:
    explicit MeasurementWidget(
        const QString& title,
        const QString& unit,
        QWidget* parent = nullptr
    );

private:
    QLabel* mean_value_label_{nullptr};
    QLabel* min_value_label_{nullptr};
    QLabel* max_value_label_{nullptr};

    QLabel* min_time_label_{nullptr};
    QLabel* max_time_label_{nullptr};
};
