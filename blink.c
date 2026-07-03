#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define HEIGHT 7
#define WIDTH 12

/*
    BLINK v0.1 - Step 1

    _ = player
    # = wall
    . = floor
    E = exit, visually representing []
*/

char room[HEIGHT][WIDTH + 1] = {
    "############",
    "#..........#",
    "#..........#",
    "#.....###..#",
    "#..........#",
    "#.........E#",
    "############"
};

int playerRow = 1;
int playerCol = 1;

void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void drawRoom(void) {
    clearScreen();

    printf("========================\n");
    printf("         BLINK\n");
    printf("========================\n\n");

    for (int row = 0; row < HEIGHT; row++) {
        for (int col = 0; col < WIDTH; col++) {
            if (row == playerRow && col == playerCol) {
                printf("_");
            } else if (room[row][col] == 'E') {
                printf("[]");
            } else {
                printf("%c", room[row][col]);
            }
        }
        printf("\n");
    }

    printf("\n");
    printf("Goal: reach []\n");
    printf("Controls: W A S D to move, Q to quit\n");
}

void movePlayer(char command) {
    int newRow = playerRow;
    int newCol = playerCol;

    if (command == 'w') {
        newRow--;
    } else if (command == 's') {
        newRow++;
    } else if (command == 'a') {
        newCol--;
    } else if (command == 'd') {
        newCol++;
    }

    if (room[newRow][newCol] != '#') {
        playerRow = newRow;
        playerCol = newCol;
    }
}

int main(void) {
    char input[20];
    int gameRunning = 1;

    while (gameRunning) {
        drawRoom();

        if (room[playerRow][playerCol] == 'E') {
            printf("\nYou escaped the room.\n");
            printf("BLINK.\n");
            break;
        }

        printf("\nCommand: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        char command = tolower(input[0]);

        if (command == 'q') {
            gameRunning = 0;
        } else {
            movePlayer(command);
        }
    }

    printf("\nGame closed.\n");
    return 0;
}
