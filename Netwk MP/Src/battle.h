#ifndef BATTLE_H
#define BATTLE_H
#include <winsock2.h>
#include "protocols.h"
#include "network.h"

typedef struct {
    char name[50];
    char type1[20];
    char type2[20];
    int hp;
    int attack;
    int defense;
    int sp_attack;
    int sp_defense;
    int speed;
    float against_bug;
    float against_dark;
    float against_dragon;
    float against_electric;
    float against_fairy;
    float against_fight;
    float against_fire;
    float against_flying;
    float against_ghost;
    float against_grass;
    float against_ground;
    float against_ice;
    float against_normal;
    float against_poison;
    float against_psychic;
    float against_rock;
    float against_steel;
    float against_water;
} Pokemon;



void start_battle(SOCKET sock, ROLE role, struct sockaddr_in *peer, int seed,
                  Pokemon myPokemon, Pokemon oppPokemon, int my_sa, int my_sd, int opp_sa, int opp_sd);

int calculate_damage(int basePower, int attackerStat, float type1Effectiveness,float type2Effectiveness, int defenderStat);
int load_pokedex(const char *filename, Pokemon pokedex[]);
void use_stat_boost(int *boost_count);

// Battle State Enumeration
typedef enum {
    STATE_SETUP,          // Initial state before BATTLE_SETUP exchange
    STATE_WAITING_FOR_MOVE, // Ready to start a turn, waiting for a move decision
    STATE_ATTACK_PHASE,   // Attacker sends ATTACK_ANNOUNCE
    STATE_DEFENSE_PHASE,  // Defender responds with DEFENSE_ANNOUNCE
    STATE_CALCULATION,    // Both send CALCULATION_REPORT
    STATE_RESOLUTION,     // Sync check or Resolution Request
    STATE_GAME_OVER
} BattleState;

// --- New: Move Structures (Required for Turn Logic) ---
typedef enum {
    CATEGORY_PHYSICAL, // Uses Attack/Defense
    CATEGORY_SPECIAL,  // Uses Special Attack/Special Defense
    CATEGORY_STATUS    // No damage
} MoveCategory;

typedef struct {
    char name[50];
    char type[20];
    int base_power;
    MoveCategory category; // Determines which stats are used (Attack/Defense or Sp. Attack/Sp. Defense)
} Move;

#define MAX_MOVES 10
extern Move g_moves[MAX_MOVES]; // Global list of moves for lookup

float get_type_effectiveness(const char *move_type, const Pokemon *defender); // Prototype for future use
Pokemon get_pokemon_by_name(const char *name, Pokemon pokedex[], int count);


#endif