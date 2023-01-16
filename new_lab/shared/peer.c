#include "LinkedList.h"
#include "server.h"
#include "client.h"
#include "wrapper.h"
#include "utils.h"

#define SRV_IP "10.0.0.1"

short infected = 0;
pthread_mutex_t infection_mutex = PTHREAD_MUTEX_INITIALIZER;

//Get a global copy so we can access the LinkedList without pass it as argument to the thread.
struct LinkedList *their_global;
pthread_mutex_t their_list_mutex = PTHREAD_MUTEX_INITIALIZER;

short isKnowed(char* client_address, struct LinkedList *list)
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

    //If there's no host gonna add it.
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
        found = isKnowed(client_address, loop_arg->known_hosts);
        //If I don't know him, I have to.
        if(!found)
        {
           memset(ip_saver, 0, INET_ADDRSTRLEN);
           memset(id, 0, 45);
           strcpy(ip_saver, client_address);
           loop_arg->known_hosts->insert(loop_arg->known_hosts, loop_arg->known_hosts->length, ip_saver, INET_ADDRSTRLEN);
           FullRead(loop_arg->client, id, 45);
           printf("\nGot %s from %s\n", id, client_address);
           pthread_mutex_lock(&their_list_mutex);
           their_global->insert(their_global, their_global->length, id, 45);
           pthread_mutex_unlock(&their_list_mutex);
        }
        //Already know him.
        else
        {
            memset(id, 0, 45);
            FullRead(loop_arg->client, id, 45);
            printf("\nGot %s from %s\n", id, client_address);
            pthread_mutex_lock(&their_list_mutex);
            their_global->insert(their_global, their_global->length, id, 45);
            pthread_mutex_unlock(&their_list_mutex);
        }
    }
    close(loop_arg->client);
}

void * server_function(void *arg)
{
    struct Server server = server_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, ID_PORT, 20);
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
    struct PackedArguments *loop_arg = arg;
    char *client_address = inet_ntoa(loop_arg->server->address.sin_addr);
    printf("\nGot infection notify from %s\n", client_address);

    pthread_mutex_lock(&infection_mutex);
    infected = 1;
    pthread_mutex_unlock(&infection_mutex);
    
    close(loop_arg->client);
}

void * server_function3(void *arg)
{
    struct Server server = server_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, NOTIFY_PORT, 20);
    socklen_t slen = (socklen_t)sizeof(server.address);
    pthread_t notify_thread;

    while(1)
    {
        struct PackedArguments loop_arg;
        loop_arg.client = Accept(server.socket, (struct sockaddr*)&server.address, &slen);
        loop_arg.server = &server;

        pthread_create(&notify_thread, NULL, notify_receiver, &loop_arg);
    }
}


void * server_loop2(void *arg)
{
    struct ServerLoopArgument *loop_arg = arg;
    struct LinkedList tmp_list = linked_list_init();
    char *client_address = inet_ntoa(loop_arg->server->address.sin_addr);
    int len, matches = 0;
    short found;
    char id[45];
    printf("\nIncoming contact list from %s\n", client_address);

    //Get the list length so know how many times we have to iterate the read.
    FullRead(loop_arg->client, &len, sizeof(int));
    len = ntohl(len);
    for(int i = 0; i < len; i++)
    {
        memset(id, 0, 45);
        FullRead(loop_arg->client, id, 45);
        tmp_list.insert(&tmp_list, i, id, 45);
    }

    //Check if there's one of our ids.
    for(int i = 0; i < tmp_list.length; i++)
    {
        memset(id, 0, 45);
        strcpy(id, tmp_list.retrieve(&tmp_list, i));
        found = check_id(id, loop_arg->known_hosts);
        if(found)
        {
            matches++;
        }
    }
    if(!matches)
    {
        printf("\n***********************************************\n");
        printf("\nNo contacts with %s, we're sifu\n", client_address);
    }
    else
    {
        printf("\n***********************************************\n");
        printf("\nGot %d contacts with %s, better go home.\n", matches, client_address);
    }
    linked_list_destroy(&tmp_list);
    close(loop_arg->client);
}

void * server_function2(void *arg)
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

        pthread_create(&server_thread, NULL, server_loop2, &loop_arg);
    }
}

void send_id(struct LinkedList *mine, char* ip_addr, char* id)
{
    struct Client client = client_init(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in dst;
    socklen_t slen = (socklen_t)sizeof(dst);

    dst.sin_family = AF_INET;
    dst.sin_port = htons(ID_PORT);
    Inet_pton(AF_INET, ip_addr, &dst.sin_addr);

    if((Connect(client.socket, (struct sockaddr*)&dst, slen)) != -1)
    {
        printf("\nSending %s to %s\n", id, ip_addr);
        FullWrite(client.socket, id, 45);
    }

    close(client.socket);
}

void sender_id_procedure(struct LinkedList *mine, struct LinkedList *known_hosts)
{
    char ip_saver[INET_ADDRSTRLEN];
    char *id;
    id = random_string(45);
    mine->insert(mine, mine->length, id, 45);

    for(int i = 0; i < known_hosts->length; i++)
    {
        memset(ip_saver, 0, INET_ADDRSTRLEN);
        strcpy(ip_saver, (char*)known_hosts->retrieve(known_hosts, i));
        send_id(mine, ip_saver, id);
    }
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
        printf("\nSending list to: %s\n", ip);
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

void sender_list_procedure(struct LinkedList *known_hosts, struct LinkedList *their)
{
    printf("\n**************************************************\n");
    printf("\nInfected\n");
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

void join_network(char* server_ip, struct LinkedList *known_hosts)
{
    struct Client client = client_init(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in serv_addr;
    socklen_t slen = (socklen_t)sizeof(serv_addr);
    char ip_saver[INET_ADDRSTRLEN];
    int len;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(CONNECTION_PORT);
    Inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);

    if((Connect(client.socket, (struct sockaddr*)&serv_addr, slen)) != -1)
    {   
        printf("\nConnected to randezvous\n");
        FullRead(client.socket, &len, sizeof(int));
        len = ntohl(len);

        if(len == -1)
        {
            printf("\nNo peers at the moment, wait for incoming connections..\n");
        }
        else
        {
            for(int i = 0; i < len; i++)
            {
                memset(ip_saver, 0, INET_ADDRSTRLEN);
                FullRead(client.socket, ip_saver, INET_ADDRSTRLEN);
                known_hosts->insert(known_hosts, known_hosts->length, ip_saver, INET_ADDRSTRLEN);
                printf("\nAdded peer #%d: %s\n",known_hosts->length, ip_saver);   
            }
        }
    }
    close(client.socket);
}


int main(int argc, char **argv)
{
    struct LinkedList known_hosts = linked_list_init();
    struct LinkedList mine_ids = linked_list_init();
    struct LinkedList their_ids = linked_list_init();
    
    pthread_t thread1, thread2, thread3;

    pthread_mutex_lock(&their_list_mutex);
    their_global = &their_ids;
    pthread_mutex_unlock(&their_list_mutex);

    join_network(SRV_IP, &known_hosts);

    pthread_create(&thread1, NULL, server_function, &known_hosts);
    pthread_create(&thread2, NULL, server_function2, &mine_ids);
    pthread_create(&thread3, NULL, server_function3, NULL);

    while(1)
    {
        if(!infected)
        {
           sender_id_procedure(&mine_ids, &known_hosts);
           sleep(5);
        }
        else
        {
            sender_list_procedure(&known_hosts, &their_ids);
        }
    }

    linked_list_destroy(&known_hosts);
    linked_list_destroy(&mine_ids);
    linked_list_destroy(&their_ids);
    exit(EXIT_SUCCESS);
}