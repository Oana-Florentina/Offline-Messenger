#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>
#include "database_functions.h"
#define ever \
    ;        \
    ;
#define PORT 3001

extern int errno;

typedef struct thread_data
{
    int thread_id;
    int client_fd;
} thread_data;

static void *treat(void *);
void raspunde(void *);

int main()
{

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int server_fd;
    pthread_t threads[100];
    int thread_number = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[Server] Eroare la socket().\n");
        return errno;
    }

    int on = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server_addr, sizeof(server_addr));
    bzero(&client_addr, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("[Server] Eroare la bind().\n");
        return errno;
    }

    if (listen(server_fd, 5) == -1)
    {
        perror("[Server] Eroare la listen().\n");
        return errno;
    }

    for (ever)
    {
        int client_fd;
        thread_data *client_data;
        socklen_t client_addr_length = sizeof(client_addr);

        printf("[Server] Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_length)) < 0)
        {
            perror("[Server] Eroare la accept().\n");
            continue;
        }

        client_data = (struct thread_data *)malloc(sizeof(struct thread_data));
        client_data->thread_id = thread_number++;
        client_data->client_fd = client_fd;

        pthread_create(&threads[thread_number], NULL, &treat, client_data);
    }
}

static void *treat(void *args)
{
    struct thread_data client_data;
    client_data = *((struct thread_data *)args);
    printf("[Thread-%d] Asteptam mesajul...\n", client_data.thread_id);
    fflush(stdout);

    pthread_detach(pthread_self());
    raspunde((struct thread_data *)args);

    // close(client_data.client_fd);
    return NULL;
};

void raspunde(void *args)
{
    // citeste mesajul de la client si ii trimite "Cool Message Bro: {message_received}"
    // comunicarea se intampla altfel numar caracter si dupa mesajul

    struct thread_data client_data;
    client_data = *((struct thread_data *)args);
    int user_id = 0;
    char message[1000];
    int message_len;
    char message_response[1000];
    int message_response_len;
    while (1)
    {
        // request

        message_response[0] = '\0';
        if (read(client_data.client_fd, &message_len, sizeof(int)) <= 0)
        {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la read() de la client.\n");
            return;
        }

        if (read(client_data.client_fd, message, sizeof(char) * message_len) <= 0)
        {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la read() de la client.\n");
            return;
        }
        message[message_len] = '\0'; // IMPORTANT

        printf("[Thread-%d] Server received message: %s\n", client_data.thread_id, message);
        if (strcmp(message, "exit") == 0)
        {
            printf("[Thread-%d] Exiting.\n", client_data.thread_id);
            break;
        }
        else if (strncmp(message, "register", 8) == 0 && user_id == 0)
        {
            char *token = strtok(message, " ");
            char *username = strtok(NULL, " ");
            char *password = strtok(NULL, " ");
            if (username == NULL || password == NULL)
            {
                printf("[Thread-%d] Invalid registration command format.\n", client_data.thread_id);

                continue;
            }
            if (check_is_user_exists(username))
            {
                strcpy(message_response, "user already exists");
                message_response_len = strlen(message_response);
            }
            else if (handle_register(username, password))
            {
                strcpy(message_response, "user registered");
                message_response_len = strlen(message_response);
            }
            else
            {
                strcpy(message_response, "problem with register");
                message_response_len = strlen(message_response);
            }
        }
        else if (strncmp(message, "login", 5) == 0 && user_id == 0)
        {
            char *token = strtok(message, " ");
            char *username = strtok(NULL, " ");
            char *password = strtok(NULL, " ");
            if (username == NULL || password == NULL)
            {
                strcpy(message_response, "invalid login command format");
            }
            else
            {
                user_id = handle_login(username, password);
                if (user_id > 0)
                {
                    strcpy(message_response, "user logged in");
                    message_response_len = strlen(message_response);
                    handle_see_unseen_messages(&message_response, &message_response_len, user_id);
                    update_unseen_messages(user_id);       
                }
                else
                {
                    strcpy(message_response, "user or password incorrect");
                    message_response_len = strlen(message_response);
                    user_id = 0; // user didn't log in
                }
            }
        }
        else if (strncmp(message, "see users", 9) == 0 && user_id > 0)
        {
            handle_see_users(message_response, message_response_len, user_id);
        }
        else if (strncmp(message, "send message to", 20) == 0 && user_id > 0)
        {
            handle_send_message(message_response, message_response_len, message, user_id);
        }
        

        else
        {
            strcpy(message_response, "invalid command");
            message_response_len = strlen(message_response);
        }

        // response
       // strcat(message_response, " Cool Message Bro: ");
        message_response_len = strlen(message_response);

        printf("[Thread-%d] Server send message: %s\n", client_data.thread_id, message_response);

        if (write(client_data.client_fd, &message_response_len, sizeof(int)) <= 0)
        {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la write() de la client.\n");
            return;
        }
        if (write(client_data.client_fd, message_response, sizeof(char) * message_response_len) <= 0)
        {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la write() de la client.\n");
            return;
        }
        message[message_len] = '\0';
    }

    printf("[Thread-%d] Server finishing working with the client.\n", client_data.thread_id);
}
