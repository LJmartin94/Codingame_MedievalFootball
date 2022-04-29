#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

// The corner of the map representing your base
int base_x;
int base_y;
int enemy_base_x;
int enemy_base_y;

// Number of turns elapsed
int turns;

// Always 3
int heroes_per_player;
int hero_offset;

// The centre of the map
int mid_x = 8815;
int mid_y = 4500;

// My mana: Spend ten mana to cast a spell
int mana;
double estimated_wild_mana;
double enemy_estimated_wild_mana;

// Enemy visibility      
int visible_enemies;
int visible_enemies_my_base;
int visible_enemies_their_base;

int enemy_defenders;

int enemy_dist_to_base;
int nearest_hero_dist;
int nearest_hero_id;

int hostile_creeps;
int hostile_creeps_in_base;
int hostile_base_creeps_shielded;

int max_creep_hp;

// flags
int enemies_spotted;
int all_out;
int aggressive_shielding;
int mind_controlled;

typedef struct s_entity
{
    int id; // Unique identifier
    int type; // 0=monster, 1=your hero, 2=opponent hero 
    int shield_life; // Count down until shield spell fades
    int is_controlled; // Equals 1 when this entity is under a control spell
    int health; // Remaining health of this monster
    int near_base; // 0=monster with no target yet, 1=monster targeting a base
    int threat_for; // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither

    int dist_to_base;
        
    // Position of this entity
    int x;
    int y;
    // Trajectory of this monster
    int vx;
    int vy;
} t_entity;

typedef struct s_xypair
{
    int x;
    int y;
} t_xypair;

typedef struct s_enemyInfo
{
    int heroId;

    int lastX;
    int lastY;
    int turn_spotted;

    int vx;
    int vy;

    int inTheirBase;
    int inMyBase;
    int inTheirHalf;
    int inMyHalf;

    int distToMyBase;
    int distToTheirBase;

    int offensive;
    int defensive;

    int shieldHP;

    int distToHero[3];
} t_enemyInfo;

t_enemyInfo theirHeroes[3];

//UTILS/////////////////////////////////////
t_xypair vectorise(int dist, int x_offset, int y_offset)
{
    int xy_total = x_offset + y_offset;
    double normal_x = (double)x_offset / (double)xy_total;
    double normal_y = (double)y_offset / (double)xy_total;
    int multiplier = 0;

    int dist_squared = dist * dist;
    for (int meta_multi = 1000; meta_multi > 0; meta_multi = meta_multi / 10)
    {
        while (dist_squared > ((normal_x * multiplier) * (normal_x * multiplier) + (normal_y * multiplier) * (normal_y * multiplier)))
        {
            // fprintf(stderr,"dist_squared = %d, normal_x = %f, normal_y = %f, multiplier = %d\n", dist_squared, normal_x, normal_y, multiplier);
            multiplier = multiplier + meta_multi;
        }
        multiplier -= meta_multi;
    }
    t_xypair ret;
    ret.x = (int)(normal_x * multiplier);
    ret.y = (int)(normal_y * multiplier);
    return (ret);
}

t_xypair reverse_vector(int x, int y, int min_dist)
{
    // if (1) //nevermind. monsters dont update their vector in base after they have been pushed by wind, so dont always show the optimal escape vector.
    // {
    //     t_xypair ret;
    //     ret.x = (base_x == 0 ) ? 1 : -1;
    //     ret.y = (base_x == 0 ) ? 1 : -1;
    //     ret.x = ret.x * mid_x;
    //     ret.y = ret.y * mid_y;
    //     return (ret);
    // }

    // t_xypair to_travel;
    //     to_travel.x = abs(base_x - (enemyX + enemyVX));
    //     to_travel.y = abs(base_y - (enemyY + enemyVY));
    //     enemyDist = (enemyDist > 1100) ? enemyDist - 800 : 300;
    //     to_travel = vectorise(enemyDist, to_travel.x, to_travel.y);
    //     int x_togo = from_base('x', base_x, to_travel.x);
    //     int y_togo = from_base('y', base_y, to_travel.y);
    //     t_xypair improved = improve_move(x_togo, y_togo);

    int vx = (base_x - x);
    int vy = (base_y - y);

    fprintf(stderr, "target vector: %d,%d\n", vx, vy);
    if ((vx == 0) && (vy == 0)) // see if target is stationary
    {
        vx = (base_x == 0 ) ? -1 : 1; //assume stationary targets are headed towards your base
        vy = (base_x == 0 ) ? -1 : 1;
    }

    if (((vx * vx) + (vy * vy)) < (min_dist * min_dist)) // check if pushing target along reverse vector pushes them far enough
    {
        t_xypair temp = vectorise((min_dist + 1), vx, vy);
        vx = (vx < 0) ? (temp.x * -1) : temp.x;
        vy = (vy < 0) ? (temp.y * -1) : temp.y;
    }

    t_xypair ret; //invert the vector
    ret.x = -1 * vx;
    ret.y = -1 * vy;
    fprintf(stderr, "inverted vector: %d,%d\n", ret.x, ret.y);
    return (ret);
}

int dist(int Ax, int Ay, int Bx, int By)
{
    long int x_diff = Ax - Bx;
    long int y_diff = Ay - By;
    long int dist_squared = (x_diff * x_diff) + (y_diff * y_diff);
    int dist = (int)sqrt((double)dist_squared);
    return (dist);
}

int to_mid(char c, int coord, int dif)
{
    if (c == 'x' && coord < mid_x && coord + dif <= mid_x)
        return (coord + dif);
    if (c == 'x' && coord < mid_x)
        return (mid_x);
    if (c == 'y' && coord < mid_y && coord + dif <= mid_y)
        return (coord + dif);
    if (c == 'y' && coord < mid_y)
        return (mid_y);
    if (c == 'x' && coord > mid_x && coord + dif >= mid_x)
        return (coord - dif);
    if (c == 'x' && coord > mid_x)
        return (mid_x);
    if (c == 'y' && coord > mid_y && coord + dif >= mid_y)
        return (coord - dif);
    if (c == 'y' && coord > mid_y)
        return (mid_y);
}

int from_base(char c, int coord, int dif)
{
    if (c == 'x' && coord < mid_x)
        return (coord + dif);
    if (c == 'y' && coord < mid_y)
        return (coord + dif);
    if (c == 'x' && coord > mid_x)
        return (coord - dif);
    if (c == 'y' && coord > mid_y)
        return (coord - dif);
}

int to_base(char c, int coord, int dif)
{
    if (c == 'x' && coord < base_x && coord + dif <= base_x)
        return (coord + dif);
    if (c == 'x' && coord < base_x)
        return (base_x);
    if (c == 'y' && coord < base_y && coord + dif <= base_y)
        return (coord + dif);
    if (c == 'y' && coord < base_y)
        return (base_y);
    if (c == 'x' && coord > base_x && coord + dif >= base_x)
        return (coord - dif);
    if (c == 'x' && coord > base_x)
        return (base_x);
    if (c == 'y' && coord > base_y && coord + dif >= base_y)
        return (coord - dif);
    if (c == 'y' && coord > base_y)
        return (base_y);
}

t_xypair improve_target(t_entity maintarget, t_entity *peepz, int entity_count)
{
    int potential_AOE = 1;
    int aoe_x = maintarget.x;
    int aoe_y = maintarget.y;

    for (int i = 0; i < entity_count; i++) 
    {
        int dist_from_target = dist(peepz[i].x, peepz[i].y, maintarget.x, maintarget.y);
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && dist_from_target <= 1600)
        {
            potential_AOE++;
            aoe_x = aoe_x + peepz[i].x;
            aoe_y = aoe_y + peepz[i].y;
        }
    }
    t_xypair ret;
    aoe_x = aoe_x / potential_AOE;
    aoe_y = aoe_y / potential_AOE;
    ret.x = aoe_x;
    ret.y = aoe_y;
    if (dist(ret.x, ret.y, maintarget.x, maintarget.y) <= 800)
        return(ret);

    fprintf(stderr, "ERROR TARGET IMPROVEMENT MISCALCULATED\n");
    t_xypair to_travel;
    to_travel.x = abs(aoe_x - maintarget.x);
    to_travel.y = abs(aoe_y - maintarget.y);
    to_travel = vectorise(800, to_travel.x, to_travel.y);
    ret.x = (aoe_x <= maintarget.x) ? maintarget.x - to_travel.x : maintarget.x + to_travel.x;
    ret.y = (aoe_y <= maintarget.y) ? maintarget.y - to_travel.y : maintarget.y + to_travel.y;
    if (dist(ret.x, ret.y, maintarget.x, maintarget.y) <= 800)
        return(ret);
    fprintf(stderr, "ERROR TARGET IMPROVEMENT SUPER MISCALCULATED - ABORTING\n");
    ret.x = maintarget.x;
    ret.y = maintarget.y;
    return (ret);
}

t_xypair improve_target_towards_enemy(t_entity maintarget, t_entity *peepz, int entity_count, int enemyX, int enemyY)
{
    int potential_AOE = 2;
    int aoe_x = maintarget.x + enemyX;
    int aoe_y = maintarget.y + enemyY;

    for (int i = 0; i < entity_count; i++) 
    {
        int dist_from_target = dist(peepz[i].x, peepz[i].y, maintarget.x, maintarget.y);
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && dist_from_target <= 1600)
        {
            potential_AOE++;
            aoe_x = aoe_x + peepz[i].x;
            aoe_y = aoe_y + peepz[i].y;
        }
    }
    t_xypair ret;
    aoe_x = aoe_x / potential_AOE;
    aoe_y = aoe_y / potential_AOE;
    ret.x = aoe_x;
    ret.y = aoe_y;
    if (dist(ret.x, ret.y, maintarget.x, maintarget.y) <= 800)
        return(ret);

    fprintf(stderr, "ERROR TARGET IMPROVEMENT MISCALCULATED\n");
    t_xypair to_travel;
    to_travel.x = abs(aoe_x - maintarget.x);
    to_travel.y = abs(aoe_y - maintarget.y);
    to_travel = vectorise(800, to_travel.x, to_travel.y);
    ret.x = (aoe_x <= maintarget.x) ? maintarget.x - to_travel.x : maintarget.x + to_travel.x;
    ret.y = (aoe_y <= maintarget.y) ? maintarget.y - to_travel.y : maintarget.y + to_travel.y;
    if (dist(ret.x, ret.y, maintarget.x, maintarget.y) <= 800)
        return(ret);
    fprintf(stderr, "ERROR TARGET IMPROVEMENT SUPER MISCALCULATED - ABORTING\n");
    ret.x = maintarget.x;
    ret.y = maintarget.y;
    return (ret);
}

t_xypair improve_move(int x, int y)
{
    t_xypair ret;
    ret.x = x;
    ret.y = y;
    if (dist(x, y, base_x, base_y) >= 1100)
        return (ret);
    fprintf(stderr, "IMPROVE_MOVE STOPPED THIS HERO FROM RUNNING INTO BASE\n");
    ret = vectorise(1100, ret.x, ret.y);
    ret.x = (base_x == 0) ? ret.x : base_x - ret.x;
    ret.y = (base_x == 0) ? ret.y : base_y - ret.y;
    return (ret);
}

void enemyConstructor(int index)
{
    theirHeroes[index].heroId = -1;
    theirHeroes[index].lastX = (base_x + enemy_base_x) * -1;
    theirHeroes[index].lastY = (base_y + enemy_base_y) * -1;
    theirHeroes[index].turn_spotted = 0; 

    theirHeroes[index].vx = -1;
    theirHeroes[index].vy = -1;

    theirHeroes[index].inTheirBase = 0;
    theirHeroes[index].inMyBase = 0;
    theirHeroes[index].inTheirHalf = 0;
    theirHeroes[index].inMyHalf = 0;

    theirHeroes[index].distToMyBase = 0;
    theirHeroes[index].distToTheirBase = 0;

    theirHeroes[index].offensive = 0;
    theirHeroes[index].defensive = 0;

    theirHeroes[index].shieldHP = 0;
    
    for (int h = 0; h < heroes_per_player; h++)
    {
        theirHeroes[index].distToHero[h] = 0;
    }
    return;
}

t_xypair enemyUpdater(int id, int x, int y, t_entity* myHeroes, int shieldHP)
{
    int index = id % heroes_per_player;
    t_xypair vector;
    vector.x = -1;
    vector.y = -1;

    theirHeroes[index].heroId = index;
    if (x >= 0 && y >= 0) // only update the following values if enemy hero is actually spotted
    {
        vector.x = x - theirHeroes[index].lastX;
        vector.y = y - theirHeroes[index].lastY;
        int normalise_turns = turns - theirHeroes[index].turn_spotted;
        if (normalise_turns >= 1)
        {
            vector.x = vector.x / normalise_turns;
            vector.y = vector.y / normalise_turns;
        }

        theirHeroes[index].lastX = x;
        theirHeroes[index].lastY = y;
        theirHeroes[index].turn_spotted = turns;

        theirHeroes[index].vx = vector.x;
        theirHeroes[index].vy = vector.y;
        
        int distTheir = dist(x, y, enemy_base_x, enemy_base_y);
        theirHeroes[index].distToTheirBase = distTheir;
        theirHeroes[index].inTheirBase = (distTheir <= 8000) ? 1 : 0 ;
        
        int distMy = dist(x, y, base_x, base_y);
        theirHeroes[index].distToMyBase = distMy;
        theirHeroes[index].inMyBase = (distMy <= 8000) ? 1 : 0 ;
        
        theirHeroes[index].inMyHalf = ((x <= mid_x && base_x == 0) || (x >= mid_x && base_x != 0)) ? 1 : 0;
        theirHeroes[index].inTheirHalf = ((x >= mid_x && base_x == 0) || (x <= mid_x && base_x != 0)) ? 1 : 0;
        
        theirHeroes[index].offensive = (distMy <= distTheir) ? 1 : 0;
        theirHeroes[index].defensive = (distTheir <= distMy) ? 1 : 0;
        if (nearest_hero_id == -1)
            nearest_hero_id = (id % heroes_per_player) + hero_offset;
        if (nearest_hero_id != -1)
        {
            int isThisHeroCloser = dist(base_x, base_y, theirHeroes[nearest_hero_id % heroes_per_player].lastX, theirHeroes[nearest_hero_id % heroes_per_player].lastY);
            if (distMy < isThisHeroCloser)
                nearest_hero_id = (id % heroes_per_player) + hero_offset;
        }
        theirHeroes[index].shieldHP = shieldHP;
    }
    for (int h = 0; h < heroes_per_player; h++)
    {
        theirHeroes[index].distToHero[h]= dist(myHeroes[h].x, myHeroes[h].y, theirHeroes[(id % heroes_per_player)].lastX, theirHeroes[(id % heroes_per_player)].lastY);
    }
    return (vector);
}

//UTILS_END/////////////////////////////////////

//STRATEGIES /////////////////////////////////////
int should_I_shield(t_entity thisHero, t_entity target)
{
    int distance_to_target = dist(thisHero.x, thisHero.y, target.x, target.y);

    //ADD DISTANCE TO ENEMY PLAYER <2200?
    // thisHero.dist_to_base <= 15000 && 
    if ( mind_controlled == 1 && \
        (visible_enemies_my_base >= 1 || (enemy_dist_to_base <= thisHero.dist_to_base)) && \
        thisHero.shield_life <= 0 && \
        ((mana >= 10 && thisHero.id == 0) || (mana >= 50 && thisHero.id != 0)) && \
        (target.type == 0 && target.threat_for == 1) && \
        ((enemy_dist_to_base <= thisHero.dist_to_base) || (distance_to_target <= 800)) )
        return (1);
    else
        return (0);
}

int neutral_lead_two(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 4;
        to_travel.y = 7;
        to_travel = vectorise(7100, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int neutral_lead_one(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 7;
        to_travel.y = 4;
        to_travel = vectorise(7100, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int neutral_lead_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, base_x, base_y) > dist(thisHero.x, thisHero.y, base_x, base_y) && 
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana >= 10 && target.type == 0 && target.near_base == 1 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.near_base == 1)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d NEUTRAL\n", improved.x, improved.y);
        return (1);
    }
    else if (nearest_hero_id != -1)
    {
        int enemyX = theirHeroes[nearest_hero_id % heroes_per_player].lastX;
        int enemyY = theirHeroes[nearest_hero_id % heroes_per_player].lastY;
        int enemyVX = theirHeroes[nearest_hero_id % heroes_per_player].vx;
        int enemyVY = theirHeroes[nearest_hero_id % heroes_per_player].vy;
        int enemyDist = dist(base_x, base_y, enemyX, enemyY);

        t_xypair to_travel;
        to_travel.x = abs(base_x - (enemyX + enemyVX));
        to_travel.y = abs(base_y - (enemyY + enemyVY));
        enemyDist = (enemyDist > 1100) ? enemyDist - 800 : 300;
        to_travel = vectorise(enemyDist, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return(1);
    }
    else
    {
        // printf("MOVE %d %d\n", from_base('x', base_x, 300), from_base('y', base_y, 300));
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(3000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int neutral_lead_strat(int index, t_entity *peepz, int entity_count, t_entity *myHeroes)
{
    if (index == 2)
        neutral_lead_two(peepz, entity_count, myHeroes[2]);
    else if (index == 1)
        neutral_lead_one(peepz, entity_count, myHeroes[1]);
    else
        neutral_lead_zero(peepz, entity_count, myHeroes[0]);
    return (0);
}

int neutral_farm_two(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        int target_dist = dist(peepz[i].x + peepz[i].vx, peepz[i].y + peepz[i].vy, thisHero.x, thisHero.y);
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && target_dist < nearest && target_dist < 4500)
        {
            target = peepz[i];
            nearest = target_dist;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 3;
        to_travel.y = 10;
        to_travel = vectorise(7100, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int neutral_farm_one(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        int target_dist = dist(peepz[i].x + peepz[i].vx, peepz[i].y + peepz[i].vy, thisHero.x, thisHero.y);
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && target_dist < nearest && target_dist < 4500)
        {
            target = peepz[i];
            nearest = target_dist;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 10;
        to_travel.y = 1;
        to_travel = vectorise(7100, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int neutral_farm_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, base_x, base_y) > dist(thisHero.x, thisHero.y, base_x, base_y) && 
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana >= 10 && target.type == 0 && target.near_base == 1 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.near_base == 1 && target.threat_for == 1)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d FARMING\n", improved.x, improved.y);
        return (1);
    }
    else if (target.type == 0 && (target.dist_to_base <= 7100 || enemies_spotted == 0 || (visible_enemies == 0 && target.dist_to_base <= 10000)) && \
    (nearest_hero_id == -1 || theirHeroes[nearest_hero_id % heroes_per_player].distToMyBase > 8800))
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d FARMING\n", improved.x, improved.y);
        return (1);
    }
    else if (target.type == 0 && (target.dist_to_base <= 7100 || enemies_spotted == 0 || (visible_enemies == 0 && target.dist_to_base <= 10000)) && \
    (nearest_hero_id != -1 || dist(target.x, target.y, theirHeroes[nearest_hero_id % heroes_per_player].lastX, theirHeroes[nearest_hero_id % heroes_per_player].lastY) <= 1200))
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d FARMING\n", improved.x, improved.y);
        return (1);
    }
    else if (nearest_hero_id != -1 && theirHeroes[nearest_hero_id % heroes_per_player].distToMyBase <= 8800)
    {
        int enemyX = theirHeroes[nearest_hero_id % heroes_per_player].lastX;
        int enemyY = theirHeroes[nearest_hero_id % heroes_per_player].lastY;
        int enemyVX = theirHeroes[nearest_hero_id % heroes_per_player].vx;
        int enemyVY = theirHeroes[nearest_hero_id % heroes_per_player].vy;
        int enemyDist = dist(base_x, base_y, enemyX, enemyY);

        t_xypair to_travel;
        to_travel.x = abs(base_x - (enemyX + enemyVX));
        to_travel.y = abs(base_y - (enemyY + enemyVY));
        enemyDist = (enemyDist > 1100) ? enemyDist - 800 : 300;
        to_travel = vectorise(enemyDist, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d FARMING\n", improved.x, improved.y);
        return(1);
    }
    else if (visible_enemies == 0)
    {
        // printf("MOVE %d %d\n", from_base('x', base_x, 300), from_base('y', base_y, 300));
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(8000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    else
    {
        // printf("MOVE %d %d\n", from_base('x', base_x, 300), from_base('y', base_y, 300));
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int neutral_farm_strat(int index, t_entity *peepz, int entity_count, t_entity *myHeroes)
{
    if (index == 2)
        neutral_farm_two(peepz, entity_count, myHeroes[2]);
    else if (index == 1)
        neutral_farm_one(peepz, entity_count, myHeroes[1]);
    else
        neutral_farm_zero(peepz, entity_count, myHeroes[0]);
    return (0);
}

int ahead_defensive_two(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana >= 100 && target.type == 0 && target.dist_to_base < 8000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 2;
        to_travel.y = 10;
        to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int ahead_defensive_one(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana >= 100 && target.type == 0 && target.dist_to_base < 8000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 10;
        to_travel.y = 2;
        to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int ahead_defensive_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, base_x, base_y) > dist(thisHero.x, thisHero.y, base_x, base_y) && 
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana >= 10 && target.type == 0 && target.dist_to_base < 8000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.dist_to_base < 7100)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d AHEAD\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        // printf("MOVE %d %d\n", from_base('x', base_x, 300), from_base('y', base_y, 300));
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(4500, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int ahead_defensive_strat(int index, t_entity *peepz, int entity_count, t_entity *myHeroes)
{
    if (index == 2)
        ahead_defensive_two(peepz, entity_count, myHeroes[2]);
    else if (index == 1)
        ahead_defensive_one(peepz, entity_count, myHeroes[1]);
    else
        ahead_defensive_zero(peepz, entity_count, myHeroes[0]);
    return (0);
}

int hard_defensive_two(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana > 10 && target.type == 0 && target.dist_to_base < 8000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 2;
        to_travel.y = 10;
        to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int hard_defensive_one(t_entity *peepz, int entity_count, t_entity thisHero)
{
    if (all_out == 1 && thisHero.dist_to_base > dist(thisHero.x, thisHero.y, enemy_base_x, enemy_base_y))
        return (mana_aggressive_one(peepz, entity_count, thisHero));
    
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana > 10 && target.type == 0 && target.dist_to_base < 8000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 10;
        to_travel.y = 2;
        to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int hard_defensive_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, base_x, base_y) > dist(thisHero.x, thisHero.y, base_x, base_y) && 
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana >= 10 && target.type == 0 && target.dist_to_base < 8000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.dist_to_base < 7100 || (target.shield_life > 0 && target.threat_for == 1))
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d HARD DEF\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        // printf("MOVE %d %d\n", from_base('x', base_x, 300), from_base('y', base_y, 300));
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(4500, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d HARD DEF\n", improved.x, improved.y);
    }
    return (0);
}

int hard_defensive_strat(int index, t_entity *peepz, int entity_count, t_entity *myHeroes)
{
    if (index == 2)
        hard_defensive_two(peepz, entity_count, myHeroes[2]);
    else if (index == 1)
        hard_defensive_one(peepz, entity_count, myHeroes[1]);
    else
        hard_defensive_zero(peepz, entity_count, myHeroes[0]);
    return (0);
}

int mana_aggressive_two(t_entity *peepz, int entity_count, t_entity thisHero)
{
    //MIDFIELDER
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    int num_of_creeps_around = 0;
    for (int i = 0; i < entity_count; i++) 
    {
        int dist_from_hero = dist(thisHero.x, thisHero.y, peepz[i].x, peepz[i].y);
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && dist_from_hero <= 1280 && \
            peepz[i].shield_life == 0)
        {
            num_of_creeps_around++;
        }
        int dist_to_enemy_base = dist(peepz[i].x, peepz[i].y, enemy_base_x, enemy_base_y);
        
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && dist_from_hero <= 2200 && dist_to_enemy_base < nearest && peepz[i].health >= 10 && peepz[i].shield_life == 0)
        {
            target = peepz[i];
            nearest = dist_to_enemy_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana > 10 && num_of_creeps_around >= 2)
    {
        printf("SPELL WIND %d %d FUS RO DA\n", enemy_base_x, enemy_base_y);
        mana = mana - 10;
        return (1);
    }
    else if (mana > 10 && target.type == 0 && target.threat_for != 2 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 2200)
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL CONTROL %d %d %d\n", target.id, enemy_base_x, enemy_base_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && dist(target.x, target.y, thisHero.x, thisHero.y) > 2200 && \
        dist(mid_x, mid_y, target.x, target.y) <= 1000 && aggressive_shielding == 0)
    {
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(1000, to_travel.x, to_travel.y);
        int x_togo = to_base('x', target.x, to_travel.x);
        int y_togo = to_base('y', target.y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    else if (aggressive_shielding == 1)
    {
        t_xypair to_travel;
        to_travel.x = mid_x;
        to_travel.y = mid_y;
        to_travel = vectorise(8000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    else
    {
        t_xypair improved = improve_move(mid_x, mid_y);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int mana_aggressive_one(t_entity *peepz, int entity_count, t_entity thisHero)
{
    //ATTACKER
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        int dist_to_enemy = dist(peepz[i].x, peepz[i].y, enemy_base_x, enemy_base_y);
        if(peepz[i].type == 0 && (dist_to_enemy < nearest))
        {
            target = peepz[i];
            nearest = dist_to_enemy;
        }
    }

    nearest = INT_MAX;
    int nearestEnemyId = -1;
    for(int h = 0; h < heroes_per_player; h++)
    {
        if (theirHeroes[h].distToHero[thisHero.id % heroes_per_player] <= nearest)
        {
            nearest = theirHeroes[h].distToHero[thisHero.id % heroes_per_player];
            nearestEnemyId = theirHeroes[h].heroId + hero_offset;
        }
    }
    fprintf(stderr, "LOOKING FOR %d\n", nearestEnemyId);
    t_entity nearest_enemy = peepz[0];
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 2 && peepz[i].id == nearestEnemyId)
        {
            nearest_enemy = peepz[i];
            nearestEnemyId = 42;
            break;
        }
    }

    if (nearestEnemyId != 42)
        fprintf(stderr, "NEAREST ENEMY NOT FOUND\n");

    t_xypair to_travel;
    to_travel.x = 1;
    to_travel.y = 1;
    to_travel = vectorise(6000, to_travel.x, to_travel.y);
    int pos_x = to_base('x', enemy_base_x, to_travel.x);
    int pos_y = to_base('y', enemy_base_y, to_travel.y);

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, thisHero.x, thisHero.y) >= 1280 && 
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    if (mana > 10 && target.type == 0 && target.threat_for == 2 && \
        dist(target.x, target.y, enemy_base_x, enemy_base_y) < (400 * 12) && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 2200 && \
        target.shield_life == 0 && (target.health >= 10))
    {
        printf("SPELL SHIELD %d\n", target.id);
        mana = mana - 10;
        return (1);
    }
    else if (mana > 10 && target.type == 0 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) <= 1280 && \
        target.shield_life == 0)
    {
        printf("SPELL WIND %d %d\n", enemy_base_x, enemy_base_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && dist(target.x, target.y, thisHero.x, thisHero.y) > 2200 && \
        target.near_base == 1 && target.threat_for == 2 && \
        dist(pos_x, pos_y, target.x, target.y) <= 1000)
    {
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(1000, to_travel.x, to_travel.y);
        int x_togo = to_base('x', target.x, to_travel.x);
        int y_togo = to_base('y', target.y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    else
    {
        t_xypair improved = improve_move(pos_x, pos_y);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int mana_aggressive_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        int dist_from_hero = dist(thisHero.x, thisHero.y, peepz[i].x, peepz[i].y);
        int dist_from_post = dist(4242, 4242, peepz[i].x, peepz[i].y);
        if(peepz[i].type == 0 && (peepz[i].threat_for == 1 || (dist_from_hero <= 2200 && dist_from_post <= 5000)) && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, base_x, base_y) > dist(thisHero.x, thisHero.y, base_x, base_y) && 
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    else if (mana >= 10 && target.type == 0 && target.near_base == 1 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && (target.dist_to_base <= 7100 || (target.dist_to_base <= 8000 && visible_enemies == 0)))
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d ALL OUT\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        // printf("MOVE %d %d\n", from_base('x', base_x, 300), from_base('y', base_y, 300));
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d ALL OUT\n", improved.x, improved.y);
    }
    return (0);
}

int mana_aggressive_strat(int index, t_entity *peepz, int entity_count, t_entity *myHeroes)
{
    if (index == 2)
        mana_aggressive_two(peepz, entity_count, myHeroes[2]);
    else if (index == 1)
        mana_aggressive_one(peepz, entity_count, myHeroes[1]);
    else
        mana_aggressive_zero(peepz, entity_count, myHeroes[0]);
    return (0);
}

int bullwark_two(t_entity *peepz, int entity_count, t_entity thisHero)
{
	//DETERMINE ATTACKERS
    int enemy_attackers = 0;
    for (int i = 0; i < heroes_per_player; i++)
    {
        if (theirHeroes[i].offensive == 1)
            enemy_attackers++;
    }

	int min_away_from_base = (((max_creep_hp + 1) / 2) * 400) + 300 + (2200 * enemy_attackers);
	int max_away_from_base = min_away_from_base * 4 / 3;

    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        int mx = peepz[i].x * 10000 / (mid_x * 2);
		int my = peepz[i].y * 10000 / (mid_y * 2);
		fprintf(stderr, "target %d at %d,%d has mx,my of %d,%d\n", peepz[i].id, peepz[i].x, peepz[i].y, mx, my);
		int target_corner = (mx >= my) ? 1 : 0;
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest && \
		(target_corner != 1 || peepz[i].dist_to_base <= 6000 || (peepz[i].shield_life > 0 && peepz[i].threat_for == 1)))
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }
    
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
	if (target.type == 0 && target.dist_to_base <= min_away_from_base && mana >= 10 && \
	dist(target.x, target.y, thisHero.x, thisHero.y) <= 1280 && \
	target.shield_life == 0)
    {
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d target dist: %d, min dist: %d\n", wind_reverse.x, wind_reverse.y, target.dist_to_base, min_away_from_base);
        mana = mana - 10;
        return (1);
    }
    if (target.type == 0 && target.dist_to_base <= max_away_from_base)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 35;
        to_travel.y = 70;
		if (max_away_from_base >= 6000)
        	to_travel = vectorise(max_away_from_base, to_travel.x, to_travel.y);
		else
        	to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int bullwark_one(t_entity *peepz, int entity_count, t_entity thisHero)
{
	//DETERMINE ATTACKERS
    int enemy_attackers = 0;
    for (int i = 0; i < heroes_per_player; i++)
    {
        if (theirHeroes[i].offensive == 1)
            enemy_attackers++;
    }

	int min_away_from_base = (((max_creep_hp + 1) / 2) * 400) + 300 + (2200 * enemy_attackers);
	int max_away_from_base = min_away_from_base * 4 / 3;
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
		int mx = peepz[i].x * 10000 / (mid_x * 2);
		int my = peepz[i].y * 10000 / (mid_y * 2);
		int target_corner = (mx >= my) ? 1 : 0;
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && peepz[i].dist_to_base < nearest && \
		(target_corner == 1 || peepz[i].dist_to_base <= 6000 || (peepz[i].shield_life > 0 && peepz[i].threat_for == 1)))
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }
    
    if (should_I_shield(thisHero, target))
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
	if (target.type == 0 && target.dist_to_base <= min_away_from_base && mana >= 10 && \
	dist(target.x, target.y, thisHero.x, thisHero.y) <= 1280 && \
	target.shield_life == 0)
    {
        t_xypair wind_reverse = reverse_vector(target.x, target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d target dist: %d, min dist: %d\n", wind_reverse.x, wind_reverse.y, target.dist_to_base, min_away_from_base);
        mana = mana - 10;
        return (1);
    }
    if (target.type == 0 && target.dist_to_base <= max_away_from_base)
    {
        t_xypair improved = improve_target(target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        printf("MOVE %d %d\n", improved.x, improved.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 70;
        to_travel.y = 35;
		if (max_away_from_base >= 6000)
        	to_travel = vectorise(max_away_from_base, to_travel.x, to_travel.y);
		else
        	to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        t_xypair improved = improve_move(x_togo, y_togo);
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    return (0);
}

int bullwark_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    //DETERMINE ATTACKERS
    int enemy_attackers = 0;
    for (int i = 0; i < heroes_per_player; i++)
    {
        if (theirHeroes[i].offensive == 1)
            enemy_attackers++;
    }
    
    //DETERMINE POST
    int striking_distance = 1100 + 2200 * enemy_attackers;
    t_xypair post;
    post.x = 1;
    post.y = 1;
    if (nearest_hero_id != -1)
    {
        int enemyX = theirHeroes[nearest_hero_id % heroes_per_player].lastX + theirHeroes[nearest_hero_id % heroes_per_player].vx;
        int enemyY = theirHeroes[nearest_hero_id % heroes_per_player].lastY + theirHeroes[nearest_hero_id % heroes_per_player].vy;

        post.x = abs(base_x - enemyX);
        post.y = abs(base_y - enemyY);
    }
    post = vectorise(striking_distance, post.x, post.y);
    post.x = from_base('x', base_x, post.x);
    post.y = from_base('y', base_y, post.y);
    // if (nearest_hero_id != -1) // DONT LET ENEMY BE CLOSER TO YOUR BASE THAN YOU
    // {
    //     if (dist(post.x, post.y, base_x, base_y) > theirHeroes[nearest_hero_id % heroes_per_player].distToMyBase)
    //     {
    //         post.x = theirHeroes[nearest_hero_id % heroes_per_player].lastX + theirHeroes[nearest_hero_id % heroes_per_player].vx;
    //         post.y = theirHeroes[nearest_hero_id % heroes_per_player].lastY + theirHeroes[nearest_hero_id % heroes_per_player].vy;
    //     }
    // }

    // DETERMINE CLOSEST TARGET TO ME
    t_entity my_target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        int dist_from_hero = dist(thisHero.x, thisHero.y, peepz[i].x, peepz[i].y);
        // int dist_from_post = dist(post.x, post.y, peepz[i].x, peepz[i].y);
        if(peepz[i].type == 0 && dist_from_hero < nearest)
        {
            my_target = peepz[i];
            nearest = dist_from_hero;
        }
    }

    // DETERMINE ACTION
    if (mana >= 10 && my_target.type == 0 && \
        dist(my_target.x, my_target.y, thisHero.x, thisHero.y) <= 1280 && \
        my_target.shield_life == 0) // PUSH MONSTER
    {
        t_xypair wind_reverse = reverse_vector(my_target.x, my_target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    if (mana >= 50 && nearest_hero_id != -1 && \
        dist(theirHeroes[nearest_hero_id % 3].lastX, theirHeroes[nearest_hero_id % 3].lastY, thisHero.x, thisHero.y) <= 1280 && \
        theirHeroes[nearest_hero_id % 3].shieldHP == 0) // PUSH PLAYER
    {
        t_xypair wind_reverse = reverse_vector(my_target.x, my_target.y, 3000);
        wind_reverse.x = thisHero.x + wind_reverse.x;
        wind_reverse.y = thisHero.y + wind_reverse.y;
        printf("SPELL WIND %d %d FUS RO DA\n", wind_reverse.x, wind_reverse.y);
        mana = mana - 10;
        return (1);
    }
    if(should_I_shield(thisHero, my_target) || \
        (thisHero.shield_life <= 0 && mana >= 200 && nearest_hero_id != -1) || \
        (thisHero.shield_life <= 0 && mana >= 20 && my_target.type != 0 && thisHero.dist_to_base < (striking_distance - 100) && hostile_creeps_in_base == 0)) // SHIELD
    {
        printf("SPELL SHIELD %d\n", thisHero.id);
        mana = mana - 10;
        return (1);
    }
    else if (my_target.type == 0 && my_target.threat_for == 1 && my_target.near_base == 1 && my_target.dist_to_base <= thisHero.dist_to_base)
    {
        t_xypair improved;
        if (nearest_hero_id != -1)
        {
            int enemyX = theirHeroes[nearest_hero_id % heroes_per_player].lastX + theirHeroes[nearest_hero_id % heroes_per_player].vx;
            int enemyY = theirHeroes[nearest_hero_id % heroes_per_player].lastY + theirHeroes[nearest_hero_id % heroes_per_player].vy;
            improved = improve_target_towards_enemy(my_target, peepz, entity_count, enemyX, enemyY);
        }
        else
            improved = improve_target(my_target, peepz, entity_count);
        improved = improve_move(improved.x, improved.y);
        t_xypair towards_enemy = improved;
        printf("MOVE %d %d\n", improved.x, improved.y);
    }
    else
    {
        // printf("MOVE %d %d\n", from_base('x', base_x, 300), from_base('y', base_y, 300));
        t_xypair improved = improve_move(post.x, post.y);
        printf("MOVE %d %d ALL OUT\n", improved.x, improved.y);
    }
    return (0);
}


int bullwark_strat(int index, t_entity *peepz, int entity_count, t_entity *myHeroes)
{
    // if (index == 2 && aggressive_shielding == 1)
    //     return(hard_defensive_two(peepz, entity_count, myHeroes[2]));
    // else if (index == 2)
    //     return(neutral_farm_two(peepz, entity_count, myHeroes[2]));
    // else if (index == 1 && aggressive_shielding == 1)
    //     return(hard_defensive_one(peepz, entity_count, myHeroes[1]));
    // else if (index == 1)
    //     return(neutral_farm_one(peepz, entity_count, myHeroes[1]));
    if (index == 2)
        bullwark_two(peepz, entity_count, myHeroes[2]);
    else if (index == 1)
        bullwark_one(peepz, entity_count, myHeroes[1]);
    else
        bullwark_zero(peepz, entity_count, myHeroes[0]);
    return (0);
}

//STRATEGIES_END /////////////////////////////////////

int main()
{
    // The corner of the map representing your base
    scanf("%d%d", &base_x, &base_y);
    enemy_base_x = (base_x == 0) ? 17630 : 0;
    enemy_base_y = (base_y == 0) ? 9000 : 0;
    // fprintf(stderr, "My base: %d,%d // Their base: %d,%d\n", base_x, base_y, enemy_base_x, enemy_base_y);

    scanf("%d", &heroes_per_player);
    hero_offset = (base_x == 0) ? heroes_per_player : 0;

    int myHealth = 0;
    int myMana = 0;
    int theirHealth = 0;
    int theirMana = 0;
    estimated_wild_mana = 0;
    nearest_hero_id = -1;

    turns = 0;
	max_creep_hp = 10;

    // flags
    all_out = 0;
    enemies_spotted = 0;
    aggressive_shielding = 0;
    mind_controlled = 0;

    t_entity myHeroes[3];
    for (int i = 0; i < heroes_per_player; i++)
    {
        enemyConstructor(i);
    }
    
    // game loop
    while (1) 
    {
        int theirOldMana = theirMana;
        int myOldMana = myMana;
        int myManaDiff = 0;
        scanf("%d%d%d%d", &myHealth, &myMana, &theirHealth, &theirMana);
        mana = myMana;
        if (myMana <= 100 && turns < 200)
            all_out = 0;

        if (theirMana >= theirOldMana)
            enemy_estimated_wild_mana += theirMana - theirOldMana;
        if (myMana >= myOldMana)
            myManaDiff = myMana - myOldMana;
        int mana_multiplier = 0;

        // Amount of heros and monsters you can see
        int entity_count;
        scanf("%d", &entity_count);

        // Array of all entities
        t_entity peepz[entity_count];
        hostile_creeps = 0;
        hostile_creeps_in_base = 0;
        hostile_base_creeps_shielded = 0;

        visible_enemies = 0;
        visible_enemies_my_base = 0;
        visible_enemies_their_base = 0;

        enemy_dist_to_base = INT_MAX;
        nearest_hero_dist = INT_MAX;

        for (int i = 0; i < entity_count; i++) 
        {
            int id; // Unique identifier
            int type; // 0=monster, 1=your hero, 2=opponent hero 
            int shield_life; // Count down until shield spell fades
            int is_controlled; // Equals 1 when this entity is under a control spell
            int health; // Remaining health of this monster
            int near_base; // 0=monster with no target yet, 1=monster targeting a base
            int threat_for; // Given this monster's trajectory, is it a threat to 1=your base, 2=your opponent's base, 0=neither
            
            // Position of this entity
            int x;
            int y;
            // Trajectory of this monster
            int vx;
            int vy;
            scanf("%d%d%d%d%d%d%d%d%d%d%d", &id, &type, &x, &y, &shield_life, &is_controlled, &health, &vx, &vy, &near_base, &threat_for);
            
            peepz[i].id = id;
            peepz[i].type = type;
            peepz[i].shield_life = shield_life;
            // fprintf(stderr, "shield life = %d\n", peepz[i].shield_life);
            peepz[i].is_controlled = is_controlled;
            peepz[i].health = health;
            peepz[i].near_base = near_base;
            peepz[i].threat_for = threat_for;
            peepz[i].x = x;
            peepz[i].y = y;
            peepz[i].vx = vx;
            peepz[i].vy = vy;
            peepz[i].dist_to_base = dist(x, y, base_x, base_y);
            
            if (peepz[i].type == 1 && peepz[i].dist_to_base > 6000)
                mana_multiplier++;
            if (peepz[i].type == 0 && peepz[i].threat_for == 1)
                hostile_creeps++;
            if (peepz[i].type == 0 && peepz[i].threat_for == 1 && peepz[i].near_base == 1)
                hostile_creeps_in_base++;
            if (peepz[i].type == 0 && peepz[i].threat_for == 1 && peepz[i].shield_life > 0)
            {
                hostile_base_creeps_shielded++;
                aggressive_shielding = 1;
            }
            if (peepz[i].type == 2)
            {
                visible_enemies++;
                enemies_spotted = 1;
            }
            if (peepz[i].type == 2 && dist(peepz[i].x, peepz[i].y, base_x, base_y) <= 10000)
                visible_enemies_my_base++;
            if (peepz[i].type == 2 && dist(peepz[i].x, peepz[i].y, enemy_base_x, enemy_base_y) <= 6000)
                visible_enemies_their_base++;
            if (peepz[i].type == 1 && peepz[i].is_controlled == 1)
                mind_controlled = 1;
            if (peepz[i].type == 2 && peepz[i].dist_to_base <= enemy_dist_to_base)
                enemy_dist_to_base = peepz[i].dist_to_base;
             if (peepz[i].type == 1 && peepz[i].dist_to_base <= nearest_hero_dist)
                nearest_hero_dist = peepz[i].dist_to_base;
            if (peepz[i].type == 2)
            {
                t_xypair movement_vector;
                movement_vector = enemyUpdater(peepz[i].id, peepz[i].x, peepz[i].y, myHeroes, peepz[i].shield_life);
                peepz[i].vx = movement_vector.x;
                peepz[i].vy = movement_vector.y;
                fprintf(stderr, "Hero %d has vector %d,%d\n", peepz[i].id, peepz[i].vx, peepz[i].vy);
            }
			if (peepz[i].type == 0 && peepz[i].health > max_creep_hp)
				max_creep_hp = peepz[i].health;
        }
        for (int i = 0; i < entity_count; i++)
        {
            if (peepz[i].type == 1)
            {
                myHeroes[peepz[i].id % heroes_per_player] = peepz[i];
            }
        }

        for (int i = 0; i < heroes_per_player; i++) //updates the distance between where enemies were last seen and your heroes, even if no enemies spotted this turn
        {
            enemyUpdater(i, -1, -1, myHeroes, -1);
        }
        fprintf(stderr, "Hero %d at %d,%d \nHero %d at %d,%d \nHero %d at %d,%d \n", theirHeroes[0].heroId + hero_offset, theirHeroes[0].lastX, theirHeroes[0].lastY,  theirHeroes[1].heroId + hero_offset, theirHeroes[1].lastX, theirHeroes[1].lastY, theirHeroes[2].heroId + hero_offset, theirHeroes[2].lastX, theirHeroes[2].lastY);
        fprintf(stderr, "Nearest hero seen to base: %d at %d,%d (distance == %d)\n", nearest_hero_id, theirHeroes[nearest_hero_id % 3].lastX, theirHeroes[nearest_hero_id % 3].lastY, dist(base_x, base_y, theirHeroes[nearest_hero_id % 3].lastX, theirHeroes[nearest_hero_id % 3].lastY));
        estimated_wild_mana = estimated_wild_mana + ((double)myManaDiff * (double)mana_multiplier / (double)heroes_per_player);
        
        enemy_defenders = 0;
        for (int e = 0; e < heroes_per_player; e++)
        {
            if (theirHeroes[e].inTheirBase == 1)
                enemy_defenders++;
        }

        turns++;
        for (int i = 0; i < heroes_per_player; i++) 
        {
            // Write an action using printf(). DON'T FORGET THE TRAILING \n
            // To debug: fprintf(stderr, "Debug messages...\n");
			if (hostile_base_creeps_shielded >= 1)
            {
                hard_defensive_strat(i, peepz, entity_count, myHeroes);
            }
            else if ((myHealth == 1 && theirHealth == 1) || hostile_creeps >= 8 || hostile_creeps_in_base >= 5 || hostile_base_creeps_shielded >= 1)
            {
                // hard_defensive_strat(i, peepz, entity_count, myHeroes);
				bullwark_strat(i, peepz, entity_count, myHeroes);
            }
            else if (myHealth > (theirHealth + 1))
            {
                // ahead_defensive_strat(i, peepz, entity_count, myHeroes);
				bullwark_strat(i, peepz, entity_count, myHeroes);
            }
            else if (myHealth <= (theirHealth + 1) && (myMana >= 200 || all_out))
            {
                all_out = 1;
                mana_aggressive_strat(i, peepz, entity_count, myHeroes);
            }
            else if (estimated_wild_mana > enemy_estimated_wild_mana && visible_enemies_my_base >= 1)
            {
                // neutral_lead_strat(i, peepz, entity_count, myHeroes);
				bullwark_strat(i, peepz, entity_count, myHeroes);
            }
            else
                neutral_farm_strat(i, peepz, entity_count, myHeroes);
        }
    }

    return 0;
}