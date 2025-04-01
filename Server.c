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
#include <libpq-fe.h> 
#include <pthread.h>

#define PORT 8080
#define BUFF_SIZE 1024

typedef struct {
    int sock;
} ThreadArgs;

PGconn* connect_db() {
    PGconn* connection = PQconnectdb("dbname=transaction_db user=postgres password=2612lfDF host=localhost");
    if (PQstatus(connection) != CONNECTION_OK) {
        fprintf(stderr, "Connection Error: %s\n", PQerrorMessage(connection));
    }
    return connection;
}

void save_transaction(PGconn *connection, char* card_number, int amount, char *status){
    char query[256];
    snprintf(
        query,
        sizeof(query),
        "INSERT INTO transactions (card_number, amount, status) VALUES ('%s','%d','%s')",
        card_number, amount, status
    );
    PGresult *result = PQexec(connection, query);

    if (PQresultStatus(result) != PGRES_COMMAND_OK){
        fprintf(stderr, "Error executin query: %s\n", PQerrorMessage(connection));
        return;
    }

    PQclear(result);
}

int is_valid_card_number(const char *card_number) {
    if (strlen(card_number) != 16) {
        return 0;  // Not exactly 16 digits
    }
    for (int i = 0; i < 16; i++) {
        if (card_number[i] < '0' || card_number[i] > '9') {
            return 0;  // Non numerical char
        }
    }
    return 1;  // Valid
}

void process_transaction(char *request, char *response, PGconn *connection) {
    char card_number[17];
    int amount;
    char status[3];

    if (sscanf(request, "0100|%16[^|]|%d", card_number, &amount) != 2 || !is_valid_card_number(card_number)) {
        sprintf(response, "Error: Incorrect Format");
        return;
    }

    printf("Received Transaction -> Card: %s, Amount: %d\n", card_number, amount);

    // Simulate simple validation
    if (amount > 7500) { 
        strcpy(status, "05"); // CODE ISO 8583 "05" = Denied
    } else {
        strcpy(status, "00"); // CODE ISO 8583 "00" = Approved
    }

    sprintf(response, "%s|Transaction %s", status, (strcmp(status, "00") == 0) ? "Approved" : "Denied");
    // sscanf(response, "%s|Transaction %s", status, (strcmp(status, "00") == 0) ? "Approved" : "Denied");

    //Store in Database
    save_transaction(connection, card_number, amount, status);
}

char* recv_request(int sock) {
    char* request = malloc(BUFF_SIZE);
    if (request == NULL) {
        perror("Memory allocation failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    memset(request, 0, BUFF_SIZE);

    int msg_len;
    recv(sock, &msg_len, sizeof(msg_len), 0);  // Recibir tamaño del mensaje
    recv(sock, request, msg_len, 0);  // Recibir mensaje

    printf("Message received: %s\n", request);
    
    return request;
}

void send_response(int sock, char* request, PGconn *connection) {
    // -- Extract Data from transaction and Simulate response
    char response[BUFF_SIZE];
    process_transaction(request, response, connection);

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

void* handle_client(void *args){
    ThreadArgs *thread_args = (ThreadArgs*) args;
    int sock = thread_args->sock;
    free(thread_args);

    //Connecting to Postgre database
    PGconn *connection = connect_db();

    char* request = recv_request(sock);
    if (request != NULL) {
        send_response(sock, request, connection);
        free(request);
    }

    close(sock);

    //Close connection with database
    PQfinish(connection);
    pthread_exit(NULL);
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
            continue;
        }
        printf("New connection accepted\n");

        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        args->sock = sock;

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, args);
        pthread_detach(thread);
    }

    return 0;
}