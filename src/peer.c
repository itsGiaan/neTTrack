#include "LinkedList.h"
#include "server.h"
#include "client.h"
#include "wrapper.h"
#include "utils.h"


short infected = 0;
pthread_mutex_t infection_mutex = PTHREAD_MUTEX_INITIALIZER;

struct LinkedList *mine_global;
pthread_mutex_t mine_list_mutex = PTHREAD_MUTEX_INITIALIZER;

struct LinkedList *their_global;
pthread_mutex_t their_list_mutex = PTHREAD_MUTEX_INITIALIZER;

short isKnown(char* client_address, struct LinkedList *list)
{
    char *ip_saver;
    for(int i = 0; i < list->length; i++)
    {
        ip_saver =(char*)list->retrieve(list, i);
        if(strcmp(ip_saver, client_address) == 0)
        {
            return 1;
        }
    }
    return 0;
}

short check_id(char* id, struct LinkedList *list)
{
    char *id_saver;
    for(int i = 0; i < list->length; i++)
    {
        id_saver = (char*)list->retrieve(list, i);
        if(strcmp(id, id_saver) == 0)
        {
            return 1;
        }
    }
    return 0;
}

void * server_loop(void *arg)
{
    struct ServerLoopArgument *loop_arg = arg;
    int len = loop_arg->known_hosts->length;
    short flag, found;
    char *client_address = inet_ntoa(loop_arg->server->address.sin_addr);
    char ip_saver[INET_ADDRSTRLEN], id[45];

    if(!len)
    {
        memset(ip_saver, 0, INET_ADDRSTRLEN);
        memset(id, 0, 45);
        strcpy(ip_saver, client_address);
        loop_arg->known_hosts->insert(loop_arg->known_hosts, loop_arg->known_hosts->length, ip_saver, INET_ADDRSTRLEN);
        printf("\nNew peer added: %s\n", ip_saver);
        FullRead(loop_arg->client, id, 45);
        printf("\nGot %s from %s\n", id, client_address);
        //Salvo l'id nella lista
        pthread_mutex_lock(&their_list_mutex);
        their_global->insert(their_global, their_global->length, id, 45);
        pthread_mutex_unlock(&their_list_mutex);
        
    }
    else
    {
        found = isKnown(client_address, loop_arg->known_hosts);
        if(!found)
        {
           memset(ip_saver, 0, INET_ADDRSTRLEN);
           memset(id, 0, 45);
           strcpy(ip_saver, client_address);
           loop_arg->known_hosts->insert(loop_arg->known_hosts, loop_arg->known_hosts->length, ip_saver, INET_ADDRSTRLEN);
           FullRead(loop_arg->client, id, 45);
           printf("\nGot %s from %s\n", id, ip_saver);
           //Salvo l'id nella lista
            pthread_mutex_lock(&their_list_mutex);
            their_global->insert(their_global, their_global->length, id, 45);
            pthread_mutex_unlock(&their_list_mutex);
        }
        else
        {
            memset(id, 0, 45);
            FullRead(loop_arg->client, id, 45);
            printf("\nGot %s from %s\n", id, ip_saver);
            //Salvo l'id in lista
            pthread_mutex_lock(&their_list_mutex);
            their_global->insert(their_global, their_global->length, id, 45);
            pthread_mutex_unlock(&their_list_mutex);
        }
    }
    close(loop_arg->client);
}

void * server_function(void *arg)
{
    struct Server server = server_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, 20);
    socklen_t slen = (socklen_t)sizeof(server.address);
    pthread_t server_thread;

    while(1)
    {
        struct ServerLoopArgument loop_arg;
        loop_arg.client = Accept(server.socket, (struct sockaddr*)&server.address, &slen);
        loop_arg.server = &server;
        loop_arg.known_hosts = arg;

        pthread_create(&server_thread, NULL, server_loop, &loop_arg);
    }
}


void * notify_receiver(void *arg)
{
    struct Server server = server_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, PORT_RECEIVER, 20);
    socklen_t slen = (socklen_t)sizeof(server.address);
    int connfd;
    if(connfd = Accept(server.socket, (struct sockaddr*)&server.address, &slen))
    {
        pthread_mutex_lock(&intection_mutex);
        infected = 1;
        pthread_mutex_unlock(&infection_mutex);
    }
    close(connfd);
}


void server_loop2(void *arg)
{
    struct ServerLoopArgument *loop_arg = arg;
    struct LinkedList tmp_list = linked_list_init();
    char *client_address = inet_ntoa(loop_arg->server->address.sin_addr);
    int len;
    short found;
    char id[45];
    printf("\nIncoming contact list from %s\n", client_address);

    FullRead(loop_arg->client, &len, sizeof(int));
    len = ntohl(len);
    for(int i = 0; i < len; i++)
    {
        memset(id, 0, 45);
        FullRead(loop_arg->client, id, 45);
        tmp_list.insert(&tmp_list, i, id, 45);
    }

    for(int i = 0; i < tmp_list.length; i++)
    {
        memset(id, 0, 45);
        strcpy(id, tmp_list.retrieve(&tmp_list, i));
        found = check_id(id, loop_arg->known_hosts);
        if(found)
        {
            printf("\nGot a contact with %s\n", client_address);
        }
        else
        {
            printf("\nNo contact, stay safu\n");
        }
    }
    linked_list_destroy(tmp_list);
    close(loop_arg->client);
}

void server_function2(void *arg)
{
    struct Server server = server_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, LIST_PORT, 20);
    socklen_t slen = (socklen_t)sizeof(server.address);
    pthread_t server_thread;

    while(1)
    {
        struct ServerLoopArgument loop_arg;
        loop_arg.client = Accept(server.socket, (struct sockaddr*)&server.address, &slen);
        loop_arg.server = &server;
        loop_arg.known_hosts = arg;

        pthread_create(&server_thread, NULL, server_loop2, loop_arg);
    }
}

void sender_id_procedure(struct LinkedList *mine, struct LinkedList *known_hosts)
{
    char ip_saver[INET_ADDRSTRLEN];
    char *id;
    id = random_string(45);

    for(int i = 0; i < known_hosts->length; i++)
    {
        memset(ip_saver, 0, INET_ADDRSTRLEN);
        strcpy(ip_saver, (char*)known_hosts->retrieve(known_hosts, i));
        send_id(&mine, &ip_saver, id);
    }
}

void send_id(struct LinkedList *mine, char* ip_addr, char* id)
{
    struct Client client = client_init(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in dst;
    socklen_t slen = (socklen_t)sizeof(dst);

    dst.sin_family = AF_INET;
    dst.sin_port = htons(SENDING_PORT);
    Inet_pton(AF_INET, ip_addr, &dst.sin_addr);

    if((Connect(client.socket, (struct sockaddr*)&dst, slen)) != -1)
    {
        printf("\nSending %s to %s\n", id, ip_addr);
        FullWrite(client.socket, id, 45);
    }

    close(client.socket);
}

void sender_list_procedure(struct LinkedList *known_hosts, struct LinkedList *their)
{
    char ip[INET_ADDRSTRLEN];
    for(int i = 0; i < known_hosts->length; i++)
    {
        memset(ip, 0, INET_ADDRSTRLEN);
        strcpy(ip, (char*)known_hosts->retrieve(known_hosts, i));
        send_list(their, ip);
    }
    pthread_mutex_lock(&infection_mutex);
    infected = 0;
    pthread_mutex_unlock(&infection_mutex);
}

void send_list(struct LinkedList *their, char* ip)
{
    struct Client client = client_init(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in dst;
    socklen_t slen = (socklen_t)sizeof(dst);
    int len = their->length;
    char id[45];

    dst.sin_family = AF_INET;
    dst.sin_port = htons(LIST_PORT);
    Inet_pton(AF_INET, ip, &dst.sin_addr);

    if((Connect(client.socket, (struct sockaddr*)&dst, slen)) != -1)
    {
        len = htonl(len);
        printf("Sending list to: %s", ip);
        FullWrite(client.socket, &len, sizeof(int));
        len = ntohl(len);

        for(int i = 0; i < len; i++)
        {
            memset(id, 0, 45);
            strcpy(id, (char*)their->retrieve(their, i));
            FullWrite(client.socket, id, 45);
        }
    }
    close(client.socket);
}

int main()
{
    struct LinkedList known_hosts = linked_list_init();
    struct LinkedList mine_ids = linked_list_init();
    struct LinkedList their_ids = linked_list_init();
    
    pthread_t thread1, thread2, thread3;

    pthread_mutex_lock(&mine_list_mutex);
    mine_global = &mine_ids;
    pthread_mutex_unlock(&mine_list_mutex);

    pthread_mutex_lock(&their_list_mutex);
    their_global = &their_ids;
    pthread_mutex_unlock(&their_list_mutex);

    pthread_create(&thread1, NULL, server_function, &known_hosts);
    pthread_create(&thread2, NULL, server_function2, &mine_ids);

    while(1)
    {
        while(!infected)
        {
           sender_procedure(&mine_ids, &known_hosts);
        }
        if(infected)
        {
            sender_list_procedure(&known_hosts, &their_ids);
        }
    }

    linked_list_destroy(&known_hosts);
    linked_list_destroy(&mine_ids);
    linked_list_destroy(&their_ids);
    exit(EXIT_SUCCESS);
}