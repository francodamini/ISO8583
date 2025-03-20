/*
-- Listen for TCP socket connections in port 8080
-- Receive ISO8583 message sent by the client
-- Extract Data from transaction
-- Simulate response
-- Send response to client
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFF_SIZE 1024

void process_transaction(char *request, char *response) {
    char card_number[17];
    int amount;

    if (sscanf(request, "0100|%16[^|]|%d", card_number, &amount) != 2) {
        sprintf(response, "Error: Incorrect Format");
        return;
    }

    printf("Received Transaction -> Card: %s, Amount: %d\n", card_number, amount);

    // Simulate simple validation
    if (amount > 50000) { 
        sprintf(response, "05|Transaction Denied"); // CODE ISO 8583 "05" = Denied
    } else {
        sprintf(response, "00|Transaction Approved"); // CODE ISO 8583 "00" = Approved
    }
}

char* recv_request(int sock) {
    char* request = malloc(BUFF_SIZE);
    if (request == NULL) {
        perror("Memory allocation failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    memset(request, 0, BUFF_SIZE);
    int total_bytes_recv = 0;
    int bytes_recv;

    while ((bytes_recv = recv(sock, request + total_bytes_recv, BUFF_SIZE - total_bytes_recv - 1, 0)) > 0){
        total_bytes_recv += bytes_recv;
    }

    if (bytes_recv < 0) {
        perror("Error receiving request");
        close(sock);
        exit(EXIT_FAILURE);
    }
    if (total_bytes_recv == 0) {
        printf("Client closed the connection.\n");
        free(request);
        close(sock);
        return NULL;
    }
    request[total_bytes_recv] = '\0';

    printf("Message received: %s\n", request);

    return request;
}

void send_response(int sock, char* request) {
    // -- Extract Data from transaction and Simulate response
    char response[BUFF_SIZE];
    process_transaction(request, response);

    // -- Send response to client

    int total_sent = 0;
    int msg_len = strlen(response);
    int sent;

    while (total_sent < msg_len){
        sent = send(sock, response + total_sent, msg_len - total_sent, 0);
        if (sent < 0) {
            perror("Error sending message");
            break;
        } 
        total_sent += sent;
    }

    printf("Response sent: %s\n", response);

}

int main() {

    // -- Listen for TCP socket connections in port 8080
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0){
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; //Accept any IP
    address.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //Avoiding "Address already in use"
    int binding = bind(server, (struct sockaddr *)&address, sizeof(address));
    if (binding < 0){
        perror("Error in bind");
        exit(EXIT_FAILURE);
    }

    int listening = listen(server, 5); //Listening up to 5 connection
    if (listening < 0){
        perror("Error in listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening in PORT %d...\n", PORT);

    while (1) {
        // -- Receive ISO8583 message sent by the client
        int sock = accept(server, (struct sockaddr *)&address, &addrlen);
        if (sock < 0) {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }
        printf("New connection accepted\n");

        char* request = recv_request(sock);
        if (request != NULL) {
            send_response(sock, request);
            free(request);
        }
        close(sock);
    }

    return 0;
}