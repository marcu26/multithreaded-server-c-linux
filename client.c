#pragma region includes

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "comune.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <ctype.h>

#pragma endregion

int socket_desc;
struct sockaddr_in server_addr;

int Initialize()
{

    socket_desc = socket(AF_INET, SOCK_STREAM, TCP);

    if (socket_desc < 0)
    {
        printf("Unable to create socket\n");
        return -1;
    }

    printf("Socket created successfully\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");
    return 1;
}

#pragma region Functii

void PrintOptions()
{
    printf("1. LIST\n");
    printf("2. GET;<filename>\n");
    printf("3. DELETE;<filename>\n");
    printf("4. PUT;<filename>\n");
    printf("5. UPDATE;<filename>;<start octet>;<dim>;<continut>\n");
    printf("6. SEARCH;<word>\n");
}

int List()
{
    char server_message[2048];
    memset(server_message, '\0', sizeof(server_message));

    // Trimit pentru a primi status
    send(socket_desc, "aaa", 3, 0);

    // status
    recv(socket_desc, server_message, sizeof(server_message), 0);
    send(socket_desc, "aaa", 3, 0);
    printf("%s; ", server_message);
    memset(server_message, '\0', sizeof(server_message));

    // primesc number of files
    recv(socket_desc, server_message, sizeof(server_message), 0);
    int numberOfBytes = atoi(server_message);
    memset(server_message, '\0', sizeof(server_message));
    printf("%d; ", numberOfBytes);

    // anunt ca am primit
    send(socket_desc, "aaa", 3, 0);

    int i = 0;
    printf("\n");
    while (i <= numberOfBytes)
    {
        memset(server_message, '\0', 15);
        recv(socket_desc, server_message, 10, 0);
        printf("%s", server_message);
        send(socket_desc, "aaa", 3, 0);
        i += 10;
    }
    return 1;
}

int Get()
{
    char server_message[1024];
    memset(server_message, '\0', sizeof(server_message));
    send(socket_desc, "aaa", 3, 0);

    // Status

    recv(socket_desc, server_message, sizeof(server_message), 0);
    printf("%s; ", server_message);

    // Number of bytes
    send(socket_desc, "aaa", 3, 0);
    recv(socket_desc, server_message, sizeof(server_message), 0);

    int nrOfBytes = atoi(server_message);

    printf("%d; ", nrOfBytes);

    send(socket_desc, "aaa", 3, 0);

    for (int i = 0; i < nrOfBytes; i += 1024)
    {
        char message[1024];
        memset(message, '\0', sizeof(message));
        recv(socket_desc, message, 1024, 0);
        printf("%s", message);
        send(socket_desc, "aaa", 3, 0);
    }

    return 1;
}

void Delete()
{
    char server_message[1024];
    memset(server_message, '\0', sizeof(server_message));
    send(socket_desc, "aaa", 3, 0);

    recv(socket_desc, server_message, sizeof(server_message), 0);
    printf("%s; ", server_message);
    send(socket_desc, "aaa", 3, 0);
}

void Update()
{
    char server_message[1024];
    memset(server_message, '\0', sizeof(server_message));
    send(socket_desc, "aaa", 3, 0);

    recv(socket_desc, server_message, sizeof(server_message), 0);
    printf("%s; ", server_message);
    send(socket_desc, "aaa", 3, 0);
}

int Put()
{
    send(socket_desc, "aaa", 3, 0);

    char file[100];
    memset(file, '\0', 100);
    recv(socket_desc, file, sizeof(file), 0);

    int file_fd = open(file, O_RDONLY);
    struct stat stat;

    if (file < 0)
    {
        printf("File Not Found!\n");
        send(socket_desc, "File not found", 14, 0);

        return -1;
    }

    fstat(file_fd, &stat);
    char buff[10];
    int res = stat.st_size;

    // Dimensiunea fisier
    char fileSize[10];
    snprintf(fileSize, 10, "%d", res);
    send(socket_desc, fileSize, sizeof(fileSize), 0);
    recv(socket_desc, buff, 10, 0);
    off_t offset;

    for (int i = 0; i < res; i += 1024)
    {
        char message[1024];
        memset(message, '\0', 1024);
        sendfile(socket_desc, file_fd, &offset, 1024);
        recv(socket_desc, buff, 10, 0);
    }

    close(file_fd);
    return 1;
}

#pragma endregion

#pragma region SendAndListen

void SendMessage()
{

    PrintOptions();

    char client_message[2048];
    memset(client_message, '\0', sizeof(client_message));
    printf("Enter message: ");
    fgets(client_message, 2048, stdin);

    if (send(socket_desc, client_message, strlen(client_message), 0) < 0)
    {
        printf("Unable to send message\n");
        return;
    }
}

int ListenForMessage()
{

    char server_message[2048];
    memset(server_message, '\0', sizeof(server_message));

    if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0)
    {
        printf("Error while receiving server's msg\n");
        return -1;
    }

    printf("\nServer message: %s\n", server_message);

    if (server_message[0] == '1')
    {
        List();
        return 1;
    }
    if (server_message[0] == '2')
    {
        Get();
        return 1;
    }
    if (server_message[0] == '3')
    {
        Put();
        return 1;
    }
    if (server_message[0] == '4')
    {
        Delete();
        return 1;
    }
    if (server_message[0] == '6')
    {
        List();
        return 1;
    }
    if (server_message[0] == '5')
    {
        Update();
    }
    else
    {
        return -1;
    }
}

#pragma endregion

int main(void)
{
    int test = Initialize();

    if (test == 1)
    {

        SendMessage();
        int a = ListenForMessage();

        printf("\n");
    }

    return 0;
}