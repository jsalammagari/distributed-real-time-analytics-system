#include <QCoreApplication>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QThread>

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QTcpSocket socket;
    socket.connectToHost("192.168.1.107", 12351); // Connect to the metadata node server

    if (!socket.waitForConnected(5000)) {
        qDebug() << "Failed to connect.";
        return -1;
    }

    // Prepare ingestion data
    QJsonArray dataArray;
    dataArray.append(QJsonArray({
        "2020-08-10T01:00@1",
        "41.75613",
        "-124.20347",
        "PM2.5",
        "17.3",
        "UG/M3",
        "18.0",
        "62",
        "2",
        "Crescent City",
        "North Coast Unified Air Quality Management District",
        "840060150007",
        "840060150007"
    }));

    dataArray.append(QJsonArray({
        "2020-08-10T01:00@1",
        "41.75613",
        "-124.20347",
        "PM2.5",
        "19.3",
        "UG/M3",
        "18.0",
        "62",
        "2",
        "Crescent City",
        "North Coast Unified Air Quality Management District",
        "840060150007",
        "840060150007"
    }));

    dataArray.append(QJsonArray({
        "2020-08-10T01:00@1",
        "41.75613",
        "-124.20347",
        "PM2.5",
        "20",
        "UG/M3",
        "18.0",
        "62",
        "2",
        "Max-Crescent City",
        "North Coast Unified Air Quality Management District",
        "840060150007",
        "840060150007"
    }));

    QJsonObject ingestionRequest{
        {"requestType", "ingestion"},
        {"data", dataArray}
    };

    // Send ingestion data
    socket.write(QJsonDocument(ingestionRequest).toJson());
    socket.flush();
    qDebug() << "Ingestion request sent.";

    QThread::sleep(3);
    // Prepare a query request
    QJsonArray queryParam = QJsonArray::fromVariantList({0});
    QJsonObject queryRequest{
        {"requestType", "query"},
        {"param", queryParam}
    };

    // Send query request
    socket.write(QJsonDocument(queryRequest).toJson());
    socket.flush();
    qDebug() << "Query request sent.";
    if (!socket.waitForBytesWritten(5000)) {
        qDebug() << "Failed to write data.";
    }

    return a.exec();
}
