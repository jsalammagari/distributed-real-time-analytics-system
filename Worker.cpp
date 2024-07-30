#include "Worker.h"
#include <QDebug>

Worker::Worker(QObject *parent) : QObject(parent) {}

void Worker::storeData(const QJsonArray &dataArray) {
    for (const QJsonValue &value : dataArray) {
        aqiData.append(value.toArray());
    }
    qDebug() << "Data stored successfully in worker. Current data count:" << aqiData.size();
    emit dataStored();
}

void Worker::processQuery(const QJsonObject &message) {
    int requestId = message["requestID"].toInt();
    double maxAqi = 0;
    double totalAqi = 0;
    QString maxArea;
    int count = 0;

    for (const QJsonArray &entry : aqiData) {
        if (entry.size() >= 5) {
            double aqi = entry[9].toString().toDouble();
            QString area = entry[10].toString();
            totalAqi += aqi;
            count++;

            if (aqi > maxAqi) {
                maxAqi = aqi;
                maxArea = area;
            }
        }
    }

    double averageAqi = (count > 0) ? totalAqi / count : 0;
    qDebug() << "Worker: Max Area:" << maxArea << ", Max AQI:" << maxAqi << ", Average AQI:" << averageAqi;

    QJsonObject response{
        {"requestType", "query response"},
        {"requestID", requestId},
        {"maxArea", maxArea},
        {"maxAverage", averageAqi}
    };
    emit queryProcessed(response);
}
