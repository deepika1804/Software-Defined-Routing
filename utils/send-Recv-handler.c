
#include <stdlib.h>
#include <sys/socket.h>

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    
    bytes = recv(sock_index, buffer, nbytes, 0);
    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);
    
    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);
    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);

    return bytes;
}

ssize_t sendAllUdpPkt(int sock_index, char *buffer, ssize_t nbytes,char *ip_addr,uint16_t toRouterPort)
{
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(struct sockaddr_in));
    ssize_t bytes = 0;

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr (ip_addr);
    dest.sin_port = htons(toRouterPort);

    bytes = sendto(sock_index, buffer, nbytes, 0,(struct sockaddr*)&dest, sizeof dest);
    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += sendto(sock_index, buffer+bytes, nbytes-bytes, 0,(struct sockaddr*)&dest, sizeof dest);
    return bytes;
}

ssize_t recvAllUdpPkt(int sock_index, char *buffer, ssize_t nbytes)
{
    struct sockaddr_in dest_addr;
    socklen_t fromlen;
    ssize_t bytes = 0;
    char ipstr[INET_ADDRSTRLEN];
    
    fromlen = sizeof(dest_addr);
   
    bytes = recvfrom(sock_index, buffer, nbytes, 0,(struct sockaddr*)&dest_addr, &fromlen);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recvfrom(sock_index, buffer+bytes, nbytes-bytes, 0,(struct sockaddr*)&dest_addr,&fromlen);

    //printf("recv()'d %d bytes of data in buf\n", bytes);

    return bytes;
}