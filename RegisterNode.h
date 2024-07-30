#ifndef RegisterNode_H
#define RegisterNode_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

class RegisterNode : public QObject {
    Q_OBJECT
public:
    explicit RegisterNode(quint16 port, QObject *parent = nullptr);
    ~RegisterNode();
    QString localIP; //"192.168.1.107";

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QTcpServer *server;
    QList<QTcpSocket*> clients;
    QList<QJsonObject> nodeList;
    int myId;
    QString leaderIP;

    void processMessage(QTcpSocket* client, const QJsonObject &message);
    QJsonObject createMessage(const QString &type, const QVariantMap &data);
    void updateNodeList(const QJsonObject &nodeData);
    void broadcastNodeList();
    QList<QJsonObject> getRegisterNodes();
    QString getLocalIPAddress() const;
    void sendMessageToNode(const QString &ip, const QJsonDocument &doc);
    void sendAnalyticsRequest(const QJsonArray& data);
    void processQueryRequest(const QJsonObject &message);
    void forwardQueryToAnalyticsNode(const QJsonDocument &doc);
};

#endif
