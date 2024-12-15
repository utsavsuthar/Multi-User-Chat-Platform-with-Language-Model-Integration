#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include "uuid4.c"
#define BUFFER_SIZE 1024
#define FILENAME "chathistory.log"
#define TEMPFILENAME "temp.log"
void receive_message(int socket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer,0,BUFFER_SIZE);
    ssize_t bytes_received;

    // Receive message from server
    bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0)
    {
        perror("Error receiving message from server");
        exit(EXIT_FAILURE);
    }
    else if (bytes_received == 0)
    {
        printf("Server closed the connection\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        buffer[bytes_received] = '\0'; // Null-terminate the received message
        printf("\n%s", buffer);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Incorrect program usage: gcc <program name> <ip address> <port number>");
        exit(1);
    }

    int client_socket;
    struct sockaddr_in addr;
    socklen_t addr_size;
    char buffer[1024];
    int n;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = atoi(argv[2]);
    // printf("%d",addr.sin_port);
    // addr.sin_addr.s_addr = inet_addr(argv[1]);
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }
    // connect to server
    //  connect(client_sock, (struct sockaddr*)&addr, sizeof(addr));
    if (connect(client_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to the server.\n");
    char client_unique_id[UUID4_STR_BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive message from server
    bytes_received = recv(client_socket, client_unique_id, 50, 0);
    receive_message(client_socket);
    
    bzero(buffer, 1024);

    strcpy(buffer, "HELLO, THIS IS CLIENT.");
    printf("\nClient: %s\n", buffer);

    fd_set read_fds;
    int max_sd;
    int should_print_prompt = 1; // Flag to control printing of the prompt

    // printf("user>");
    // usleep(500000);
    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client_socket, &read_fds);
        max_sd = (client_socket > STDIN_FILENO) ? client_socket : STDIN_FILENO;
        printf("user>");
        fflush(stdout);
        // Wait for activity on any of the sockets
        if (select(max_sd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // Check for activity on client_socket
        if (FD_ISSET(client_socket, &read_fds))
        {
            receive_message(client_socket);
            should_print_prompt = 1; // Set flag to allow printing prompt after receiving server response
        }

        // Check for activity on stdin
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            char cmd[BUFFER_SIZE];
            char temp_cmd[BUFFER_SIZE];
            
            fgets(cmd, BUFFER_SIZE, stdin);
            strcpy(temp_cmd,cmd);
            // Remove trailing newline character, if present
            cmd[strcspn(cmd, "\n")] = '\0';

            char buffer[BUFFER_SIZE];
            if (strcmp(cmd, "/active") == 0)
            {
                snprintf(buffer, BUFFER_SIZE, "/active");
                send(client_socket, buffer, strlen(buffer), 0);
            }

            else if (strcmp(cmd, "/chatbot login") == 0)
            {
                // printf("hii");
                snprintf(buffer, BUFFER_SIZE, "/chatbot login");
                send(client_socket, buffer, strlen(buffer), 0);
            }
            else if (strcmp(cmd, "/chatbot logout") == 0)
            {
                snprintf(buffer, BUFFER_SIZE, "/chatbot logout");
                // buffer = "/active";
                send(client_socket, buffer, strlen(buffer), 0);
            }
            else if (strcmp(cmd, "/chatbot_v2 login") == 0)
            {
                // printf("hii");
                snprintf(buffer, BUFFER_SIZE, "/chatbot_v2 login");
                send(client_socket, buffer, strlen(buffer), 0);
            }
            else if (strcmp(cmd, "/chatbot_v2 logout") == 0)
            {
                snprintf(buffer, BUFFER_SIZE, "/chatbot_v2 logout");
                // buffer = "/active";
                send(client_socket, buffer, strlen(buffer), 0);
            }
            else if (strcmp(cmd, "/send") == 0)
            {
                char cid[BUFFER_SIZE], msg[BUFFER_SIZE];
                printf("Enter recipient ID: ");
                scanf("%s", cid);
                printf("Enter message: ");
                getchar(); // Clear the input buffer
                if (fgets(msg, BUFFER_SIZE, stdin) != NULL)
                {
                    // Remove newline character if present
                    msg[strcspn(msg, "\n")] = '\0';
                    snprintf(buffer, BUFFER_SIZE, "/send %s %s", cid, msg);
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                else
                {
                    printf("Error reading message.\n");
                }
            }
            else if (strcmp(cmd, "/logout") == 0)
            {
                snprintf(buffer, BUFFER_SIZE, "/logout");
                send(client_socket, buffer, strlen(buffer), 0);
                receive_message(client_socket);
                break;
            }

            else
            {
                send(client_socket, cmd, strlen(cmd), 0);
                receive_message(client_socket);
            }
            // printf("user>");
        }
    }

    // Close socket
    close(client_socket);

    return EXIT_SUCCESS;
}
