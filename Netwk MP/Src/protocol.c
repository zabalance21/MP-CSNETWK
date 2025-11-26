/*
 * protocols.c
 *
 * Implements the PokeProtocol message send/receive helpers over UDP.
 * Each function corresponds to a protocol message described in the RFC:
 * - handshake (HANDSHAKE_REQUEST / HANDSHAKE_RESPONSE / SPECTATOR_REQUEST)
 * - battle setup (BATTLE_SETUP)
 * - turn messages (ATTACK_ANNOUNCE / DEFENSE_ANNOUNCE)
 * - calculation phase (CALCULATION_REPORT / CALCULATION_CONFIRM / RESOLUTION_REQUEST)
 * - game termination (GAME_OVER)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "protocols.h"   
#include "network.h"    
#include <winsock2.h>    


// ---------------------------------------------------------
// HANDSHAKE
// ---------------------------------------------------------
// PROTOCOL: Establishes a logical session and shares a random seed.
// - HOST: waits for a HANDSHAKE_REQUEST or SPECTATOR_REQUEST, generates a seed,
//   and returns HANDSHAKE_RESPONSE with that seed.
// - JOINER/SPECTATOR: sends HANDSHAKE_REQUEST / SPECTATOR_REQUEST and waits for response.
// The shared seed allows deterministic, synchronized RNG use (damage RNG, etc.).
int perform_handshake(SOCKET sock, ROLE role, struct sockaddr_in *peer, int *seed){
    char buffer[512];
    int from_len = sizeof(*peer);
    int rv;

    if(role == HOST){
        // Host blocks waiting for an initial request from a joiner or a spectator.
        // This is the logical "connection" step for UDP.
        printf("Waiting for Joiner or Spectator Handshake...\n");
        rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
        if(rv <= 0){
            // recv failed or returned 0
            return 0;
        }
        buffer[rv] = '\0';
        printf("Handshake request received:\n%s\n", buffer);

        // Detect type of requester
        if(strstr(buffer, "HANDSHAKE_REQUEST") != NULL){
            printf("Detected JOINER handshake request.\n");
        } else if(strstr(buffer, "SPECTATOR_REQUEST") != NULL){
            printf("Detected SPECTATOR handshake request.\n");
        } else {
            // Unknown handshake content — reject
            printf("Unknown handshake request.\n");
            return 0;
        }

        // Generate a shared seed and send it back to the peer.
        srand((unsigned)time(NULL));
        *seed = rand() % 100000; // reasonably sized seed

        // Compose HANDSHAKE_RESPONSE (newline-separated key: value pairs)
        snprintf(buffer, sizeof(buffer),
                 "message_type: HANDSHAKE_RESPONSE\n"
                 "seed: %d\n",
                 *seed);

        rv = send_udp(sock, buffer, peer);
        if(rv < 0){
            printf("Failed to send HANDSHAKE_RESPONSE.\n");
            return 0;
        }

        return 1; // success

    } else {
        // JOINER or SPECTATOR: send handshake to host and wait for response
        if(role == JOINER){
            snprintf(buffer, sizeof(buffer), "message_type: HANDSHAKE_REQUEST\n");
        } else {
            snprintf(buffer, sizeof(buffer), "message_type: SPECTATOR_REQUEST\n");
        }

        rv = send_udp(sock, buffer, peer);
        if(rv < 0){
            printf("Failed to send handshake to host.\n");
            return 0;
        }

        // Wait for HOST response
        rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
        if(rv <= 0){
            printf("No handshake response from host (recv returned %d).\n", rv);
            return 0;
        }
        buffer[rv] = '\0';
        printf("Handshake response received:\n%s\n", buffer);

        // Extract seed field (simple parsing -- expects well-formed message)
        char *seed_ptr = strstr(buffer, "seed:");
        if(seed_ptr){
            *seed = atoi(seed_ptr + 5);
            printf("Shared seed: %d\n", *seed);
            return 1;
        } else {
            printf("HANDSHAKE_RESPONSE missing seed.\n");
            return 0;
        }
    }
}


// ---------------------------------------------------------
// BATTLE_SETUP
// ---------------------------------------------------------
// PROTOCOL: Exchange starting configuration for the battle.
// - Sent by both playing peers after handshake.
// - Contains communication_mode, pokemon_name, and stat_boost allocations.
// Purpose: ensures both peers build identical initial battle state.
int send_battle_setup(SOCKET sock, struct sockaddr_in *peer, const char *pokemonName,
                      int sa_uses, int sd_uses, const char *mode){
    char buffer[1024];

    snprintf(buffer, sizeof(buffer),
             "message_type: BATTLE_SETUP\n"
             "communication_mode: %s\n"
             "pokemon_name: %s\n"
             "stat_boosts: {\"special_attack_uses\": %d, \"special_defense_uses\": %d}\n",
             mode, pokemonName, sa_uses, sd_uses);

    return send_udp(sock, buffer, peer) >= 0 ? 1 : 0;
}

// Receives a BATTLE_SETUP message and extracts pokemon_name, sa_uses, sd_uses, and mode.
// NOTE: Parsing here assumes the sender used the canonical message format.
int receive_battle_setup(SOCKET sock, struct sockaddr_in *peer, char *pokemonName,
                         int *sa_uses, int *sd_uses, char *mode){
    char buffer[1024];
    int from_len = sizeof(*peer);
    int rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
    if(rv <= 0) return 0;
    buffer[rv] = '\0';

    // Extract pokemon_name
    char *p = strstr(buffer, "pokemon_name:");
    if(p){
        // read token until newline or space
        if(sscanf(p, "pokemon_name: %49s", pokemonName) != 1) pokemonName[0] = '\0';
    } else {
        pokemonName[0] = '\0';
    }

    // Extract stat boosts
    char *s = strstr(buffer, "stat_boosts:");
    if(s){
        // Expect: stat_boosts: {"special_attack_uses": 5, "special_defense_uses": 5}
        if(sscanf(s, "stat_boosts: {\"special_attack_uses\": %d, \"special_defense_uses\": %d}",
                  sa_uses, sd_uses) != 2){
            *sa_uses = *sd_uses = 0;
        }
    } else {
        *sa_uses = *sd_uses = 0;
    }

    // Extract communication mode
    char *m = strstr(buffer, "communication_mode:");
    if(m && mode != NULL){
        if(sscanf(m, "communication_mode: %9s", mode) != 1) mode[0] = '\0';
    } else if(mode != NULL) {
        mode[0] = '\0';
    }

    return 1;
}


// ---------------------------------------------------------
// ATTACK_ANNOUNCE
// ---------------------------------------------------------
// PROTOCOL: Attacker announces chosen move for current turn.
// - Contains move_name and sequence_number.
// - Sequence numbers are used to order turn messages over UDP.
int send_attack_announce(SOCKET sock, struct sockaddr_in *peer, const char *move_name, int seq_num){
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "message_type: ATTACK_ANNOUNCE\n"
             "move_name: %s\n"
             "sequence_number: %d\n",
             move_name, seq_num);
    return send_udp(sock, buffer, peer) >= 0 ? 1 : 0;
}

int receive_attack_announce(SOCKET sock, struct sockaddr_in *peer, char *move_name, int *seq_num){
    char buffer[256];
    int from_len = sizeof(*peer);
    int rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
    if(rv <= 0) return 0;
    buffer[rv] = '\0';

    char *m = strstr(buffer, "move_name:");
    char *s = strstr(buffer, "sequence_number:");
    if(m) sscanf(m, "move_name: %49s", move_name);
    if(s && seq_num) sscanf(s, "sequence_number: %d", seq_num);

    return 1;
}


// ---------------------------------------------------------
// DEFENSE_ANNOUNCE
// ---------------------------------------------------------
// PROTOCOL: Defender confirms receipt of ATTACK_ANNOUNCE and signals readiness.
// - This provides a synchronization point so both peers move to calculation phase.
int send_defense_announce(SOCKET sock, struct sockaddr_in *peer, int seq_num){
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "message_type: DEFENSE_ANNOUNCE\n"
             "sequence_number: %d\n",
             seq_num);
    return send_udp(sock, buffer, peer) >= 0 ? 1 : 0;
}

int recv_defense_announce(SOCKET sock, struct sockaddr_in *peer, int *seq_num){
    char buffer[128];
    int from_len = sizeof(*peer);
    int rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
    if(rv <= 0) return 0;
    buffer[rv] = '\0';

    char *s = strstr(buffer, "sequence_number:");
    if(s && seq_num) sscanf(s, "sequence_number: %d", seq_num);

    return 1;
}


// ---------------------------------------------------------
// CALCULATION_REPORT
// ---------------------------------------------------------
// PROTOCOL: After both sides compute damage, each sends a CALCULATION_REPORT.
// - Contains attacker, move_used, remaining_health, damage_dealt, defender_hp_remaining,
//   status_message and sequence_number.
// - Acts as a "checksum" / authoritative report for the turn so peers can compare results.
int send_calculation_report(SOCKET sock, struct sockaddr_in *peer,
                            const char *attacker, const char *move_used,
                            int remainingHealth, int damageDealt, int defender_hp_remaining,
                            const char *status_msg, int seq_num){
    char buffer[1024];

    // Format all fields as newline-separated key:value pairs
    snprintf(buffer, sizeof(buffer),
             "message_type: CALCULATION_REPORT\n"
             "attacker: %s\n"
             "move_used: %s\n"
             "remaining_health: %d\n"
             "damage_dealt: %d\n"
             "defender_hp_remaining: %d\n"
             "status_message: %s\n"
             "sequence_number: %d\n",
             attacker ? attacker : "unknown",
             move_used ? move_used : "unknown",
             remainingHealth, damageDealt, defender_hp_remaining,
             status_msg ? status_msg : "", seq_num);

    return send_udp(sock, buffer, peer) >= 0 ? 1 : 0;
}

int recv_calculation_report(SOCKET sock, struct sockaddr_in *peer,
                            char *attacker, char *move_used, int *remainingHealth,
                            int *damageDealt, int *defender_hp_remaining, char *status_msg, int *seq_num){
    char buffer[1024];
    int from_len = sizeof(*peer);
    int rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
    if(rv <= 0) return 0;
    buffer[rv] = '\0';

    char *ptr;
    if((ptr = strstr(buffer, "attacker:")) && attacker) sscanf(ptr, "attacker: %49s", attacker);
    if((ptr = strstr(buffer, "move_used:")) && move_used) sscanf(ptr, "move_used: %49s", move_used);
    if((ptr = strstr(buffer, "remaining_health:")) && remainingHealth) sscanf(ptr, "remaining_health: %d", remainingHealth);
    if((ptr = strstr(buffer, "damage_dealt:")) && damageDealt) sscanf(ptr, "damage_dealt: %d", damageDealt);
    if((ptr = strstr(buffer, "defender_hp_remaining:")) && defender_hp_remaining) sscanf(ptr, "defender_hp_remaining: %d", defender_hp_remaining);
    if((ptr = strstr(buffer, "status_message:")) && status_msg) {
        // status_message may contain spaces until newline
        sscanf(ptr, "status_message: %511[^\n]", status_msg);
    }
    if((ptr = strstr(buffer, "sequence_number:")) && seq_num) sscanf(ptr, "sequence_number: %d", seq_num);

    return 1;
}


// ---------------------------------------------------------
// CALCULATION_CONFIRM
// ---------------------------------------------------------
// PROTOCOL: Confirms that the received CALCULATION_REPORT matches the receiver's local calculation.
// - This ACK finalizes the turn's calculation phase (if both peers confirm).
int send_calculation_confirm(SOCKET sock, struct sockaddr_in *peer, int seq_num){
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "message_type: CALCULATION_CONFIRM\n"
             "sequence_number: %d\n",
             seq_num);
    return send_udp(sock, buffer, peer) >= 0 ? 1 : 0;
}

int recv_calculation_confirm(SOCKET sock, struct sockaddr_in *peer, int *seq_num){
    char buffer[128];
    int from_len = sizeof(*peer);
    int rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
    if(rv <= 0) return 0;
    buffer[rv] = '\0';

    char *s = strstr(buffer, "sequence_number:");
    if(s && seq_num) sscanf(s, "sequence_number: %d", seq_num);

    return 1;
}


// ---------------------------------------------------------
// RESOLUTION_REQUEST
// ---------------------------------------------------------
// PROTOCOL: Sent when two peers' CALCULATION_REPORTs don't match.
// - Contains the sender's calculated values so the remote can re-evaluate.
// - The receiving peer should compare, and if it agrees, apply and ACK; otherwise terminate.
int send_resolution_request(SOCKET sock, struct sockaddr_in *peer,
                            const char *attacker, const char *move_used,
                            int damageDealt, int defender_hp_remaining, int seq_num){
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "message_type: RESOLUTION_REQUEST\n"
             "attacker: %s\n"
             "move_used: %s\n"
             "damage_dealt: %d\n"
             "defender_hp_remaining: %d\n"
             "sequence_number: %d\n",
             attacker ? attacker : "unknown", move_used ? move_used : "unknown",
             damageDealt, defender_hp_remaining, seq_num);
    return send_udp(sock, buffer, peer) >= 0 ? 1 : 0;
}

int recv_resolution_request(SOCKET sock, struct sockaddr_in *peer,
                            char *attacker, char *move_used, int *damageDealt,
                            int *defender_hp_remaining, int *seq_num){
    char buffer[512];
    int from_len = sizeof(*peer);
    int rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
    if(rv <= 0) return 0;
    buffer[rv] = '\0';

    char *ptr;
    if((ptr = strstr(buffer, "attacker:")) && attacker) sscanf(ptr, "attacker: %49s", attacker);
    if((ptr = strstr(buffer, "move_used:")) && move_used) sscanf(ptr, "move_used: %49s", move_used);
    if((ptr = strstr(buffer, "damage_dealt:")) && damageDealt) sscanf(ptr, "damage_dealt: %d", damageDealt);
    if((ptr = strstr(buffer, "defender_hp_remaining:")) && defender_hp_remaining) sscanf(ptr, "defender_hp_remaining: %d", defender_hp_remaining);
    if((ptr = strstr(buffer, "sequence_number:")) && seq_num) sscanf(ptr, "sequence_number: %d", seq_num);

    return 1;
}


// ---------------------------------------------------------
// GAME_OVER
// ---------------------------------------------------------
// PROTOCOL: Notifies peers that the battle has ended and provides winner/loser info.
// - This is the final synchronization message before closing the logical session.
int send_game_over(SOCKET sock, struct sockaddr_in *peer, const char *winner, const char *loser, int seq_num){
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "message_type: GAME_OVER\n"
             "winner: %s\n"
             "loser: %s\n"
             "sequence_number: %d\n",
             winner ? winner : "unknown", loser ? loser : "unknown", seq_num);
    return send_udp(sock, buffer, peer) >= 0 ? 1 : 0;
}

int receive_game_over(SOCKET sock, struct sockaddr_in *peer, char *winner, char *loser, int *seq_num){
    char buffer[256];
    int from_len = sizeof(*peer);
    int rv = recv_udp(sock, buffer, (int)sizeof(buffer) - 1, peer, &from_len);
    if(rv <= 0) return 0;
    buffer[rv] = '\0';

    char *ptr;
    if((ptr = strstr(buffer, "winner:")) && winner) sscanf(ptr, "winner: %49s", winner);
    if((ptr = strstr(buffer, "loser:")) && loser) sscanf(ptr, "loser: %49s", loser);
    if((ptr = strstr(buffer, "sequence_number:")) && seq_num) sscanf(ptr, "sequence_number: %d", seq_num);

    return 1;
}
