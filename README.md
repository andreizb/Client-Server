# Client-Server Application

This project is a Client-Server Application that runs a very simple protocol built over TCP. It consists of a server and multiple possible subscribers. Those can subscribe to certain topics and the data about those is provided by an UDP client.

## Functionality

* Subscriber: it has a unique ID, it can subscribe or unsubscribe to certain topics and close its connection, with the possibility to reconnect in the future. The subscriber can opt for updates while he is online for certain topics. If that's the case, when the subscriber reconnects, he will receive all the information that was sent about the chosen topics;
* Server: it receives the connection, subscribe, unsubscribe and exit requests from the clients and the datagrams from the UDP client. When the server exits, all the clients are asked to close as well. The server will refuse connections in the situation when a user attempts to connect with the same ID of an already connected user. Our server is responsible for parsing the messages from the UDP client and either send them directly to the subscribed clients which are connected, or enqueue them for the disconnected subscribers which opted to receive the updates when they reconnect;
* UDP client: it's the one that sends the datagrams, very simple implementation.

## Protocol summary

The datagram is sent from the server to all the clients. The first 4 bytes of the datagram will indicate the size of the content and the rest of the datagram is the actual data. By implementing it this way, we will avoid the problems caused by TCP's datagram concatenations at higher speed very easily.
