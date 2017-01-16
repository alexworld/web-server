#include <iostream>
#include <algorithm>
#include <set>
#include <map>
#include <cassert>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <sys/epoll.h>

#include <errno.h>
#include <string.h>

int set_nonblock(int fd) {
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("3 arguments needed\n");
        return 0;
    }

    int ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (ClientSocket == -1) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }
    int port;
    sscanf(argv[3], "%d", &port);

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(port);
    SockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int Result = connect(ClientSocket, (struct sockaddr *) &SockAddr, sizeof(SockAddr));

    if (Result == -1) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }
    set_nonblock(ClientSocket);

    char Buffer[1030];
    
    int name, bound;
    sscanf(argv[1], "%d", &name);
    sscanf(argv[2], "%d", &bound);
    int state = 0;

    struct pollfd Set[2];
//    Set[0].fd = STDIN_FILENO;
//    Set[0].events = POLLIN;
    Set[0].fd = ClientSocket;
    Set[0].events = POLLIN;

    int ALL = 0;
 //   time_t start = time(NULL);

    while (true) {
        poll(Set, 1, -1);

        if (ALL == bound) {
//            printf("Elapsed time for %d messages: %d milliseconds\n", ALL, int((time(NULL) - start) * 1000));
            exit(0);
        }
        
//        if (Set[0].revents & EPOLLIN) {
//            fgets(Buffer, 1026, stdin);
//        }
        if (Set[0].revents & EPOLLIN) {
            int RecvSize = int(recv(ClientSocket, Buffer, 1025, MSG_NOSIGNAL));
            Buffer[RecvSize] = 0;
            printf("%s", Buffer);

            if (state == 0) {
                sprintf(Buffer, "L\n");
                state++;
            } else if (state == 1) {
                sprintf(Buffer, "%d\n", name);
                state++;
            } else if (state == 2) {
                sprintf(Buffer, "%d\n", name);
                state++;
            } else {
                sprintf(Buffer, "S %d a\n", (name ^ 1));
            }


            printf("%s", Buffer);

            fflush(stdout);
/*
            send(ClientSocket, Buffer, strlen(Buffer), MSG_NOSIGNAL);
*/
            ALL++;
        }
    }

    return 0;
}
