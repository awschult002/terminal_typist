#define TB_IMPL
#include "termbox.h"
#include "box.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "timer.h"
#include <math.h>
#ifdef WIN_32
#include "windows.h"
#define SLEEP(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP(x) usleep(x*1000)
#endif

#define MIN(x,y) ((x < y) ? x : y)
#define MAX(x,y) ((x > y) ? x : y)

#define DICTIONARY_FILE "/usr/share/dict/american-english"

// GLOBALS 
struct timer_t game_timer;
struct timer_t fall_rate_timer;
struct timer_t spawn_rate_timer;
unsigned int game_seconds = 0;

bool game_finished = false;
bool state_change = true;

#define USR_BUF_SIZE 32
char usr_buf[USR_BUF_SIZE];

unsigned int screen_height = 0;
unsigned int screen_width = 0;
unsigned int finish_line = 0;
unsigned int words_killed = 0;

//global word list
size_t num_lines = 0;
size_t max_len = 0;
char* word_list;

void read_system_dictionary()
{
    FILE* dict = fopen(DICTIONARY_FILE, "r");

    if(!dict)
    {
        fputs("ERROR: System dictionary file not found.", stderr);
        exit(EXIT_FAILURE);
    }

    char buf[128] = {0};
    //filter the words
    while(fgets(buf,128,dict) != NULL)
    {
        if(!strstr(buf, "\'"))
        {
            num_lines++;
            max_len = MAX(strlen(buf), max_len);
        }
    }

    // allocat the storage for all of the possible words
    word_list = malloc(num_lines * (max_len+1));
    if(!word_list)
    {
        fputs("ERROR: Cannot malloc enough space.", stderr);
        exit(EXIT_FAILURE);
    }
    
    //fill the newly created array
    rewind(dict);
    num_lines = 0;
    while(fscanf(dict, "%s", buf) != EOF)
    {
        if(!strstr(buf, "\'s"))
        {
            strncpy((word_list+(num_lines * max_len)), buf, max_len+1); 
            num_lines++;
        }
    }

    fclose(dict);
}

//structure of enemy words on screen
struct enemy {
    char* word;
    int x,y,len;
    struct enemy *next;
};
struct enemy *enemy_list = NULL;


void clear_usr_buf()
{
    int len = USR_BUF_SIZE-1;
    while(len--)
        usr_buf[len] = 0;
}



void print_horizontal_rule(unsigned int y)
{
    int len = tb_width();
    for(int i = 0; i < len; i++)
    {
        tb_set_cell(i, y, BOX_DRAWINGS_HEAVY_HORIZONTAL, TB_DEFAULT, TB_DEFAULT);
    }
}



void render_game()
{
    if(!state_change)
        return;

    // render each enemy
    struct enemy *trv = enemy_list;
    while(trv)
    {
        //clear previous
        for(int i = 0; i < trv->len; i++)
            tb_set_cell(trv->x+i, trv->y-1, ' ', TB_DEFAULT, TB_DEFAULT);

        //print new
        tb_print(trv->x, trv->y, TB_DEFAULT, TB_DEFAULT, trv->word);

        trv = trv->next;
    }


    // render HUD
    print_horizontal_rule(finish_line);
    for(int i = 0; i < USR_BUF_SIZE; i++)
        tb_set_cell(i+2, finish_line + 1, ' ', TB_DEFAULT, TB_DEFAULT);

    tb_print(2, finish_line + 1, TB_DEFAULT, TB_DEFAULT, usr_buf);
    print_horizontal_rule(finish_line+2);
    tb_printf(2, screen_height-1, TB_DEFAULT, TB_DEFAULT, "Words Killed: %d       Time: %d", words_killed, game_seconds);
    tb_print(screen_width-20, screen_height-1, TB_DEFAULT, TB_DEFAULT, "Press \':\' to quit.");


    tb_present();
    state_change = 0;
}

void create_enemy()
{
    struct enemy *trv;
    if(!enemy_list)
    {
        enemy_list = malloc(sizeof(struct enemy));
        trv = enemy_list;
    }
    else
    {
        trv = enemy_list;
        while(trv->next)
            trv = trv->next;
        trv->next = malloc(sizeof(struct enemy));
        trv = trv->next;
    }

    int idx = rand() % num_lines;
    trv->word = word_list + (idx*max_len);
    trv->len = strlen(trv->word);

    trv->y = 0;
    trv->x = rand() % (screen_width - trv->len); //make sure the word fits in
                                                 //the screen
    trv->next = NULL;
}

//deletes the enemy matching the word provided
int check_remove_word(char* word)
{
    if(!enemy_list)
        return 0;

    struct enemy **trv = &enemy_list;
    while(*trv)
    {
        //found a word, increment counter, clear buffer
        if(!strncmp((*trv)->word, usr_buf, (*trv)->len))
        {
            struct enemy *tmp = *trv;
            words_killed++;

            //clear off screen
            for(int i = 0; i < (*trv)->len; i++)
                tb_set_cell((*trv)->x+i, (*trv)->y, ' ', TB_DEFAULT, TB_DEFAULT);

            if(tmp == enemy_list)
                enemy_list = tmp->next;
            else
                *trv = tmp->next;

            free(tmp);

            //clear usr input buffer
            clear_usr_buf();
            return 1;
        }
        trv = &((*trv)->next);
    }

    return 0;
}

// sets up initial game parameters
void init_game()
{
    tb_init();
    screen_height = tb_height();
    screen_width = tb_width();
    finish_line = screen_height - 4;

    srand(time(NULL));
    create_enemy();
}

//ignore all input except characters
int eval_input()
{
    struct tb_event ev;
    if(tb_peek_event(&ev,0) == TB_OK)
    {
        if(ev.type == TB_EVENT_KEY) 
        {
            int len = strlen(usr_buf);
            if(ev.key == TB_KEY_BACKSPACE2)
            {
                if(len)
                    usr_buf[len-1] = '\0';
            }
            else if(ev.ch >= 'A' && ev.ch <= 'z')
            {
                usr_buf[len] = ev.ch;
            }
            
            if(ev.ch == ':')
                game_finished = 1;

            return 1;
        }
    }
    return 0;
}


int update_game_time()
{
    int ret = 0;
    if(timer_elapsed(&game_timer))
    {
        game_seconds++;
        start_timer(&game_timer);
        ret |= 1;
    }

    if(timer_elapsed(&fall_rate_timer))
    {
        //move all pieces
        struct enemy *trv = enemy_list;
        while(trv)
        {
            trv->y++;
            trv = trv->next;
        }
        set_timer(&fall_rate_timer, 1000/((game_seconds/10)+1)); //increase fall rate every 10 kills
        start_timer(&fall_rate_timer);
        ret |= 1;
    }

    if(timer_elapsed(&spawn_rate_timer))
    {
        create_enemy();
        start_timer(&spawn_rate_timer);
        ret |= 1;
    }

    return ret;
}

// game is over when an enemy crosses the finish line
bool check_game_over()
{
    struct enemy *trv = enemy_list;
    while(trv)
    {
        if(trv->y >= finish_line)
            return true;

        trv = trv->next;
    }
    return false;
}

void display_score_modal()
{
    char* top = "┌─────────────────────────────┐";
    char* mid = "│                             │";
    char* bot = "└─────────────────────────────┘";

    int x = screen_width/2 - 31/2; 
    int y = screen_height/2 - 10;

    tb_print(x,y, TB_DEFAULT, TB_DEFAULT, top);
    tb_print(x,y+10, TB_DEFAULT, TB_DEFAULT, bot);
    for(int i = 1; i < 10; i++)
        tb_print(x,y+i, TB_DEFAULT, TB_DEFAULT, mid);

    // print some scores
    tb_print(x+2,y+1, TB_DEFAULT, TB_DEFAULT, "GAME OVER!");
    tb_printf(x+2,y+3, TB_DEFAULT, TB_DEFAULT, "Words Destroyed: %d", words_killed);
    tb_printf(x+2,y+4, TB_DEFAULT, TB_DEFAULT, "Time Spent: %d", game_seconds);
    tb_printf(x+2,y+6, TB_DEFAULT, TB_DEFAULT, "Score (wpm): %f", (double)words_killed/(double)game_seconds*60.0);

    tb_print(x+2,y+8, TB_DEFAULT, TB_DEFAULT, "Press any key to quit.");

    tb_present();

    struct tb_event ev;
    tb_poll_event(&ev);
}

int main()
{
    // get a list of words
    read_system_dictionary();

    // start the game
    init_game();

    //game loop
    set_timer(&game_timer, 1000);//set to 1 second
    set_timer(&fall_rate_timer, 1000/((game_seconds/10)+1)); //increase fall rate every 10 seconds
    set_timer(&spawn_rate_timer, 2000); //spawn every 2 seconds
    start_timer(&game_timer);
    start_timer(&fall_rate_timer);
    start_timer(&spawn_rate_timer);
                                        
    while(!game_finished)
    {
        //render first
        render_game();
        
        // check for event
        state_change = eval_input();

        //update game state
        state_change |= check_remove_word(usr_buf);

        //update game ticks and move enemies
        state_change |= update_game_time();

        game_finished |= check_game_over();

        SLEEP(10); // don't waste those CPU cycles
    }

    display_score_modal();

    tb_shutdown();
    //clean up
    free(word_list);
    if(enemy_list)
    {
        while(enemy_list->next)
        {
            struct enemy *prv = enemy_list;
            enemy_list = enemy_list->next;
            free(prv);
        }

        free(enemy_list);
        enemy_list = NULL;
    }
    return EXIT_SUCCESS;
}
