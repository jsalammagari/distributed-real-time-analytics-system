#include "MetadataNode.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QTcpSocket>
#include <QCryptographicHash>

MetadataNode::MetadataNode(const QString &serverAddress, quint16 port, QObject *parent)
    : QObject(parent), socket(new QTcpSocket(this)), server(new QTcpServer(this)), myId(10), electionInitiated(false)
{
    connect(socket, &QTcpSocket::readyRead, this, &MetadataNode::onReadyRead);
    connect(server, &QTcpServer::newConnection, this, &MetadataNode::onNewConnection);
    connect(&electionTimer, &QTimer::timeout, this, &MetadataNode::handleElectionTimeout);
    connect(&registrationTimer, &QTimer::timeout, this, &MetadataNode::initiateElection);
    server->listen(QHostAddress::Any, port);
    socket->connectToHost(serverAddress, port);

    registrationTimer.start(10000);  // 10 sec timer
    connect(&initAnalyticsTimer, &QTimer::timeout, this, &MetadataNode::initAnalyticsNodes);
    initAnalyticsTimer.setSingleShot(true);
}

MetadataNode::~MetadataNode() {
    server->close();
    qDebug() << "[MetadataNode] Server shut down.";
}

QString extractIPv4Address(const QString& ipAddress) {
    if (ipAddress.startsWith("::ffff:")) {
        return ipAddress.mid(7);
    }
    return ipAddress;
}

QString MetadataNode::getLocalIPAddress() const {
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    for (int i = 0; i < list.count(); i++) {
        if (!list[i].isLoopback() && list[i].protocol() == QAbstractSocket::IPv4Protocol) {
            return list[i].toString();
        }
    }
    return "127.0.0.1";
}

QList<QJsonObject> MetadataNode::getMetadataNodes() {
    QList<QJsonObject> metadataNodes;
    for (const QJsonObject &node : nodeList) {
        if (node["nodeType"].toString() == "metadata Analytics") {
            metadataNodes.append(node);
        }
    }
    return metadataNodes;
}
double calculateComputingCapacity() {
    return 0.75; // Hardcoded
}
quint64 hashToInteger(const QString &input) {
    QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256);
    quint64 result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) + static_cast<unsigned char>(hash[i]);
    }
    return result;
}

void MetadataNode::generateNodeUID() {
    // Combine the IP address, type and port to create a unique
    QString combined = QString("%1: %2: %3")
                           .arg("metadata Analytics")
                           .arg(localIP)
                           .arg(12351);
   // myId = hashToInteger(combined); // instead of qhash to reduce
    //qDebug() << "Node UID of " << combined << " is: " << myId;
}

void MetadataNode::registerNode() {
    double capacity = calculateComputingCapacity();
    qDebug() << "In register node";

    if (!socket->waitForConnected(5000)) {
        qDebug() << "Failed to connect to" << socket->peerAddress().toString() << "on port" << socket->peerPort();
        socket->deleteLater();
        return;
    }
    localIP = socket->localAddress().toString();
    qDebug() << "IP: => " << socket->localAddress().toString();
    generateNodeUID();
    QJsonObject registrationRequest {
        {"requestType", "registering"},
        {"IP", socket->localAddress().toString()},
        {"nodeType", "metadata Analytics"},
        {"computingCapacity", capacity}
    };
    QJsonDocument doc(registrationRequest);
    socket->write(doc.toJson());
    qDebug() << "Register Node: Sent registration request from" << doc.toJson(QJsonDocument::Compact);
}

void MetadataNode::onNewConnection() {
    QTcpSocket *client = server->nextPendingConnection();
    clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &MetadataNode::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &MetadataNode::onClientDisconnected);
    qDebug() << "Metadata Node: New connection " << client;
}

void MetadataNode::onReadyRead() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    QJsonDocument doc = QJsonDocument::fromJson(client->readAll());
    processMessage(client, doc.object());
}

void MetadataNode::onClientDisconnected() {
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
    //broadcastNodeList();
}

void MetadataNode::processMessage(QTcpSocket* client, const QJsonObject &message) {
    QString type = message["requestType"].toString();

    if (type == "Node Discovery") {
        qDebug() << "Metadata Node: node desc request from Register node" << message;

        updateNodeList(message);
    } else if (type == "Election Message") {
        int candidateId = message["Current Number"].toInt();
        QString ip = message["IP"].toString();
        qDebug() << "Metadata Node: Election message";
        if (candidateId > myId) {
            forwardElectionMessage(candidateId, ip);
        }
        else if (candidateId == myId){
            leaderIP = socket->localAddress().toString();
            qDebug() << "New leader elected candidateId == myId:" << leaderIP;
            electionTimer.stop();
            announceLeader(leaderIP);
            initAnalyticsTimer.start(7000);
        }else{
            forwardElectionMessage(myId, socket->localAddress().toString());
        }
    } else if (type == "Leader Announcement") {
        leaderIP = message["leaderIP"].toString();
        qDebug() << "New leader elected:" << leaderIP;
        electionTimer.stop();
        initAnalyticsTimer.start(7000);
    }else if (type == "Heartbeat") {
        sendHeartBeat(client);
    }
    else if (type == "ingestion") {
        QJsonArray dataArray = message["Data"].toArray();
        sendAnalyticsRequest(dataArray);
    } else if (type == "query") {
        processQueryRequest(message);
    } else if (type == "query response"){
        qDebug() << "Received query response:" << message;
    } else if (type == "analytics acknowledgment"){
        qDebug() << "Received analytics acknowledgment:" << message;
    }
    else {
        qDebug() << "Received unrecognized message type:" << type << message;
    }
}

QJsonObject MetadataNode::createMessage(const QString &type, const QVariantMap &data) {
    QJsonObject message;
    message["requestType"] = type;
    for (auto it = data.begin(); it != data.end(); ++it) {
        message[it.key()] = QJsonValue::fromVariant(it.value());
    }
    return message;
}

void MetadataNode::sendHeartBeat(QTcpSocket *clientSocket) {
    QJsonObject responseObj;
    responseObj["requestType"] = "Heartbeat Response";
    responseObj["message"] = "I am alive";
    responseObj["status"] = "OK";

    QJsonDocument responseDoc(responseObj);
    QByteArray responseData = responseDoc.toJson();

    clientSocket->write(responseData);
}
void MetadataNode::updateNodeList(const QJsonObject &nodeData) {
    nodeList.clear();
    QJsonArray nodes = nodeData["nodes"].toArray();
    for (const QJsonValue &value : nodes) {
        QJsonObject node = value.toObject();
        QString nodeType = node["nodeType"].toString();
        QString ip = node["IP"].toString();
        double computingCapacity = node["computingCapacity"].toDouble();

        // Add the node to the nodeList
        QJsonObject newNode{
            {"IP", ip},
            {"nodeType", nodeType},
            {"computingCapacity", computingCapacity}
        };
        nodeList.append(newNode);
    }
    qDebug() << "[MetadataNode] Updated node list. Total nodes:" << nodeList.toList();
    if(nodeData["metadataAnalyticsLeader"] == ""){
        initiateElection();
    }
}

void MetadataNode::broadcastNodeList() {
    QVariantMap data;
    QJsonArray nodeArray;
    for (const QJsonObject &node : nodeList) {
        nodeArray.append(node);
    }
    data["nodes"] = nodeArray;
    data["metadataAnalyticsLeader"] = leaderIP;
    data["metadataIngestionLeader"] = "";
    data["initElectionIngestion"] = "192.168.1.104";
    QJsonObject message = createMessage("Node Discovery", data);
    QJsonDocument doc(message);
    //QString messageString = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Clients:" << clients.size();
    for (QTcpSocket *client : clients) {
        client->write(doc.toJson());
        qDebug() << "[MetadataNode] Broadcasting node list to" << extractIPv4Address(client->peerAddress().toString());
        qDebug() << "Processed broadcasting request, message: " << message;
    }
}

void MetadataNode::startElection() {
    if (!electionInitiated) {
        QList<QJsonObject> metadataNodes = getMetadataNodes();
        if (metadataNodes.size() <= 1) {
            leaderIP = socket->localAddress().toString();
            announceLeader(socket->localAddress().toString());
        } else {
            forwardElectionMessage(myId, socket->localAddress().toString());
            //electionTimer.start(30000);  // 30 seconds timeout
        }
        electionInitiated = true;
    }
    //}else{}
}

void MetadataNode::handleElectionTimeout() {
    qDebug() << "Election timeout occurred. No leader elected.";
}

void MetadataNode::initiateElection() {
    registrationTimer.stop();
    QList<QJsonObject> metadataNodes = getMetadataNodes();
    if(metadataNodes.size() > 0 and metadataNodes[0]["IP"] == socket->localAddress().toString()){
        qDebug() << "I'm the first in nodelist. Initiating election process after registration period.";
        startElection();
    }
}

void MetadataNode::forwardElectionMessage(int candidateId, QString ip) {
    QVariantMap data;
    data["Current Number"] = candidateId;
    data["IP"] = ip;
    QJsonObject message = createMessage("Election Message", data);
    QJsonDocument doc(message);
    //sendMessageToNextNode(doc, ip);
}

void MetadataNode::sendMessageToNextNode(const QJsonDocument &doc, QString ip) {
    int myIndex = -1;
    qDebug() << ip << "IN sendMessageToNextNode ip.";
    for (int i = 0; i < nodeList.size(); ++i) {
        qDebug() << nodeList[i]["IP"].toString()  << socket->localAddress().toString() << nodeList[i]["nodeType"].toString();
        if (nodeList[i]["IP"].toString() == socket->localAddress().toString() && nodeList[i]["nodeType"].toString() == "metadata Analytics") {
            myIndex = i;
            break;
        }
    }
    if (myIndex == -1) {
        qDebug() << "Current metadata node not found in nodeList.";
        return;
    }

    int nextIndex = -1;
    for (int i = 1; i <= nodeList.size(); ++i) {
        int idx = (myIndex + i) % nodeList.size();
        if (nodeList[idx]["nodeType"].toString() == "metadata Analytics" && idx != myIndex) {
            nextIndex = idx;
            break;
        }
    }
    if (nextIndex == -1 || nextIndex == myIndex) {
        qDebug() << "No next metadata node found or single metadata node in the network. Now want announce the leader";
        announceLeader(ip);
        return;
    }
    QString nextNodeIp = nodeList[nextIndex]["IP"].toString();
    QTcpSocket *nextNodeClient = new QTcpSocket(this);
    qDebug() << "Next metadata node: " << nextNodeIp;
    nextNodeClient->connectToHost(nextNodeIp, 12351);

    nextNodeClient->write(doc.toJson());
    qDebug() << "Sent message to next metadata node:" << nextNodeIp;
}

void MetadataNode::announceLeader(QString ip) {
    QVariantMap data;
    data["leaderIP"] = ip;
    data["nodeType"] = "metadata Analytics";
    QJsonObject message = createMessage("Leader Announcement", data);
    QJsonDocument doc(message);

    for (const QJsonObject &node : nodeList) {
        QString analyticsNodeIp = node["IP"].toString();

        sendMessageToNode(analyticsNodeIp, doc);
    }
    sendMessageToRegisterNode(doc);
    qDebug() << "Leader elected: Analytics timer start. " << message;
    initAnalyticsTimer.start(7000);
}

void MetadataNode::initAnalyticsNodes() {
    if(leaderIP == socket->localAddress().toString()){
        for (const QJsonObject &node : nodeList) {
            if (node["nodeType"].toString() == "analytics") {
                QString analyticsNodeIp = node["IP"].toString();
                QJsonArray replicas = getReplicasFor(analyticsNodeIp);

                QJsonObject message;
                message["requestType"] = "Init Analytics";
                message["Replicas"] = replicas;

                QJsonDocument doc(message);
                sendMessageToNode(analyticsNodeIp, doc);
            }
        }
    }
}

QJsonArray MetadataNode::getReplicasFor(const QString &ip) {
    QJsonArray replicas;
    for (const QJsonObject &node : nodeList) {
        if (node["IP"].toString() != ip && node["nodeType"].toString() == "analytics") {
            replicas.append(node["IP"].toString());
        }
    }
    return replicas;
}

void MetadataNode::sendMessageToNode(const QString &ip, const QJsonDocument &doc) {
    QTcpSocket *analyticsClient = new QTcpSocket(this);
    analyticsClient->connectToHost(ip, 12351);
    analyticsClient->write(doc.toJson());
    qDebug() << "send message to IP:" << ip;
}

void MetadataNode::sendMessageToRegisterNode(const QJsonDocument &doc) {
    QTcpSocket *registerClient = new QTcpSocket(this);
    //registerClient->connectToHost("192.168.1.107", 12351); //Jahnvi's IP
    registerClient->connectToHost("192.168.1.102", 27500);  //Harshit's IP
    registerClient->write(doc.toJson());
    qDebug() << "send to leader ip to register node.";
}

void MetadataNode::sendAnalyticsRequest(const QJsonArray& data) {
    for (const QJsonObject &node : nodeList) {
        if (node["nodeType"].toString() == "analytics") {
            QString analyticsNodeIp = node["IP"].toString();
            QJsonObject requestObj;
            requestObj["requestType"] = "analytics";
            requestObj["requestID"] = 123;
            requestObj["Data"] = data;


            QJsonDocument doc(requestObj);
            sendMessageToNode(analyticsNodeIp, doc);
            qDebug() << "Analytics request sent: -> " << analyticsNodeIp;
        }
    }
}

void MetadataNode::processQueryRequest(const QJsonObject &message) {
    int requestId = 1;
    int queryType = message["param"].toInt(); // Assuming 0 or 1 indicates different types of queries

    QJsonObject queryRequest{
        {"requestType", "query"},
        {"requestId", requestId},
        {"query", queryType}
    };
    QJsonDocument doc(queryRequest);
    forwardQueryToAnalyticsNode(doc);
}

void MetadataNode::forwardQueryToAnalyticsNode(const QJsonDocument &doc) {
    for (const QJsonObject &node : nodeList) {
        if (node["nodeType"].toString() == "analytics") {
            QString analyticsNodeIp = node["IP"].toString();
            sendMessageToNode(analyticsNodeIp, doc);
            qDebug() << "Send query to analytics node:" << analyticsNodeIp;
        }
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    MetadataNode node("192.168.1.102", 12351); // Harshit's IP
    //MetadataNode node("127.0.0.1", 12351); // Local IP
    //MetadataNode node("192.168.1.107", 12351);
    node.registerNode();
    return app.exec();
}
