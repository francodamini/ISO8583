/*
--Ask user for card number and amount
--With that data, create de ISO 8583 msg
--Connect to server using TCP Sockets
--Send msg and wait for response
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFF_SIZE 4096
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define AMOUNT_OF_CLIENTS 5

typedef struct {
    char card_number[17];
    int amount;
} transactionData;

transactionData user_data(int client_id) {
    transactionData t;
    sprintf(
        t.card_number, 
        "%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d", 
        client_id, client_id, client_id, client_id, 
        client_id, client_id, client_id, client_id, 
        client_id, client_id, client_id, client_id, 
        client_id, client_id, client_id, client_id
    );
    t.card_number[16] = '\0';
    t.amount = (rand() % 10000) + 1;
return t;
}

void build_iso8583_msg(char* buff, transactionData t) {
    snprintf(buff, BUFF_SIZE, "0100|%s|%d", t.card_number, t.amount);
}

int server_connection(){
    //AF_INET -> IPv4
    //SOCK_STREAM, 0 -> TCP
    int master = socket(AF_INET, SOCK_STREAM, 0);
    if (master < 0){
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address; //Alocate server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    int binaryIP = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr); //Turns Server IP into binary and stores it in server_address.sin_addr
    if (binaryIP <= 0) {
        perror("Invalid or unsupported address");
        close(master);
        exit(EXIT_FAILURE);
    }

    int client = connect(master, (struct sockaddr *)&server_address, sizeof(server_address));
    if (client < 0){
        perror("Error connecting to server");
        close(master);
        exit(EXIT_FAILURE);
    }

    return master;
}

void send_msg(int master, char* buff) {
    int msg_len = strlen(buff);
    send(master, &msg_len, sizeof(msg_len), 0); //First we send the message lenght
    send(master, buff, msg_len, 0); 
    printf("Message sent to the server\n");
}

void receive_response(int master) {
    char response[BUFF_SIZE];
    int total_bytes_recv = 0;
    int bytes_recv;
    
    while ((bytes_recv = recv(master, response + total_bytes_recv, BUFF_SIZE - total_bytes_recv, 0)) > 0){
        total_bytes_recv += bytes_recv;
    }
    
    
    if (bytes_recv < 0) {
        perror("Error receiving response");
        close(master);
        exit(EXIT_FAILURE);
    }
    
    if (total_bytes_recv  == 0) {
        printf("Server closed the connection.\n");
    } else {
        response[total_bytes_recv ] = '\0';
        printf("Server response: %s\n", response);
    }
}

void* client_thread(void* args){
    int client_id = *((int*) args);
    free(args);

    char buff[BUFF_SIZE]; //Buffer for the msg
    transactionData t = user_data(client_id); // --Ask user for card number and amount
    build_iso8583_msg(buff, t); // --With that data, create a simple ISO 8583 msg
    int master = server_connection(); // --Connect to server using TCP Sockets
    send_msg(master, buff); // --Send msg
    receive_response(master); //--Wait for response

    close(master);
    pthread_exit(NULL);
}


int main() {
    srand(time(NULL)); //For random numbers
    
    pthread_t clients[AMOUNT_OF_CLIENTS];

    for (int i = 0; i < AMOUNT_OF_CLIENTS; i++){
        int *client_id = malloc(sizeof(int));
        *client_id = i + 1;

        pthread_create(&clients[i], NULL, client_thread, client_id);
        // usleep(500000); //Avoid output collisions
    }

    for (int i = 0; i < AMOUNT_OF_CLIENTS; i++){
        pthread_join(clients[i], NULL);
    }

    return 0;
}