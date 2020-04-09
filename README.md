# ECE243-Project: Brick Breaker

This project is to make the game Brick Breaker that is able to run on the DE1-SOC board. To test and present this code, an emulator of the DE1-SOC board was used. 

This simulator is called cpulator found here: https://cpulator.01xz.net/?sys=arm-de1soc 

## Instructions

The goal of the game is to earn as many points as you can by breaking bricks. Bricks can be broken by deflecting a ball into the bricks using a paddle. As you play the game, powerups may fall from broken bricks which will give you an advantage for a small period of time. The game is won when all bricks are broken.

## Running The Game
Start by going to cpulator and changing the language from ARMv7 (default) to C (experimental). 

Click File>Open and open the BrickBreaker.c file.

Click the "compile and load" button or press f5.

Press "Continue" on the top bar.

Optionally increase the size of the VGA pixel buffer by clicking the triangle on the side.

Under PS/2 Keyboard or Mouse click in the field title "type here: "; after doing so, the program will be able to read your keyboard's input.

## Controls
Use the left and right arrow keys on your keyboard to control the paddle. The paddle will continue to move until you press the up or down arrows to stop the paddle.

To pause the game, press the return key on your keyboard.