#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "uuid4.c"
#define MAX_DATA_SIZE 1024
#define MAX_PAIRS 1000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define FILENAME "chathistory.log"
#define TEMPFILENAME "temp.log"
struct PhraseResponsePair
{
    char phrase[MAX_DATA_SIZE];
    char response[MAX_DATA_SIZE];
};

typedef struct
{
    int socket_id;
    bool status;
    bool chatbot_status;
    char client_unique_id[UUID4_STR_BUFFER_SIZE];
} clientdict;
clientdict clients[MAX_CLIENTS];
void search(struct PhraseResponsePair pair[], char *question, char *answer)
{
    memset(answer, 0, BUFFER_SIZE); // Initialize the answer buffer
    // printf("%s", question);
    for (int i = 0; i < MAX_PAIRS; ++i)
    {

        if (strstr(question, pair[i].phrase) != NULL)
        {
            strcpy(answer, pair[i].response);
            return;
        }
    }

    // If the question is not found, store a default response in the answer buffer
    strcpy(answer, "System Malfunction, I couldn't understand your query.\n");
}

void receive_message(int socket)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive message from server
    bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0)
    {
        printf("Error receiving message from client(%d)\n", socket);
        exit(EXIT_FAILURE);
    }
    else if (bytes_received == 0)
    {
        printf("Client closed the connection\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        buffer[bytes_received] = '\0'; // Null-terminate the received message
        // printf("\n%s\n", buffer);
        printf("\nclient(%d):%s\n", socket, buffer);
    }
}

// ClientInfo clients[MAX_CLIENTS];

int main(int argc, char *argv[])
{
    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);
    int server_socket, client_socket, max_socket, activity;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // printf("server %d\n", server_socket);
    if (server_socket < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = port;
    server_address.sin_addr.s_addr = inet_addr(ip);

    int n = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (n < 0)
    {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Bind to the port number: %d\n", port);

    if (listen(server_socket, 5) < 0)
    {
        perror("Listening Failed!!");
        exit(1);
    }

    printf("Listening on port number %d...\n", port);
    struct PhraseResponsePair pair[MAX_PAIRS];
    FILE *file = fopen("FAQs.txt", "r");

    if (file == NULL)
    {
        perror("error opening file.");
        exit(EXIT_FAILURE);
    }
    char line[2 * MAX_DATA_SIZE];
    int pairnum = 0;
    while (fgets(line, 2 * MAX_DATA_SIZE, file) != NULL)
    {

        char *token = strtok(line, "|||");
        while (token != NULL)
        {
            size_t token_len = strlen(token); // Get the length of the token
            if (token_len > 0)
            {
                // Remove the last character from the token
                token[token_len - 1] = '\0';
            }
            strncpy(pair[pairnum].phrase, token, MAX_DATA_SIZE - 1);
            pair[pairnum].phrase[MAX_DATA_SIZE - 1] = '\0';
            token = strtok(NULL, "|||");
            if (token != NULL)
            {
                strncpy(pair[pairnum].response, token, MAX_DATA_SIZE - 1);
                pair[pairnum].response[MAX_DATA_SIZE - 1] = '\0';
                pairnum += 1;
            }
        }
    }
    fclose(file);
    // for (int i = 0; i < pairnum; i++)
    // {
    //     printf("Phrase:%s\n", pair[i].phrase);
    //     printf("Response:%s\n", pair[i].response);
    //     printf("\n");
    // }
    // Accept incoming connections
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_socket = server_socket;

        // Add client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i].status)
            {
                FD_SET(clients[i].socket_id, &readfds);
                if (clients[i].socket_id > max_socket)
                {
                    max_socket = clients[i].socket_id;
                }
            }
        }

        // Wait for activity on any of the sockets
        activity = select(max_socket + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            perror("Select error");
        }

        // Check for activity on the server socket (new connection)
        if (FD_ISSET(server_socket, &readfds))
        {
            client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
            printf("Received client socket ids: %d\n", client_socket);
            if (client_socket < 0)
            {
                perror("Accepting connection failed");
                continue;
            }

            printf("New connection established.\n");

            // Add the new client to the clients array
            int curr_client;
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (!clients[i].status)
                {
                    curr_client = i;
                    break;
                }
            }

            // Generate a unique ID for the client
            UUID4_STATE_T state;
            UUID4_T uuid;
            uuid4_seed(&state);
            uuid4_gen(&state, &uuid);
            if (!uuid4_to_s(uuid, clients[curr_client].client_unique_id, sizeof(clients[curr_client].client_unique_id)))
            {
                exit(EXIT_FAILURE);
            }

            clients[curr_client].socket_id = client_socket;
            clients[curr_client].status = true;

            // Send welcome message along with unique ID to the client
            snprintf(buffer, BUFFER_SIZE, "Server: Welcome! Your unique ID is: %s", clients[curr_client].client_unique_id);
            send(client_socket, clients[curr_client].client_unique_id, strlen(clients[curr_client].client_unique_id), 0);
            send(client_socket, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
        }

        // Check for activity on client sockets
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i].status && FD_ISSET(clients[i].socket_id, &readfds))
            {
                client_socket = clients[i].socket_id;
                ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
                char cmd[BUFFER_SIZE];
                strcpy(cmd,buffer);
                // cmd[strcspn(cmd, "\n")] = '\0';
                if (bytes_received <= 0)
                {
                    // Connection closed or error
                    printf("Client disconnected.\n");
                    close(client_socket);
                    clients[i].status = false;
                }
                else
                {
                    // Process received data
                    buffer[bytes_received] = '\0'; // Null-terminate the received data
                    // printf("%s", buffer);
                    // Handle client communication here
                    if (strcmp(buffer, "/active") == 0)
                    {
                        // Prepare response with active client IDs
                        char response_buffer[BUFFER_SIZE];
                        memset(response_buffer, 0, sizeof(response_buffer));
                        strcat(response_buffer, "Server: List of Active Clients:\n");
                        for (int j = 0; j < MAX_CLIENTS; j++)
                        {

                            if (clients[j].status)
                            {

                                strcat(response_buffer, clients[j].client_unique_id);
                                strcat(response_buffer, "\n");
                            }
                        }
                        // strcat(response_buffer, "\n");
                        // Send response to the client
                        send(client_socket, response_buffer, strlen(response_buffer), 0);
                        // receive_message(so)
                    }
                    else if (strcmp(buffer, "/logout") == 0)
                    {
                        // Send farewell message to the client
                        char *logout_message = "Server: Bye!! Have a nice day\n";
                        send(client_socket, logout_message, strlen(logout_message), 0);
                        // Mark the client as inactive and close the socket
                        close(client_socket);
                        clients[i].status = false;
                        printf("Client logged out.\n");
                    }
                    else if (strncmp(buffer, "/chatbot login", 14) == 0)
                    {
                        char response[BUFFER_SIZE];
                        memset(response, 0, sizeof(response));
                        clients[i].chatbot_status = true;
                        strcat(response, "stupidbot> Hi,I am stupidbot,I am able to answer a limited set of your questions\n");
                        send(client_socket, response, strlen(response), 0);
                    }
                    else if (strncmp(buffer, "/chatbot logout", 15) == 0)
                    {
                        char response[BUFFER_SIZE];
                        memset(response, 0, sizeof(response));
                        clients[i].chatbot_status = false;
                        strcat(response, "stupidbot>Bye! Have a nice day and do not complain about me\n");
                        send(client_socket, response, strlen(response), 0);
                    }
                    else if (clients[i].chatbot_status == true)
                    {
                        // printf("%s", buffer);
                        // char *question;
                        // // memset(question, 0, sizeof(question));
                        // strcpy(question, buffer);
                        // strcat(question, " ");
                        char answer[BUFFER_SIZE];
                        search(pair, buffer, answer);
                        if (answer[0] == '\0') {
                            strcpy(answer, "System Malfunction, I couldn't understand your query.\n");
                        }
                        char final_answer[BUFFER_SIZE];
                        strcpy(final_answer, "stupidbot> ");
                        strcat(final_answer, answer);
                        send(client_socket, final_answer, strlen(final_answer), 0);
                    }
                    else if (strncmp(cmd, "/history_delete", 15) == 0)
                        {
                            // printf("%s\n", cmd);
                            char dest_id[50], command[30];
                            int flag = 0;
                            if (sscanf(cmd, "%17s %36s", command, dest_id) == 2)
                            {

                                // printf("%s %s\n", command, dest_id);
                                FILE *fptr = fopen(FILENAME, "r");
                                FILE *file = fopen(TEMPFILENAME, "a");
                                char sender[50], receiver[50], message[200];
                                while (fscanf(fptr, "%s\t%s\t%[^\n]", sender, receiver, message) == 3)
                                {
                                
                                    if (!(((strcmp(sender, clients[i].client_unique_id) == 0) && (strcmp(receiver, dest_id) == 0)) ||
                                        ((strcmp(sender, dest_id) == 0) && (strcmp(receiver, clients[i].client_unique_id) == 0))))
                                    {
                                        // printf("Sender: %s\tReceiver: %s\tMessage: %s\n", sender, receiver, message);
                                        // flag = 1;
                                        
                                        fprintf(file, "%s\t%s\t%s\n", sender, receiver, message);
                                        
                                    }
                                    else{
                                        flag =1;
                                    }
                                }
                                if (flag){
                                    send(client_socket, "Server: History deleted Successfully!!\n", strlen("Server: History deleted Successfully!!\n"), 0);
                                    // printf("History deleted Successfully!!\n");
                                }
                                else{
                                    // printf("No History is found for the client!!\n");
                                    send(client_socket, "Server: No History is found for the client!!\n", strlen("Server: No History is found for the client!!\n"), 0);
                                }
                                fclose(file);
                                fclose(fptr);
                                system("rm chathistory.log");
                                system("mv temp.log chathistory.log");
                            }
                        }
                        else if (strncmp(cmd, "/history", 8) == 0)
                        {

                            // printf("%s\n",cmd);
                            char dest_id[50], command[20];
                            int flag = 0;
                            if (sscanf(cmd, "%10s %36s", command, dest_id) == 2)
                            {
                                // printf("%s %s\n",command,dest_id);
                                FILE *fptr = fopen(FILENAME, "r");
                                char sender[50], receiver[50], message[200];
                            
                                while (fscanf(fptr, "%s\t%s\t%[^\n]", sender, receiver, message) == 3)
                                {
                                    // printf("%s %s %s\n", sender, receiver, message);

                                    // Check if the message involves the specified sender and receiver
                                    if (((strcmp(sender, clients[i].client_unique_id) == 0) && (strcmp(receiver, dest_id) == 0)) ||
                                        ((strcmp(sender, dest_id) == 0) && (strcmp(receiver, clients[i].client_unique_id) == 0)))
                                    {
                                        flag = 1;
                                        char answer[BUFFER_SIZE];
                                        memset(answer, 0, sizeof(answer));
                                        snprintf(answer, BUFFER_SIZE,"Sender: %s\tReceiver: %s\tMessage: %s\n", sender, receiver, message);   
                                        send(client_socket, answer, strlen(answer), 0);
                                        // printf("Sender: %s\tReceiver: %s\tMessage: %s\n", sender, receiver, message);
                                    }
                                }

                                if (flag == 0){
                                    send(client_socket, "Server: No History is found!\n", strlen("Server: No History is found!\n"), 0);
                                    // printf("No History is found!\n");
                                }
                                
                                fclose(fptr);
                            }
                        }

                        else if (strncmp(cmd, "/delete_all", 11) == 0)
                        {
                            FILE *fptr = fopen(FILENAME, "r");
                            FILE *file = fopen(TEMPFILENAME, "a");
                            char sender[50], receiver[50], message[200];

                            while (fscanf(fptr, "%s\t%s\t%[^\n]", sender, receiver, message) == 3)
                            {
                                // printf("%s %s %s\n", sender, receiver, message);

                                // Check if the message involves the specified sender and receiver
                                if ((strcmp(sender, clients[i].client_unique_id) != 0) && (strcmp(receiver, clients[i].client_unique_id) != 0)) 
                                {

                                    fprintf(file, "%s\t%s\t%s\n", sender, receiver, message);
                                }
                            }
                            
                            fclose(fptr);
                            fclose(file);
                            system("rm chathistory.log");
                            system("mv temp.log chathistory.log");
                            send(client_socket, "Server: All the chat history deleted for the client successfully.\n", strlen("Server: All the chat history deleted for the client successfully.\n"), 0);
                            // printf("All the chat history deleted for the client successfully.\n");
                        }

                    else if (strncmp(buffer, "/send", 5) == 0)
                    {
                        // Process "send" command
                        char command[10], dest_id[37], message[100];
                        if (sscanf(buffer, "%9s %36s %99[^\n]", command, dest_id, message) == 3)
                        {
                            // Message received, process further
                            printf("Received message from %s to %s: %s\n", clients[i].client_unique_id, dest_id, message);
                            int flag = 0;
                            for (int j = 0; j < MAX_CLIENTS; j++)
                            {
                                if (strcmp(clients[j].client_unique_id, dest_id) == 0 && clients[j].status == true)
                                {
                                    char message_buffer[BUFFER_SIZE];
                                    memset(message_buffer, 0, sizeof(message_buffer));
                                    snprintf(message_buffer, BUFFER_SIZE, "Server: Message From %s: %s\n", clients[i].client_unique_id, message);
                                    send(clients[j].socket_id, message_buffer, strlen(message_buffer), 0);
                                    char *send_message = "Server: message has been sent to user!!\n";
                                    send(client_socket, send_message, strlen(send_message), 0);
                                    flag = 1;
                                    FILE *fptr = fopen(FILENAME, "a");
                                    if (fptr == NULL)
                                    {
                                        perror("Error opening log file");
                                        exit(EXIT_FAILURE);
                                    }
                                    fprintf(fptr, "%s\t%s\t%s\n", clients[i].client_unique_id, dest_id, message);
                                    // fprintf(fptr,"%s\t%s\t%s\n",clients[i].client_unique_id,dest_id,message);
                                    fclose(fptr);
                                }
                            }
                            if (!flag)
                            {
                                char *send_message = "Server: user is offline!!!\n";
                                send(client_socket, send_message, strlen(send_message), 0);
                            }
                        }
                        else
                        {
                            // Invalid message format
                            printf("Invalid message format.\n");
                        }
                    }
                    else
                    {
                        char *send_message = "Server: Enable the chatbot function!!\n";
                        send(client_socket, send_message, strlen(send_message), 0);
                    }
                }
            }
        }
    }

    // Close server socket
    close(server_socket);

    return 0;
}
