#include "network.h"
#include <stdio.h>
#include <time.h>


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

// Sends an immediate ACK message with the corresponding sequence number.
int send_ack(SOCKET sock, struct sockaddr_in *to, int ack_num) {
    char ack_msg[64];
    snprintf(ack_msg, sizeof(ack_msg), "message_type: ACK\nack_number: %d\n", ack_num);
    return send_udp(sock, ack_msg, to) >= 0 ? 1 : 0;
}

// Wrapper around send_udp that handles retries and ACKs
int send_reliable_message(SOCKET sock, const char *msg, struct sockaddr_in *to, int seq_num) {
    char buffer[BUFFER_SIZE];
    int retries = 0;

    // Use select for a non-blocking receive with a timeout
    struct timeval tv;

    while (retries < MAX_RETRIES) {

        // 1. SEND MESSAGE
        printf("[REL] Attempting send (Seq: %d, Retry: %d/%d)... ", seq_num, retries + 1, MAX_RETRIES);
        if (send_udp(sock, msg, to) < 0) {
            printf("Error sending initial packet.\n");
            return 0; // Fatal send error
        }

        // 2. WAIT FOR ACK (Using select for timeout)
        tv.tv_sec = 0;
        tv.tv_usec = TIMEOUT_MS * 1000; // Convert ms to microseconds

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        // Select blocks until data is available or timeout expires
        int ready = select(0, &read_fds, NULL, NULL, &tv);

        if (ready == SOCKET_ERROR) {
            printf("Socket error during select.\n");
            return 0;
        }

        if (ready > 0) {
            // Data is available (Potential ACK)
            struct sockaddr_in from;
            int from_len = sizeof(from);

            // recv_udp must be able to receive an ACK
            int rv = recv_udp(sock, buffer, BUFFER_SIZE - 1, &from, &from_len);
            if (rv > 0) {
                buffer[rv] = '\0';

                // 3. CHECK FOR CORRECT ACK
                char type[64];
                int ack_num = -1;

                // Simple parsing to check message type and ack_number
                if (sscanf(buffer, "message_type: %s\nack_number: %d\n", type, &ack_num) == 2) {
                    if (strcmp(type, "ACK") == 0 && ack_num == seq_num) {
                        // SUCCESS: Correct ACK received!
                        printf("ACK received for Seq: %d. Success!\n", seq_num);
                        return 1;
                    }
                }

                // If it's not the correct ACK, it might be another message.
                // For simplicity in this reliable send function, we treat it as a missed ACK
                // and loop to re-send (unless you implement filtering/queuing).
            }
        }

        // 4. RETRY (Timeout or invalid response)
        printf("WARN: Timeout or wrong ACK for Seq: %d. Retrying (%d/%d)...\n", seq_num, retries + 1, MAX_RETRIES);
        retries++;

        // Brief sleep to avoid flooding the network/CPU before the next retry
        Sleep(50); // Winsock Sleep function
    }

    // 5. TERMINATE
    printf("FATAL: Max retries reached for Seq: %d. Connection lost.\n", seq_num);
    return 0;
}

