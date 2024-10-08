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
#include <sys/types.h>
#include <pwd.h>

#define BUFF_SIZE 1048
#define LOCAL 0
#define REMOTE (LOCAL + 1)
#define ok_msg_len 2
#define num_polls 2
#define term_msg_len 41

int client_side(int domain, int type, int flags, char *port, bool v, bool N,
		 char *host_name)
{  
    /* set up the host addr struct */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = domain;
    hints.ai_socktype = type;
    hints.ai_flags = 0;

    /* get host address info */
    struct addrinfo *host_info;
    int host_status = getaddrinfo(host_name, port, &hints, &host_info);
    /* check if we can get host address info */
    if(host_status != 0)
    {
	perror("getaddrinfo");
	exit(1);
    }
 
    if(v)
    {
	printf("Got host info!\n");
    }

    /* create socket for client */
    int sockfd = socket(domain, type, flags);
    if(sockfd == -1)
    {
	perror("socket");
        exit(1);
    }   

    if(v)
    {
	printf("Created a socket!\n");
    }

    /* try to connect to the host */
    if(connect(sockfd, host_info->ai_addr, host_info->ai_addrlen) == -1)
    {
	perror("connect");
	close(sockfd);
        exit(1);
    }

    if(v)
    {
	printf("Connected to the host!\n");
    }

    /* get my username to send over to server */
    uid_t uid = getuid();
    struct passwd *pwuid;
    pwuid = getpwuid(uid);

    /* check if we got the username correctly */
    if(pwuid == NULL)
    {
	perror("getpwuid");
	close(sockfd);
	exit(1);
    }	

    /* send over username to server */
    char uid_msg[BUFF_SIZE] = {0};
    strncat(uid_msg, pwuid->pw_name, strlen(pwuid->pw_name));
    uid_msg[strlen(pwuid->pw_name)] = '\0';
    printf("Waiting for response from %s\n", host_name);
    int recieved_msg = send(sockfd, uid_msg, strlen(uid_msg), 0);
    /* check if we successfully sent over the username */
    if(recieved_msg == -1)
    {
	perror("send() -> uid)");
	close(sockfd);
	exit(1);
    }
    if(v)
    {
	printf("Sent UID: %s\n", uid_msg);
    }

   /* recieve the ok message */
    char ok_msg[ok_msg_len] = {0};
    int ok_bytes = recv(sockfd, ok_msg, sizeof(ok_msg), 0);
    /* check if we successfully recieved the ok message */
    if(ok_bytes < 0)
    {
	perror("recv() -> ok msg");
        close(sockfd);
	exit(1);
    }
    /* if we got ok, then proceed to talking! */
    if(strcmp(ok_msg, "ok") != 0)
    {
	printf("%s declined connection.\n", host_name);
	close(sockfd);
	exit(1);
    }

    /* init variables for sending and recieving messages */
    bool terminated = false;
    char message[BUFF_SIZE];
    char outmessage[BUFF_SIZE];
    int received_bytes;

    /*create a poll struct */
    struct pollfd fds[num_polls];
    fds[LOCAL].fd = STDIN_FILENO;
    fds[LOCAL].events = POLLIN;
    fds[LOCAL].revents = 0;
    fds[REMOTE].events = POLLIN;
    fds[REMOTE].fd = sockfd;
    fds[REMOTE].revents = 0;

    /* if windowing mode */
    if(!N)
    {
	start_windowing();

        /* go until someone terminates */	
	while(!terminated)
	{
	    /* check if we can poll */
	    if(poll(fds, num_polls, -1) == -1)
	    {
		close(sockfd);
		perror("poll");
		exit(1);
	    }
	    
	    if(fds[LOCAL].revents & POLLIN)
	    {
		/* update the input buffer */
		update_input_buffer();
		/* see if a whole line is ready to be shipped */
		if(has_whole_line())
		{
		    /* read input! */
		    read_from_input(message, BUFF_SIZE);
		    /* send message over :) */
		    send(sockfd, message, strlen(message), 0);
		    /* is there a CTRL D? */
		    if(has_hit_eof())
		    {
			terminated = true;
			break;
                    }
		    /* reset our buffer YAY */
		    memset(message, 0, BUFF_SIZE);
		    fds[LOCAL].revents = 0;
		}
	    }
	    
            if(fds[REMOTE].revents & POLLIN)
	    {
		/* recieve a shipment when ready! */
		received_bytes = recv(sockfd, outmessage, sizeof(outmessage),
		 0);
		/* write it to output */
		write_to_output(outmessage, received_bytes);
		/* check if we got a CTRL D or CTRL C */
		if(received_bytes == 0)
		{
		   write_to_output(
		   "\nConnection disconnected, ^C to terminate.",term_msg_len);
		   pause();
		}
		/* reset our buffer YAY */
		memset(outmessage, 0, BUFF_SIZE);
		fds[REMOTE].revents = 0;
	    }
	}
    }

    /* if we are in no window mode */
    else if(N)
    {
	/* go until someone terminates */
	while(!terminated)
	{
	    /* check if we can poll */
	    if(poll(fds, num_polls, -1) == -1)
	    {
		close(sockfd);
		perror("poll");
		exit(1);
	    }
	    if(fds[LOCAL].revents & POLLIN)
	    {
		/* get message from stdin */
		read_from_input(message, sizeof(message));
		/* send that message over :) */
		send(sockfd, message, strlen(message), 0);
		/* check if client types a CTRL D */
		if(feof(stdin))
		{
		    terminated = true;
		    break;
		}
		/* Once more, reset our buffers */
		memset(message, 0, BUFF_SIZE);
		fds[LOCAL].revents = 0;
	    }
	    
	    if(fds[REMOTE].revents & POLLIN)
	    {
		/* receive our lovely message */
		received_bytes = recv(sockfd, outmessage, sizeof(outmessage),
				 0);
		/* write it to stdout so we can see*/
		fprint_to_output(outmessage);
		/* see if we got a CTRL D or CTRL C */
		if(received_bytes == 0 || has_hit_eof())
		{
		    write_to_output(
			"\nConnection disconnected, ^C to terminate.\n",
			term_msg_len);
		    close(sockfd);
		    pause();	
		}
		/* Last time, reset our buffer :) */
		memset(outmessage, 0, BUFF_SIZE);
		fds[REMOTE].revents = 0;
	    }
	}

    }
    /* stop windowing if we are in window mode */
    if(!N)
    {
        stop_windowing();
    }
    /* close those sockets! */
    close(sockfd);

    return 0;
}
