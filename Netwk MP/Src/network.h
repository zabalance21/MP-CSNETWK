// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>

//Constants for reliability layer
#define MAX_RETRIES 3
#define TIMEOUT_MS 500
#define BUFFER_SIZE 2048

// Creates a UDP socket bound to a given port.
SOCKET create_udp_socket(int port);

// Sends a message using UDP.
int send_udp(SOCKET sock, const char *msg, struct sockaddr_in *to);

// Receives a message using UDP.
int recv_udp(SOCKET sock, char *buffer, int max, struct sockaddr_in *from, int *fromLen);

// Closes a UDP socket.
void close_udp(SOCKET sock);

// Sends a message with retries and timeout.
// It returns 1 on success (ACK received), 0 on failure (timeout/max retries).
int send_reliable_message(SOCKET sock, const char *msg, struct sockaddr_in *to, int seq_num);

// Send an immediate ACK message.
int send_ack(SOCKET sock, struct sockaddr_in *to, int ack_num);

#endif