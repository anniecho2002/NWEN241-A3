/**
 * Skeleton file for server.c
 *
 * You are free to modify this file to implement the server specifications
 * as detailed in Assignment 3 handout.
 *
 * As a matter of good programming habit, you should break up your imple-
 * mentation into functions. All these functions should contained in this
 * file as you are only allowed to submit this file.
 */

#include <stdio.h>
#include<sys/socket.h>
#include<netinet/in.h> 
#include<string.h> 
#include<ctype.h> 
#include<stdlib.h> 
#include<unistd.h> 



/**
 * Read a message from the client to the server
 */
void readTo(char *incoming, int client){
	
	// grabs the user input and reads it
	char input[10000] = {0};
	strncpy(input, incoming, strlen(incoming));
	int re = recv(client, &input, 10000, 0);
	if(re < 0) { 
		printf("Error receiving message\n"); 
	}
	else{ printf("Received file input: %s \n", input); }
}


/**
 * Write a message to the client from the server
 */
void writeTo(char *incoming, int client){
	
	char buffer[10000] = {0};
	strncpy(buffer, incoming, strlen(incoming));
	int s = write(client, buffer, 10000);
	if(s < 0){ 
		printf("Write error\n"); // if it fails
	} 
	else { printf("Write successful\n"); }
  
}







/**
 * Open the selected file using the filename (read), write file contents to the client
 */
void getFile(char filename[], int clientId){
	FILE *file;
	file = fopen(filename, "r"); // opens file to read
	
	// if opening the file is unsuccessful
	if(file == NULL){
		writeTo("SERVER 404 Not Found\n", clientId);
		return;
	}

	char c; // to store the character as we read the contents
	char contents[10000] = {0}; // stores the contents of the opened file
	int index = strlen("SERVER 200 OK\n\n"); // counter of the index of array (starts after message)
	strncpy(contents, "SERVER 200 OK\n\n", index);

	while((c = fgetc(file)) != EOF){
		strcpy(&contents[index], &c);
		index++;
	}
	for(int i = 0; i < 3; i++){
		strcpy(&contents[index], "\n");
		index++; // adds the extra \n characters to end of the contents to display
	}

	// send contents back to client
	writeTo(contents, clientId);
	fclose(file);
}



/**
 * 
 * Open the selected file using the filename (write), clear the contents of file
 * Write the incoming contents into the same file
 */
void putFile(char filename[], int clientId){
	
	FILE *file;
	file = fopen(filename, "w");

	// if opening the file is unsuccessful
	if(file == NULL){
		writeTo("SERVER 501 Put Error\n", clientId);
		return;
	}

	int prevnull = 0;
	int closefile = 0;
	while(1){
		char newcontents[10000] = {0}; 
		int re = recv(clientId, &newcontents, 10000, 0); // read new contents from the client
		if(re < 0) { 
			printf("Error receiving message\n");
			return;
		}
		else { printf("Received file input: %s \n", newcontents); }


		
		int newcontentslen = sizeof(newcontents)/sizeof(char);
		for(int i = 0; i < newcontentslen; i++){
			fputc(newcontents[i], file);
			if(i > 0){
				if(prevnull){
					if(!(newcontents[i] == 'n' || newcontents[i] == '\\')){ prevnull = 0; }
				}
				if(newcontents[i] == 'n' && newcontents[i-1] == '\\'){
					if(prevnull){
						closefile = 1; // now need to close the file
						fclose(file);
						break;
					}
					prevnull = 1;
				}
			}
		}
		if(closefile){ break; }
	}
	writeTo("SERVER 201 Created\n", clientId);
}


/**
 * Reads the filename, stops when encounters a new line character
 */
void editFileName(char incoming[], int isGet, int clientId){
	char filename[1000] = {0};
	if(strlen(incoming) <= 4){
		if(isGet){ writeTo("SERVER 500 Get Error\n", clientId); }
		else{ writeTo("SERVER 501 Put Error\n", clientId); }
		return; // no file name received, so finish here
	}

	for(int i = 4; i < 1000; i++){
		if(incoming[i] == '\n'){ break; } // if it is at the end, then stops recording filename
		filename[i-4] = incoming[i]; // sets the filename
	}

	if(isGet){ getFile(filename, clientId); }
	else{ putFile(filename, clientId); }
}







/**
 * Accepts a single command line argument, consisting of the port number
 * Creates a client connection and allows client to communicate with the server and vice versa
 */
int main(int argc, char *argv[]){
	
    int sockfd;

    if(argc == 1 || atoi(argv[1]) < 1024){
      printf("Error initialising port number.\n");
      return -1;
    }
    int portnum = atoi(argv[1]);

    // Make the socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){ printf("Error creating socket\n"); }
    printf("Socket created\n");


    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr= INADDR_ANY;
    serveraddr.sin_port=htons(portnum);
    printf("Address created\n");


    // Bind address to socket
    int br;
    br = bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
    if(br < 0){ printf("Error Binding\n"); }
    printf("Successful Binding\n");


    // Listen for client connection
	if(listen(sockfd,5) < 0){ printf("Listening error\n"); }
	printf("Listening success\n");
	
	
	int running = 1;
		while(running){

			struct sockaddr_in clientaddr;
			int clientfd = accept(sockfd,(struct sockaddr*)&clientaddr,(socklen_t*)&clientaddr);
			if(clientfd < 0){ printf("Error accepting client\n"); }
			printf("Client connection accepted\n");
			
			writeTo("Hello from server!\n", clientfd); // says hello 

			while(1){
				char incoming[100] = {0};
				char incomingCase[100] = {0};

				int r = recv(clientfd, &incoming, 100, 0);
				if(r < 0) { printf("Error receiving client message\n"); }
				printf("Received message: %s\n", incoming);

				for(int i = 0; i < 100; i++){ incomingCase[i] = toupper(incoming[i]); } // ensures that it is case insensitive

				if(strncmp(incomingCase, "BYE", 3) == 0){
					printf("Closing client connection.\n");
					close(clientfd);
					running = 0;
					break; // closes the client and socket connection
				}
				else if(strncmp(incomingCase, "GET", 3) == 0){ editFileName(incoming, 1, clientfd); }
				else if(strncmp(incomingCase, "PUT", 3) == 0){ editFileName(incoming, 0, clientfd); }
				else{ printf("No action performed."); }
			}
		}
	close(sockfd);
	return 0;
}
