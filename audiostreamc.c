#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define FIFO "anotherone"
#define BUF_SIZE 4096
int RECEIVED = 0;
int fp_write, fp_read;
sem_t mutex;

void handle_alarm(int sig) {
    printf("Exit\n");
    exit(0);
}
int global_descriptor = 0;

void fifo_handler(int signum) { // Every 333 ms being called
    printf("The fifo handler is being called\n");
    sem_wait(&mutex);
    printf("Fifo handler has accessed fifo_handler\n");
    
    char * buf = malloc(BUF_SIZE);
    int read_len = read(fp_read, buf, BUF_SIZE);
    sem_post(&mutex);
    printf("The read_len is : %d\n", read_len);
}

void close_fifo(int signum) {
    close(fp_read);
    close(fp_write);
    remove(FIFO);
    printf("Program terminated\n");
    fflush(stdout);
    exit(1);
}

void main(int argc, char * argv[])
{
    signal(SIGINT, close_fifo);
    if(argc != 8) {
        printf("Bad input. Exiting..\n");
        // audiostreamc audiofile blocksize buffersize targetbuf server-IP server-port logfileC
    }
    printf("String: %s and size: %ld\n", argv[1], strlen(argv[1]));
    long sizeoffilename = strlen(argv[1]);
    char * audiofile = malloc(sizeoffilename);
    strcpy(audiofile, argv[1]);
    printf("String: %s and size: %ld and size is: %ld\n", audiofile, strlen(audiofile), sizeof(audiofile));
    unsigned short blocksize = (unsigned short)atoi(argv[2]);
    unsigned short buffersize = (unsigned short)atoi(argv[3]);
    int targetbuf = atoi(argv[4]);
    if(targetbuf % 4096 != 0) {
        printf("Target Buffer size needs to be a multiple of 4096\n");
        return;
    }
    char serverIP[32];
    strcpy(serverIP, argv[5]);
    int serverPort = atoi(argv[6]);
    char logfileC[30];
    strcpy(logfileC, argv[7]);

    int socket_return = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_return < 0) {
        return;
    }
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    // myaddr.sin_addr.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = 0;

    int bind_return = bind(socket_return, (struct sockaddr *) &myaddr, sizeof(myaddr));
    if(bind_return < 0) {
        perror("Could not bind");
        return;
    }
    else {
        char stp[30];
        inet_ntop(AF_INET, &myaddr.sin_addr, stp, INET_ADDRSTRLEN);
        // printf("Connected to ip: %s and port: %d\n", stp, ntohl(myaddr.sin_port));
    }
    /* The following needs to be changed */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP, &server_addr.sin_addr);
    server_addr.sin_port = htons(serverPort);

    int sendto_return = sendto(socket_return, audiofile, strlen(audiofile), 0, (struct sockaddr*) &server_addr, sizeof(server_addr));
    printf("Sendto return: %d\n", sendto_return);
    if(sendto_return < 0) {
        perror("Sending failed");
        return;
    }
    char payload[4];
    printf("The blocksize is: %hd\n", blocksize);
    memcpy(payload + 2, &blocksize, 2);
    memset(payload, ' ', 2);
    sendto_return = sendto(socket_return, payload, sizeof(payload), 0, (struct sockaddr*) &server_addr, sizeof(server_addr));
    
    RECEIVED = 0;
    printf("SIngal\n");
    signal(SIGALRM, handle_alarm);
    sem_init(&mutex, 0, 1);

    char recv_buf[blocksize];// = malloc();
    server_addr.sin_port = 0;
    int server_addr_len = sizeof(server_addr);
    // ualarm(.333 * 1000000, 0);
    printf("Waiting for the 1st recvfrom\n");
    printf("MY IP address: %s\n", inet_ntoa(myaddr.sin_addr));
    printf("My port: %d\n", ntohs(myaddr.sin_port));
    struct sockaddr_in new;
    new.sin_family = AF_INET;
    
    signal(SIGALRM, SIG_DFL);

    
    int idk_man = sizeof(new);
    int recvfrom_result = recvfrom(socket_return, recv_buf, blocksize, 0, (struct sockaddr *) &server_addr, &server_addr_len);

    if(recvfrom_result <= 0) {
        return;
    }
    // ualarm(0, 0);
    unsigned short bufferstate = 1000;
    sendto_return = sendto(socket_return, &bufferstate, sizeof(bufferstate), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // The first block may be subject to different time scedule. 
    // Thus, it will be handled later.
    // The initialization of FIFO


    signal(SIGALRM, fifo_handler);
    ualarm(333333, 333333); // Should be changed
    // printf("Signal for 333 ms is initiated\n");

    // int fp = open(FIFO, O_WRONLY | O_NONBLOCK);
    int mkfifo_ret = mkfifo(FIFO, 0777);
    printf("MKFIFO: %d\n", mkfifo_ret);
    if(mkfifo_ret == -1) {
        perror("mkfifo");
        exit(1);
    }
    

    fp_read = open(FIFO, O_RDONLY | O_NONBLOCK);
    if(fp_read < 0) {
        perror("Bad file for reading");
    }

    fp_write = open(FIFO, O_WRONLY | O_NONBLOCK);
    // fcntl(fp_write, F_SETFL, O_NONBLOCK);
    if(fp_write < 0) {
        perror("Bad file for writing");
    }
    
    int error_counter = 0;

    while(1) {
        RECEIVED = 0;
        struct timeval start;
	    struct timeval end;

        time_t mytime = time(NULL);
        char * time_str = ctime(&mytime);
        time_str[strlen(time_str)-1] = '\0';
        fprintf(stdout, "BEFORE RECEIVES: %s\n", time_str); 
        
        gettimeofday(&start, 0);
        int recvfrom_result = recvfrom(socket_return, recv_buf, blocksize, 0, (struct sockaddr *) &server_addr, &server_addr_len);
        
        time_t mytime1 = time(NULL);
        char * time_str2 = ctime(&mytime1);
        time_str2[strlen(time_str2)-1] = '\0';
        fprintf(stdout, "AFTER RECEIVES: %s\n", time_str2);

        if(recvfrom_result < 0) {
            // Placeholder
            return;
        }
        // else {
        //     printf("BUG SUCCESSSS\n");
        //     exit(0);
        // }

        gettimeofday(&end, 0);
        sem_wait(&mutex);
        printf("==========The mutex is now allocated=============\n");
        
        // printf("FIFO OPENED %d\n", fp);
        // fflush(stdout);
        printf("before going into write\n");
        fflush(stdout);
        int write_len = write(fp_write, recv_buf, BUF_SIZE);
        printf("Write done\n");
        fflush(stdout);
        if(write_len < 0) {
            perror("Write error");
            printf("Hit error after: %d successive writes\n", error_counter);
            error_counter = 0;
        }
        else {
            error_counter++;
        }
        sem_post(&mutex);
        printf("==========The write status is: %d============\n", write_len);
        fflush(stdout);
        unsigned long e_usec = ((end.tv_sec * 1000000) + end.tv_usec) - ((start.tv_sec * 1000000) + start.tv_usec);
        printf("File recieved of size %d completion time: %.3fmsec\n", recvfrom_result, e_usec / 1000.00);
        sendto_return = sendto(socket_return, &bufferstate, sizeof(bufferstate), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        struct timespec req;
        req.tv_nsec = 3330000000;
        nanosleep(&req, NULL);
    }
    close(fp_read);
    close(fp_write);
    remove(FIFO);
}