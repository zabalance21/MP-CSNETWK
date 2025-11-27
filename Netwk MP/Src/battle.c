#include <stdio.h>
#include <time.h>
#include <string.h>
#include "battle.h"
//HERES JUST FOR TESTING
// Hardcoded Move List (Definition for g_moves from battle.h)
Move g_moves[MAX_MOVES] = {
    {"Tackle", "normal", 40, CATEGORY_PHYSICAL},
    {"Flamethrower", "fire", 90, CATEGORY_SPECIAL},
    {"Earthquake", "ground", 100, CATEGORY_PHYSICAL},
    {"Ice Beam", "ice", 90, CATEGORY_SPECIAL},
    // Add more moves as needed for the 4-move set
};

// Helper function to find a Pokémon by name (if not already implemented)
Pokemon get_pokemon_by_name(const char *name, Pokemon pokedex[], int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(pokedex[i].name, name) == 0) {
            return pokedex[i];
        }
    }
    // Return an empty/zeroed struct if not found
    Pokemon empty = {0};
    return empty;
}

int calculate_damage(int basePower, int attackerStat, float type1Effectiveness,float type2Effectiveness, int defenderStat){
    return (int)(basePower * attackerStat * type1Effectiveness * type2Effectiveness / defenderStat);
}

void use_stat_boost(int *boost_count){
    if(*boost_count > 0){
        (*boost_count)--;
    }
}

int load_pokedex(const char *filename, Pokemon pokedex[]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("ERROR: Could not open %s\n", filename);
        return 0;
    }

    char line[2048];
    int count = 0;

    // Skip header
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return 0;
    }

    while (fgets(line, sizeof(line), file) && count < 500) {
        int field = 0;
        char *ptr = line;
        int in_brackets = 0;
        char token[256];
        int token_idx = 0;
        
        while (*ptr) {
            if (*ptr == '[') {
                in_brackets = 1;
            } else if (*ptr == ']') {
                in_brackets = 0;
            } else if (*ptr == ',' && !in_brackets) {
                // End of field
                token[token_idx] = '\0';
                
                // Process the token based on field number
                switch(field) {
                    case 1: pokedex[count].against_bug = (float)atof(token); break;
                    case 2: pokedex[count].against_dark = (float)atof(token); break;
                    case 3: pokedex[count].against_dragon = (float)atof(token); break;
                    case 4: pokedex[count].against_electric = (float)atof(token); break;
                    case 5: pokedex[count].against_fairy = (float)atof(token); break;
                    case 6: pokedex[count].against_fight = (float)atof(token); break;
                    case 7: pokedex[count].against_fire = (float)atof(token); break;
                    case 8: pokedex[count].against_flying = (float)atof(token); break;
                    case 9: pokedex[count].against_ghost = (float)atof(token); break;
                    case 10: pokedex[count].against_grass = (float)atof(token); break;
                    case 11: pokedex[count].against_ground = (float)atof(token); break;
                    case 12: pokedex[count].against_ice = (float)atof(token); break;
                    case 13: pokedex[count].against_normal = (float)atof(token); break;
                    case 14: pokedex[count].against_poison = (float)atof(token); break;
                    case 15: pokedex[count].against_psychic = (float)atof(token); break;
                    case 16: pokedex[count].against_rock = (float)atof(token); break;
                    case 17: pokedex[count].against_steel = (float)atof(token); break;
                    case 18: pokedex[count].against_water = (float)atof(token); break;
                    case 19: pokedex[count].attack = atoi(token); break;
                    case 25: pokedex[count].defense = atoi(token); break;
                    case 28: pokedex[count].hp = atoi(token); break;
                    case 30:
                        strncpy(pokedex[count].name, token, 49);
                        pokedex[count].name[49] = '\0';
                        break;
                    case 33: pokedex[count].sp_attack = atoi(token); break;
                    case 34: pokedex[count].sp_defense = atoi(token); break;
                    case 35: pokedex[count].speed = atoi(token); break;
                    case 36:
                        strncpy(pokedex[count].type1, token, 19);
                        pokedex[count].type1[19] = '\0';
                        break;
                    case 37:
                        strncpy(pokedex[count].type2, token, 19);
                        pokedex[count].type2[19] = '\0';
                        break;
                }
                
                field++;
                token_idx = 0;
                ptr++;
                continue;
            }
            
            // Add character to current token (skip newlines)
            if (*ptr != '\r' && *ptr != '\n') {
                token[token_idx++] = *ptr;
            }
            ptr++;
        }
        
        // Process last token (type2, which ends with newline not comma)
        if (token_idx > 0) {
            token[token_idx] = '\0';
            if (field == 37) {
                strncpy(pokedex[count].type2, token, 19);
                pokedex[count].type2[19] = '\0';
            }
        }
        
        count++;
    }

    fclose(file);
    printf("Successfully loaded %d Pokemon!\n", count);
    return count;
}


void start_battle(SOCKET sock, ROLE role, struct sockaddr_in *peer, int seed,
                  Pokemon myPokemon, Pokemon oppPokemon, int my_sa, int my_sd, int opp_sa, int opp_sd) {srand(seed);

    // --- State and HP Tracking ---
        BattleState currentState = STATE_SETUP; // Or STATE_WAITING_FOR_MOVE if skipping initial setup
        int battleOver = 0;

        // Track current health of both Pokémon
        int my_current_hp = myPokemon.hp;
        int opp_current_hp = oppPokemon.hp;

        // Host starts with sequence number 1.
        int current_seq_num = 1;

        // Host is designated to go first.
        int current_turn_is_my_turn = (role == HOST) ? 1 : 0;

        // Store the move used in the current turn for damage calculation/reporting
        char last_move_name[50] = {0};
        Move selected_move; // The full move structure

        // Variables for damage/HP calculation results
        int damage_dealt_this_turn = 0;
        int local_target_hp = 0;
    int myTurn = (role == HOST) ? 1 : 0; // Host attacks first
    int battleOver = 0;

    // 1. Initialize State
    BattleState currentState = STATE_WAITING_FOR_MOVE;

    printf("\nBattle Start! Your Pokémon: %s vs Opponent Pokémon: %s.\n", myPokemon.name, oppPokemon.name);
    printf("Initial HP: %d vs %d\n", myPokemon.hp, oppPokemon.hp);

    // Use local mutable copies of HP and boosts
    int current_my_hp = myPokemon.hp;
    int current_opp_hp = oppPokemon.hp;
    int current_my_sa = my_sa;
    int current_my_sd = my_sd;

    // --- Main Battle Loop (State Machine) ---
    while (!battleOver) {

        switch (currentState) {

            case STATE_WAITING_FOR_MOVE:
                printf("\n--- Turn Start ---\n");

                if (myTurn) {
                    // Current Peer is the ATTACKER
                    printf("It's your turn to attack!\n");
                    // PROMPT USER FOR MOVE HERE

                    // TODO: The next step is to transition to STATE_ATTACK_PHASE
                    // For now, we simulate the move selection and exit to prevent infinite loop.
                    currentState = STATE_ATTACK_PHASE;

                } else {
                    // Current Peer is the DEFENDER
                    printf("Waiting for opponent's move...\n");
                    // TODO: The next step is to call a function to listen for ATTACK_ANNOUNCE

                    // For now, we simulate receiving a message and exit to prevent infinite loop.
                    // This blocking call needs to be implemented.
                    // currentState = STATE_DEFENSE_PHASE;
                }

                // Add a small sleep to avoid tight loop when testing full flow
                Sleep(100);
                break;

            case STATE_ATTACK_PHASE:
                printf("[STATE] Attacking phase. Sending ATTACK_ANNOUNCE...\n");
                // TODO: 1. Send ATTACK_ANNOUNCE (including selected move and SA boost usage).
                // TODO: 2. Transition to STATE_DEFENSE_PHASE

                // Temporarily exit to prevent an infinite loop
                battleOver = 1;
                break;

            case STATE_DEFENSE_PHASE:
                // This state is reached when the DEFENDER receives ATTACK_ANNOUNCE
                printf("[STATE] Defense phase. Responding with DEFENSE_ANNOUNCE...\n");
                // TODO: 1. Prompt Defender for SD boost usage.
                // TODO: 2. Send DEFENSE_ANNOUNCE (including SD boost usage).
                // TODO: 3. Transition to STATE_CALCULATION

                // Temporarily exit
                battleOver = 1;
                break;

            // ... (Add remaining states: STATE_CALCULATION, STATE_RESOLUTION, STATE_GAME_OVER) ...

            default:
                // For safety, handle unhandled states
                printf("Error: Reached unhandled battle state.\n");
                battleOver = 1;
                break;
        }

        // Add a check to prevent infinite loop during early development
        if (battleOver) {
            printf("\n--- Debug Stop: Battle Over ---\n");
            break;
        }
    }
}

