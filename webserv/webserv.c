/*
 * Author: Mahir Gulrajani (mahirkg@bu.edu)
 * Teammmate: Justina Choi
 * Purpose: Server-side code for the web server.
 * Date: 25th April 2014
 * Citations: Based off of TCP server code we discussed in lecture.
 * Notes: The server files include everything in the a3 folder
 *        so if a request comes in to simply list the current directory,
 *        the a3 directory is "ls"ed.
 */

#include <sys/stat.h> 
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

static void requestHandler(char *, int);
static void printFinal(char *, char *, char *, char *);
static char *substring(char *, int, int);


int main (int argc, char *argv[]) {
	/* Verify correct command-line call */
	if (argc != 2) {
		perror("Correct usage: ./webserv port-number.\n");
		exit(-1);
	}

	/* Get the port from the command line */
	int port = atoi(argv[1]);

	/* Create the socket */
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("The socket() syscall failed.\n");
		exit(-1);
	}

	/* Set socket as SO_REUSEADDR */
	int sock_opt_val = 1;
	if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_opt_val,
				sizeof(sock_opt_val)) < 0) {
		perror ("(servConn): Failed to set SO_REUSEADDR on INET socket");
		exit (-1);
	}

	/* Set up the socket/port information */
	struct sockaddr_in  server;
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Bind the socket to the port */
	int ret = bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr));
	if (ret == -1) {
		perror("The bind() call failed.\n");
		exit(-1);
	}

	/* Listen to the socket with a backlog of 5 */
	listen(sd, 5);

	/* Receive and handle new connections */
	struct sockaddr_in client;
	int len;
	int cd;
	for ( ; ; ) {
		len = sizeof(client);
		cd = accept(sd, (struct sockaddr *) &client, &len);
		if (cd == -1) {
			perror ("(servConn): accept() error");
			exit (-1);
		}		
		if (fork() == 0) {
			close(sd);
			char buffer[1024];
			int n = read(cd, buffer, 1024);
			if (n == 0) {
				exit(0);
			}
			requestHandler(buffer, cd);
			exit(0);
		}
		close(cd);
	}
}

static void requestHandler(char *buffer, int cd) {
	/* Find the "GET /request HTTP/version" */
	char firstLine[1024];
	int i = 0;
	for ( ; i < 1024; i++) {
		firstLine[i] = buffer[i];
		if (buffer[i] == '\n') {
			firstLine[i + 1] = '\0';
			break;
		}
	}
	char *tmp = strchr(firstLine, '\n');
	if (tmp) *tmp = 0;

	/* What is the request and what is the version */
	char request[1024];
	char *version;
	char *tokens[3];
	char *p = strtok(firstLine, " ");
	i = 0;
	while(p != NULL) {
		tokens[i] = p;
		p = strtok(NULL, " ");
		i++;
	}
	request[0] = '.';
	request[1] = '\0';
	strcat(request, tokens[1]);
	version = tokens[2];
	version[strlen(version) - 1] = '\0';

        /* Initialize other important data variables */
        char *status = "status";
        char *message = "message";
        char *content = "content";
	
	/* If the request has a question mark, we know its for dynamic content. */
	int isDynamic = 0;
	i = 0;
	for ( ; i < strlen(request); i++) {
		if (request[i] == '?') {
			isDynamic = 1;
		}
	}

	/* Handle the dynamic content request */
	char *requestTokens[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	if (isDynamic) {
		char *p = strtok(request, "?");
		i = 0;
		while(p != NULL) {
			requestTokens[i] = p;
			p = strtok(NULL, "?");
			i++;
		}
		// i is now the number of tokens
	
		if (i <= 2 || i > 7) {
			status = "501";
			message = "Not Implemented";
			content = "Usage error for my-histogram.pl";
			char output[1024];
			int num = sprintf(output, "%s %s %s\ntext/plain\n\n%s\n", version, status, message, content);
			write(cd, output, num);
			exit(1);
		}

		// The file name is now stored in requestTokens[1]
		// The program to run is now stored in requestTokens[0]
		// The optional arguments are stored in requestTokens[2] to requestTokens[i-1]

		if (strcmp(requestTokens[0], "./my-histogram") != 0) {
			status = "501";
			message = "Not Implemented";
			content = "Only my-histogram is implemented to handle optional arguments";
			char output[1024];
			int num = sprintf(output, "%s %s %s\ntext/plain\n\n%s\n", version, status, message, content);
			write(cd, output, num);
			exit(1);
		}

		// Got to check if the file we're calling my-histogram on exists!
		struct stat file;
		int statreturn = stat(requestTokens[1], &file);
		if (statreturn == -1 || S_ISDIR(file.st_mode)) {
			status = "404";
			message = "Not Found";
			content = "No content for a file that doesn't exist/isn't a regular file!";
			char output[1024];
			int num = sprintf(output, "%s %s %s\ntext/plain\n\n%s\n", version, status, message, content);
			write(cd, output, num);
			exit(1);
		}
		
		char *program = "./my-histogram";
		
		char *arguments[] = {program, requestTokens[1], requestTokens[2], requestTokens[3], requestTokens[4], requestTokens[5], requestTokens[6]};
		
		pid_t childpid = fork();
		if (childpid == 0) {
			execvp(program, arguments);
			perror("Failed my-histogram execvp");
		}
		
		int status;
		waitpid(childpid, &status, 0);
		
		childpid = fork();
		if (childpid == 0) {
			char *args[] = {"./plot.cgi", NULL};
			execvp("./plot.cgi", args);
			perror("Failed to call plot.cgi");
			exit(1);
		}
		int status2;
		waitpid(childpid, &status2, 0);
	
		dup2(cd, 1);
		dup2(cd, 2);
		
		char output[1024];
		int num = sprintf(output, "%HTTP/1.1 200 Successful Request\nimage/jpeg\n\n");
		write(cd, output, num);

		char *args[] = {"cat", "my_graph.jpeg", NULL};
		execvp("cat", args);
		perror("Failed to exec cat");
		exit(1); 
	}

	/* Has it been implemented (i.e. is it a GET?) */
	char *type = tokens[0];
	if (strcmp(type, "GET") != 0) {
		status = "501";
		message = "Not Implemented";
		content = "No content for a request we cannot handle (502 Not Implemented)!";
		char output[1024];
		int num = sprintf(output, "%s %s %s\ntext/plain\n\n%s\n", version, status, message, content);
		write(cd, output, num);
		exit(1);
	}

	/* If the request is for a nonexistent file */
	struct stat file;
	int statreturn = stat(request, &file);
	if (statreturn == -1) {
		status = "404";
		message = "Not Found";
		content = "No content for a non-existent file (404 Not Found)!";
		char output[1024];
		int num = sprintf(output, "%s %s %s\ntext/plain\n\n%s\n", version, status, message, content);
		write(cd, output, num);
		exit(1);
	}

	/* If the request is a directory name */
	if (S_ISDIR(file.st_mode)) {
		status = "200";
		message = "Successful request";
		char output[1024];
		int num = sprintf(output, "%s %s %s\ntext/plain\n\n", version, status, message);
		write(cd, output, num);	

		// redirect future output of execvp
		dup2(cd, 1);
		dup2(cd, 2);
		close(cd);

		char *arguments[] = {"ls", request, NULL};
		execvp("ls", arguments);
		perror("Failed the exec in child.\n");
		exit(1);
	}		


	/* If the request ends in .html */
	if (strlen(request) - 2 > strlen(".html")) {
		if (strcmp(".html", substring(request, strlen(request) - 4, 5)) == 0) {
			status = "200";
			message = "Successful request";
			char output[1024];
			int num = sprintf(output, "%s %s %s\ntext/html\n\n", version, status, message);
			write(cd, output, num);

			// redirect future output of execvp
			dup2(cd, 1);
			dup2(cd, 2);
			close(cd);

			char *arguments[] = {"cat", request, NULL};
			execvp("cat", arguments);
			perror("Failed the exec in child.\n");
			exit(1);
		}
	}

	/* If the request has a file ending of .jpg */
	if (strlen(request) - 2 > strlen(".jpg")) {
		if (strcmp(".jpg", substring(request, strlen(request) - 3, 4)) == 0) {
			status = "200";
			message = "Successful request";
			char output[1024];
			int num = sprintf(output, "%s %s %s\nimage/jpeg\n\n", version, status, message);
			write(cd, output, num);

			// redirect future output of execvp
			dup2(cd, 1);
			dup2(cd, 2);
			close(cd);

			char *arguments[] = {"cat", request, NULL};
			execvp("cat", arguments);
			perror("Failed the exec in child.\n");
			exit(1);
		}
	}

	/* If the request has a file ending of .jpeg */
	if (strlen(request) - 2 > strlen(".jpeg")) {
		if (strcmp(".jpeg", substring(request, strlen(request) - 4, 5)) == 0) {
			status = "200";
			message = "Successful request";
			char output[1024];
			int num = sprintf(output, "%s %s %s\nimage/jpeg\n\n", version, status, message);
			write(cd, output, num);

			// redirect future output of execvp
			dup2(cd, 1);
			dup2(cd, 2);
			close(cd);

			char *arguments[] = {"cat", request, NULL};
			execvp("cat", arguments);
			perror("Failed the exec in child.\n");
			exit(1);
		}
	}

	/* If the request has a file ending of .gif */
	if (strlen(request) - 2 > strlen(".gif")) {
		if (strcmp(".gif", substring(request, strlen(request) - 3, 4)) == 0) {
			status = "200";         
			message = "Successful request";
			char output[1024];
			int num = sprintf(output, "%s %s %s\nimage/gif\n\n", version, status, message);
			write(cd, output, num);

			// redirect future output of execvp
			dup2(cd, 1);
			dup2(cd, 2);
			close(cd);

			char *arguments[] = {"cat", request, NULL};
			execvp("cat", arguments);
			perror("Failed the exec in child.\n");
			exit(1);

		}
	}

	/* If the request is a cgi script that requires execution of a basic shell command */
	if (strlen(request) - 2 > strlen(".cgi")) {
		if (strcmp(".cgi", substring(request, strlen(request) - 3, 4)) == 0) {
			status = "200";
			message = "Successful request";
			char output[1024];
			int num = sprintf(output, "HTTP/1.1 %s %s\n", status, message);
			write(cd, output, num);
			// Assuming any CGI file we use will have the "Content-type: text/html" line!

			// redirect future output of execvp
			dup2(cd, 1);
			dup2(cd, 2);
			close(cd);

			char *arguments[] = {request, NULL};
			execvp(request, arguments);
			perror("Failed the exec in child.\n");
			exit(1);
		}
	}

	/* Else, has not been implemented */
	dup2(cd, 1);
	dup2(cd, 2);

	status = "501";
	message = "Not Implemented";
	content = "No content for a request we cannot handle (501 Not Implemented)!";
	char output[1024];
	printf("%s %s %s\ntext/plain\n\n%s\n", version, status, message, content);
	exit(1);

}

/* A format string to print out the request result in the server's terminal */
static void printFinal(char *version, char *status, char *message, char *request) {
	printf("%s\t %s %s\nWill handle the request %s\n\n", version, status, message, request);
}

/* From http://www.programmingsimplified.com/c/source-code/c-substring
 * C substring function: It returns a pointer to the substring */
static char *substring(char *string, int position, int length) {
	char *pointer;
	int c;

	pointer = malloc(length+1);

	if (pointer == NULL)
	{
		printf("Unable to allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	for (c = 0 ; c < position -1 ; c++) 
		string++; 

	for (c = 0 ; c < length ; c++)
	{
		*(pointer+c) = *string;      
		string++;   
	}

	*(pointer+c) = '\0';

	return pointer;
}
