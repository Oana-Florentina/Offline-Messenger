#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

extern int errno;

int port;

int main (int argc, char *argv[]) {
    int server_fd;
    struct sockaddr_in server_addr; 

    if(argc < 3) {
        printf("Usage: %s <server_host> <server_port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[Client] Error creating socket.\n");
        return errno;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(port);

    if(connect(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("[Client] Error connecting to server.\n");
        return errno;
    }

    printf("[Client] Input message ('exit' to quit): ");
    fflush(stdout);

    while(1) {
        char message[1000];
        int message_len;

        scanf(" %[^\n]", message); // Notice the space before %[^\n] to discard leading whitespace
        
        message_len = strlen(message);

        printf("[Client] Message read: %s\n", message);

        // Send the message to the server
        if(write(server_fd, &message_len, sizeof(int)) <= 0) {
            perror("[Client] Error writing to server.\n");
            return errno;
        }
        if(write(server_fd, message, sizeof(char)*message_len) <= 0) {
            perror("[Client] Error writing to server.\n");
            return errno;
        }

        // Check for the exit condition
        if (strcmp(message, "exit") == 0) {
            printf("[Client] Exiting.\n");
            break;
        }

        // Receive and display the server's response
        char message_response[1000];
        int message_response_len;

        if(read(server_fd, &message_response_len, sizeof(int)) <= 0) {
            perror("[Client] Error reading from server.\n");
            return errno;
        }

        if(read(server_fd, message_response, sizeof(char)*message_response_len) <= 0) {
            perror("[Client] Error reading from server.\n");
            return errno;
        }
        message_response[message_response_len] = '\0'; // IMPORTANT

        printf("[Client] Received message: %s\n", message_response);
    }

    close(server_fd);
    return 0;
}
