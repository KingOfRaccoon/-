#include <89c51rd2.H>
#include<stdio.h>
#include<math.h>

//Variables
#define dt   P1 //data line LCD
#define buzzer P2_1
#define RS   P2_7 // mode comand/data LCD
#define RW   P2_6 // mode read/write LCD
#define EN   P2_5 // sync
#define startButton    P0_0 // reset
#define firstPlayerButton P0_1
#define plusModeButton P0_2
#define playPauseButton P0_3 // start/stop
#define minusModeButton P0_4
#define secondPlayerButton P0_5

typedef enum Button {
    Start,
    FirstPlayer,
    PlusMode,
    PlayPause,
    MinusMode,
    SecondPlayer
};

typedef struct {
    long time;     
    long increment;
} TimeMode;

idata TimeMode chess_time_modes[] = {
        {900,  10},   // 15 minutes = 900 seconds, 10 seconds increment
        {1800, 5},    // 30 minutes = 1800 seconds, 5 seconds increment
        {3600, 0},    // 60 minutes = 3600 seconds, 0 seconds increment
        {5400, 30},   // 90 minutes = 5400 seconds, 30 seconds increment
        {7200, 10},   // 120 minutes = 7200 seconds, 10 seconds increment
        {300,  3},    // 5 minutes = 300 seconds, 3 seconds increment
        {600,  2},    // 10 minutes = 600 seconds, 2 seconds increment
        {2400, 5},    // 40 minutes = 2400 seconds, 5 seconds increment
        {180,  2},    // 3 minutes = 180 seconds, 2 seconds increment
        {2700, 1}     // 45 minutes = 2700 seconds, 1 second increment
};

enum Button currentButton;
int gameMode;
int ms;
int sec;
int min;
int needUpdateDisplay; // flag for display update

long player1Time;
long player2Time;
int currentPlayer = 1; // 1 for Player 1, 2 for Player 2
int isPaused = -1; // -1 means not started, 0 for running, 1 for paused

// Delay function for LCD
void delay(long int time) {
    long int i;
    for (i = 1; i <= time; i++) { ; }
}

// Activates buzzer for 1 second
void activateBuzzer() {
    buzzer = 1;
    delay(10000);
    buzzer = 0;
}

// Sends command to LCD
void sendComand(int Comand) {
    RS = 0;
    RW = 0;
    EN = 1;
    dt = Comand;
    EN = 0;
    delay(2);
}

// Sends a string to LCD
void sendData(char s_data[], long int speed) {
    int i = 0;
    while (s_data[i] != 0) {
        RS = 1;
        RW = 0;
        EN = 1;
        dt = s_data[i];
        EN = 0;
        i++;
        delay(speed);
    }
}

// Displays game over message and activates buzzer
void displayGameOver(int loser) {
    sendComand(1); // Clear LCD
    sendComand(127); // Move cursor to center
    sendData(" Game Over", 1);
    sendComand(192); // Move to second line
    if (loser == 1) {
        sendData("Player 1 lost   ", 1);
    } else if (loser == 2) {
        sendData("Player 2 lost   ", 1);
    }
    activateBuzzer();
}

// Timer interrupt for countdown
void timeInterrupt(void) interrupt 5 using 2 {
    TF2 = 0; // Clear interrupt flag
    TH2 = 0xD8; // Reload for 10 ms
    TL2 = 0xEF;

    if (!isPaused) {
        needUpdateDisplay = 1; // Flag for updating display

        if (currentPlayer == 1) {
            if (player1Time > 0) {
                player1Time -= 10;
            } else {
                isPaused = 1; // Pause the game
                displayGameOver(1); // Player 1 lost
            }
        } else if (currentPlayer == 2) {
            if (player2Time > 0) {
                player2Time -= 10;
            } else {
                isPaused = 1; // Pause the game
                displayGameOver(2); // Player 2 lost
            }
        }
    }
}

// Sends an integer as characters to LCD
void sendInteger(int integer, long int speed) {
    RS = 1;
    RW = 0;
    EN = 1;
    dt = integer;
    EN = 0;
    delay(speed);
}

// Prints a number to LCD
void printNumber(int number) {
    if (number >= 0) {
        int divisor = 1;
        while (number / divisor >= 10) {
            divisor *= 10;
        }
        while (divisor > 0) {
            int digit = number / divisor;
            sendInteger(digit + 48, 1);
            number %= divisor;
            divisor /= 10;
        }
    }
}

// Initializes LCD
void lcd(void) {
    int initCommands[] = {12, 56, 6}, t;
    for (t = 0; t < 2; t++) sendComand(initCommands[t]);
}

// Displays the start message on LCD
void displayStartMessage(void) {
    sendComand(2);
    sendData(" Chess clock", 1);
}

// Displays time in HH:MM:SS format
void displayTime(long time) {
    long seconds = (time) % 60;
    long minutes = (time / 60) % 60;
    long hours = time / (60 * 60);

    if (hours > 0) {
        sendData(" ", 1);
        printNumber(hours);
        sendData(":", 1);
        if (minutes < 10) {
            sendData("0", 1);
        }
        printNumber(minutes);
    } else {
        if (minutes > 20) {
            sendData("00", 1);
            sendData(":", 1);
            printNumber(minutes);
        } else {
            if (minutes < 10) {
                sendData("0", 1);
            }
            printNumber(minutes);
            sendData(":", 1);
            if (seconds < 10) {
                sendData("0", 1);
            }
            printNumber(seconds);
        }
    }
}

// Updates the LCD display with player times
void displayLCD(void) {
    sendComand(128); // Top line
    displayTime(player1Time / 1000);
    sendData("  --  ", 1);
    displayTime(player2Time / 1000);
}

void printStartTime(int currentGameMode) {
	sendComand(1);
	sendComand(128); // Move to top line
	displayTime((chess_time_modes[currentGameMode].time + chess_time_modes[currentGameMode].increment));
	sendData("  ", 1);
	sendData("--", 1);
	sendData("  ", 1);
	displayTime((chess_time_modes[currentGameMode].time + chess_time_modes[currentGameMode].increment));
	sendComand(192); // Move to bottom line
	sendData("Set mode: ", 1);
	printNumber(currentGameMode);
}

// Handles button press to start the game
void startClick() {
    while (startButton) {
        min = 0;
        sec = 0;
        ms = 0;
        TR2 = 0;
        gameMode = 0;
        currentButton = Start;
        printStartTime(gameMode);
        while (startButton) {}
    }
}

// Cycles to the next time mode
void plusMode() {
    while (plusModeButton) {
        if (gameMode == ((int) (sizeof(chess_time_modes) / sizeof(chess_time_modes[0])) - 1)) {
            gameMode = 0;
        } else {
            gameMode++;
        }
        printStartTime(gameMode);
        while (plusModeButton) {}
    }
}

// Cycles to the previous time mode
void minusMode() {
    while (minusModeButton) {
        if (gameMode == 0) {
            gameMode = ((int) (sizeof(chess_time_modes) / sizeof(chess_time_modes[0])) - 1);
        } else {
            gameMode--;
        }
        printStartTime(gameMode);
        while (minusModeButton) {}
    }
}

// Toggles between play and pause
void togglePause() {
    if (playPauseButton) {
        if (isPaused == -1) {
            long timeAll = (chess_time_modes[gameMode].time + chess_time_modes[gameMode].increment) * 1000;
            player1Time = timeAll;
            player2Time = timeAll;
            isPaused = 0;
            TR2 = 1;
            sendComand(192);
            sendData("Set player: ", 1);
            printNumber(currentPlayer);
        } else {
            isPaused = !isPaused;
        }
        while (playPauseButton) {}
    }
}

// Switches the current player
void switchPlayer() {
    while (firstPlayerButton) {
        currentPlayer = 2;
        sendComand(192);
        sendData("Set player: ", 1);
        printNumber(currentPlayer);
        if (!isPaused) {
            player1Time += chess_time_modes[gameMode].increment * 1000;
        }
        while (firstPlayerButton) {}
    }
    while (secondPlayerButton && currentPlayer == 2) {
        currentPlayer = 1;
        sendComand(192);
        sendData("Set player: ", 1);
        printNumber(currentPlayer);
        if (!isPaused) {
            player2Time += chess_time_modes[gameMode].increment * 1000;
        }
        while (secondPlayerButton) {}
    }
}

void main(void) {
    lcd();
    sendComand(1);
    T2CON = 0x00; 
    T2MOD = 0x00;
    ET2 = 1;
    TR2 = 0;
    EA = 1;
    buzzer = 0;

    player1Time = 0;
    player2Time = 0;
    currentPlayer = 1;
    displayStartMessage();
    while (1) {
        switchPlayer();
        togglePause();
        while (needUpdateDisplay) {
            displayLCD();
            needUpdateDisplay = 0;
        }
        startClick();
        plusMode();
        minusMode();
    }
}