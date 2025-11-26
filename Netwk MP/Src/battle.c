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

int load_pokedex(const char *filename, Pokemon pokedex[]){
    FILE *file = fopen(filename, "r");
    if(!file) return 0;

    char line[1024];
    int count = 0;

    fgets(line,sizeof(line),file);
    while(fgets(line,sizeof(line),file)){
        sscanf(line, 
    "%*[^,],%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%*f,%*f,%*f,%*f,%*[^,],%*d,%*f,%*f,%*f,%*[^,],%[^,],%*[^,],%[^,],%*f,%*d,%d,%d,%d,%[^,],%[^,],"
    , &pokedex[count].against_bug
    , &pokedex[count].against_dark
    , &pokedex[count].against_dragon
    , &pokedex[count].against_electric
    , &pokedex[count].against_fairy
    , &pokedex[count].against_fight
    , &pokedex[count].against_fire
    , &pokedex[count].against_flying
    , &pokedex[count].against_ghost
    , &pokedex[count].against_grass
    , &pokedex[count].against_ground
    , &pokedex[count].against_ice
    , &pokedex[count].against_normal
    , &pokedex[count].against_poison
    , &pokedex[count].against_psychic
    , &pokedex[count].against_rock
    , &pokedex[count].against_steel
    , &pokedex[count].against_water
    , &pokedex[count].hp
    , pokedex[count].name
    , pokedex[count].type1
    , pokedex[count].type2
    , &pokedex[count].attack
    , &pokedex[count].defense
    , &pokedex[count].sp_attack
    , &pokedex[count].sp_defense
    , &pokedex[count].speed
);

        count++;
    }
    fclose(file);
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
