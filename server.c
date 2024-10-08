#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <talk.h>
#include <netdb.h>
#include <poll.h>

#define BUFF_SIZE 1048
#define LOCAL 0
#define REMOTE (LOCAL + 1)
#define term_msg_len 41
#define num_polls 2
#define acc_msg_len 4


/* this func creates a socket and binds it to an address
 * also waits for a connection using listen and accepts once
 * a connection is present */
int server_side(int domain, int type, int protocol, int port, bool v,
		 bool N, bool a)
{
    bool terminated = false;
    bool accepted = false;

    /* create a socket */
    int sockfd = socket(domain, type, protocol);
    if(sockfd == -1)
    {
	perror("socket");
    }
    if(v)
    {
        printf("Created a socket!: %d\n", sockfd);
    }
    /* bind the socket */
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    if (bind(sockfd,(struct sockaddr *)&server_address,
	 sizeof(server_address)) == -1)
    {
	perror("bind");
	close(sockfd);
	exit(1);
    }
    else
    {
        if(v)
	{
	    printf("Binded a socket\n");
	}
    }
    /* wait for a connection using listen */
    if(listen(sockfd, 1) == -1)
    {
	perror("listen");
        close(sockfd);
	exit(1);
    }
    else
    {
	if(v)
	{
	    printf("listening on port %d\n", port);
	}
    } 
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    if(v)
    {
        printf("Waiting for client to accept connection\n");
    }

    /* wait for client to accept the connection */
    int client_socket = accept(sockfd, (struct sockaddr *)&client_address,
			 &client_address_len);
    while (!accepted)
    {
        if(client_socket == -1)
        {
            perror("accept");
            close(sockfd);
            exit(EXIT_FAILURE);
        } 
	else
	{
	    /* break once client accepts */
	    accepted = true;
	}
    }

    /* give some verbose information abt the clients addr */
    if(v)
    {
	printf("Accepted from %s\n", inet_ntoa(client_address.sin_addr));
    }
 
    /* get the clients hostname */
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(client_socket, (struct sockaddr*)&addr, &addr_len) == -1) 
    {
        perror("getpeername");
	close(sockfd);
	close(client_socket);
        exit(1);
    }

    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    if (getnameinfo((struct sockaddr*)&addr, addr_len, host, sizeof(host),
	service, sizeof(service), 0) == 1) 
    {
        perror("getnameinfo");
	close(sockfd);
	close(client_socket);
        exit(1);
    }

    /* initializing variables to receive messages */ 
    ssize_t received_bytes;
    char user_message[BUFF_SIZE];
    memset(user_message, 0, BUFF_SIZE);

    /* get the clients username, they will send it to server */
    received_bytes = recv(client_socket, user_message, sizeof(user_message),
		0);
    /* check if server wants to accept */
    printf("Mytalk request from %s@%s Accept (y/n)?\n", user_message, host);
    /* if y | yes -> accept connection
       if n | no -> decline connection  */
   char response[acc_msg_len] = {0};
   if(!a)
   {
        scanf("%4s", &response);
   }
    char *okstr = "ok";
    int okmsg;
    /* reset our string buffers */
    char message[BUFF_SIZE];
    char outmessage[BUFF_SIZE];
    memset(message, 0, BUFF_SIZE);
    memset(outmessage, 0, BUFF_SIZE);

    /*create a poll struct */
    struct pollfd fds[num_polls];
    fds[LOCAL].fd = STDIN_FILENO;
    fds[LOCAL].events = POLLIN;
    fds[LOCAL].revents = 0;
    fds[REMOTE].events = POLLIN;
    fds[REMOTE].fd = client_socket;
    fds[REMOTE].revents = 0;

    /* close sockets if user enters "n" */
    if((!strcmp(response, "n")) || (!strcmp(response, "no")) || 
	(!strcmp(response, "No")) || (!strcmp(response, "N"))
	 && (!a))
    {
	printf("Connection Declined\n");
	close(sockfd);
        close(client_socket);
	exit(1);
    }
    /* begin messaging if "y" */
    else if((!strcmp(response, "y")) || a || (!strcmp(response, "yes")) || 
	(!strcmp(response, "Yes")) || (!strcmp(response, "Y")))
    {
	/* send out "ok" message */
        okmsg = send(client_socket, okstr, strlen(okstr), 0);
	if(okmsg == -1)
	{
	    perror("send ok");
	    close(sockfd);
	    close(client_socket);
	    exit(1);
	}

	/* start windowing if N flag is not set */
	if(!N)
	{
	    start_windowing();
	
	    /* go until we get a control D */
	    while(!terminated)
	    {
		if(poll(fds, num_polls, -1) == -1)
		{
		    close(sockfd);
		    close(client_socket);
		    perror("poll");
		    exit(1);
		}

                if(fds[LOCAL].revents & POLLIN)
 		{ 
	            /* update the input buffer line */
	            update_input_buffer();
		    if(has_whole_line())
		    {
		        /* read input, send it out */
	                read_from_input(message, BUFF_SIZE);
	                send(client_socket, message, strlen(message), 0);
			/* break if there is a CTRL D */
		        if(has_hit_eof())
	     	        {
		            terminated = true;
		            break;
		        }		    
		        /* reset buffer */
	                memset(message, 0, BUFF_SIZE);
		        fds[LOCAL].revents = 0;
		    }

		}
	        if(fds[REMOTE].revents & POLLIN)
		{
	            /* receive input from client, write to server's output */
	            received_bytes = recv(client_socket, outmessage,
					 sizeof(outmessage), 0);  
	            write_to_output(outmessage, received_bytes);
		    /* check if client has terminated */
		    if(received_bytes == 0)
		    {
		        write_to_output(
			"\nConnection disconnected, ^C to terminate.",
			 term_msg_len);
			/* do nothing, wait for a CTRL C */
			pause();
			    /*do nothing, wait for a CRTL C */			
		    }
	            /* reset buffer */
	            memset(outmessage, 0, BUFF_SIZE);
		    fds[REMOTE].revents = 0;		    
		}
	    }
	}
      
	/* no windowing */
	else if(N)
	{
	    /* go until one person terminates */
	    while(!terminated)
	    {
		/* check if we can poll */
	        if(poll(fds, num_polls, -1) == -1)
	        {
		    close(sockfd);
		    close(client_socket);
		    perror("poll");
		    exit(1);
		}

		if(fds[LOCAL].revents & POLLIN)
 		{ 
		    /* get message from stdin, sent to client */
		    read_from_input(message, sizeof(message));
		    send(client_socket, message, strlen(message), 0); 
		    /* check if CTRL D */ 
		    if(feof(stdin))
		    {
			terminated = true;
			break;
		    }
		    /* reset buffer */
		    memset(message, 0, BUFF_SIZE);
		    fds[LOCAL].revents = 0;
		}

	 	if(fds[REMOTE].revents & POLLIN)
		{
		    /* receive from client, write to stdout */
	   	    received_bytes = recv(client_socket, outmessage,
					 sizeof(outmessage), 0);  
		    write_to_output(outmessage, received_bytes);
		    /* check if client terminated */
		    if(received_bytes == 0)
		    {
		        fprint_to_output(
			"\nConnection disconnected, ^C to terminate.\n");
	     	         /* do nothing, wait for a CTRL C */
		         pause();
		    } 
		    /* reset buffer */
		    memset(outmessage, 0, BUFF_SIZE);
		    fds[REMOTE].revents = 0;	
		}
            }   
	}
    }
    /* stop windowing and also close sockets */
    if(!N)
    {
        stop_windowing();
    }
    close(sockfd);
    close(client_socket);
 
    return 0;

}
