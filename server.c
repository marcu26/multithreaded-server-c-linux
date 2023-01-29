#pragma region Includes

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
#include "linkedlist.h"
#include "logger.h"

#pragma endregion

#pragma region Variables

#define MAX_WORD_LEN 100

typedef struct
{
    char word[MAX_WORD_LEN];
    int count;
} WordCount;

struct params
{
    int client_socket;
    int index;
};

struct listOfFiles
{
    char files[100][100];
    char wordFromFiles[100][10][100];
    int numberOfFiles;
    int numberOfBytes;
};

int indexOperations;

typedef struct listOfFiles listOfFiles;
typedef struct params params;

listOfFiles list; 

__thread int client_socket_fd;
__thread int client_index;

int server_socket_fd;
struct sockaddr_in server_addr;
int isStillRunning;
int nrOfClients;

pthread_t update_thread; 
pthread_t thread_handle; 

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Node *head = NULL;

#pragma endregion

#pragma region Signals

static void sig_handler(int signum)
{
    switch (signum)
    {
    case SIGINT:
        printf("\nInchidem server...\n");
        isStillRunning = 0;
        LoggerCleanup();

        if (nrOfClients == 0)
        {
            pthread_cancel(thread_handle);
            pthread_cancel(update_thread);
        }
        close(server_socket_fd);
        break;
    }
}

void SignalHandler()
{
    struct sigaction signals;
    sigset_t mask;

    sigfillset(&mask);

    memset(&signals, 0, sizeof(struct sigaction));
    signals.sa_mask = mask;

    signals.sa_handler = sig_handler;

    sigaction(SIGINT, &signals, NULL);
}

#pragma endregion

#pragma region Utils

int Compare(const void *a, const void *b)
{
    return ((WordCount *)b)->count - ((WordCount *)a)->count;
}

int GetTopTenWords(char *filename, int index)
{
    FILE *file;
    char word[MAX_WORD_LEN];
    WordCount *wordCounts;
    int wordCount = 0;
    int i;
    wordCounts = (WordCount *)calloc(100000, sizeof(WordCount));

    char fullpath[1000];
    strcpy(fullpath, "./files/");
    strcat(fullpath, filename);
    file = fopen(fullpath, "r");

    if (file == NULL)
    {
        printf("Error opening file.\n");
        return 1;
    }

    while (fscanf(file, "%s", word) == 1)
    {

        for (i = 0; i < strlen(word); i++)
        {
            word[i] = tolower(word[i]);
        }

        for (i = 0; i < wordCount; i++)
        {
            if (strcmp(word, wordCounts[i].word) == 0)
            {
                wordCounts[i].count++;
                break;
            }
        }

        if (i == wordCount)
        {
            strcpy(wordCounts[wordCount].word, word);
            wordCounts[wordCount].count = 1;
            wordCount++;
        }
    }

    qsort(wordCounts, wordCount, sizeof(WordCount), Compare);

    for (i = 0; i < 10; i++)
    {
        // printf("%s: %d\n", wordCounts[i].word, wordCounts[i].count);
        strcpy(list.wordFromFiles[index][i], wordCounts[i].word);
    }

    fclose(file);

    return 0;
}

void UpdateListRecursive(char *folder)
{
    DIR *directory;
    struct dirent *file;
    char buff[10];
    int numberOfBytes = 0;
    list.numberOfFiles = 0;
    list.numberOfBytes = 0;

    directory = opendir(folder);

    if (directory)
    {
        while ((file = readdir(directory)) != NULL)
        {
            if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
            {

                if (file->d_type == DT_DIR)
                {
                    char newPath[1000];
                    memset(newPath, '\0', 1000);
                    strcpy(newPath, folder);
                    strcat(newPath, "/");
                    strcat(newPath, file->d_name);
                    UpdateListRecursive(newPath);
                }
                else
                {
                    char filePath[1000];
                    memset(filePath, '\0', 1000);
                    if (strlen(folder) >= 8)
                    {
                        strcpy(filePath, folder + 8);
                        strcat(filePath, "/");
                    }
                    strcat(filePath, file->d_name);
                    strcpy(list.files[list.numberOfFiles], filePath);
                    GetTopTenWords(filePath, list.numberOfFiles);
                    list.numberOfFiles += 1;
                    list.numberOfBytes += strlen(list.files[list.numberOfFiles]);
                }
            }
        }
        closedir(directory);
    }
}

void *UpdateList()
{
    while (isStillRunning == 1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);

        char *folder = "./files";
        UpdateListRecursive(folder);
        sleep(0.5);
        pthread_mutex_unlock(&mutex);
    }
}

void FirstUpdate()
{
    char *folder = "./files";
    UpdateListRecursive(folder);
    sleep(0.5);
}

char **GetComanda(char string[2048], int *nrWords)
{
    char **comanda = (char **)malloc(5 * sizeof(char *));
    int _index = 0;
    char *token = strtok(string, ";\n\t\0");

    comanda[_index] = strdup(token);

    _index++;

    while (_index < 6 && token != NULL)
    {
        token = strtok(NULL, ";\n\0\t");
        if (token == NULL)
        {
            break;
        }
        comanda[_index] = strdup(token);
        _index++;
    }

    *nrWords = _index;

    return comanda;
}

int IsWordInFile(char *word, int index)
{
    for (int i = 0; i < 10; i++)
    {
        if (strcmp(list.wordFromFiles[index][i], word) == 0)
        {
            return 1;
        }
    }
    return -1;
}

#pragma endregion

int Initialize()
{
    server_socket_fd = socket(AF_INET, SOCK_STREAM, TCP);
    nrOfClients = 0;
    list.numberOfBytes = 0;
    list.numberOfFiles = 0;
    FirstUpdate();
    indexOperations = 0;
    isStillRunning = 1;
    LogggerInit();

    // Pentru test daca asteapta dupa eliberare resursa

    // append(&head,"file","UPDATE",indexOperations);
    // indexOperations++;

    if (server_socket_fd < 0)
    {
        printf("%s", "Nu se poate crea socketul\n");
        return -1;
    }

    printf("%s", "Socket creat\n");

    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("%s", "Nu se poate face legatura cu portul\n");
        return -1;
    }

    if (listen(server_socket_fd, MAX_CLIENTS) < 0)
    {
        printf("Eroare, nu se poate asculta\n");
        return -1;
    }
    printf("Ascultand daca vin conexiuni.....\n");

    return 1;
}

#pragma region Functions

int SendFileList()
{
    // Operatia
    char buff[10];

    send(client_socket_fd, "1", 1, 0);
    recv(client_socket_fd, buff, 10, 0);

    // Status
    send(client_socket_fd, "Done", 4, 0);
    recv(client_socket_fd, buff, 10, 0);

    char files[10000];
    int numberOfBytes = 0;
    memset(files, '\0', 10000);

    for (int i = 0; i < list.numberOfFiles; i++)
    {
        strcat(files, list.files[i]);
        numberOfBytes += strlen(list.files[i]);
        files[numberOfBytes] = '\n';
        numberOfBytes += 1;
    }

    // Numarul de fisiere
    char numberOfBytesAsStrings[10];
    snprintf(numberOfBytesAsStrings, 10, "%d", numberOfBytes);
    send(client_socket_fd, numberOfBytesAsStrings, sizeof(numberOfBytesAsStrings), 0);
    recv(client_socket_fd, buff, 10, 0);

    // Executare operatiea
    int i = 0;
    int j = 0;
    while (i <= numberOfBytes)
    {
        char msg[10];
        memset(msg, '\0', 10);
        strncpy(msg, files + i, 10);
        send(client_socket_fd, msg, sizeof(msg), 0);
        recv(client_socket_fd, buff, 10, 0);
        i += 10;
    }

    Log("server sent file list", "");

    return 1;
}

int SendFile(char *filename)
{
    Append(&head, filename, "GET", indexOperations);
    int myIndex = indexOperations;
    indexOperations++;

    char file[1024] = "./files/";
    strcat(file, filename);
    int file_fd = open(file, O_RDONLY);
    struct stat stat;

    if (file < 0)
    {
        printf("File Not Found!\n");
        send(client_socket_fd, "File not found", 14, 0);

        return -1;
    }

    fstat(file_fd, &stat);
    char buff[10];
    int res = stat.st_size;

    // Tipul de operatie
    send(client_socket_fd, "2", 1, 0);
    //

    recv(client_socket_fd, buff, 10, 0);

    // Status
    send(client_socket_fd, "Done", 4, 0);
    recv(client_socket_fd, buff, 10, 0);

    // Dimensiunea fisier
    char fileSize[10];
    snprintf(fileSize, 10, "%d", res);
    send(client_socket_fd, fileSize, sizeof(fileSize), 0);
    recv(client_socket_fd, buff, 10, 0);
    off_t offset;

    while (Search(head, filename, "UPDATE") != NULL)
    {
        // wait
    }

    for (int i = 0; i < res; i += 1024)
    {
        char message[1024];
        memset(message, '\0', 1024);
        sendfile(client_socket_fd, file_fd, &offset, 1024);
        recv(client_socket_fd, buff, 10, 0);
    }

    close(file_fd);
    DeleteNode(&head, myIndex);
    indexOperations--;
    Log("Server sent file", filename);
    return 1;
}

void DeleteFile(char *filename)
{

    char file[1024] = "./files/";
    strcat(file, filename);
    int a = remove(file);
    char buff[10];

    send(client_socket_fd, "4", 1, 0);
    recv(client_socket_fd, buff, 10, 0);

    if (a == -1)
    {
        send(client_socket_fd, "Fisier inexistent!", 17, 0);
        return;
    }
    else
        send(client_socket_fd, "Done", 4, 0);

    recv(client_socket_fd, buff, 10, 0);

    Log("Server deleted file", filename);

    pthread_cond_signal(&cond);
}

int PutFile(char *file)
{
    char buff[10];
    send(client_socket_fd, "3", 1, 0);
    recv(client_socket_fd, buff, 10, 0);

    char fullpath[100];

    strcpy(fullpath, "./files/");
    strcat(fullpath, file);

    FILE *fptr = fopen(fullpath, "w");

    send(client_socket_fd, file, sizeof(file), 0);
    char client_message[100];
    memset(client_message, '\0', 100);

    // Number of bytes
    recv(client_socket_fd, client_message, sizeof(client_message), 0);

    int nrOfBytes = atoi(client_message);

    printf("%d; ", nrOfBytes);
    send(client_socket_fd, "aaa", 3, 0);

    for (int i = 0; i < nrOfBytes; i += 1024)
    {
        char message[1024];
        memset(message, '\0', sizeof(message));
        recv(client_socket_fd, message, 1024, 0);
        fprintf(fptr, "%s", message);
        send(client_socket_fd, "aaa", 3, 0);
    }

    fclose(fptr);

    Log("Server put file", file);

    pthread_cond_signal(&cond);

    return 1;
}

void SearchWord(char *word)
{
    char buff[10];

    send(client_socket_fd, "6", 1, 0);
    recv(client_socket_fd, buff, 10, 0);

    // Status
    send(client_socket_fd, "Done", 6, 0);
    recv(client_socket_fd, buff, 10, 0);

    char files[10000];
    int numberOfBytes = 0;
    memset(files, '\0', 10000);

    for (int i = 0; i < list.numberOfFiles; i++)
    {
        if (IsWordInFile(word, i) == 1)
        {
            strcat(files, list.files[i]);
            strcat(files, "\n");
            numberOfBytes += strlen(list.files[i]);
            numberOfBytes += 1;
        }
    }

    // Numarul de fisiere
    char numberOfBytesAsStrings[10];
    snprintf(numberOfBytesAsStrings, 10, "%d", numberOfBytes);
    send(client_socket_fd, numberOfBytesAsStrings, sizeof(numberOfBytesAsStrings), 0);
    recv(client_socket_fd, buff, 10, 0);

    // Executare operatiea
    int i = 0;
    int j = 0;
    while (i <= numberOfBytes)
    {
        char msg[10];
        memset(msg, '\0', 10);
        strncpy(msg, files + i, 10);
        send(client_socket_fd, msg, sizeof(msg), 0);
        recv(client_socket_fd, buff, 10, 0);
        i += 10;
    }

    Log("Server searched for word", word);
}

int UpdateFile(char *file_name, char *s_start, char *s_dim, char *chars)
{
    Append(&head, file_name, "UPDATE", indexOperations);
    int myIndex = indexOperations;
    indexOperations++;

    send(client_socket_fd, "5", 1, 0);
    char buff[10];
    recv(client_socket_fd, buff, 10, 0);

    int start = atoi(s_start);
    int dim = atoi(s_dim);

    char fullpath[100];
    strcpy(fullpath, "./files/");
    strcat(fullpath, file_name);
    FILE *file = fopen(fullpath, "r+");
    if (!file)
    {
        return -1;
    }

    if (fseek(file, start, SEEK_SET) != 0)
    {
        fclose(file);
        return -2;
    }

    while (Search(head, file_name, "UPDATE") != NULL && Search(head, file_name, "GET") != NULL)
    {
        // wait
    }

    size_t bytes_written = fwrite(chars, 1, dim, file);
    fclose(file);
    send(client_socket_fd, "Done", 4, 0);

    Log("Server updated file", file_name);

    pthread_cond_signal(&cond);

    return 1;
}

#pragma endregion

#pragma region treating client

void Execute(char client_message[2048])
{
    int size = 0;
    char **comanda = GetComanda(client_message, &size);

    if (strncmp(comanda[0], "LIST", 4) == 0)
    {
        if (size != 1)
        {
            send(client_socket_fd, "Input incorect", 15, 0);
            return;
        }
        SendFileList();
    }
    else if (strncmp(comanda[0], "GET", 3) == 0)
    {
        if (size != 2)
        {
            send(client_socket_fd, "Input incorect", 15, 0);
            return;
        }
        SendFile(comanda[1]);
    }
    else if (strncmp(comanda[0], "DELETE", 6) == 0)
    {
        if (size != 2)
        {
            send(client_socket_fd, "Input incorect", 11, 0);
            return;
        }
        DeleteFile(comanda[1]);
    }
    else if (strncmp(comanda[0], "PUT", 3) == 0)
    {
        if (size != 2)
        {
            send(client_socket_fd, "Input incorect", 11, 0);
            return;
        }
        PutFile(comanda[1]);
    }
    else if (strncmp(comanda[0], "SEARCH", 6) == 0)
    {
        if (size != 2)
        {
            send(client_socket_fd, "Input incorect", 11, 0);
            return;
        }
        SearchWord(comanda[1]);
    }
    else if (strncmp(comanda[0], "UPDATE", 6) == 0)
    {
        if (size != 5)
        {
            send(client_socket_fd, "Input incorect", 11, 0);
            return;
        }
        UpdateFile(comanda[1], comanda[2], comanda[3], comanda[4]);
    }
    else
    {
        char server_message[] = "Comanda necunosuta";
        send(client_socket_fd, server_message, strlen(server_message), 0);

        printf("Clientul %d a trimis o comanda necunoscuta\n", client_index);
    }

    printf("Clientul %d se deconecteaza de la server\n", client_index);
}

void *HandleClient(void *parameters)
{
    params *p = (struct params *)parameters;

    client_socket_fd = p->client_socket;
    client_index = p->index;

    char client_message[2048];
    memset(client_message, '\0', sizeof(client_message));

    if (recv(client_socket_fd, client_message, sizeof(client_message), 0) < 0)
    {
        printf("Couldn't receive\n");
        return NULL;
    }

    printf("Msg from client %d: %s\n", client_index, client_message);

    Execute(client_message);

    close(client_socket_fd);

    nrOfClients--;

    if (nrOfClients == 0 && isStillRunning == 0)
    {
        pthread_cancel(thread_handle);
        pthread_cancel(update_thread);
    }
}

void *HandleIncomingConnections()
{
    while (isStillRunning == 1)
    {
        struct sockaddr_in client_addr;
        int client_size = sizeof(client_addr);

        int client_sock = accept(server_socket_fd, (struct sockaddr *)&client_addr, &client_size);

        if (isStillRunning == 1)
        {

            if (client_sock < 0)
            {
                printf("Can't accept\n");
                break;
            }
            printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            pthread_t tid;
            struct params p;
            p.client_socket = client_sock;
            p.index = nrOfClients;
            nrOfClients++;

            pthread_create(&tid, NULL, HandleClient, &p);
        }
        else
        {
            printf("\nNu mai primim conexiuni\n");
            send(client_sock, "Server oprit", 12, 0);
        }
    }
}


#pragma endregion

int main(int argc, char const *argv[])
{
    SignalHandler();

    int test = Initialize();

    if (test == 1)
    {

        pthread_create(&update_thread, NULL, UpdateList, NULL);
        pthread_create(&thread_handle, NULL, HandleIncomingConnections, NULL);

        pthread_join(update_thread, NULL);
        pthread_join(thread_handle, NULL);
    }

    
}