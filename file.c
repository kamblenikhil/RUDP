#include "file.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
/*
 *  Here is the starting point for your netster part.2 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */

void tcp_file_server(long port, FILE *fp)
{
    int server_fd = 0;
    // int server_socket= 0;
    int client_socket= 0;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int n;
    char buffer[255] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("Socket Creation Failed"); exit(0); }
       
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){ perror("setsockopt Error"); exit(0); }

    memset(&address, 0, sizeof(address));
    
    //ip address and port number assignment
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
       
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ perror("Socket Bind Failed Error"); exit(0); }
    
    if (listen(server_fd, 3) < 0){ perror("Socket Listen Failed"); exit(0); }

    while(1)
    {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0){ perror("Socket Accept Failed"); exit(0); }
        else {
                
                while(!feof(fp))
                {
                    //recv returns number of bytes from the IO stream
                    n = recv( client_socket , buffer, sizeof(buffer), 0);

                    if(n <= 0)
                    {
                        memset(buffer, 0, sizeof(buffer));
                        fclose(fp);
                        close(client_socket);
                        exit(0);
                    }
                    fwrite(buffer, n, 1, fp);
                }
            }   
    }

}

void tcp_file_client(long port, FILE *fp)
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[255] = {0};
    int b;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ printf("\n Socket creation error \n"); exit(0); }
   
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    //ip address and port number assignment
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
       
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0){ printf("\n IP Address not valid \n"); exit(0); }
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ printf("\n Socket Failed Connection\n"); exit(0); }
    
    if(fp == NULL){ printf("\n Error in file reading\n"); exit(0); }

    while(!feof(fp))
    {
        //fread returns number of bytes from the IO stream
        b = fread(buffer, 1, sizeof(buffer), fp);
        send(sock, buffer, b, 0);
    }
    memset(buffer, 0, sizeof(buffer));
    fclose(fp); 
    close(sock);
    exit(0);
}

void udp_file_server(long port, FILE *fp){

    int sockfd;
    char buffer[255] = {0};
    int n;
    struct sockaddr_in servaddr, cliaddr;
      
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Socket Creation Failed");
        exit(EXIT_FAILURE);
    }
      
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
      
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
      
    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
    {
        perror("Socket Bind Failed");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        unsigned int len = sizeof(cliaddr);

        while(!feof(fp))
        {
            //recvfrom returns number of bytes from the IO stream
            n = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, ( struct sockaddr *) &cliaddr, &len);
            
            if(n < 0){
                perror("Receieved Failed");
                break;
            }
            else if(n == 0)
            {
                fclose(fp);
                close(sockfd);
                exit(0);
            }
            else
            {
                fwrite(buffer, n, 1, fp);
            }
        }
    }
}

void udp_file_client(long port, FILE *fp){
    int sockfd;
    char buffer[255] = {0};
    struct sockaddr_in     servaddr;
    int b;
  
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Socket Creation Failed");
        exit(EXIT_FAILURE);
    }
  
    memset(&servaddr, 0, sizeof(servaddr));
      
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if(fp == NULL){ printf("\n Error in file reading\n"); exit(0); }

    while(1)
    {
        // fseek(fp, 0, SEEK_SET);

        while(!feof(fp))
        {
            //fread returns number of bytes from the IO stream
            b = fread(buffer, 1, sizeof(buffer), fp);
            sendto(sockfd, buffer, b, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        }
        sendto(sockfd, 0, 0, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        memset(buffer, 0, sizeof(buffer));
        fclose(fp);
        close(sockfd);
        exit(0);
    }
}

void file_server(char* iface, long port, int use_udp, FILE* fp) 
{
    if(use_udp)
    {
        // printf("udp file server");
        udp_file_server(port, fp);
    }
    else
    {
        // printf("tcp file server");
        tcp_file_server(port, fp);
    }  
}

void file_client(char* host, long port, int use_udp, FILE* fp) 
{
    if(use_udp)
    {
        // printf("udp file client");
        udp_file_client(port, fp);
    }
    else
    {
        // printf("tcp file client");
        tcp_file_client(port, fp);
    }
}
