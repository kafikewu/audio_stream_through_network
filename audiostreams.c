#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <sys/wait.h> // Remove

void main(int argc, char * argv[])
{
    if(argc != 7) {
        printf("Bad input. Exiting..\n");
        // audiostreams lambda epsilon gamma logfileS server-IP server-port
    }

    int lambda = atoi(argv[1]);
    int epsilon = atof(argv[2]);
    int gamma = atof(argv[3]);
    char logfileS[30];
    strcpy(logfileS, argv[4]);
    char serverIP[32];
    strcpy(serverIP, argv[5]);
    int serverPort = atoi(argv[6]);

    int socket_return = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_return < 0) {
        return;
    }
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP, &myaddr.sin_addr);
    myaddr.sin_port = htons(serverPort);

    int bind_return = bind(socket_return, (struct sockaddr *) &myaddr, sizeof(myaddr));
    if(bind_return < 0) {
        perror("Could not bind");
        return;
    }
    
    // printf("Recvfrom_result: %d\n", recvfrom_result);
    char filename[21];
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    client_addr.sin_family = AF_INET;
    int recvfrom_result = recvfrom(socket_return, filename, sizeof(filename), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    // char client_IP[30];
    // inet_ntop(AF_INET, &client_addr.sin_addr, client_IP, INET_ADDRSTRLEN);

    long int filename_size = strlen(filename);
    if(filename_size > 20 && filename[21] != '\0') {
        printf("Filename is too big \n");
        return;
    }
    // HAVE TO CHECK FOR SPACE AS WELL
    char payload[4];
    recvfrom_result = recvfrom(socket_return, payload, sizeof(payload), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    unsigned short blocksize;
    memcpy(&blocksize, payload + 2, 2);
    int fork_return = fork();
    if(fork_return == 0) {
        FILE* fp = fopen(filename, "r");
        if(fp == NULL) {
            perror("File read error");
            return;
        }

        char buf[blocksize]; // = (char *)malloc(blocksize); 
        printf("sizeof buffer is: %ld\n", sizeof(buf));
        assert(buf != NULL);
        int UDP_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(UDP_socket < 0) {
            perror("Socket creation error");
            exit(1);
        }
        struct sockaddr_in child_addr;
        child_addr.sin_family = AF_INET;
        child_addr.sin_port = 0;
        inet_pton(AF_INET, serverIP, &child_addr.sin_addr);
        int child_bind = bind(UDP_socket, (struct sockaddr *) &child_addr, sizeof(child_addr));
        if(child_bind < 0) {
            perror("Bind error");
        }
        else {
            printf("Bind on port: %d\n", ntohl(child_addr.sin_port));
        }
        struct timespec req;
        if(lambda == 1) {
            req.tv_sec = 1; 
        }
        else {
            req.tv_nsec = (1/(float)lambda) * 1000000000;
            printf("The sleeping seconds is: %ld\n", req.tv_nsec);
        }
        // sleep(1);
        fseek(fp, 0, SEEK_END);
        int total = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        while (fread(buf, blocksize, 1, fp) != EOF) {
            int sendto_return = sendto(UDP_socket, buf, blocksize, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
            fprintf(stdout, "AFTER THE DATA IS SENT: %u\n", (unsigned)time(NULL)); 
            if(sendto_return == -1) {
                perror("Error");
            }
            char recv_buf[2];
            struct sockaddr_in bruh;
            int sizeoofclient = sizeof(bruh);
            int cur_pos = ftell(fp);
            printf("Current percentage: %f and position: %d\n", (float)cur_pos * 100/total, cur_pos);
            int recvfrom_result = recvfrom(UDP_socket, recv_buf, sizeof(unsigned short), 0, (struct sockaddr *) &bruh, &sizeoofclient);
            if(cur_pos * 100.00/total == 100) break;
            nanosleep(&req, NULL);
            // req.tv_nsec /= 2;
        }
        fclose(fp);
        
    }
    else {
        int status;
        wait(&status);
    }
    
}