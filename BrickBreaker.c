#include <stdlib.h>
#include <stdbool.h>

#define ROWS 8
#define COLS 7

volatile int pixel_buffer_start; // global variable

typedef struct Player {
	int x;
	int y;
	int width;
	int height;
	int dx;
} Player;

typedef struct Ball {
	int x;
	int y;
	int width;
	int dx;
	int dy;
} Ball;

bool map1[ROWS][COLS] = {{1,1,1,1,1,1,1},
				  		 {1,1,0,1,0,1,1},
				  		 {1,1,0,1,0,1,1},
				  		 {1,1,1,1,1,1,1},
						 {1,0,1,1,1,0,1},
				  		 {1,0,0,0,0,0,1},
				  		 {1,1,0,0,0,1,1},
				  		 {1,1,1,1,1,1,1}};

short int rowColors[] = {0xF800, 0xFFE0, 0x07E0, 0x001F};

void draw_player(const Player *player, short int color);
void update_player(Player *player);
void draw_ball(const Ball *ball, short int color);
void update_ball(Ball *ball);
void draw_map(bool (*map)[COLS]);
void wait_for_vsync();
void clear_screen();
void plot_pixel(int x, int y, short int line_color);

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;

	// create player at the bottom of the screen
	Player player = {159, 235, 40, 9, -2};
	Player oldPlayer = player;	// old player used to erase from background
	
	Ball ball = {159, 200, 7, 1, 1};
	Ball oldBall = ball;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
	draw_map(map1);
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
	clear_screen();
	
	draw_map(map1);
	
    while (1)
    {
        // erase the old player
		draw_player(&oldPlayer, 0);
		draw_ball(&oldBall, 0);
		
		// update the player
		oldPlayer = player;
		update_player(&player);
		
		oldBall = ball;
		update_ball(&ball);

        // draw player
		draw_player(&player, 0xFFFF);
		draw_ball(&ball, 0xFFFF);

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

void draw_map(bool (*map)[COLS]) {
	for (int i = 0; i < ROWS; i++) {
		for (int j = 0; j < COLS; j++) {
			if (map[i][j]) {
				// draw the brick
				for (int x = 45*j+5; x < 45*j+45; x++) {
					for (int y = 15*i+5; y < 15*i+15; y++) {
						plot_pixel(x,y,rowColors[i/2]);
					}
				}
			}
		}
	}
}

void draw_ball(const Ball *ball, short int color) {
	for (int i = ball->x - ball->width/2; i < ball->x + ball->width/2; i++) {
		for (int j = ball->y - ball->width/2; j < ball->y + ball->width/2; j++) {
			plot_pixel(i, j, color);
		}
	}
}

void update_ball(Ball *ball) {
	ball->x = ball->x + ball->dx;
	ball->y = ball->y + ball->dy;
	if (ball->x - ball->width/2 < 0 || ball->x + ball->width/2 > 320) {
		ball->x = ball->x - 2*ball->dx;
		ball->dx = -1*ball->dx;
	}
	if (ball->y - ball->width/2 < 0 || ball->y + ball->width/2 > 240) {
		ball->y = ball->y - 2* ball->dy;
		ball->dy = -1*ball->dy;
	}
}

void draw_player(const Player *player, short int color) {
	for (int i = player->x - player->width/2; i < player->x + player->width/2; i++) {
		for (int j = player->y - player->height/2; j < player->y + player->height/2; j++) {
			plot_pixel(i, j, color);
		}
	}
}

void update_player(Player *player) {
	player->x = player->x + player->dx;
	if (player->x - player->width/2 < 0 || player->x + player->width/2 > 320) {
		player->x = player->x - player->dx;
	}
}

// code for waiting for the vertical sync
void wait_for_vsync() {
	volatile int *pixel_ctrl_ptr = 0xFF203020; // pixel controller
	register int status;
	
	*pixel_ctrl_ptr = 1; // start the synchronization process
	
	do {
		status = *(pixel_ctrl_ptr + 3);
	} while ((status & 0x01) != 0); // wait for s to become 0
}

// code for clearing the screen
void clear_screen() {
	for (int i = 0; i < 320; i++) {
		for (int j = 0; j < 240; j++) {
			plot_pixel(i, j, 0);
		}
	}
}

// code for plotting a point on the buffer
void plot_pixel(int x, int y, short int line_color) {
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}
