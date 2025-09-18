#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PORT 8080		// Port to connect to

// Function to receive messages from the server
void *receive_messages(void *socket_desc){
	int sock = *(int *)socket_desc;
	char buffer[BUFFER_SIZE];

	while(1){
		memset(buffer, 0, BUFFER_SIZE);
		int valread = read(sock, buffer, BUFFER_SIZE);
		if(valread > 0){
			printf("Server: %s\n", buffer);
		} else{
			break;
		}
	}
	return NULL;
}


int main(){
	int sock = 0;
	struct sockaddr_in serv_addr;
	char *message = "Hello from Client!";
	char buffer[BUFFER_SIZE] = {0};
	pthread_t recv_thread;

	// Create socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	// Define server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 address from text to binary
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
		perror("Invalid address/ Address not supported");
		exit(EXIT_FAILURE);
	}

	// Connect to the server
	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		perror("Connection failed");
		exit(EXIT_FAILURE);
	}
	printf("Connected to the server\n");

	// Start thread to receive messages
	pthread_create(&recv_thread, NULL, receive_messages, &sock);

	// Send messages to server
	while(1){
		fgets(buffer, BUFFER_SIZE, stdin);
		send(sock, buffer, strlen(buffer), 0);
	}


	// Close the socket
	close(sock);

	return 0;
}
