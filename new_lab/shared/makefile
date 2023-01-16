all: randezvous peer

randezvous: randezvous.c LinkedList.c Node.c server.c client.c utils.c wrapper.c
	gcc -o randezvous randezvous.c LinkedList.c Node.c server.c client.c utils.c wrapper.c -lpthread 

peer: peer.c LinkedList.c Node.c server.c client.c utils.c wrapper.c peer.c
	gcc -o peer peer.c LinkedList.c Node.c server.c client.c utils.c wrapper.c -lpthread
