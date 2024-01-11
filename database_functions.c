#include <sqlite3.h>
#include "database_functions.h"
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
