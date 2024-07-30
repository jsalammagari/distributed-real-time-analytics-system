#ifndef METADATANODE_H
#define METADATANODE_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

class MetadataNode : public QObject {
    Q_OBJECT
public:
    explicit MetadataNode(const QString &serverAddress,quint16 port, QObject *parent = nullptr);
    ~MetadataNode();
    QString localIP; //"192.168.1.107";
    void registerNode();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void startElection();
    void handleElectionTimeout();
    void initiateElection();

private:
    QTcpSocket *socket;
    QTcpServer *server;
    QList<QTcpSocket*> clients;
    QList<QJsonObject> nodeList;
    int myId;
    QString leaderIP;
    bool electionInitiated;
    QTimer electionTimer;
    QTimer registrationTimer;
    QTimer initAnalyticsTimer;

    void processMessage(QTcpSocket* client, const QJsonObject &message);
    QJsonObject createMessage(const QString &type, const QVariantMap &data);
    void updateNodeList(const QJsonObject &nodeData);
    void broadcastNodeList();
    void forwardElectionMessage(int candidateId, QString ip);
    void announceLeader(QString ip);
    void sendMessageToNextNode(const QJsonDocument &doc, QString ip);
    QList<QJsonObject> getMetadataNodes();
    QString getLocalIPAddress() const;
    void initAnalyticsNodes();
    QJsonArray getReplicasFor(const QString &ip);
    void sendMessageToNode(const QString &ip, const QJsonDocument &doc);
    void sendAnalyticsRequest(const QJsonArray& data);
    void processQueryRequest(const QJsonObject &message);
    void forwardQueryToAnalyticsNode(const QJsonDocument &doc);
    void sendMessageToRegisterNode(const QJsonDocument &doc);
    void generateNodeUID();
    void sendHeartBeat(QTcpSocket *clientSocket);
};

#endif
