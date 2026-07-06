#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define HEIGHT 9
#define WIDTH 20
#define MAX_GUARDS 4
#define LEVEL_COUNT 3
#define MAX_SIGNAL 5

/*
    BLINK v0.1 - Step 9

    _ = player / cursor
    # = wall
    . = floor
    * = Signal pickup
    : = bullet
    E = exit, visually representing []
    ^ > < v = guards and their line of sight

    Controls:
    W A S D = move
    .       = wait
    SPACE   = hold blink while hidden
    F       = shoot in last movement direction
    Q       = quit
*/

typedef struct {
    int row;
    int col;
    char direction;
} Guard;

typedef struct {
    char layout[HEIGHT][WIDTH + 1];
    int startRow;
    int startCol;
    int guardCount;
    Guard startingGuards[MAX_GUARDS];
    char introMessage[120];
} Level;

Level levels[LEVEL_COUNT] = {
    {
        {
            "####################",
            "#..........*.......#",
            "#..####......####..#",
            "#..................#",
            "#......######......#",
            "#....*.............#",
            "#..####......####..#",
            "#...............E..#",
            "####################"
        },
        1,
        1,
        3,
        {
            {1, 10, 'v'},
            {3, 4, '>'},
            {5, 15, '<'},
            {0, 0, '>'}
        },
        "Room 1. The terminal notices your shape."
    },
    {
        {
            "####################",
            "#..*...............#",
            "#..####..######....#",
            "#.............#....#",
            "######..####..#....#",
            "#.............#....#",
            "#....######..####..#",
            "#...............E..#",
            "####################"
        },
        7,
        1,
        3,
        {
            {1, 15, '<'},
            {3, 8, '>'},
            {5, 12, '^'},
            {0, 0, '>'}
        },
        "Room 2. The patrols are learning your rhythm."
    },
    {
        {
            "####################",
            "#......#...........#",
            "#..*...#..#######..#",
            "#......#...........#",
            "#..########..#######",
            "#..................#",
            "####..######..####.#",
            "#..............*E..#",
            "####################"
        },
        1,
        1,
        4,
        {
            {1, 10, 'v'},
            {3, 16, '<'},
            {5, 4, '>'},
            {7, 12, '^'}
        },
        "Room 3. The exit is watching back."
    }
};

char room[HEIGHT][WIDTH + 1];
Guard guards[MAX_GUARDS];

int currentLevel = 0;
int guardCount = 0;

int playerRow = 1;
int playerCol = 1;

int isVisible = 1;
int signalPower = 3;
int holdBlink = 0;
int playerCaught = 0;

int bulletActive = 0;
int bulletRow = 0;
int bulletCol = 0;
char bulletDirection = '>';
char lastDirection = '>';

char message[120] = "The cursor blinks into existence.";

void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int showTitleScreen(void) {
    char input[20];

    while (1) {
        clearScreen();

        printf("========================\n");
        printf("         BLINK\n");
        printf("========================\n\n");

        printf("You are the cursor.\n\n");
        printf("When you appear,\n");
        printf("they can see you.\n\n");
        printf("When you vanish,\n");
        printf("you can move.\n\n");

        printf("Reach [] without being seen.\n\n");

        printf("Controls:\n");
        printf("W A S D  - move\n");
        printf(".        - wait\n");
        printf("SPACE    - hold blink while hidden\n");
        printf("F        - shoot in your last direction\n");
        printf("Q        - quit\n\n");

        printf("Press ENTER to start.\n");
        printf("Press Q and ENTER to quit.\n\n");

        printf("Command: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }

        char command = tolower(input[0]);

        if (command == 'q') {
            return 0;
        }

        if (command == '\n') {
            return 1;
        }
    }
}

int askRestart(void) {
    char input[20];

    while (1) {
        printf("\nPress R to restart or Q to quit.\n");
        printf("Command: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }

        char command = tolower(input[0]);

        if (command == 'r') {
            return 1;
        }

        if (command == 'q') {
            return 0;
        }
    }
}

void getDirection(char direction, int *rowDirection, int *colDirection) {
    *rowDirection = 0;
    *colDirection = 0;

    if (direction == '^') {
        *rowDirection = -1;
    } else if (direction == 'v') {
        *rowDirection = 1;
    } else if (direction == '<') {
        *colDirection = -1;
    } else if (direction == '>') {
        *colDirection = 1;
    }
}

void loadLevel(int levelIndex) {
    currentLevel = levelIndex;

    for (int row = 0; row < HEIGHT; row++) {
        strcpy(room[row], levels[levelIndex].layout[row]);
    }

    playerRow = levels[levelIndex].startRow;
    playerCol = levels[levelIndex].startCol;

    guardCount = levels[levelIndex].guardCount;
    for (int i = 0; i < guardCount; i++) {
        guards[i] = levels[levelIndex].startingGuards[i];
    }

    isVisible = 1;
    holdBlink = 0;
    playerCaught = 0;
    bulletActive = 0;
    lastDirection = '>';
    strcpy(message, levels[levelIndex].introMessage);
}

void resetGame(void) {
    signalPower = 3;
    loadLevel(0);
}

int guardAt(int row, int col) {
    for (int i = 0; i < guardCount; i++) {
        if (guards[i].row == row && guards[i].col == col) {
            return 1;
        }
    }

    return 0;
}

char guardSymbolAt(int row, int col) {
    for (int i = 0; i < guardCount; i++) {
        if (guards[i].row == row && guards[i].col == col) {
            return guards[i].direction;
        }
    }

    return '\0';
}

int bulletAt(int row, int col) {
    return bulletActive && bulletRow == row && bulletCol == col;
}

char reverseDirection(char direction) {
    if (direction == '^') {
        return 'v';
    } else if (direction == 'v') {
        return '^';
    } else if (direction == '<') {
        return '>';
    } else if (direction == '>') {
        return '<';
    }

    return direction;
}

void removeGuardByIndex(int index) {
    for (int i = index; i < guardCount - 1; i++) {
        guards[i] = guards[i + 1];
    }

    guardCount--;
}

int removeGuardAt(int row, int col) {
    for (int i = 0; i < guardCount; i++) {
        if (guards[i].row == row && guards[i].col == col) {
            removeGuardByIndex(i);
            return 1;
        }
    }

    return 0;
}

void drawRoom(void) {
    clearScreen();

    printf("========================\n");
    printf("         BLINK\n");
    printf("========================\n\n");

    printf("Room:       %d/%d\n", currentLevel + 1, LEVEL_COUNT);
    printf("State:      %s\n", isVisible ? "VISIBLE" : "HIDDEN");
    printf("Signal:     %d/%d\n", signalPower, MAX_SIGNAL);
    printf("Hold Blink: %s\n", holdBlink ? "ACTIVE" : "OFF");
    printf("Facing:     %c\n\n", lastDirection);

    for (int row = 0; row < HEIGHT; row++) {
        for (int col = 0; col < WIDTH; col++) {
            if (row == playerRow && col == playerCol) {
                if (isVisible) {
                    printf("_");
                } else {
                    printf(" ");
                }
            } else if (bulletAt(row, col)) {
                printf(":");
            } else if (guardAt(row, col)) {
                printf("%c", guardSymbolAt(row, col));
            } else if (room[row][col] == 'E') {
                printf("[]");
            } else {
                printf("%c", room[row][col]);
            }
        }
        printf("\n");
    }

    printf("\n");
    printf("Goal: reach [] without being seen.\n");
    printf("Controls: W A S D to move, . to wait, SPACE to hold blink, F to shoot, Q to quit\n");
    printf("Message: %s\n", message);
}

void collectSignalIfNeeded(void) {
    if (room[playerRow][playerCol] == '*') {
        if (signalPower < MAX_SIGNAL) {
            signalPower++;
            strcpy(message, "Signal restored. The cursor burns brighter.");
        } else {
            strcpy(message, "Signal is already full.");
        }

        room[playerRow][playerCol] = '.';
    }
}

int movePlayer(char command) {
    int newRow = playerRow;
    int newCol = playerCol;

    if (command == 'w') {
        newRow--;
        lastDirection = '^';
    } else if (command == 's') {
        newRow++;
        lastDirection = 'v';
    } else if (command == 'a') {
        newCol--;
        lastDirection = '<';
    } else if (command == 'd') {
        newCol++;
        lastDirection = '>';
    }

    if (room[newRow][newCol] != '#' && !guardAt(newRow, newCol)) {
        playerRow = newRow;
        playerCol = newCol;
        return 1;
    }

    return 0;
}

void updateBlinkAfterTurn(void) {
    if (holdBlink) {
        isVisible = 0;
        holdBlink = 0;
    } else {
        isVisible = !isVisible;
    }
}

void fireBullet(void) {
    if (bulletActive) {
        strcpy(message, "Only one shot can exist at a time.");
        return;
    }

    bulletActive = 1;
    bulletRow = playerRow;
    bulletCol = playerCol;
    bulletDirection = lastDirection;

    strcpy(message, "You fire a thin pulse of signal.");
}

void moveBullet(void) {
    if (!bulletActive) {
        return;
    }

    int rowDirection;
    int colDirection;

    getDirection(bulletDirection, &rowDirection, &colDirection);

    int newRow = bulletRow + rowDirection;
    int newCol = bulletCol + colDirection;

    if (room[newRow][newCol] == '#') {
        bulletActive = 0;
        strcpy(message, "The shot breaks against a wall.");
        return;
    }

    bulletRow = newRow;
    bulletCol = newCol;

    if (removeGuardAt(bulletRow, bulletCol)) {
        bulletActive = 0;
        strcpy(message, "The shot deletes a guard.");
    }
}

int guardSeesPlayer(void) {
    if (!isVisible) {
        return 0;
    }

    for (int i = 0; i < guardCount; i++) {
        int rowDirection;
        int colDirection;

        getDirection(guards[i].direction, &rowDirection, &colDirection);

        int visionRow = guards[i].row + rowDirection;
        int visionCol = guards[i].col + colDirection;

        while (room[visionRow][visionCol] != '#') {
            if (visionRow == playerRow && visionCol == playerCol) {
                return 1;
            }

            visionRow += rowDirection;
            visionCol += colDirection;
        }
    }

    return 0;
}

void moveGuards(void) {
    for (int i = 0; i < guardCount; i++) {
        int rowDirection;
        int colDirection;

        getDirection(guards[i].direction, &rowDirection, &colDirection);

        int newRow = guards[i].row + rowDirection;
        int newCol = guards[i].col + colDirection;

        if (newRow == playerRow && newCol == playerCol) {
            playerCaught = 1;
            return;
        }

        if (bulletActive && newRow == bulletRow && newCol == bulletCol) {
            bulletActive = 0;
            removeGuardByIndex(i);
            i--;
            strcpy(message, "A guard walks into your shot.");
            continue;
        }

        if (room[newRow][newCol] == '#' ||
            room[newRow][newCol] == 'E' ||
            room[newRow][newCol] == '*' ||
            guardAt(newRow, newCol)) {
            guards[i].direction = reverseDirection(guards[i].direction);
        } else {
            guards[i].row = newRow;
            guards[i].col = newCol;
        }
    }
}

void advanceWorldAfterAction(void) {
    updateBlinkAfterTurn();
    moveBullet();
    moveGuards();
}

int reachedExit(void) {
    return room[playerRow][playerCol] == 'E';
}

int advanceLevelIfPossible(void) {
    if (!reachedExit()) {
        return 0;
    }

    if (currentLevel == LEVEL_COUNT - 1) {
        return 0;
    }

    loadLevel(currentLevel + 1);
    return 1;
}

int playGame(void) {
    char input[20];
    int gameRunning = 1;

    resetGame();

    while (gameRunning) {
        drawRoom();

        if (reachedExit()) {
            if (advanceLevelIfPossible()) {
                continue;
            }

            printf("\nYou reached the final [].\n");
            printf("The terminal loses your signal.\n");
            printf("You escaped.\n");
            printf("BLINK.\n");

            return 1;
        }

        if (playerCaught || guardSeesPlayer()) {
            printf("\nYou blinked into sight.\n");
            printf("A guard saw you.\n");
            printf("GAME OVER.\n");

            return 0;
        }

        printf("\nCommand: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }

        char command = tolower(input[0]);

        if (command == 'q') {
            gameRunning = 0;
        } else if (command == ' ') {
            if (!isVisible && signalPower > 0) {
                signalPower--;
                holdBlink = 1;
                strcpy(message, "You hold the blink. Your next move stays hidden.");
            } else if (isVisible) {
                strcpy(message, "You can only hold blink while hidden.");
            } else {
                strcpy(message, "No Signal left.");
            }
        } else if (command == 'f') {
            fireBullet();
            advanceWorldAfterAction();
        } else if (command == '.' || command == '\n') {
            strcpy(message, "You wait. The terminal breathes.");
            advanceWorldAfterAction();
        } else if (command == 'w' || command == 'a' || command == 's' || command == 'd') {
            int moved = movePlayer(command);

            if (moved) {
                strcpy(message, "You move through the room.");
                collectSignalIfNeeded();
            } else {
                strcpy(message, "A wall blocks your movement.");
            }

            advanceWorldAfterAction();
        } else {
            strcpy(message, "Unknown command.");
        }
    }

    return 0;
}

int main(void) {
    if (!showTitleScreen()) {
        printf("\nGame closed.\n");
        return 0;
    }

    while (1) {
        playGame();

        if (!askRestart()) {
            break;
        }
    }

    printf("\nGame closed.\n");
    return 0;
}