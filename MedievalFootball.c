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

// The centre of the map
int mid_x = 8815;
int mid_y = 4500;

// My mana: Spend ten mana to cast a spell
int mana;
double estimated_wild_mana;
double enemy_estimated_wild_mana;

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

int dist(int Ax, int Ay, int Bx, int By)
{
    int x_diff = Ax - Bx;
    int y_diff = Ay - By;
    int dist_squared = (x_diff * x_diff) + (y_diff * y_diff);
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

//UTILS_END/////////////////////////////////////

//STRATEGIES /////////////////////////////////////
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
    if (target.type == 0)
    {
        printf("MOVE %d %d\n", target.x, target.y);
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
        printf("MOVE %d %d\n", x_togo, y_togo);
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
    if (target.type == 0)
    {
        printf("MOVE %d %d\n", target.x, target.y);
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
        printf("MOVE %d %d\n", x_togo, y_togo);
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
    if (mana >= 10 && target.type == 0 && target.near_base == 1 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.near_base == 1)
    {
        printf("MOVE %d %d NEUTRAL\n", target.x, target.y);
        return (1);
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
        printf("MOVE %d %d NEUTRAL\n", x_togo, y_togo);
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
            // fprintf(stderr, "target_dist = %d\n", target_dist);
            target = peepz[i];
            nearest = target_dist;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (target.type == 0)
    {
        // fprintf(stderr, "Attacking target, %d,%d\n", target.x + target.vx, target.y + target.vy);
        printf("MOVE %d %d\n", target.x + target.vx, target.y + target.vy);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 10;
        to_travel = vectorise(7100, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
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
    if (target.type == 0)
    {
        printf("MOVE %d %d\n", target.x + target.vx, target.y + target.vy);
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
        printf("MOVE %d %d\n", x_togo, y_togo);
    }
    return (0);
}

int neutral_farm_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for == 1 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, base_x, base_y) > dist(thisHero.x, thisHero.y, base_x, base_y) && 
    if (mana >= 10 && target.type == 0 && target.near_base == 1 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.dist_to_base <= 7100)
    {
        printf("MOVE %d %d FARMING\n", target.x, target.y);
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
        printf("MOVE %d %d FARMING\n", x_togo, y_togo);
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
    if (mana >= 100 && target.type == 0 && target.dist_to_base < 6000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        printf("MOVE %d %d\n", target.x, target.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 2;
        to_travel.y = 10;
        to_travel = vectorise(5000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
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
    if (mana >= 100 && target.type == 0 && target.dist_to_base < 6000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        printf("MOVE %d %d\n", target.x, target.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 10;
        to_travel.y = 2;
        to_travel = vectorise(5000, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
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
    if (mana >= 10 && target.type == 0 && target.dist_to_base < 6000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.dist_to_base < 7100 && target.near_base == 1)
    {
        printf("MOVE %d %d AHEAD DEF\n", target.x, target.y);
        return (1);
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
        printf("MOVE %d %d AHEAD DEF\n", x_togo, y_togo);
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
    if (mana > 10 && target.type == 0 && target.dist_to_base < 6000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        printf("MOVE %d %d\n", target.x, target.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 2;
        to_travel.y = 10;
        to_travel = vectorise(4500, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
    }
    return (0);
}

int hard_defensive_one(t_entity *peepz, int entity_count, t_entity thisHero)
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
    if (mana > 10 && target.type == 0 && target.dist_to_base < 6000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0)
    {
        printf("MOVE %d %d\n", target.x, target.y);
        return (1);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 10;
        to_travel.y = 2;
        to_travel = vectorise(4500, to_travel.x, to_travel.y);
        int x_togo = from_base('x', base_x, to_travel.x);
        int y_togo = from_base('y', base_y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
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
    if (mana >= 10 && target.type == 0 && target.dist_to_base < 6000 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.dist_to_base < 7100 && target.near_base == 1)
    {
        printf("MOVE %d %d HARD DEF\n", target.x, target.y);
        return (1);
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
        printf("MOVE %d %d HARD DEF\n", x_togo, y_togo);
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
        if(peepz[i].type == 0 && peepz[i].threat_for != 2 && dist_from_hero <= 2200 && \
            dist_to_enemy_base < nearest && \
            peepz[i].health >= 10 && peepz[i].shield_life == 0);
        {
            target = peepz[i];
            nearest = dist_to_enemy_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (mana > 10 && num_of_creeps_around >= 2)
    {
        printf("SPELL WIND %d %d FUS RO DA\n", enemy_base_x, enemy_base_y);
        mana = mana - 10;
        return (1);
    }
    else if (mana > 10 && target.type == 0)
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL CONTROL %d %d %d\n", target.id, enemy_base_x, enemy_base_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && dist(target.x, target.y, thisHero.x, thisHero.y) > 2200)
    {
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(1000, to_travel.x, to_travel.y);
        int x_togo = to_base('x', target.x, to_travel.x);
        int y_togo = to_base('y', target.y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
    }
    else
    {
        printf("MOVE %d %d\n", mid_x, mid_y);
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

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    if (mana > 10 && target.type == 0 && target.near_base == 1 && target.threat_for == 2 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 2200 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) >= 1280 && \
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
        target.near_base == 1 && target.threat_for == 2)
    {
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(1000, to_travel.x, to_travel.y);
        int x_togo = to_base('x', target.x, to_travel.x);
        int y_togo = to_base('y', target.y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
    }
    else
    {
        t_xypair to_travel;
        to_travel.x = 1;
        to_travel.y = 1;
        to_travel = vectorise(6000, to_travel.x, to_travel.y);
        int x_togo = to_base('x', enemy_base_x, to_travel.x);
        int y_togo = to_base('y', enemy_base_y, to_travel.y);
        printf("MOVE %d %d\n", x_togo, y_togo);
    }
    return (0);
}

int mana_aggressive_zero(t_entity *peepz, int entity_count, t_entity thisHero)
{
    t_entity target = peepz[0];
    int nearest = INT_MAX;
    for (int i = 0; i < entity_count; i++) 
    {
        if(peepz[i].type == 0 && peepz[i].threat_for == 1 && peepz[i].dist_to_base < nearest)
        {
            target = peepz[i];
            nearest = peepz[i].dist_to_base;
        }
    }

    // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
    // dist(target.x, target.y, base_x, base_y) > dist(thisHero.x, thisHero.y, base_x, base_y) && 
    if (mana >= 10 && target.type == 0 && target.near_base == 1 && \
        dist(target.x, target.y, thisHero.x, thisHero.y) < 1280 && \
        target.shield_life == 0 && (target.health >= 3 || target.dist_to_base < thisHero.dist_to_base))
    {
        // printf("SPELL WIND %d %d\n", from_base('x', thisHero.x, 1500), from_base('y', thisHero.y, 1500));
        printf("SPELL WIND %d %d FUS RO DA\n", mid_x, mid_y);
        mana = mana - 10;
        return (1);
    }
    else if (target.type == 0 && target.dist_to_base <= 7100)
    {
        printf("MOVE %d %d ALL OUT\n", target.x, target.y);
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
        printf("MOVE %d %d ALL OUT\n", x_togo, y_togo);
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
//STRATEGIES_END /////////////////////////////////////

int main()
{
    // The corner of the map representing your base
    scanf("%d%d", &base_x, &base_y);
    enemy_base_x = (base_x == 0) ? 17630 : 0;
    enemy_base_y = (base_y == 0) ? 9000 : 0;
    // fprintf(stderr, "My base: %d,%d // Their base: %d,%d\n", base_x, base_y, enemy_base_x, enemy_base_y);

    // Always 3
    int heroes_per_player;
    scanf("%d", &heroes_per_player);

    int myHealth = 0;
    int myMana = 0;
    int theirHealth = 0;
    int theirMana = 0;
    estimated_wild_mana = 0;

    // game loop
    while (1) 
    {
        int theirOldMana = theirMana;
        int myOldMana = myMana;
        int myManaDiff = 0;
        scanf("%d%d%d%d", &myHealth, &myMana, &theirHealth, &theirMana);
        mana = myMana;

        if (theirMana >= theirOldMana)
            enemy_estimated_wild_mana += theirMana - theirOldMana;
        if (myMana >= myOldMana)
            myManaDiff = myMana - myOldMana;
        int mana_multiplier = 0;
        fprintf(stderr, "start mana %d, diff %d\n", myMana, myManaDiff);

        // Amount of heros and monsters you can see
        int entity_count;
        scanf("%d", &entity_count);

        // Array of all entities
        t_entity peepz[entity_count];
        int hostile_creeps = 0;
        int hostile_creeps_in_base = 0;

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
        }
        // fprintf(stderr, "estimated wild mana %f, myManadiff %f, mana_multi %f\n", estimated_wild_mana, myManaDiff, (mana_multiplier / heroes_per_player));
        estimated_wild_mana = estimated_wild_mana + ((double)myManaDiff * (double)mana_multiplier / (double)heroes_per_player);
        
        // (double)(myManaDiff * (double)(mana_multiplier / heroes_per_player));

        // t_entity heroZero;
        // t_entity heroOne;
        // t_entity heroTwo;
        t_entity myHeroes[heroes_per_player];
        
        for (int i = 0; i < entity_count; i++)
        {
            if (peepz[i].type == 1)
            {
                myHeroes[peepz[i].id % heroes_per_player] = peepz[i];
            }
        }

        for (int i = 0; i < heroes_per_player; i++) 
        {
            // Write an action using printf(). DON'T FORGET THE TRAILING \n
            // To debug: fprintf(stderr, "Debug messages...\n");
            if (myHealth == 1 || hostile_creeps >= 8 || hostile_creeps_in_base >= 5)
            {
                hard_defensive_strat(i, peepz, entity_count, myHeroes);
            }
            else if (myHealth > theirHealth)
            {
                ahead_defensive_strat(i, peepz, entity_count, myHeroes);
            }
            else if (myHealth <= theirHealth && myMana >= 200)
            {
                mana_aggressive_strat(i, peepz, entity_count, myHeroes);
            }
            else if (estimated_wild_mana > enemy_estimated_wild_mana)
            {
                fprintf(stderr, "estimated wild mana mine: %f vs theirs: %f\n", estimated_wild_mana, enemy_estimated_wild_mana);
                neutral_lead_strat(i, peepz, entity_count, myHeroes);
            }
            else
                neutral_farm_strat(i, peepz, entity_count, myHeroes);
        }
    }

    return 0;
}