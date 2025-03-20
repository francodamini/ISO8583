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

#define BUFF_SIZE 1024
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

typedef struct {
    char card_number[17];
    int amount;
} transactionData;

transactionData user_data() {
    transactionData t;

    printf("Card Number: ");
    scanf("%16s", t.card_number);
    printf("Amount: ");
    scanf("%d", &t.amount);
    return t;
}

void build_iso8583_msg(char* buff, transactionData t) {
    sprintf(buff, "0100|%s|%d", t.card_number, t.amount);
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
    int total_sent = 0;
    int msg_len = strlen(buff);
    int sent;

    while (total_sent < msg_len){
        sent = send(master, buff + total_sent, msg_len - total_sent, 0);
        if (sent < 0) {
            perror("Error sending message");
            close(master);
            exit(EXIT_FAILURE);
        } 
        total_sent += sent;

    }
    printf("Message sent to the server");
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

int main() {
    char buff[BUFF_SIZE]; //Buffer for the msg

    transactionData t = user_data(); // --Ask user for card number and amount
    build_iso8583_msg(buff, t); // --With that data, create a simple ISO 8583 msg
    int master = server_connection(); // --Connect to server using TCP Sockets
    send_msg(master, buff); // --Send msg
    receive_response(master); //--Wait for response
    close(master);

    return 0;
}