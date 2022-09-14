// Written according to ISO/IEC 9899:1999

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <windows.h> // used for Sleep(). Under Linux, use <unistd.h> for sleep() instead and divide any delay constant by 1000.

#define SCV_TRAINING_DELAY 4000
#define SCV_TRAVELING_DELAY 3000
#define SCV_TRANSPORT_DELAY 2000
#define MARINE_TRAINING_DELAY 1000

#define START_SCVS 5
#define MAX_SCVS 180
#define MAX_MARINES 20
#define MAX_UNITS (MAX_MARINES + MAX_SCVS)

#define SCV_LOAD 8
#define UNIT_COST 50
#define NODE_MINERALS 500

static long player_minerals = 0;
static long map_minerals;
static int nodes_count = -1;
static int *nodes;
static pthread_mutex_t command_center_mutex;
static pthread_mutex_t *node_muteces;

void *scv(void *scv_id){
    Sleep(SCV_TRAINING_DELAY);
    printf("SCV good to go, sir.\n");

    short *is_empty_node = (short*)malloc(nodes_count * sizeof(short));
    int i;
    for (i = 0; i < nodes_count; ++i)
        is_empty_node[i] = 0;

    while (map_minerals){
        for (i = 0; i < nodes_count; ++i){
            if (!is_empty_node[i]){ // if it's not empty in my book, I'll go to it
                Sleep(SCV_TRAVELING_DELAY);

                if (pthread_mutex_trylock(&node_muteces[i])) // if I don't get a lock, I move on
                    continue;
                else {
                    if (!nodes[i]){ // once there, I check if it's been emptied
                        pthread_mutex_unlock(&node_muteces[i]);
                        is_empty_node[i] = 1;
                        continue;
                    }

                    printf("SCV %d is mining from mineral block %d\n", (int)scv_id, i);
                    int collected_minerals;
                    if (nodes[i] <= SCV_LOAD){
                        collected_minerals = nodes[i];
                        nodes[i] = 0;
                        pthread_mutex_unlock(&node_muteces[i]);
                        is_empty_node[i] = 1;
                    } else {
                        nodes[i] -= SCV_LOAD;
                        pthread_mutex_unlock(&node_muteces[i]);
                        collected_minerals = SCV_LOAD;
                    }

                    printf("SCV %d is transporting minerals\n", (int)scv_id);
                    Sleep(SCV_TRANSPORT_DELAY);
                    pthread_mutex_lock(&command_center_mutex);
                    player_minerals += (long)collected_minerals;
                    map_minerals -= (long)collected_minerals;
                    pthread_mutex_unlock(&command_center_mutex);
                }
            }
        }
    }
    free(is_empty_node);
    return NULL;
}

// Instead of passing the number of nodes to the program directly, we ask for it at the start, because
// it's easier. :) The main focus is synchronizing threads anyway

int main(int argc, char *argv[])
{
    while (nodes_count < 2){
        printf("Enter the number of nodes (at least 2) the map will have: \n");
        scanf("%d", &nodes_count);
    }

    map_minerals = ((long)nodes_count) * NODE_MINERALS;

    nodes = (int*)malloc(nodes_count * sizeof(int));
    if ((nodes == NULL)){
        printf("Memory not allocated!\n");
        return 1;
    }

    node_muteces = (pthread_mutex_t*)malloc(nodes_count * sizeof(pthread_mutex_t));
    if ((node_muteces == NULL)){
        printf("Memory not allocated!\n");
        return 1;
    }

    int i, err;
    for (i = 0; i < nodes_count; ++i){
        nodes[i] = NODE_MINERALS;
        err = pthread_mutex_init(&node_muteces[i], NULL);
        if (err) {
            printf("Failed to initialize mutex!\n");
            return 1;
        }
    }

    err = pthread_mutex_init(&command_center_mutex, NULL);
    if (err) {
        printf("Failed to initialize mutex!\n");
        return 1;
    }

    pthread_t scvs[MAX_SCVS];
    int scv_count = START_SCVS;
    int marine_count = 0;

    for (i = 0; i < scv_count; ++i){
        err = pthread_create(&scvs[i], NULL, scv, (void*)i);
        if (err){
            printf("Failed to create thread!\n");
            return 1;
        }
    }

    while (marine_count < MAX_MARINES || map_minerals){
        char input;
        scanf(" %c", &input);
        if (input != 'm' && input != 's'){
            printf("Command not recognized!\n", input);
            continue;
        }

        if (player_minerals < UNIT_COST){
            printf("Not enough minerals! Try again after a few moments.\n");
            continue;
        }

        switch (input){
        case 's':
            if (scv_count == MAX_SCVS){
                printf("You can't train any more SCVs!\n");
            }
            player_minerals -= UNIT_COST;
            err = pthread_create(&scvs[scv_count], NULL, scv, (void*)scv_count);
            if (err){
                printf("Failed to create thread!\n");
                return 1;
            }
            ++scv_count;
            break;

        case 'm':
            if (marine_count == MAX_MARINES){
                printf("You can't train any more Marines!\n");
                break;
            }
            player_minerals -= UNIT_COST;
            Sleep(MARINE_TRAINING_DELAY);
            printf("You wanna piece of me, boy?\n");
            ++marine_count;
        }

    }

    printf("Map Minerals %ld, player minerals %ld, SCVs %d, Marines %d\n", nodes_count * NODE_MINERALS,
                player_minerals, scv_count, marine_count);

    for (i = 0; i < scv_count; ++i)
        pthread_join(scvs[i], NULL);

    for (i = 0; i < nodes_count; ++i)
        pthread_mutex_destroy(&node_muteces[i]);

    pthread_mutex_destroy(&command_center_mutex);

    free(node_muteces);
    free(nodes);
    return 0;
}
