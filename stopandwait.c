#include <stdio.h>
#include "file.h"

#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

// creating a data packet
typedef struct packet{
    char data[240];
}Packet;

// creating a packet with frame_kind (ACK, SEQ, FIN), data packet and packet size
typedef struct frame{
    int frame_kind; //ACK:0, SEQ:1 FIN:2
    int sq_no;
    int ack;
    int p_size;
    Packet packet;
}Frame;

void stopandwait_server(char* iface, long port, FILE* fp) {

    int sockfd;
    // socklen_t len;
    char buffer[240];
    //int timeout;
    int f_recv_size;
    // char some_string[1];
    int k = 0;
    struct sockaddr_in servaddr, cliaddr;
      
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Socket Creation Failed");
        exit(EXIT_FAILURE);
    }
      
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    bzero(&servaddr.sin_zero, 8);
      
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    int frame_id=0;
    Frame frame_recv;
    Frame frame_send;
      
    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
    {
        perror("Socket Bind Failed");
        exit(EXIT_FAILURE);
    }

    //struct timeval timeout;
    //timeout.tv_sec = 1;
    //timeout.tv_usec = 70 * 1000;
    int xyz = 1;
    setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &xyz, sizeof(xyz));

    socklen_t len = sizeof(cliaddr);

    while(1)
    {
            f_recv_size = recvfrom(sockfd, &frame_recv, sizeof(Frame), 0, ( struct sockaddr *) &cliaddr, &len);

            if(f_recv_size <= 0 || frame_recv.p_size == 0)
            {
                close(sockfd);
                return;
            }
            else
            {
                k = k + 1;

                //writing the frame to the file
                memcpy(buffer, frame_recv.packet.data, 240);
		if(frame_recv.p_size == 0)
		{
			//fflush(fp);
			close(sockfd);
			return;
		}
                fwrite(buffer, frame_recv.p_size, 1, fp);
                
                frame_send.sq_no = 0;
                frame_send.frame_kind = 0;
                frame_send.ack = frame_recv.sq_no + 1;

                //sending the acknowledgement
                sendto(sockfd, &frame_send, sizeof(frame_send), 0, (struct sockaddr*) &cliaddr, len);

                printf("Packet [%d] Received \n", k);
		memset(buffer, 0, sizeof(buffer));
            }
            frame_id++;
    }
}

void stopandwait_client(char* host, long port, FILE* fp) {

    int sockfd;
    // socklen_t len;
    int k = 0, x, b;
    char buffer[240];
   // int time_out = 1;
    struct sockaddr_in     servaddr;
    int f_recv_size;

    // timeout
    struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = 50 * 1000;
  
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { perror("Socket Creation Failed"); exit(EXIT_FAILURE); }
  
    memset(&servaddr, 0, sizeof(servaddr));
      
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    struct hostent *hostname = gethostbyname(host);
    unsigned int server_address = *(unsigned long*) hostname->h_addr_list[0];
    servaddr.sin_addr.s_addr = server_address;

	int frame_id = 0;
	Frame frame_send;
	Frame frame_recv;

    // timeout thingy
    setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    if(fp == NULL){ printf("\n Error in File Reading\n"); exit(0); }

    while(1)
    {
        k = k + 1;
        int ack_recv = 1;
        // printf("%d", ack_recv);
        
        if(ack_recv == 1)
        {   
            frame_send.sq_no = frame_id;
            frame_send.frame_kind = 1;
            frame_send.ack = 0;
        }
            while(!feof(fp))
            {        
                //fread returns number of bytes from the IO stream
                b = fread(buffer, 1, 240, fp);
                memcpy(frame_send.packet.data, buffer, 240);
                frame_send.p_size = b;
                // printf("MSG - %s %d \n", frame_send.packet.data, b);

                resend: x = sendto(sockfd, &frame_send, sizeof(frame_send), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                if(x == -1){ perror("Error in File Transfer");}
                // printf("SENT");
            
                socklen_t len = sizeof(servaddr);
                f_recv_size = recvfrom(sockfd, &frame_recv, sizeof(frame_recv), 0 ,(struct sockaddr*)&servaddr, &len);

                    if( f_recv_size > 0 && frame_recv.sq_no == 0 && frame_recv.ack == frame_id+1)
                    {
                        printf("Acknowledge Receieved for Packet [%d] \n", k);
			memset(buffer, 0, sizeof(buffer));
                        ack_recv = 1;
                    }
                    else if(f_recv_size == -1)
                    {
                        printf("Acknowledge Not Receieved for Packet [%d] \n", k);
                        // printf("%s \n", buffer);
                        ack_recv = 0;
                        goto resend;
                    }
                k = k + 1;
            }
            memset(buffer, 0, sizeof(buffer));
            ack_recv = 0;
            for(int i = 0; i < 7; i++)
	    {
            	sendto(sockfd, 0, 0, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            }
            close(sockfd);
	    return;
    }
}
