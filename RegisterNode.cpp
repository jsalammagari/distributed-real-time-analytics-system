#include "RegisterNode.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QJsonArray>
#include <QNetworkInterface>

RegisterNode::RegisterNode(quint16 port, QObject *parent)
    : QObject(parent), server(new QTcpServer(this)), myId(QDateTime::currentMSecsSinceEpoch() % 1000), localIP(getLocalIPAddress())
{
    connect(server, &QTcpServer::newConnection, this, &RegisterNode::onNewConnection);
    server->listen(QHostAddress::Any, port);
}

RegisterNode::~RegisterNode() {
    server->close();
    qDebug() << "[RegisterNode] Server shut down.";
}

QString extractIPv4Address(const QString& ipAddress) {
    if (ipAddress.startsWith("::ffff:")) {
        return ipAddress.mid(7);
    }
    return ipAddress;
}

QString RegisterNode::getLocalIPAddress() const {
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    for (int i = 0; i < list.count(); i++) {
        if (!list[i].isLoopback() && list[i].protocol() == QAbstractSocket::IPv4Protocol) {
            return list[i].toString();
        }
    }
    return "127.0.0.1";  // Default to localhost if no suitable address is found
}

QList<QJsonObject> RegisterNode::getRegisterNodes() {
    QList<QJsonObject> RegisterNodes;
    for (const QJsonObject &node : nodeList) {
        if (node["nodeType"].toString() == "metadata Analytics") {
            RegisterNodes.append(node);
        }
    }
    return RegisterNodes;
}

void RegisterNode::onNewConnection() {
    QTcpSocket *client = server->nextPendingConnection();
    clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &RegisterNode::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &RegisterNode::onClientDisconnected);
    qDebug() << "Metadata Node: New connection " << client;
}

void RegisterNode::onReadyRead() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    QJsonDocument doc = QJsonDocument::fromJson(client->readAll());
    processMessage(client, doc.object());
}

void RegisterNode::onClientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    QString clientIp = extractIPv4Address(client->peerAddress().toString());
    clients.removeAll(client);
    client->deleteLater();
    for (auto it = nodeList.begin(); it != nodeList.end(); ++it) {
        if ((*it)["IP"].toString() == clientIp) {
            qDebug() << "Removing node from nodeList: " << clientIp;
            nodeList.erase(it);
            break;
        }
    }
    broadcastNodeList();
}
void RegisterNode::sendAnalyticsRequest(const QJsonArray& data) {
    for (const QJsonObject &node : nodeList) {
        if (node["nodeType"].toString() == "metadata Analytics") {
            QString analyticsNodeIp = node["IP"].toString();
            QJsonObject requestObj;
            requestObj["requestType"] = "analytics";
            requestObj["requestID"] = 123;
            requestObj["data"] = data;


            QJsonDocument doc(requestObj);
            sendMessageToNode(analyticsNodeIp, doc);
            qDebug() << "Analytics request sent: -> " << analyticsNodeIp;
        }
    }
}

void RegisterNode::processQueryRequest(const QJsonObject &message) {
    int requestId = 123;
    int queryType = message["param"].toInt(); // Assuming 0 or 1 indicates different types of queries

    QJsonObject queryRequest{
        {"requestType", "query"},
        {"requestId", requestId},
        {"query", queryType}
    };
    QJsonDocument doc(queryRequest);
    forwardQueryToAnalyticsNode(doc);
}

void RegisterNode::forwardQueryToAnalyticsNode(const QJsonDocument &doc) {
    for (const QJsonObject &node : nodeList) {
        if (node["nodeType"].toString() == "metadata Analytics") {
            QString analyticsNodeIp = node["Ip"].toString();
            sendMessageToNode(analyticsNodeIp, doc);
            qDebug() << "Send query to analytics node:" << analyticsNodeIp;
        }
    }
}

void RegisterNode::sendMessageToNode(const QString &ip, const QJsonDocument &doc) {
    QTcpSocket *analyticsClient = new QTcpSocket(this);
    analyticsClient->connectToHost(ip, 12351);
    analyticsClient->write(doc.toJson());
    qDebug() << "No client found with IP:" << ip;
}

void RegisterNode::processMessage(QTcpSocket* client, const QJsonObject &message) {
    QString type = message["requestType"].toString();

    if (type == "registering") {
        qDebug() << "Register Node: Registration request from" << message["IP"].toString();
        updateNodeList(message);
        broadcastNodeList();
    }
    else if (type == "ingestion") {
        QJsonArray dataArray = message["data"].toArray();
        qDebug() << "Register Node: Ingestion request" ;
        sendAnalyticsRequest(dataArray);
    } else if (type == "query") {
        qDebug() << "Register Node: Query request from" << message["IP"].toString();
        processQueryRequest(message);
    }
    else if (type == "Leader Announcement") {
        leaderIP = message["leaderIP"].toString();
        qDebug() << "New leader elected:" << leaderIP;
    }
    else {
        qDebug() << "Received unrecognized message type:" << type << message;
    }
}



QJsonObject RegisterNode::createMessage(const QString &type, const QVariantMap &data) {
    QJsonObject message;
    message["requestType"] = type;
    for (auto it = data.begin(); it != data.end(); ++it) {
        message[it.key()] = QJsonValue::fromVariant(it.value());
    }
    return message;
}

void RegisterNode::updateNodeList(const QJsonObject &nodeData) {
    QJsonObject tamp = nodeData;
    tamp.remove("requestType");
    nodeList.append(tamp);
    qDebug() << "[RegisterNode] Updated node list. Total nodes:" << nodeList.size();
}

void RegisterNode::broadcastNodeList() {
    QVariantMap data;
    QJsonArray nodeArray;
    for (const QJsonObject &node : nodeList) {
        nodeArray.append(node);
    }
    data["nodes"] = nodeArray;
    data["metadataAnalyticsLeader"] = leaderIP;
    data["metadataIngestionLeader"] = "";
    data["initElectionIngestion"] = "192.168.1.108";
    QJsonObject message = createMessage("Node Discovery", data);
    QJsonDocument doc(message);

    qDebug() << "Clients:" << clients.size();
    for (QTcpSocket *client : clients) {
        client->write(doc.toJson());
        qDebug() << "[RegisterNode] Broadcasting node list to" << extractIPv4Address(client->peerAddress().toString());
        qDebug() << "Processed broadcasting request, message: " << message;
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    RegisterNode node(12351);
    return app.exec();
}
