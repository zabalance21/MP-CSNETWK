#include "network.h"
#include <stdio.h>

// Creates and binds a UDP socket into a port
// Returns the socket so user can call it 
SOCKET create_udp_socket(int port){
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket

    if (s == INVALID_SOCKET) {
        printf("Error creating socket.\n");
        return INVALID_SOCKET;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // Listens on all interfaces
    addr.sin_port = htons(port); // Converts to network order

    if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){ // Bind the socket to the port
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

// Sends a UDP Message to a specific address (IP + port)
int send_udp(SOCKET sock, const char *msg, struct  sockaddr_in *to){
    return sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) to, sizeof(*to));
}

// Receives a UDP message (blocking wait).
// Stores sender's address in 'from' so we know who sent it.
int recv_udp(SOCKET sock, char *buffer, int max, struct sockaddr_in *from, int *from_len){
    return recvfrom(sock, buffer, max, 0, (struct sockaddr*) from, from_len);
}


// Closes a UDP socket
void close_udp(SOCKET sock){
    closesocket(sock);
}

