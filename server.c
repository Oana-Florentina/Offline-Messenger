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
int check_is_user_exists(char *username)
{
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK)
    {
        return 0;
    }
    char *sql = "SELECT * FROM users WHERE username = ?";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    else
    {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
}
int handle_register(char *username, char *password)
{
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK)
    {
        return 0;
    }

    char *sql = "INSERT INTO users (username, password) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, strlen(password), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        return 0;
    }
    sqlite3_finalize(stmt);
    return 1;
}
int handle_login(char *username, char *password)
{
    int user_id = -1;
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK)
    {
        return 0;
    }
    char *sql = "SELECT id FROM users WHERE username = ? AND password = ?";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, strlen(password), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        user_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return user_id;
}
void handle_see_users(char message_response[1000], int message_response_len, int user_id)
{
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK)
    {
        perror("Cannot open database");
    }
    char *sql = "SELECT username FROM users";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        perror("Cannot prepare statement");
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        strcpy(message_response, "users: ");

        // Fetch the first username
        char *username = sqlite3_column_text(stmt, 0);
        strcat(message_response, username);

        // Fetch subsequent usernames
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            strcat(message_response, ", ");
            username = sqlite3_column_text(stmt, 0);
            strcat(message_response, username);
        }

        message_response_len = strlen(message_response);
    }
    else
    {
        strcpy(message_response, "no users");
        message_response_len = strlen(message_response);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
void handle_send_message(char message_response[1000], int message_response_len, char message[100], int user_id)
{
    char *token = strtok(message, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    printf("Username before query: '%s'\n", username);

    char *message_to_send = strtok(NULL, " ");
    if (username == NULL || message_to_send == NULL)
    {
        strcpy(message_response, "invalid send message command format");
        message_response_len = strlen(message_response);
    }
    else
    {
        sqlite3 *db;
        int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
        if (rc != SQLITE_OK)
        {
            perror("Cannot open database");
            return; // Exit or handle the error appropriately
        }

        // Query to get user_id_to_send based on username
        char *select_sql = "SELECT id FROM users WHERE username = ? COLLATE NOCASE";

        sqlite3_stmt *select_stmt;
        rc = sqlite3_prepare_v2(db, select_sql, -1, &select_stmt, NULL);
        if (rc != SQLITE_OK)
        {
            perror("Cannot prepare SELECT statement");
            sqlite3_close(db);
            return; // Exit or handle the error appropriately
        }
        sqlite3_bind_text(select_stmt, 1, username, strlen(username), SQLITE_STATIC);

        rc = sqlite3_step(select_stmt);
        if (rc == SQLITE_ROW)
        {
            int user_id_to_send = sqlite3_column_int(select_stmt, 0);
            printf("user_id_to_send: %d\n", user_id_to_send);

            // Reset the statement for the second query
            rc = sqlite3_reset(select_stmt);
            if (rc != SQLITE_OK)
            {
                perror("Cannot reset SELECT statement");
                sqlite3_finalize(select_stmt);
                sqlite3_close(db);
                return; // Exit or handle the error appropriately
            }

            // Query to insert the message
            char *insert_sql = "INSERT INTO Messages (SENDER, RECEIVER, MESSAGE, SEEN) VALUES (?, ?, ?, 0)";
            sqlite3_stmt *insert_stmt;
            rc = sqlite3_prepare_v2(db, insert_sql, -1, &insert_stmt, NULL);
            if (rc != SQLITE_OK)
            {
                perror("Cannot prepare INSERT statement");
                sqlite3_finalize(select_stmt);
                sqlite3_close(db);
                return; // Exit or handle the error appropriately
            }

            sqlite3_bind_text(insert_stmt, 3, message_to_send, strlen(message_to_send), SQLITE_STATIC);
            sqlite3_bind_int(insert_stmt, 2, user_id_to_send); // Receiver
            sqlite3_bind_int(insert_stmt, 1, user_id);         // Sender

            rc = sqlite3_step(insert_stmt);
            if (rc != SQLITE_DONE)
            {
                perror("Cannot execute INSERT statement");
            }
            else
            {
                strcpy(message_response, "message sent");
                message_response_len = strlen(message_response);
            }

            // Finalize the statements
            sqlite3_finalize(select_stmt);
            sqlite3_finalize(insert_stmt);
        }
        else
        {
            perror("Cannot execute SELECT statement");
            strcpy(message_response, "user does not exist");
            message_response_len = strlen(message_response);
        }

        sqlite3_close(db);
    }
}
void handle_see_unseen_messages(char message_response[1000], int *message_response_len, int user_id)
{
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK)
    {
        perror("Cannot open database");
        return; // Exit or handle the error appropriately
    }

    // Query to get the unseen messages
    char *select_sql = "SELECT Messages.id, users.username, Messages.message FROM Messages INNER JOIN users ON Messages.sender = users.id WHERE receiver = ? AND seen = 0";

    sqlite3_stmt *select_stmt;
    rc = sqlite3_prepare_v2(db, select_sql, -1, &select_stmt, NULL);
    if (rc != SQLITE_OK)
    {
        perror("Cannot prepare SELECT statement");
        sqlite3_close(db);
        return; // Exit or handle the error appropriately
    }
    sqlite3_bind_int(select_stmt, 1, user_id);

    int message_count = 0;

    // Fetch messages
    while (sqlite3_step(select_stmt) == SQLITE_ROW)
    {
        int message_id = sqlite3_column_int(select_stmt, 0);
        char *sender_username = sqlite3_column_text(select_stmt, 1);
        char *message_text = sqlite3_column_text(select_stmt, 2);

        if (message_count == 0)
        {
            sprintf(message_response, "<%d> %s: %s", message_id, sender_username, message_text);
        }
        else
        {
            strcat(message_response, "\n");
            sprintf(message_response + strlen(message_response), "<%d> %s: %s", message_id, sender_username, message_text);
        }

        message_count++;
    }

    if (message_count > 0)
    {
        *message_response_len = strlen(message_response);
    }
    else
    {
        strcpy(message_response, "no unseen messages");
        *message_response_len = strlen(message_response);
    }

    sqlite3_finalize(select_stmt);
    sqlite3_close(db);
}

void update_unseen_messages(int user_id){
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK)
    {
        perror("Cannot open database");
        return; // Exit or handle the error appropriately
    }

    // Query to get the unseen messages
    char *update_sql = "UPDATE Messages SET seen = 1 WHERE receiver = ? and seen = 0";

    sqlite3_stmt *update_stmt;
    rc = sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, NULL);
    if (rc != SQLITE_OK)
    {
        perror("Cannot prepare UPDATE statement");
        sqlite3_close(db);
        return; // Exit or handle the error appropriately
    }
    sqlite3_bind_int(update_stmt, 1, user_id);

    rc = sqlite3_step(update_stmt);
    if (rc != SQLITE_DONE)
    {
        perror("Cannot execute UPDATE statement");
    }

    sqlite3_finalize(update_stmt);
    sqlite3_close(db);
}
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
