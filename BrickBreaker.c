#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
	int lives;
} Ball;

bool map1[ROWS][COLS] = {{1,1,1,1,1,1,1},
				  		 {1,1,0,1,0,1,1},
				  		 {1,1,0,1,0,1,1},
				  		 {1,1,1,1,1,1,1},
						 {1,0,1,1,1,0,1},
				  		 {1,0,0,0,0,0,1},
				  		 {1,1,0,0,0,1,1},
				  		 {1,1,0,1,1,1,1}};

short int rowColors[] = {0xF800, 0xFFE0, 0x07E0, 0x001F};

bool brickBroke = false;
int brokenBrick[2] = {0, 0};

void draw_player(const Player *player, short int color);
void update_player(Player *player, unsigned char byte);
void draw_ball(const Ball *ball, short int color);
void update_ball(Ball *ball, const Player *player, bool (*map)[COLS]);
void draw_map(bool (*map)[COLS]);
bool check_for_brick_collision(int x, int y, bool (*map)[COLS]);
bool check_for_player_collision(int x, int y, const Player *player);
void break_brick(int row, int col, bool (*map)[COLS]);
void wait_for_vsync();
void clear_screen();
void plot_pixel(int x, int y, short int line_color);
short int get_pixel(int x, int y);
void draw_string(int x, int y, char str[]);
void draw_char(int x, int y, char letter);
void reset_ball(Ball *ball);
void read_keyboard(unsigned char *pressedKey);
void draw_level(volatile int *pixel_ctrl_ptr);
void clear_all_text();

typedef enum {
	title,
	level,
	pause,
	gameover
}  gamestate;

gamestate state;

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	
	// create player at the bottom of the screen
	Player player = {159, 235, 40, 9, -1};
	Player oldPlayer = player;	// old player used to erase from background
	
	Ball ball = {159, 200, 7, 1, 3, 3};
	Ball oldBall = ball;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();

    pixel_buffer_start = *pixel_ctrl_ptr;
	clear_screen();
    *(pixel_ctrl_ptr + 1) = 0xC8000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
	clear_screen();

	state = title;

	clear_all_text();
	draw_string(25, 30, "Press the space bar to begin!");
	

	unsigned char pressedKey = 0;
	
    while (1)
    {
		switch (state) {
			case title:
				read_keyboard(&pressedKey);
				if (pressedKey == 0x29) {
					draw_level(pixel_ctrl_ptr);
					state = level;
				}
				break;
			case level:
				// erase the old player
				read_keyboard(&pressedKey);
				draw_player(&oldPlayer, 0);
				draw_ball(&oldBall, 0);

				// if a brick was broken in the last frame, break it in this frame as well
				if (brickBroke) {
					break_brick(brokenBrick[0], brokenBrick[1], map1);
					brickBroke = false;
				}
				
				// update the player
				oldPlayer = player;
				update_player(&player, pressedKey);
				
				oldBall = ball;
				update_ball(&ball, &player, map1);

				// draw player
				draw_player(&player, 0xFFDF);
				draw_ball(&ball, 0xFFFF);

				wait_for_vsync(); // swap front and back buffers on VGA vertical sync
				pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				if (pressedKey == 0x5A) {
					draw_string(37, 30, "paused");
					draw_string(28, 32, "press space to continue");
					state = pause;
				}
				break;
			case pause:
				read_keyboard(&pressedKey);
				if (pressedKey == 0x29) {
					state = level;
					draw_string(37, 30, "      ");
					draw_string(28, 32, "                       ");
				}
				break;
			case gameover:
				clear_screen();
				draw_string(2,2,"                                                                            ");
				wait_for_vsync(); // swap front and back buffers on VGA vertical sync
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				break;
		}
    }
}

void draw_level(volatile int *pixel_ctrl_ptr) {
	draw_string(25, 30, "                             ");
	draw_string(2,2,"Score: You suck             Lives: you have none    High score: Hah you wish");
	draw_map(map1);
	wait_for_vsync();
	pixel_buffer_start = *(pixel_ctrl_ptr + 1);
	draw_map(map1);
	wait_for_vsync();
	pixel_buffer_start = *(pixel_ctrl_ptr + 1);
}

void read_keyboard(unsigned char *pressedKey) {
	volatile int * PS2_ptr = (int *) 0xFF200100;
	int data = *PS2_ptr;
	*pressedKey = data & 0xFF;

	while (data & 0x8000) {
		data = *PS2_ptr;
	}
}

void draw_map(bool (*map)[COLS]) {
	clear_screen();
	for (int i = 0; i < ROWS; i++) {
		for (int j = 0; j < COLS; j++) {
			if (map[i][j]) {
				// draw the brick
				for (int x = 45*j+5; x < 45*j+45; x++) {
					for (int y = 15*(i+1)+5; y < 15*(i+1)+15; y++) {
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

void update_ball(Ball *ball, const Player *player, bool (*map)[COLS]) {
	ball->x = ball->x + ball->dx;
	ball->y = ball->y + ball->dy;
	
	// Check for wall collisions
	if (ball->y + ball->width/2 >= 240) {
		if (ball->lives == 0) {
			state = gameover;
			draw_string(35, 30, "Game Over!");
			return;
		}
		ball->lives = ball->lives - 1;
		reset_ball(ball);
		return;
	}
	if (ball->x - ball->width/2 < 0 || ball->x + ball->width/2 >= 320) {
		ball->x = ball->x - 2*ball->dx;
		ball->dx = -1*ball->dx;
		return;
	} else if (ball->y - ball->width/2 < 15) {
		ball->y = ball->y - 2* ball->dy;
		ball->dy = -1*ball->dy;
		return;
	}

	// Check for player collisions
	if (check_for_player_collision(ball->x - ball->width/2, ball->y + ball->width/2, player) ||
		check_for_player_collision(ball->x + ball->width/2, ball->y + ball->width/2, player)) {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
			if (player->dx != 0) {
				if (player->dx * ball->dx < 0) {
					ball->dx = -1*ball->dx;
				}
			}
			return;
	}

	//Check each corner for brick collisions
	if (check_for_brick_collision(ball->x - ball->width/2, ball->y - ball->width/2, map)) {
		if (check_for_brick_collision(ball->x - ball->width/2, ball->y - ball->width/2 - ball->dy, map)) {
			ball->x = ball->x - 2*ball->dx;
			ball->dx = -1*ball->dx;
		} else {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
		}
		return;
	}
	if (check_for_brick_collision(ball->x + ball->width/2, ball->y - ball->width/2, map)) {
		if (check_for_brick_collision(ball->x + ball->width/2, ball->y - ball->width/2 - ball->dy, map)) {
			ball->x = ball->x - 2*ball->dx;
			ball->dx = -1*ball->dx;
		} else {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
		}
		return;
	}
	if (check_for_brick_collision(ball->x - ball->width/2, ball->y + ball->width/2, map)) {
		if (check_for_brick_collision(ball->x - ball->width/2, ball->y + ball->width/2 - ball->dy, map)) {
			ball->x = ball->x - 2*ball->dx;
			ball->dx = -1*ball->dx;
		} else {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
		}
		return;
	}
	if (check_for_brick_collision(ball->x + ball->width/2, ball->y + ball->width/2, map)) {
		if (check_for_brick_collision(ball->x + ball->width/2, ball->y + ball->width/2 - ball->dy, map)) {
			ball->x = ball->x - 2*ball->dx;
			ball->dx = -1*ball->dx;
		} else {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
		}
		return;
	}
}

bool check_for_player_collision(int x, int y, const Player *player) {
	if (get_pixel(x,y) == (short int)0xFFDF) {
		return true;
	}
	return false;
}

bool check_for_brick_collision(int x, int y, bool (*map)[COLS]) {
	short int collidingPixel = get_pixel(x,y);
	if (collidingPixel != (short int)0xFFFF && collidingPixel != (short int)0x0) {
		int rowIndex = y/15 - 1;
		int colIndex = x/45;
		break_brick(rowIndex, colIndex, map);
		brickBroke = true;
		brokenBrick[0] = rowIndex;
		brokenBrick[1] = colIndex;
		return true;
	}
	return false;
}

void break_brick(int row, int col, bool (*map)[COLS]) {
	map[row][col] = 0;
	for (int x = 45*col+5; x < 45*col+45; x++) {
		for (int y = 15*(row+1)+5; y < 15*(row+1)+15; y++) {
			plot_pixel(x,y,0);
		}
	}
}

void draw_player(const Player *player, short int color) {
	for (int i = player->x - player->width/2; i < player->x + player->width/2; i++) {
		for (int j = player->y - player->height/2; j < player->y + player->height/2; j++) {
			plot_pixel(i, j, color);
		}
	}
}

void update_player(Player *player, unsigned char byte) {
	if (byte == 0x74) {
		player->dx = 4;
	} else if (byte == 0x6B) {
		player->dx = -4;
	} else {
		player->dx = 0;
	}
	player->x = player->x + player->dx;
	if (player->x - player->width/2 < 0 || player->x + player->width/2 > 320) {
		player->x = player->x - player->dx;
		player->dx = 0;
	}
}

// code for waiting for the vertical sync
void wait_for_vsync() {
	volatile int *pixel_ctrl_ptr = (int*)0xFF203020; // pixel controller
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

short int get_pixel(int x, int y) {
	return *(short int *)(*(int *)0xFF203020 + (y << 10) + (x << 1));
}

void draw_string(int x, int y, char str[]) {
	for (int i = 0; i < strlen(str); i++) {
		draw_char(x+i, y, str[i]);
	}
}

void draw_char(int x, int y, char letter) {
	volatile int charBuffer = 0xc9000000;
	*(char *)(charBuffer + (y << 7) + x) = letter;
}

void clear_all_text() {
	for (int x = 0; x < 80; x++) {
		for (int y = 0; y < 60; y++) {
			draw_char(x, y, ' ');
		}
	}
}

void reset_ball(Ball *ball) {
	ball->x = 159;
	ball->y = 150;
	ball->dx = 1;
	ball->dy = 3;
}
