#include <stdio.h>
#include <pthread.h>
#include <time.h>

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *log_file;

void LogggerInit()
{
    log_file = fopen("log.txt", "a");
    if (!log_file)
    {
        printf("Error opening log file\n");
        return;
    }
}

void Log(const char *message, char *file)
{
    pthread_mutex_lock(&file_mutex);

    time_t current_time;
    struct tm *local_time;
    char date_string[20];

    time(&current_time);
    local_time = localtime(&current_time);
    strftime(date_string, sizeof(date_string), "%m/%d/%Y, %H:%M", local_time);
    fprintf(log_file, "%s", date_string);

    fprintf(log_file, ", %s %s\n", message, file);
    fflush(log_file);
    pthread_mutex_unlock(&file_mutex);
}

void LoggerCleanup()
{
    pthread_mutex_destroy(&file_mutex);
    fclose(log_file);
}