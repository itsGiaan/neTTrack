#include "client.h"
#include "utils.h"
#include "wrapper.h"

#define SRV_IP "127.0.0.1"
#define PORT 4444

typedef struct
{
    int gg;
    int mm;
    int aa;
} DATA;

typedef struct
{
    char name[20];
    DATA start;
    DATA end;
} GREEN_PASS;



int main()
{
    struct Client client = client_init(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in serv_addr;
    socklen_t slen = (socklen_t)sizeof(serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    Inet_pton(AF_INET, SRV_IP, &serv_addr.sin_addr);

    GREEN_PASS gp;
    strcpy(gp.name, "Name");
    gp.start.gg = 10;
    gp.start.mm = 2;
    gp.start.aa = 2022;
    gp.end.gg = 20;
    gp.end.mm = 4;
    gp.end.aa = 2022;

    if((Connect(client.socket, (struct sockaddr*)&serv_addr, slen)) != -1)
    {
        FullWrite(client.socket, &gp, sizeof(GREEN_PASS));
    }

    printf("\nSended\n");
    exit(EXIT_SUCCESS);
}