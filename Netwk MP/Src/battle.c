#include <stdio.h>
#include <time.h>

#include "battle.h"

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
                  Pokemon myPokemon, Pokemon oppPokemon, int my_sa, int my_sd, int opp_sa, int opp_sd) {
    srand(seed);
    int myTurn = (role == HOST) ? 1 : 0;
    int battleOver = 0;

    printf("\nBattle Start! Your Pokémon: %s vs Opponent Pokémon: %s\n", myPokemon.name, oppPokemon.name);

    while (!battleOver) {
        if (myTurn) {
            // TODO: Host / active player turn logic
        } else {
            // TODO: Joiner / passive player turn logic
        }
        myTurn = !myTurn;
    }
}
