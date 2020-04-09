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
	bool powerUp;
} Player;

typedef struct Ball {
	int x;
	int y;
	int width;
	int dx;
	int dy;
	int lives;
    int score;
	short int last_touched;
	int bricks_broken;
} Ball;

typedef struct PowerUp {
	int x;
	int y;
	int width;
	bool active;
} PowerUp;

bool map1[ROWS][COLS] = {{1,1,1,1,1,1,1},
				  		 {1,1,0,1,0,1,1},
				  		 {1,1,0,1,0,1,1},
				  		 {1,1,1,1,1,1,1},
						 {1,0,1,1,1,0,1},
				  		 {1,0,0,0,0,0,1},
				  		 {1,1,0,0,0,1,1},
				  		 {1,1,0,1,1,1,1}};

short int rowColors[] = {0xF800, 0xFFE0, 0x07E0, 0x001F};

int numPowerUps = 0;
int increase = 10;

bool brickBroke = false;
int brokenBrick[2] = {0, 0};

void draw_player(const Player *player, short int color);
void update_player(Player *player, unsigned char byte);
void draw_ball(const Ball *ball, short int color);
void update_ball(Ball *ball, const Player *player, bool (*map)[COLS]);
void draw_map(bool (*map)[COLS]);
bool check_for_brick_collision(int x, int y, Ball* ball, bool (*map)[COLS]);
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
void write_status(int score, int lives);
int score_add(short int colour);
void update_power_up(PowerUp *powerUp, Player *player);
void draw_power_ups(PowerUp powerUps[5], short int colour, int number);
bool all_bricks_broken(bool (*map)[COLS]);
void activate_power_up(Player *player);
void remove_power_up(Player *player);

typedef enum {
	title,
	level,
	pause,
	winner,
	gameover
}  gamestate;

gamestate state;

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	volatile int *interrupt = (int *)0xFFFEC60C;
	
	// create player at the bottom of the screen
	Player player = {159, 235, 40, 9, -1, false};
	Player oldPlayer = player;	// old player used to erase from background
	
	Ball ball = {159, 200, 7, 1, 3, 3, 0, 0xFFFF, 0};
	Ball oldBall = ball;

	PowerUp powerUps[5] = {{0,0,0,0,false}, {0,0,0,0,false}, {0,0,0,0,false}, {0,0,0,0,false}, {0,0,0,0,false}};
	PowerUp oldPowerUps[5];
	int oldNumber = 0;

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
				if(oldNumber > 0) {
					draw_power_ups(oldPowerUps, 0, oldNumber);
				}
				draw_ball(&oldBall, 0);


				// if a brick was broken in the last frame, break it in this frame as well
				if (brickBroke) {
					break_brick(brokenBrick[0], brokenBrick[1], map1);
					ball.score += score_add(ball.last_touched);
					brickBroke = false;
					if (all_bricks_broken(map1)) {
						state = winner;
						break;
					}
				}

				if (ball.bricks_broken == 5) {
					powerUps[numPowerUps].active = true;
					powerUps[numPowerUps].x = ball.x;
					powerUps[numPowerUps].y = ball.y;
					powerUps[numPowerUps].width = 7;
					numPowerUps++;
					ball.bricks_broken = ball.bricks_broken-5;
				}

				// update the player
				oldPlayer = player;
				if(player.powerUp == true) {
					if(*interrupt == 1) {
						remove_power_up(&player);
					}
				}
				update_player(&player, pressedKey);
				
				oldBall = ball;
				update_ball(&ball, &player, map1);

				oldNumber = numPowerUps;
				for (int i = 0; i < oldNumber; i++) {
					oldPowerUps[i] = powerUps[i];
					update_power_up(&powerUps[i], &player);
					if (powerUps[i].active == false) {
						draw_power_ups(&powerUps[i], 0x0000, 1);
						numPowerUps--;
					}
				}

				// draw player
				draw_player(&player, 0xFFDF);

				draw_power_ups(powerUps, 0xF81F, numPowerUps);

				draw_ball(&ball, 0xFFFF);

                write_status(ball.score, ball.lives);

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
			case winner:
				clear_screen();
				draw_string(2,2,"                                                                            ");
				draw_string(35, 30, "Winner!");
                wait_for_vsync(); // swap front and back buffers on VGA vertical sync
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				break;
			case gameover:
				clear_screen();
				draw_string(2,2,"                                                                            ");
				draw_string(35, 30, "Game Over!");
                wait_for_vsync(); // swap front and back buffers on VGA vertical sync
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				break;
		}
    }
}

void draw_level(volatile int *pixel_ctrl_ptr) {
	draw_string(25, 30, "                             ");
	draw_string(2,2,"                                                                            ");
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
	if (check_for_brick_collision(ball->x - ball->width/2, ball->y - ball->width/2, ball, map)) {
		if (check_for_brick_collision(ball->x - ball->width/2, ball->y - ball->width/2 - ball->dy, ball, map)) {
			ball->x = ball->x - 2*ball->dx;
			ball->dx = -1*ball->dx;
		} else {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
		}
		return;
	}
	if (check_for_brick_collision(ball->x + ball->width/2, ball->y - ball->width/2, ball, map)) {
		if (check_for_brick_collision(ball->x + ball->width/2, ball->y - ball->width/2 - ball->dy, ball, map)) {
			ball->x = ball->x - 2*ball->dx;
			ball->dx = -1*ball->dx;
		} else {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
		}
		return;
	}
	if (check_for_brick_collision(ball->x - ball->width/2, ball->y + ball->width/2, ball, map)) {
		if (check_for_brick_collision(ball->x - ball->width/2, ball->y + ball->width/2 - ball->dy, ball, map)) {
			ball->x = ball->x - 2*ball->dx;
			ball->dx = -1*ball->dx;
		} else {
			ball->y = ball->y - 2* ball->dy;
			ball->dy = -1*ball->dy;
		}
		return;
	}
	if (check_for_brick_collision(ball->x + ball->width/2, ball->y + ball->width/2, ball, map)) {
		if (check_for_brick_collision(ball->x + ball->width/2, ball->y + ball->width/2 - ball->dy, ball, map)) {
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

bool check_for_brick_collision(int x, int y, Ball *ball, bool (*map)[COLS]) {
	short int collidingPixel = get_pixel(x,y);
	if (collidingPixel != (short int)0xFFFF && collidingPixel != (short int)0x0 && collidingPixel != (short int)0xF81F) {
		int rowIndex = y/15 - 1;
		int colIndex = x/45;
		break_brick(rowIndex, colIndex, map);
		brickBroke = true;
		brokenBrick[0] = rowIndex;
		brokenBrick[1] = colIndex;
		ball->last_touched = collidingPixel;
		ball->bricks_broken += 1;
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

void write_status(int score, int lives) {
    char scoreString[6];
    char scoreTitle[8] = "Score: ";
    itoa(score, scoreString, 10);
    strcat(scoreTitle, scoreString);
    
    char livesString[2];
    char livesTitle[8] = "Lives: ";
    itoa(lives, livesString, 10);
    strcat(livesTitle, livesString);  

    draw_string(2,2,"                                                                            ");

    draw_string(2, 2, scoreTitle);
	draw_string(34, 2, "Brick Breaker");
	draw_string(70, 2, livesTitle);
}

int score_add(short int colour) {
	if(colour == rowColors[0]) {
		return 100;
	} else if(colour == rowColors[1]) {
		return 50;
	} else if(colour == rowColors[2]) {
		return 20;
	} else if(colour == rowColors[3]) {
		return 10;
	} else {
		return 0;
	}
}

void update_power_up(PowerUp *powerUp, Player *player) {
	if (check_for_player_collision(powerUp->x - powerUp->width/2, powerUp->y + powerUp->width/2, player) ||
		check_for_player_collision(powerUp->x + powerUp->width/2, powerUp->y + powerUp->width/2, player)) {
			powerUp->active = false;
			activate_power_up(player);
	}else if (powerUp->y + powerUp->width/2 > 239) {
		powerUp->active = false;
	} else {
		powerUp->y = powerUp->y + 1;
	}
}

void draw_power_ups(PowerUp powerUps[5], short int colour, int number) {
	for (int i = 0; i < number; i++) {	
		for (int j = powerUps[i].x - powerUps[i].width/2; j < powerUps[i].x + powerUps[i].width/2; j++) {
			for (int k = powerUps[i].y - powerUps[i].width/2; k < powerUps[i].y + powerUps[i].width/2; k++) {
				plot_pixel(j, k, colour);
			}
		}
	}
}

bool all_bricks_broken(bool (*map)[COLS]) {
	for (int i = 0; i < ROWS; i++) {
		for (int j = 0; j < COLS; j++) {
			if (map[i][j]) {
				return false;
			}
		}
	}
	return true;
}

void activate_power_up(Player *player) {
	volatile int *timer_addr = (int *)0xFFFEC600;
	volatile int *interrupt = (int *)0xFFFEC60C;
	volatile int *enable = (int *)0xFFFEC608;
	int timer_time = 0x77359400;

	if(player->x - (player->width + increase)/2 < 0) {
		player->x = (player->width + increase)/2;
		player->dx = 0;
	} else if (player->x + (player->width + increase)/2 > 319) {
		player->x = 319 - (player->width + increase)/2;
		player->dx = 0;
	}
	player->width += increase;

	*timer_addr = timer_time;
	
	*interrupt = 0x1;
	*enable = 0x1;

	player->powerUp = true;
}

void remove_power_up(Player *player) {
	draw_player(player, 0);
	player->width -= increase;
	player->powerUp = false;
}
