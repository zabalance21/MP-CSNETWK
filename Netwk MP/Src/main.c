#include "protocols.h"
#include "network.h"
#include "battle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

int main() {
    printf("Program started!\n");
    fflush(stdout);

    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in peer;
    ROLE role;
    int seed;

    printf("About to load pokedex...\n");
    fflush(stdout);

    Pokemon pokedex[500];
    int pokedex_count = load_pokedex("pokemon.csv", pokedex);
    printf("Pokedex loaded: %d pokemon\n", pokedex_count);
    fflush(stdout);
    if (pokedex_count== 0) {
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
    fflush(stdout);
    scanf("%d", &choice);
    role = (ROLE)choice;

    // Create & bind sockets
    if (role == HOST) {
        sock = create_udp_socket(7000);
        if (sock == INVALID_SOCKET) {
            printf("Failed to create/bind host socket on port 7000\n");
            WSACleanup();
            return 1;
        }
    } else {
        sock = create_udp_socket(0);
        if (sock == INVALID_SOCKET) {
            printf("Failed to create local socket\n");
            WSACleanup();
            return 1;
        }

        char host_ip[64];
        printf("Enter host IP: ");
        fflush(stdout);
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

    // Battle setup
    if (role == HOST || role == JOINER) {
        char pokemon_name[50];
        int sa_uses = 5, sd_uses = 5;
        char comm_mode[10] = "P2P";

        printf("\nAvailable Pokemon:\n");
        fflush(stdout);
        for(int i = 0; i < pokedex_count; i++){
            printf("%d: %s (%s/%s)\n", i+1, pokedex[i].name, pokedex[i].type1, pokedex[i].type2);
        }

        int poke_choice;
        printf("Choose a Pokemon by number: ");
        fflush(stdout);
        scanf("%d", &poke_choice);
        if(poke_choice < 1 || poke_choice > pokedex_count){
            printf("Invalid choice!\n");
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        strcpy(pokemon_name, pokedex[poke_choice-1].name);

        printf("Special Attack uses (default 5): ");
        fflush(stdout);
        scanf("%d", &sa_uses);
        printf("Special Defense uses (default 5): ");
        fflush(stdout);
        scanf("%d", &sd_uses);

        // Send own setup
        send_battle_setup(sock, &peer, pokemon_name, sa_uses, sd_uses, comm_mode);

        // Receive opponent setup
        char opp_pokemon[50], opp_mode[10];
        int opp_sa, opp_sd;
        receive_battle_setup(sock, &peer, opp_pokemon, &opp_sa, &opp_sd, opp_mode);

        printf("\nOpponent Pokémon: %s\n", opp_pokemon);
        printf("Opponent SA uses: %d, SD uses: %d\n", opp_sa, opp_sd);
        printf("Communication mode: %s\n\n", opp_mode);
    }

    printf("Setup complete. Ready for battle!\n");
    fflush(stdout);

    closesocket(sock);
    WSACleanup();
    return 0;
}
