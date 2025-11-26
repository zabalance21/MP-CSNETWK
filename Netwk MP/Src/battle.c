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

void start_battle(SOCKET sock, ROLE role, struct sockaddr_in *peer, int seed){
    srand(seed);
    int myTurn = (role == HOST) ? 1 : 0;
    int battleOver = 0;

    while(!battleOver){
        if(myTurn){
            // HOST LOGIC FOR ATTACKING
        }else{
            // JOINER LOGIC FOR ATTACKING
        }
        myTurn = !myTurn;

    }
}
