
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

int check_is_user_exists(char *username);
int handle_register(char *username, char *password);
int handle_login(char *username, char *password);
void handle_see_users(char message_response[1000], int message_response_len, int user_id);
void handle_send_message(char message_response[1000], int message_response_len, char message[100], int user_id);
void handle_see_unseen_messages(char message_response[1000], int *message_response_len, int user_id);
void update_unseen_messages(int user_id);
void handle_see_my_messages(char message_response[1000], int message_response_len, int user_id);    
void handle_reply_to_id_message(char message_response[1000], int *message_response_len, char message[100], int user_id);
