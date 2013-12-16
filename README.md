OSS---D4
========
By Jonas Serych & Kacka Klimova

compile by 
$ make

run by
$ cd bin
$ ./main node_number [config file, default routing.cfg]

Watch nodes connecting and disconnecting :)

commands:
r ... show routing table
s ... show status table
b ... show messages in buffer
2 Hi there, number 2! ... send message "Hi there, number 2!" to node 2
  ... if the message cannot be delivered at the moment, it will be saved in buffer and the program will try to     resend it later, when the recipient connects


Task:
----------------------------------------------------------------

You have to implement an application that can be server and client in one time. Each running application (node) accepts connection from other nodes that try to connect so that the communication network allows message transmission between any (two) nodes. In this network no central point is present, all nodes are equal.

Each node in the network is identified by its unique ID (1-byte unsigned number: 1 to 255). The network topology is specified by the configuration file: a text file with two type of lines:

node: physical placement of the node in the network
link: definition of a link between two nodes
The node line structure is:

node <ID> <port> <ip_address>
where the ID is the node ID, port is the port where the node is running and the ip_address is an optional address of the node (for the case the nodes run on different machines).

The link line structure is:

link <ID_client> <ID_server>
where the ID_client is the node ID that initiates (establishes) the connection and ID_server is the node ID to which the client connects.

Each application instance is run with one mandatory parameter (a node ID). Using this ID the application can find relevant lines in the configuration file.

Each instance must allow at leas 4 connections as a server and 4 connections as a client. All the connections established are bi-directional and no longer is there any differentiation between client or server.

The program uses one of the known |routing algorithms, or your own approach.

The program reads messages from stdin in the following format:

ID:message
where ID is the ID of target node. The message is a text string that can be of limited size for simplification (e.g. 128 bytes). Your task is to design (and implement) a communication protocol for the nodes that allows message delivery and delivery of helper information for the routing.

To demonstrate the ability of dynamic routing it is crucial for the network to operate even when the nodes are connecting and disconnecting. This means when the node is not reachable, you need to try the connection in regular intervals (e.g. 1 s) for case the node appears online.

Use the datagram socket for communication (UDP). Using the UDP protocol you do not have to search for the starts/ends of datagrams sent between nodes. In addition, you need only one socket for communication with all nodes connected.

The system must guarantee the message delivery in case there exists a route between sender and recipient. One message should in no case be delivered multiple times (when sent by multiple paths for example). Messages that cannot be delivered are dropped. It is also allowed to drop messages that do not fit in the program buffer (in case of the network overload).

If there is no possibility to deliver the message (no route exists), the system should inform the sender about the undeliveable message (if you do not implement this, the task is acceptable, but point penalty will result).

For the parsing of configuration file we have prepared a function parseRouteConfiguration:

int parseRouteConfiguration(const char * fileName, int localId, int * connectionCount, TConnection * connections)
You can download it via the route_cfg_parser.zip file that contains the .h file with header and a demonstration program (with a sample cfg file).

The function reads a list of connections for the current node (specified by the localID parameter from a file. For each connection you get ID, port and eventually the IP address in the following form:

typedef struct {
  int id;
  int port;
  char ip_address[IP_ADDRESS_MAX_LENGTH];
} TConnection;
Note that the code can be compiled under both C and C++.
-----------------------------------------------------------------------------
