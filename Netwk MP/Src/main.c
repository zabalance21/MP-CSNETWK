
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

    // Battle setup (HOST and JOINER only)
    if (role == HOST || role == JOINER) {
        char pokemon_name[50];
        int sa_uses = 5, sd_uses = 5; // default to 5
        char comm_mode[10] = "P2P";

        // Show available Pokémon to choose from
        printf("\nAvailable Pokémon:\n");
        for(int i = 0; i < pokedex_count; i++){
            printf("%d: %s (%s/%s)\n", i+1, pokedex[i].name, pokedex[i].type1, pokedex[i].type2);
        }

        int poke_choice;
        printf("Choose a Pokémon by number: ");
        scanf("%d", &poke_choice);
        if(poke_choice < 1 || poke_choice > pokedex_count){
            printf("Invalid choice!\n");
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        strcpy(pokemon_name, pokedex[poke_choice-1].name);

        // Optionally ask for special boosts
        printf("Special Attack uses (default 5): ");
        scanf("%d", &sa_uses);
        printf("Special Defense uses (default 5): ");
        scanf("%d", &sd_uses);

        // Send own BATTLE_SETUP
        send_battle_setup(sock, &peer, pokemon_name, sa_uses, sd_uses, comm_mode);

        // Receive opponent BATTLE_SETUP
        char opp_pokemon[50], opp_mode[10];
        int opp_sa, opp_sd;
        receive_battle_setup(sock, &peer, opp_pokemon, &opp_sa, &opp_sd, opp_mode);

        printf("\nOpponent Pokémon: %s\n", opp_pokemon);
        printf("Opponent SA uses: %d, SD uses: %d\n", opp_sa, opp_sd);
        printf("Communication mode: %s\n\n", opp_mode);
    }
    // ----------------------------
    // 6. Ready to start turn-based battle (Step 7)
    // ----------------------------
    printf("Setup complete. Ready for battle!\n");

    closesocket(sock);
    WSACleanup();
    return 0;
}
