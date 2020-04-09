# ECE243-Project: Brick Breaker

This project is to make the game Brick Breaker that is able to run on the DE1-SOC board. To test and present this code, an simulator of the DE1-SOC board was used. 

The simulator used is called cpulator found here: https://cpulator.01xz.net/?sys=arm-de1soc 

## Instructions

The goal of the game is to earn as many points as you can by breaking bricks. Bricks can be broken by deflecting a ball into the bricks using a paddle. As you play the game, powerups may fall from broken bricks which will increase the size of your paddle for a small period of time (you can stack multiple powerups at once). The game is won when all bricks are broken. You are given three lives, and if the ball misses your paddle you will be able to respawn but you will lose a life. If you miss the ball with no lives left you will lose.

### Running The Game
Start by going to cpulator and changing the language from ARMv7 (default) to C (experimental). 

Click File>Open and open the BrickBreaker.c file.

Click the "compile and load" button or press f5.

Press "Continue" on the top bar or press f3.

Optionally increase the size of the VGA pixel buffer by clicking the triangle on the side of the pixel buffer.

Under PS/2 Keyboard or Mouse, click in the field titled "type here: "; after doing so, the program will be able to read your keyboard's input. Clicking anywhere else on the screen will cause the game not to be able to read your inputs; to fix this click into the field again.

To restart the game, press the "Reload" button on the top of cpulator (don't forget to click into the "type here: " field again).

### Controls
Use the left and right arrow keys on your keyboard to control the paddle. The paddle will continue to move until you press the up or down arrows to stop the paddle.

To pause the game, press the return key on your keyboard.