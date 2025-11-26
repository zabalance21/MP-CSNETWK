
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <battle.h>
#include "protocols.h"
#include "network.h"
#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in peer;
    ROLE role;
    int seed;

    Pokemon pokedex[500];
    int pokedex_count = load_pokedex("pokemon.csv", pokedex);
    if (pokedex_count == 0) {
        printf("Failed to load pokedex.\n");
        return 1;
    }

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

    Pokemon myPokemon, oppPokemon;
    int my_sa = 5, my_sd = 5, opp_sa, opp_sd;
    char comm_mode[10] = "P2P";

    if (role == HOST || role == JOINER) {
        char pokemon_name[50];
        printf("Enter your Pokémon name: ");
        scanf("%s", pokemon_name);

        // Lookup chosen Pokémon in pokedex
        int found = 0;
        for (int i = 0; i < pokedex_count; i++) {
            if (strcmp(pokedex[i].name, pokemon_name) == 0) {
                myPokemon = pokedex[i];
                found = 1;
                break;
            }
        }
        if (!found) { printf("Pokémon not found!\n"); closesocket(sock); WSACleanup(); return 1; }

        send_battle_setup(sock, &peer, pokemon_name, my_sa, my_sd, comm_mode);

        char opp_name[50], opp_mode[10];
        receive_battle_setup(sock, &peer, opp_name, &opp_sa, &opp_sd, opp_mode);

        // Lookup opponent Pokémon
        found = 0;
        for (int i = 0; i < pokedex_count; i++) {
            if (strcmp(pokedex[i].name, opp_name) == 0) {
                oppPokemon = pokedex[i];
                found = 1;
                break;
            }
        }
        if (!found) { printf("Opponent Pokémon not found!\n"); closesocket(sock); WSACleanup(); return 1; }

        printf("Opponent Pokémon: %s\n", oppPokemon.name);
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
