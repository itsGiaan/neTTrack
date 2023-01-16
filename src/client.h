#ifndef client_h
#define client_h

#include "wrapper.h"

struct Client
{
    int domain;
    int service;
    int protocol;
    int socket;
};

struct Client client_init(int domain, int service, int protocol);

#endif