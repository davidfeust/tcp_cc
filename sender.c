#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define FILE_TO_SEND    "1mb.txt"
#define SERVER_PORT 5060

int fd;
struct stat file_stat;

int main(int argc, char **argv) {
    char buf[256];
    socklen_t len;
    int file_num = 1;

    for (int j = 0; j < 2; j++) {
        if (j == 0) {
            strcpy(buf, "cubic");
        } else {
            strcpy(buf, "reno");
        }

        for (int i = 0; i < 5; i++) {

//          1. open new socket
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) {
                perror("failed to open socket");
                return -1;
            }
            printf("New socket opened\n");

            len = sizeof(buf);
            if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0) {
                perror("setsockopt");
                return -1;
            }

            if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0) {
                perror("getsockopt");
                return -1;
            }

            printf("Current: %s\n", buf);


//          2. create connection to measure
//          Create a new sockaddr_in, and puting the server port into it
            struct sockaddr_in serverAddress;
            memset(&serverAddress, 0, sizeof(serverAddress));
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(SERVER_PORT);

            int connect_flag = connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

            printf("connect_flag = %d\n", connect_flag);

//          3. sends large file 5 times
            FILE *file_ptr = fopen("1mb.txt", "rb");
            if (file_ptr == NULL) {
                fprintf(stderr, "Error opening file");
                return 1;
            }
            fd = open(FILE_TO_SEND, O_RDONLY);
            if (fd == -1) {
                fprintf(stderr, "Error opening file: %s", strerror(errno));

                exit(EXIT_FAILURE);
            }

//          Get file stats
            if (fstat(fd, &file_stat) < 0) {
                fprintf(stderr, "Error fstat: %s", strerror(errno));

                exit(EXIT_FAILURE);
            }

            fprintf(stdout, "File Size: %ld bytes\n", file_stat.st_size);

            // get permit from server to sent the file
            char getReply[10];
            bzero(getReply, sizeof(getReply));
            read(sock, getReply, sizeof(getReply));
            if (strcmp(getReply, "OK") == 0) {
                printf("From Server : %s\n", getReply);
            } else {
                printf("The Server didnt answer\n");
            }
//            Get permit from server
//            Receive a reply from the server
            char sendbuffer[100];
            int b;
            int sum = 0;
            do {
                b = fread(sendbuffer, 1, sizeof(sendbuffer), file_ptr);
                int bytesSent = send(sock, sendbuffer, b, 0);
                sum += bytesSent;
            } while (!feof(file_ptr));

            printf("The client send %d bytes, file number %d \n", sum, file_num++);
            sleep(1);

//          6. close socket
            close(sock);
        }
    }
    return 0;
}
