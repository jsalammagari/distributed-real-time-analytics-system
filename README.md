# distributed-real-time-analytics-system

Distributed Real-Time Analytics System Report (Analytics Team)

**System Overview**
This project implements a distributed system designed for data ingestion and analytics. The primary components include Ingestion Nodes, Analytics Nodes, and Metadata Management. The system is designed to handle large volumes of data, perform efficient data retrieval, and execute complex analytical queries.
**Components**
**Ingestion Nodes**: Responsible for collecting and preprocessing data from various sources, storing it temporarily for quick access.
**Analytics Nodes**: Perform data analysis based on a list of predefined queries. These nodes dynamically request data from the appropriate ingestion nodes.
**Metadata Management**: Maintains a catalog of data held by ingestion nodes and ensures that analytics nodes have up-to-date metadata for efficient query execution.
**Leader Election**: Ensures a designated leader node is responsible for coordinating tasks and maintaining system stability.
**Data Flow Design**
**Data Collection**: Ingestion nodes continuously read and preprocess data, storing it locally for short periods.
**Query Input**: Analytics nodes receive an input file containing a list of queries, which dictates the data requirements.
**Data Request Execution**: Analytics nodes dynamically contact the appropriate ingestion nodes based on metadata to execute queries.
**Data Retrieval and Transfer**: Ingestion nodes retrieve requested data from local storage and send it to the analytics nodes.
**Data Analysis**: Analytics nodes process the received data to fulfill the analytical tasks.
**Leader Election**: Nodes participate in an election process to designate a leader responsible for coordination and stability.
**Key Components**
**Ingestion Nodes**:
**Local Data Storage**: Fast, indexed storage solutions for quick data retrieval.
**Data Handling**: Capable of handling multiple simultaneous data requests.
**Analytics Nodes**:
**Query Management**: Manages query lists and schedules their execution based on internal logic.
**Dynamic Query Execution**: Sends queries to ingestion nodes and processes the incoming data.
**Query Input File**: Receives a file containing the necessary queries for operation.
**Communication Protocols**
**Direct Node-to-Node Communication**: Uses TCP/IP for robust communication or UDP for faster, less reliable transmission.
**Data Format**: Uses efficient serialization formats like Protocol Buffers to minimize bandwidth usage and parsing time.
**Metadata Management**
**Ingestion Node Metadata Registration**:
**Data Catalog**: Maintains a detailed catalog of the data held, including types, sources, and time ranges.
**Metadata Registration**: Sends catalog information to a centralized registry or distributes it among analytics nodes.
**Analytics Node Metadata Awareness**:
**Metadata Sync**: Retrieves and synchronizes metadata from ingestion nodes through a push or pull model.
**Metadata-Driven Query Analysis**: Uses metadata tags in queries to match them with appropriate ingestion nodes.
**Implementation Overview**
The implementation of our data ingestion and analytics system involves four main files: AnalyticsNode, Worker, DemoIngestionNode, MetadataNode, and RegisterNode. Each of these files plays a critical role in ensuring the system functions efficiently and effectively. Below, we detail the components, technologies, and methods used in each file and the rationale behind these choices.
### 1. AnalyticsNode
The AnalyticsNode class is responsible for managing the communication between the analytics nodes and the ingestion nodes. It handles query execution, data retrieval, and analytical processing.
Key Components:
QTcpSocket and QTcpServer: Used for network communication. QTcpSocket enables the analytics node to connect to the server and send/receive data, while QTcpServer listens for incoming connections from ingestion nodes.
Threading with QThread: To prevent the main event loop from blocking during intensive tasks, we offload data processing to a separate worker thread using QThread.
JSON Handling: QJsonDocument, QJsonObject, and QJsonArray are used for serializing and deserializing data, ensuring efficient communication between nodes.
Rationale:
QTcpSocket and QTcpServer provide robust and reliable communication channels, essential for transmitting critical data and ensuring the analytics process runs smoothly.
Threading is necessary to maintain responsiveness, particularly when handling large data sets or complex queries that could otherwise block the main thread.
JSON Handling is chosen for its simplicity and efficiency in data serialization, making it easy to parse and manage the data structure.
Corner Cases Handled:
Connection Timeout: If the connection to the server times out (socket->waitForConnected(5000)), the system logs an error and stops further execution to prevent indefinite blocking.
Incomplete Data Reception: The system ensures that the entire message is received before processing (socket->waitForReadyRead()), preventing partial data handling.
Client Disconnection: Handles scenarios where clients disconnect unexpectedly by cleaning up resources (onClientDisconnected).

### 2. Worker
The Worker class encapsulates the data storage and query processing logic, running these tasks in a separate thread to ensure the main application remains responsive.
Key Components:
QObject: Worker inherits from QObject, enabling it to use Qt's signal and slot mechanism for communication with AnalyticsNode.
Signal and Slot Mechanism: Facilitates asynchronous communication between the main AnalyticsNode class and the Worker class.
Data Storage (QList<QJsonArray>): Stores the preprocessed data temporarily for quick access during query processing.
Rationale:
QObject provides the necessary infrastructure for signal and slot communication, essential for threading.
Signal and Slot Mechanism ensures efficient and safe communication between threads, allowing the main thread to remain responsive.
Data Storage using QList<QJsonArray> is chosen for its simplicity and direct access capabilities, making it suitable for temporarily holding data.
Corner Cases Handled:
Empty Data Arrays: Checks for empty data arrays before processing to avoid unnecessary operations.
Invalid Data Format: Validates the structure of incoming data arrays to ensure they meet expected formats before storing or processing them.
### 3. MetadataNode
The MetadataNode class manages the metadata of the data held by ingestion nodes, ensuring that analytics nodes can efficiently determine where to send their queries.
Key Components:
Metadata Catalog (QJsonObject): Maintains a catalog of the data held by ingestion nodes, including types, sources, and time ranges.
Synchronization Mechanism: Updates the metadata catalog periodically or upon changes in data.
Rationale:
Metadata Catalog provides a structured way to keep track of data characteristics, essential for efficient query routing.
Synchronization Mechanism ensures that the metadata is always up-to-date, allowing analytics nodes to make informed decisions when sending queries.
Corner Cases Handled:
Metadata Inconsistencies: Ensures that any discrepancies in metadata updates are resolved promptly to maintain consistent and accurate metadata.
Network Failures: Handles scenarios where metadata synchronization fails due to network issues by retrying the synchronization process.

### 4. DemoIngestionNode
This header file defines the structure and functionalities of the ingestion node, including data collection, preprocessing, and local storage.
Key Components:
Local Data Storage: Uses fast storage solutions that are indexed for quick data retrieval.
Data Handling: Capable of handling multiple simultaneous data requests from analytics nodes.
QTcpSocket: Used for network communication to connect to the metadata node server and send/receive data.
QJsonDocument, QJsonObject, QJsonArray: Used for creating and parsing JSON data to be sent to the server.
QCoreApplication: Manages the application's control flow and main settings.
Rationale:
Local Data Storage is essential for ensuring quick access to data, which is critical for the timely processing of queries.
Data Handling capabilities ensure that the system can scale and handle concurrent requests, providing robustness and reliability.
Network Communication: QTcpSocket ensures robust communication with the metadata node server.
JSON Data Handling: Efficient serialization format for data exchange.
Application Management: QCoreApplication ensures proper application control and event loop management.
Corner Cases Handled:
Data Overload: Implements checks to prevent the local storage from being overwhelmed by too much data, ensuring stable performance.
Simultaneous Requests: Manages concurrent data requests efficiently to avoid race conditions and ensure data integrity.
### 5. RegisterNode
The RegisterNode class handles the registration of nodes and synchronization of metadata between ingestion and analytics nodes.
Key Components:
Node Registration (QTcpSocket): Uses QTcpSocket for registering nodes with the centralized registry or among other nodes.
Metadata Synchronization: Ensures that the metadata is synchronized across all nodes.
Rationale:
Node Registration is critical for maintaining an updated list of active nodes and their capabilities, enabling efficient distribution of queries.
Metadata Synchronization ensures consistency and reliability in data processing, as all nodes have access to the latest metadata information.
Corner Cases Handled:
Duplicate Registrations: Implements checks to prevent duplicate registrations of the same node, ensuring unique node entries.
Synchronization Failures: Handles cases where metadata synchronization fails by logging errors and attempting retries to ensure eventual consistency.
### 6. Leader Election
Leader Election ensures a designated leader node is responsible for coordinating tasks and maintaining system stability.
Election Process:
Nodes periodically participate in elections to designate a leader node. This is triggered by the absence of heartbeat messages from the current leader or through scheduled election intervals.
Uses a simple majority vote or a designated election algorithm to ensure a fair and efficient election process.
Leader Responsibilities:
The leader coordinates metadata updates, manages task distribution, and ensures system stability.
If a leader fails, a new election is initiated to designate a new leader.
The implementation of our data ingestion and analytics system leverages several key technologies and methodologies to ensure robustness, efficiency, and scalability. By using Qt's networking and threading capabilities, JSON for data serialization, and a well-structured approach to metadata management, the system can handle large volumes of data and perform complex analytical tasks efficiently. Each component is designed with a clear purpose and rationale, contributing to the overall effectiveness of the system.


### Conclusion
This project effectively demonstrates a robust and scalable system for distributed real time analytics. The use of metadata management ensures efficient query execution, while the direct node-to-node communication protocol enhances the system's robustness and efficiency. The project showcases a well-structured approach to handling large volumes of data and performing complex analytical tasks in a distributed environment.

### Citations
https://www.mdpi.com/1424-8220/20/11/3166
https://www.mdpi.com/2079-9292/11/23/4037

