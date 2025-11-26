#include "protocols.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in peer;
    ROLE role;
    int seed;

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Winsock initialization failed!\n");
        return 1;
    }

    // Ask for role
    int choice;
    printf("Select role: 0=HOST, 1=JOINER, 2=SPECTATOR: ");
    scanf("%d", &choice);
    role = (ROLE)choice;

    // Create & bind sockets correctly: Host binds to 7000; others can use ephemeral port
    if (role == HOST) {
        sock = create_udp_socket(7000); // Reason why it didnt crash
        if (sock == INVALID_SOCKET) {
            printf("Failed to create/bind host socket on port 7000\n");
            WSACleanup();
            return 1;
        }
    } else {
        // Joiner / Spectator bind ephemeral port
        sock = create_udp_socket(0);
        if (sock == INVALID_SOCKET) {
            printf("Failed to create local socket\n");
            WSACleanup();
            return 1;
        }

        // Set peer (host) address for sending handshake
        char host_ip[64];
        printf("Enter host ip: ");
        scanf("%s", host_ip);

        memset(&peer, 0, sizeof(peer));
        peer.sin_family = AF_INET;
        peer.sin_port = htons(7000);
        peer.sin_addr.s_addr = inet_addr(host_ip);
    }

    // Perform handshake
    if (!perform_handshake(sock, role, &peer, &seed)) {
        printf("Handshake failed!\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("Handshake success! Shared seed: %d\n", seed);

    // Battle setup (HOST and JOINER only)
    if (role == HOST || role == JOINER) {
        char pokemon_name[50];
        int sa_uses, sd_uses;
        char comm_mode[10] = "P2P";

        // Ask user for their Pokémon and boosts
        printf("Enter your Pokémon name: ");
        scanf("%s", pokemon_name);
        printf("Special Attack uses: ");
        scanf("%d", &sa_uses);
        printf("Special Defense uses: ");
        scanf("%d", &sd_uses);

        // Send own BATTLE_SETUP
        send_battle_setup(sock, &peer, pokemon_name, sa_uses, sd_uses, comm_mode);

        // Receive opponent BATTLE_SETUP
        char opp_pokemon[50], opp_mode[10];
        int opp_sa, opp_sd;
        receive_battle_setup(sock, &peer, opp_pokemon, &opp_sa, &opp_sd, opp_mode);

        printf("Opponent Pokémon: %s\n", opp_pokemon);
        printf("Opponent SA uses: %d, SD uses: %d\n", opp_sa, opp_sd);
        printf("Communication mode: %s\n", opp_mode);
    }

    // ----------------------------
    // 6. Ready to start turn-based battle (Step 7)
    // ----------------------------
    printf("Setup complete. Ready for battle!\n");

    closesocket(sock);
    WSACleanup();
    return 0;
}
