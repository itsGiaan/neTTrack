#include "client.h"

struct Client client_init(int domain, int service, int protocol)
{
    struct Client client;
    client.domain = domain;
    client.service = service;
    client.protocol = protocol;
    client.socket = Socket(domain, service, protocol);
    return client;
}
