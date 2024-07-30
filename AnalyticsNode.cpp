#include "AnalyticsNode.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QSysInfo>
#include <cmath>
#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QOperatingSystemVersion>
#include <QStorageInfo>

AnalyticsNode::AnalyticsNode(const QString &serverAddress, quint16 port, QObject *parent)
    : QObject(parent), socket(new QTcpSocket(this)), server(new QTcpServer(this)), worker(new Worker) {
    connect(socket, &QTcpSocket::readyRead, this, &AnalyticsNode::onReadyRead);
    connect(server, &QTcpServer::newConnection, this, &AnalyticsNode::onNewConnection);
    connect(worker, &Worker::dataStored, this, &AnalyticsNode::onWorkerDataStored);
    connect(worker, &Worker::queryProcessed, this, &AnalyticsNode::onWorkerQueryProcessed);
    worker->moveToThread(&workerThread);
    workerThread.start();

    socket->connectToHost(serverAddress, port);
    server->listen(QHostAddress::Any, port);
}

int AnalyticsNode::getNumberOfProcessors() {
    return QThread::idealThreadCount();
}

double AnalyticsNode::getMemoryCapacity() {
    QStorageInfo storage = QStorageInfo::root();
    qint64 bytes = storage.bytesTotal();
    return static_cast<double>(bytes) / (1024 * 1024 * 1024);
}

double AnalyticsNode::sigmoid(double x) {
    return 1 / (1 + exp(-x));
}

double AnalyticsNode::calculateComputingCapacity() {
    int baselineCores = 4;
    double baselineMemory = 8.0;

    int myCores = getNumberOfProcessors();
    double myMemory = getMemoryCapacity();

    double coreCapacity = static_cast<double>(myCores) / baselineCores;
    double memoryCapacity = myMemory / baselineMemory;
    double averageCapacity = (coreCapacity + memoryCapacity) / 2;

    return sigmoid(averageCapacity - 1);
}

void AnalyticsNode::registerNode() {
    double capacity = calculateComputingCapacity();
    qDebug() << "In register node";
    if (!socket->waitForConnected(5000)) {
        qDebug() << "Failed to connect to" << socket->peerAddress().toString() << "on port" << socket->peerPort();
        return;
    }
    if (socket->waitForConnected()) {
        QJsonObject registrationRequest{
            {"requestType", "registering"},
            {"IP", socket->localAddress().toString()},
            {"nodeType", "analytics"},
            {"computingCapacity", capacity}
        };
        QJsonDocument doc(registrationRequest);
        socket->write(doc.toJson());
        qDebug() << "Register Node: Sent registration request from" << doc;
    }
}

void AnalyticsNode::onNewConnection() {
    QTcpSocket *client = server->nextPendingConnection();
    clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &AnalyticsNode::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &AnalyticsNode::onClientDisconnected);
    qDebug() << "New client connected:" << client->peerAddress().toString();
}

void AnalyticsNode::onReadyRead() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    QJsonDocument doc = QJsonDocument::fromJson(client->readAll());
    processMessage(client, doc.object());
}

void AnalyticsNode::onClientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    clients.removeAll(client);
    client->deleteLater();
    qDebug() << "Client disconnected:" << client->peerAddress().toString();
}

void AnalyticsNode::processMessage(QTcpSocket* client, const QJsonObject &message) {
    QString type = message["requestType"].toString();
    if (type == "Node Discovery") {
        qDebug() << "Node Discovery result received:" << message;
    } else if(type == "Leader Announcement"){
        qDebug() << "Leader election result received:" << message;
    }
    else if (type == "Heartbeat") {
        sendHeartBeat(client);
    }
    else if (type == "analytics") {
        int requestID = message["requestID"].toInt();
        QJsonArray dataArray = message["Data"].toArray();
        qDebug() << "Analytics request received:" << message;
        QMetaObject::invokeMethod(worker, "storeData", Q_ARG(QJsonArray, dataArray));
        sendAcknowledgment(requestID);
    }
    else if (type == "query") {
        qDebug() << "Query request received:" << message;
        QMetaObject::invokeMethod(worker, "processQuery", Q_ARG(QJsonObject, message));
    }
    else if (type == "Init Analytics") {
        qDebug() << "Init Analytics received:" << message;
    }
    else {
        qDebug() << "Received message of type:" << type << message;
    }
}

void AnalyticsNode::sendHeartBeat(QTcpSocket *clientSocket) {
    QJsonObject responseObj;
    responseObj["requestType"] = "Heartbeat Response";
    responseObj["message"] = "I am alive";
    responseObj["status"] = "OK";

    QJsonDocument responseDoc(responseObj);
    QByteArray responseData = responseDoc.toJson();

    clientSocket->write(responseData);
}

void AnalyticsNode::sendAcknowledgment(int requestID) {
    QJsonObject ackObj;
    ackObj["requestType"] = "analytics acknowledgment";
    ackObj["requestID"] = QString::number(requestID);
    QJsonDocument doc(ackObj);
    socket->write(doc.toJson());
}

void AnalyticsNode::processQuery(const QJsonObject &message) {
    QMetaObject::invokeMethod(worker, "processQuery", Q_ARG(QJsonObject, message));
}

void AnalyticsNode::onWorkerDataStored() {
    qDebug() << "Worker: Data stored successfully.";
}

void AnalyticsNode::onWorkerQueryProcessed(const QJsonObject &response) {
    QJsonDocument doc(response);
    socket->write(doc.toJson());
    qDebug() << "Register Node: Sent query response:" << doc;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    AnalyticsNode node("192.168.1.102", 12351); // Replace with Register node IP and port
    node.registerNode();
    return app.exec();
}
