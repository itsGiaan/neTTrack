#include "LinkedList.h"
#include "server.h"
#include "client.h"
#include "wrapper.h"
#include "utils.h"

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void * server_loop(void *arg)
{
    struct ServerLoopArgument *loop_arg = arg;
    int len;
    len = loop_arg->known_hosts->length;
    short found, flag;
    FILE *file_pointer;
    char *client_address = inet_ntoa(loop_arg->server->address.sin_addr);
    char ip_saver[INET_ADDRSTRLEN];

    if(!len)
    {
        //len = htonl(len);
        len = -1;
        len = htonl(len);
        FullWrite(loop_arg->client, &len, sizeof(int));
        loop_arg->known_hosts->insert(loop_arg->known_hosts, loop_arg->known_hosts->length, client_address, INET_ADDRSTRLEN);
        printf("\nSuccessfully added\n");
        pthread_mutex_lock(&file_mutex);
        file_pointer = fopen("hosts.dat", "a+");
        fwrite(client_address, INET_ADDRSTRLEN, 1, file_pointer);
        fclose(file_pointer);
        pthread_mutex_unlock(&file_mutex);
        len = ntohl(len);
    }
    else
    {
        found = isKnowed(client_address, loop_arg->known_hosts);
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
            pthread_mutex_lock(&file_mutex);
            file_pointer = fopen("hosts.dat", "a+");
            fwrite(client_address, INET_ADDRSTRLEN, 1, file_pointer);
            fclose(file_pointer);
            pthread_mutex_unlock(&file_mutex);
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
                strcpy(ip_saver, (char*)new_list.retrieve(&new_list, i));
                FullWrite(loop_arg->client, ip_saver, INET_ADDRSTRLEN);
            }

            linked_list_destroy(&new_list);
        }
    }
    close(loop_arg->client);
}

void * server_function(void *arg)
{
    struct Server server = server_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, CONNECTION_PORT, 20);
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

void send_notify(char *client_address)
{
    struct Client client = client_init(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in dst;
    socklen_t slen = (socklen_t)sizeof(dst);

    dst.sin_family = AF_INET;
    dst.sin_port = htons(NOTIFY_PORT);
    Inet_pton(AF_INET, client_address, &dst.sin_addr);

    if((connect(client.socket, (struct sockaddr*)&dst, slen)) != -1)
    {
        printf("\nInfection notify sended to %s\n", client_address);
    }
    else
    {
        printf("\nHost unreachable\n");
    }
    close(client.socket);
}

int main()
{
    struct LinkedList known_hosts = linked_list_init();
    pthread_t server_thread;
    int infect;
    char *client_address;
    char ip_saver[INET_ADDRSTRLEN];
    FILE *fp;
    fp = fopen("hosts.dat", "r+");

    if(fp != NULL)
    {
        while(fread(ip_saver, INET_ADDRSTRLEN, 1, fp) > 0)
        {
            known_hosts.insert(&known_hosts, known_hosts.length, ip_saver, INET_ADDRSTRLEN);
            memset(ip_saver, 0, INET_ADDRSTRLEN);
        }
        fclose(fp);
    }

    printf("\nRandezvous server running...\n");
    printf("\nCurrent peers: %d\n", known_hosts.length);
    pthread_create(&server_thread, NULL, server_function, &known_hosts);

    while(1)
    {
        if(known_hosts.length > 3)
        {
            infect = rand()%known_hosts.length;
            client_address = (char*)known_hosts.retrieve(&known_hosts, infect);
            send_notify(client_address);
            sleep(5);
        }
    }

    linked_list_destroy(&known_hosts);
    exit(EXIT_SUCCESS);
}