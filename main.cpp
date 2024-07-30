#include <QCoreApplication>
#include "MetadataNode.h"
#include "AnalyticsNode.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    MetadataNode metadataNode(12345); // Start the metadata node on port 12345
    AnalyticsNode analyticsNode("127.0.0.1", 12345); // Connect to the metadata node

    analyticsNode.registerNode(); // Register the analytics node

    return a.exec();
}
