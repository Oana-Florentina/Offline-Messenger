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
#define ever ;;
#define PORT 3001

extern int errno;

typedef struct thread_data {
    int thread_id;
    int client_fd;
} thread_data;

static void *treat(void *);
void raspunde(void *);

int main () {
   
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int server_fd; 
    pthread_t threads[100];
    int thread_number = 1;

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("[Server] Eroare la bind().\n");
        return errno;
    }

    if(listen(server_fd, 5) == -1) {
        perror("[Server] Eroare la listen().\n");
        return errno;
    }

    for(ever) {
        int client_fd;
        thread_data *client_data;
        socklen_t client_addr_length = sizeof(client_addr);

        printf("[Server] Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        if((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_length)) < 0) {
            perror("[Server] Eroare la accept().\n");
            continue;
        }

        client_data = (struct thread_data*)malloc(sizeof(struct thread_data));
        client_data -> thread_id = thread_number++;
        client_data -> client_fd = client_fd;

        pthread_create(&threads[thread_number], NULL, &treat, client_data);
    }
}

static void *treat(void *args) {
        struct thread_data client_data;
        client_data = *((struct thread_data*)args);
        printf("[Thread-%d] Asteptam mesajul...\n", client_data.thread_id);
        fflush(stdout);

        pthread_detach(pthread_self());
        raspunde((struct thread_data*)args);

        //close(client_data.client_fd);
        return NULL;
};
int check_is_user_exists(char *username) {
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK) {
        return 0;
    }
    char *sql = "SELECT * FROM users WHERE username = ?";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    } else {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
}
int handle_register(char *username, char *password) {
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK) {
        return 0;
    }
    

    
    char *sql = "INSERT INTO users (username, password) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, strlen(password), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        return 0;
    }
    sqlite3_finalize(stmt);
   return 1;
}
void handle_login(char *username, char *password, int user_id) {
    sqlite3 *db;
    int rc = sqlite3_open("/home/florentina/Downloads/server-client-tcp (1)/c-server-client-tcp/DataBase.db", &db);
    if (rc != SQLITE_OK) {
         perror("Cannot open database");
    }
    char *sql = "SELECT * FROM users WHERE username = ? AND password = ?";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        perror("Cannot prepare statement");
    }
    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, strlen(password), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0); // Fetch the user ID from the result
        printf("User logged in successfully with ID: %d\n", user_id);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    } else {
        printf("User or password incorrect\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
   
}

void raspunde(void *args) {
    // citeste mesajul de la client si ii trimite "Cool Message Bro: {message_received}"
    // comunicarea se intampla altfel numar caracter si dupa mesajul
  

    struct thread_data client_data; 
    client_data = *((struct thread_data*)args);

    while(1){
        // request
        char message[1000];
        int message_len;
        int user_id=0;
        char message_response[1000];
        int message_response_len;
        message_response[0] = '\0'; 
        if(read(client_data.client_fd, &message_len, sizeof(int)) <= 0) {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la read() de la client.\n");
             return;
        }

        if(read(client_data.client_fd, message, sizeof(char)*message_len) <= 0) {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la read() de la client.\n");
             return;
        }
        message[message_len] = '\0'; // IMPORTANT

        printf("[Thread-%d] Server received message: %s\n", client_data.thread_id, message);
        if (strcmp(message, "exit") == 0) {
            printf("[Thread-%d] Exiting.\n", client_data.thread_id);
            break;
        }
        else if (strncmp(message,"register",8) == 0)
        {
            char *token = strtok(message, " ");
            char *username = strtok(NULL, " ");
            char *password = strtok(NULL, " ");
            if (username == NULL || password == NULL) {
                printf("[Thread-%d] Invalid registration command format.\n", client_data.thread_id);
                // Send an error response to the client and continue the loop
                continue;
            }
            if(check_is_user_exists(username)) {
                strcpy(message_response, "user already exists");
                message_response_len = strlen(message_response);
            } 
            else 
            if (handle_register(username, password)) {
                strcpy(message_response, "user registered");
                message_response_len = strlen(message_response);
            } else {
                strcpy(message_response, "problem with register");
                message_response_len = strlen(message_response);
            }
        }
        else if (strncmp(message,"login",5) == 0)
        {
           
                strcpy(message_response,"You are already logged in");
            
        }
        

        

        
        // response
     
       
        printf("[Thread-%d] Server send message: %s\n", client_data.thread_id, message_response);

        if(write(client_data.client_fd, &message_response_len, sizeof(int)) <= 0) {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la write() de la client.\n");
            return;
        }
        if(write(client_data.client_fd, message_response, sizeof(char)*message_response_len) <= 0) {
            printf("[Thread-%d]\n", client_data.thread_id);
            perror("Eroare la write() de la client.\n");
            return;
        }
         message[message_len] = '\0';
       
        }

    

    printf("[Thread-%d] Server finishing working with the client.\n", client_data.thread_id);
}

