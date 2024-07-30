#ifndef ANALYTICSNODE_H
#define ANALYTICSNODE_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include "Worker.h"

class AnalyticsNode : public QObject {
    Q_OBJECT

public:
    explicit AnalyticsNode(const QString &serverAddress, quint16 port, QObject *parent = nullptr);
    void registerNode();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void processMessage(QTcpSocket* client, const QJsonObject &message);
    void sendHeartBeat(QTcpSocket *clientSocket);
    void sendAcknowledgment(int requestID);
    void processQuery(const QJsonObject &message);
    void onWorkerDataStored();
    void onWorkerQueryProcessed(const QJsonObject &response);

private:
    QTcpSocket *socket;
    QTcpServer *server;
    QList<QTcpSocket *> clients;
    QThread workerThread;
    Worker *worker;

    static int getNumberOfProcessors();
    static double getMemoryCapacity();
    static double sigmoid(double x);
    static double calculateComputingCapacity();
};

#endif
