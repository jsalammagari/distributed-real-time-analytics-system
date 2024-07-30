#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class Worker : public QObject {
    Q_OBJECT

public:
    explicit Worker(QObject *parent = nullptr);

signals:
    void dataStored();
    void queryProcessed(const QJsonObject &response);

public slots:
    void storeData(const QJsonArray &dataArray);
    void processQuery(const QJsonObject &message);

private:
    QList<QJsonArray> aqiData;
};

#endif
