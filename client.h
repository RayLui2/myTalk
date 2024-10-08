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


int client_side(int domain, int type, int flags, char* port, bool v, bool N,
 char *host_name);
