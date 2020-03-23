#include <stdlib.h>

// used a struct because it was cleaner than using several arrays
struct Box {
	short boxColor;
	short lineColor;
	int dx;
	int dy;
	int x;
	int y;
};

volatile int pixel_buffer_start; // global variable

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	
	// create a pool of colors for the boxes and lines to choose from
	short colors[10] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0xF81F, 0xFFE0, 0x08FF, 0x8BEF, 0xBEEF, 0xFEED};
	
	// initialize boxes
    struct Box boxes[8];
	struct Box oldBoxes[8];
    for (int i = 0; i < 8; i++) {
		boxes[i].x = rand()%318+1;
		boxes[i].y = rand()%238+1;
		boxes[i].dx = rand()%2*2-1;
		boxes[i].dy = rand()%2*2-1;
		boxes[i].boxColor = colors[rand()%10];
		boxes[i].lineColor = colors[rand()%10];
		oldBoxes[i] = boxes[i];
	}

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
	clear_screen();
	
    while (1)
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
		for (int i = 0; i < 8; i++) {
			draw_box(&oldBoxes[i], 0);
			draw_box_line(&oldBoxes[i], &oldBoxes[(i+1)%8], 0);
		}
		
		// code for updating the locations of boxes (not shown)
		for (int i = 0; i < 8; i++) {
			oldBoxes[i] = boxes[i];
			update_box(&boxes[i]);
		}
		
        // code for drawing the boxes and lines (not shown)
		for (int i = 0; i < 8; i++) {
			draw_box(&boxes[i], boxes[i].boxColor);
			draw_box_line(&boxes[i], &boxes[(i+1)%8], boxes[i].lineColor);
		}

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

// function to draw a box on the screen
void draw_box(const struct Box *box, short int color) {
	// draw 3 by 3 square centered at x,y
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			plot_pixel((*box).x + i, (*box).y + j, color);
		}
	}
}

// function to draw a line on the screen
void draw_box_line(const struct Box *box1, const struct Box *box2, short int color) {
	draw_line((*box1).x, (*box1).y, (*box2).x, (*box2).y, color);
}

// function for updating the box's location
void update_box(struct Box *box) {
	(*box).x = (*box).x + (*box).dx;
	(*box).y = (*box).y + (*box).dy;
	if ((*box).x == 319) {
		(*box).dx = -1;
	}
	if ((*box).x == 1) {
		(*box).dx = 1;
	}
	if ((*box).y == 239) {
		(*box).dy = -1;
	}
	if ((*box).y == 1) {
		(*box).dy = 1;
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

// Implementation of Bresenham's Algorithm
void draw_line(int x0, int y0, int x1, int y1, short int line_color) {
	int isSteep = abs(y1 - y0) > abs(x1 - x0);
	if (isSteep) {
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	if (x0 > x1) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}
	int deltaX = x1 - x0;
	int deltaY = abs(y1 - y0);
	int error = -1*(deltaX/2);
	int y = y0;
	int y_step = -1;
	if (y0 < y1) {
		y_step = 1;
	}
	for (int x = x0; x <= x1; x++) {
		if (isSteep) {
			plot_pixel(y, x, line_color);
		} else {
			plot_pixel(x, y, line_color);
		}
		error = error + deltaY;
		if (error >= 0) {
			y = y + y_step;
			error = error - deltaX;
		}
	}
}

// Function to find the absolute value of a number
int abs(int num) {
	if (num < 0) {
		return -1*num;
	}
	return num;
}

// function to swap two integers
void swap(int *a, int *b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}
