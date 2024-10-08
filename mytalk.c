#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <getopt.h>
#include "server.h"
#include "client.h"
#include <netdb.h>

#define min_arg_len 2
#define min_port_num 1024
#define max_port_num 65535
#define hostidx 2

int main(int argc, char *argv[])
{
    /* setting bools for flag */
    bool v = false;
    bool a = false;
    bool N = false;

    /* check if there are enough argc's */
    if(argc < min_arg_len)
    {
	perror("Not enough command line arguments");
	exit(1);
    }

    /* set a struct for getopt_long */
    static struct option argv_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"ncurses", no_argument, 0, 'N'},
        {"aceept", no_argument, 0, 'a'},
        {0, 0, 0, 0}  // Required to end the array
    };

    int option;
    /* check for all the flags set */
    while ((option = getopt_long(argc, argv, "vaN", argv_options, NULL)) != -1)
     {
	switch(option)
	{
	    case '?':
		perror(
		"Invalid arguments. -> mytalk [-v] [-a] [-N] [hostname] port");
		exit(1);
	    case 'v':
		v = true;
		break;
	    case 'a':
		a = true;
		break;
	    case 'N':
		N = true;
		break;
	}
    }

    /* check if port is less than 1024, or greater than 65535 */
    int portcheck = atoi(argv[argc - 1]);
    if(portcheck < min_port_num || portcheck > max_port_num)
    {
	perror("port num has to be at least 1024 and greater than 65535.");
	exit(1);
    }

    /* check if we are gonna have a server or a client */
    /* provided a server */    
    if((argc - optind ) == 1)
    {
	int port = atoi(argv[argc - 1]);
	if(v)
	{
	    printf("Port num: %d\n", port);
	}
	server_side(AF_INET, SOCK_STREAM, 0, port, v, N, a);
    }
    /* provided a client */
    else if((argc - optind) == min_arg_len)
    {
	char *port = argv[argc - 1];
	if(v)
	{
	    printf("Port num: %s\n", port);
	}
        client_side(AF_INET, SOCK_STREAM, 0, port, v, N, argv[argc - hostidx]);
    }
    else
    {
	perror("Invalid arguments. -> mytalk [-v] [-a] [-N] [hostname] port");
	exit(1);
    }
        
    return 0;

}
