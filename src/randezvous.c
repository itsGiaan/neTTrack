#include "LinkedList.h"
#include "server.h"
#include "client.h"
#include "wrapper.h"
#include "utils.h"

#define NOTIFY_PORT 4446

short isKnown(char* client_address, struct LinkedList *list);
void * server_loop(void *arg);
void * server_function(void *arg);

int main()
{
    struct LinkedList known_hosts = linked_list_init();
    pthread_t server_thread;
    int infected;
    char *client_address;
    char ip_saver[INET_ADDRSTRLEN];
    File *fp;
    fp = fopen("hosts.dat", "r+");

    if(fp != NULL)
    {
        while(fread(&ip_saver, INET_ADDRSTRLEN, 1, fp) > 0)
        {
            known_hosts.insert(&known_hosts, known_hosts.length, ip_saver, INET_ADDRSTRLEN);
        }
        fclose(fp);
    }

    printf("\nRandezvous server running...\n");
    pthread_create(&server_thread, NULL, server_function, &known_hosts);

    while(1)
    {
        if(known_hosts.length > 1)
        {
            infected = rand()%known_hosts.length;
            client_address = (char*)known_hosts.retrieve(&known_hosts, infected);
            send_notify(client_address, NOTIFY_PORT);
        }
    }

    linked_list_destroy(&known_hosts);
    exit(EXIT_SUCCESS);
}

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

void * server_loop(void *arg)
{
    struct ServerLoopArguments *loop_arg = arg;
    int len = loop_arg->known_hosts->length;
    short found, flag;
    char *client_address = inet_ntoa(loop_arg->server->address.sin_addr);
    char ip_saver[INET_ADDRSTRLEN];

    if(!len)
    {
        //len = htonl(len);
        flag = -1;
        flag = htons(flag);
        FullWrite(loop_arg->client, &flag, sizeof(short));
        close(loop_arg->client);
        loop_arg->known_hosts->insert(loop_arg->known_hosts, loop_arg->known_hosts->length, client_address, INET_ADDRSTRLEN);
        flag = ntohs(flag);
    }
    else
    {
        found = isKnown(client_address, loop_arg->known_hosts);
        if(!found)
        {
            len = htonl(len);
            FullWrite(loop_arg->client, &len, sizeof(int));
            len = ntohl(len);
            for(int i = 0; i < len; i++)
            {
                memset(ip_saver, 0, INET_ADDRSTRLEN);
                strcpy(ip_saver, (char*)loop_arg->known_hosts->retrieve(loop_arg->known_hosts, i));
                FullWrite(loop_arg->client, ip_saver, INET_ADDRSTRLEN);
            }
            loop_arg->known_hosts->insert(loop_arg->known_hosts, loop_arg->known_hosts->length, client_address, INET_ADDRSTRLEN);
            printf("\nClient: %s added\n", ip_saver);
        }
        else
        {
            //Since we already known that peer, we won't make him able to get his own endpoint.
            struct LinkedList new_list = linked_list_init();
            for(int i = 0; i < len; i++)
            {
                memset(ip_saver, 0, INET_ADDRSTRLEN);
                strcpy(ip_saver, (char*)loop_arg->known_hosts->retrieve(loop_arg->known_hosts, i));
                new_list.insert(&new_list, i, ip_saver, INET_ADDRSTRLEN);
            }
            int new_len = new_list.length;
            new_len = htonl(new_len);
            FullWrite(loop_arg->client, &new_len, sizeof(int));
            new_len = ntohl(new_len);

            for(int i = 0; i < new_len; i++)
            {
                memset(ip_saver, 0, INET_ADDRSTRLEN);
                strcpy(ip_saver, (char*)new_list->retrieve(new_list, i));
                FullWrite(loop_arg->client, ip_saver, INET_ADDRSTRLEN);
            }

            linked_list_destroy(&new_list);
        }
    }
    close(loop_arg->client);
}

void * server_function(void *arg)
{
    struct Server server = server_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, 20);
    socklen_t slen = (socklen_t)sizeof(server.address);
    pthread_t loop_thread;
    printf("\nServer function running..\n");
    while(1)
    {
        struct ServerLoopArgument loop_arg;
        loop_arg.client = Accept(server.socket, (struct sockaddr*)&server.address, &slen);
        loop_arg.server = &server;
        loop_arg.known_hosts = arg;

        pthread_create(&loop_thread, NULL, server_loop, &loop_arg);
    }
}