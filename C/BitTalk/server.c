#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5		 // Maximum number of concurrent clients
#define PORT 8080		 // Port to listen on

int client_sockets[MAX_CLIENTS] = {0}; 		// Store clients
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

// Broadcast message to all clients
void broadcast_message(const char *message, int sender_socket){
	pthread_mutex_lock(&client_mutex);
	for(int i = 0; i < MAX_CLIENTS; i++){
		if(client_sockets[i] != 0 && client_sockets[i] != sender_socket){
			send(client_sockets[i], message, strlen(message), 0);
	}
}
pthread_mutex_unlock(&client_mutex);
}


// Handle communication with a single client
void *handle_client(void *socket_desc){
	int client_socket = *(int *)socket_desc;
	free(socket_desc);

	char buffer[BUFFER_SIZE] = {0};
	char welcome_message[] = "Welcome to the chat server!\n";

	// Send a welcome message
	send(client_socket, welcome_message, strlen(welcome_message), 0);

	while(1){
		memset(buffer, 0, BUFFER_SIZE);
		int valread = read(client_socket, buffer, BUFFER_SIZE);
		if(valread <= 0){
			printf("Client disconnected\n");
			pthread_mutex_lock(&client_mutex);
			for(int i =0; i < MAX_CLIENTS; i++){
				if(client_sockets[i] == client_socket){
					client_sockets[i] = 0;
					break;
				}
			}
		pthread_mutex_unlock(&client_mutex);
		break;
		}
	printf("Client sent: %s\n", buffer);
	broadcast_message(buffer, client_socket);
	}

	close(client_socket);
	return NULL;
}


int main(){
	int server_fd, client_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	// Create Socket
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

	// Set socket options
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("setsockopt failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// Bind socket to address and port
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
		perror("Bind failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// Start Listening for connections
	if(listen(server_fd, 3) < 0){
		perror("Listen failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	printf("Server is listening on port %d\n", PORT);

	// Accept a connection
	while(1){
	if((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0){
		perror("Accept failed");
		continue;
	}
	printf("New client connected\n");

	pthread_mutex_lock(&client_mutex);
	for(int i = 0; i < MAX_CLIENTS; i++){
		if(client_sockets[i] == 0){
			client_sockets[i] = client_socket;
			break;
		}
	}
	pthread_mutex_unlock(&client_mutex);

	int *new_sock = malloc(sizeof(int));
	*new_sock = client_socket;
	pthread_t client_thread;
	pthread_create(&client_thread, NULL, handle_client, (void *)new_sock);
	pthread_detach(client_thread);

	}

	// Close Socket
	close(server_fd);

	return 0;
}
