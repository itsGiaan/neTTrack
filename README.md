# neTTrack
An hybrid p2p contact tracing

# Overview
Design and implement a p2p contact tracing application. Each peer, at regular time intervals, sends to reachable peers
a randomly generated 64-byte alphanumeric id and receives the ids of the peers from which it is reachable. Each peer keeps the ids it generates and those of the peers it has come into contact with. To join the network, each peer must register with a server that also performs a notification service. If a peer receives a notification from the server, it sends the list of peers it has come into contact with to all other peers. A peer receiving a contact list verifies if its own id is present and if so displays a message to the user. Peers must communicate directly with each other without going through the server.
