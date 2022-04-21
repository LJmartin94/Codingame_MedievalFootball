#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

// The corner of the map representing your base
int base_x;
int base_y;

// The centre of the map
int mid_x = 8815;
int mid_y = 4500;

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

int dist(int Ax, int Ay, int Bx, int By)
{
    int x_diff = Ax - Bx;
    int y_diff = Ay - By;
    int dist_squared = (x_diff * x_diff) + (y_diff * y_diff);
    return (dist_squared);
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

int main()
{
    // The corner of the map representing your base
    scanf("%d%d", &base_x, &base_y);
    // Always 3
    int heroes_per_player;
    scanf("%d", &heroes_per_player);

    // game loop
    while (1) {
        for (int i = 0; i < 2; i++) 
        {
            // Your base health
            int health;
            // Ignore in the first league; Spend ten mana to cast a spell
            int mana;
            scanf("%d%d", &health, &mana);
        }
        // Amount of heros and monsters you can see
        int entity_count;
        scanf("%d", &entity_count);

        // Array of all entities
        t_entity peepz[entity_count];

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
            peepz[i].is_controlled = is_controlled;
            peepz[i].health = health;
            peepz[i].near_base = near_base;
            peepz[i].threat_for = threat_for;
            peepz[i].x = x;
            peepz[i].y = y;
            peepz[i].vx = vx;
            peepz[i].vy = vy;
            peepz[i].dist_to_base = dist(x, y, base_x, base_y);
        }

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

        for (int i = 0; i < heroes_per_player; i++) 
        {

            // Write an action using printf(). DON'T FORGET THE TRAILING \n
            // To debug: fprintf(stderr, "Debug messages...\n");


            // In the first league: MOVE <x> <y> | WAIT; In later leagues: | SPELL <spellParams>;
            if (target.type == 0 && (i != 0 || target.near_base))
                printf("MOVE %d %d\n", target.x, target.y);
            else
                printf("MOVE %d %d\n", to_mid('x', base_x, 3000), to_mid('y', base_y, 3000));
        }
    }

    return 0;
}