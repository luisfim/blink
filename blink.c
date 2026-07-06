#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#define HEIGHT 9
#define WIDTH 20
#define MAX_GUARDS 4
#define LEVEL_COUNT 3
#define MAX_SIGNAL 5
#define GUARD_AMMO 3
#define MAX_ENEMY_BULLETS 12
#define MAX_PLAYER_BULLETS 2
#define HAT_COUNT 3
#define SAVE_FILE "blink_saves.txt"
#define PLAYER_NAME_MAX_CHARS 6
#define PLAYER_NAME_STORAGE 16
#define MAX_PROFILES 100
#define LAURA_HEART_NAME "LAURA♥"
#define LEADERBOARD_NAME_WIDTH 6
#define HEART_SYMBOL "♥"

#define BLINK_DURATION_MS 650
#define PLAYER_MOVE_DELAY_MS 110
#define MOVE_POSE_DURATION_MS 160
#define PLAYER_BULLET_DELAY_MS 70
#define ENEMY_BULLET_DELAY_MS 100
#define GUARD_PATROL_DELAY_MS 430
#define GUARD_ALERT_DELAY_MS 210
#define ENEMY_SHOOT_DELAY_MS 520
#define FRAME_DELAY_MS 33


/*
    BLINK - real-time terminal version

    Player symbols:
    o = idle player
    ò = moving right
    ó = moving left
    ø = dead player

    Unlockable hats:
    ô = touch guards to delete them
    õ = shots become ~ and pierce through guards
    ö = double shot

    Other symbols:
    # = wall internally, drawn as box-drawing symbols
    . = floor
    * = Signal pickup
    : = normal bullet, player or enemy
    ~ = piercing player bullet while using hat õ
    E = exit, visually representing []
    ^ > < v = guards and their direction

    During gameplay:
    W A S D = move without Enter
    SPACE   = blink for a short hidden duration
    F       = shoot in last movement direction
    Q       = quit run
*/

typedef struct {
    int row;
    int col;
    char direction;
} Guard;

typedef struct {
    int active;
    int row;
    int col;
    char direction;
    long long lastMoveMs;
} Bullet;

typedef struct {
    char layout[HEIGHT][WIDTH + 1];
    int startRow;
    int startCol;
    int guardCount;
    Guard startingGuards[MAX_GUARDS];
    char introMessage[160];
} Level;

typedef struct {
    char name[PLAYER_NAME_STORAGE];
    int wins;
    int deaths;
    long long bestTimeMs;
    int selectedHat;
    int unlockedHats[HAT_COUNT];
} PlayerProfile;

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
int damagedWalls[HEIGHT][WIDTH];

Guard guards[MAX_GUARDS];
int guardAmmo[MAX_GUARDS];
int guardCount = 0;
int currentLevel = 0;

int playerRow = 1;
int playerCol = 1;
int signalPower = 3;
int blinkActive = 0;
long long blinkEndMs = 0;
int playerCaught = 0;
int playerDead = 0;
int alertMode = 0;

Bullet playerBullets[MAX_PLAYER_BULLETS];
Bullet enemyBullets[MAX_ENEMY_BULLETS];

char lastDirection = '>';
char lastMoveCommand = '\0';
long long lastMovePoseMs = 0;
char message[180] = "The cursor blinks into existence.";

const char *hatSymbols[HAT_COUNT] = {"ô", "õ", "ö"};
int unlockedHats[HAT_COUNT] = {0, 0, 0};
int selectedHat = -1;
char currentPlayerName[PLAYER_NAME_STORAGE] = "------";
int hasCurrentPlayer = 0;

int hatActive(int hatIndex) {
    return selectedHat == hatIndex &&
           selectedHat >= 0 &&
           selectedHat < HAT_COUNT &&
           unlockedHats[selectedHat];
}

static struct termios originalTermios;
static int rawModeEnabled = 0;

long long runStartMs = 0;
long long finalRunTimeMs = 0;
long long lastPlayerMoveMs = 0;
long long lastGuardMoveMs = 0;
long long lastEnemyShootMs = 0;

long long nowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

void sleepMs(int ms) {
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void disableRawMode(void) {
    if (rawModeEnabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
        printf("\033[?25h");
        fflush(stdout);
        rawModeEnabled = 0;
    }
}

int visibleTextLength(const char *text) {
    int length = 0;
    int i = 0;

    while (text[i] != '\0') {
        if (strncmp(&text[i], HEART_SYMBOL, strlen(HEART_SYMBOL)) == 0) {
            length++;
            i += strlen(HEART_SYMBOL);
        } else {
            length++;
            i++;
        }
    }

    return length;
}

void printPaddedText(const char *text, int width) {
    int visibleLength = visibleTextLength(text);

    printf("%s", text);

    for (int i = visibleLength; i < width; i++) {
        printf(" ");
    }
}

void enableRawMode(void) {
    if (rawModeEnabled) {
        return;
    }

    tcgetattr(STDIN_FILENO, &originalTermios);

    struct termios raw = originalTermios;
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("\033[?25l");
    fflush(stdout);
    rawModeEnabled = 1;
}

int keyAvailable(void) {
    struct timeval tv;
    fd_set readfds;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0;
}

int readKey(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }

    return -1;
}

void clearScreen(void) {
    printf("\033[H\033[J");
}

void waitForEnter(void) {
    char input[16];
    printf("\nPress ENTER to continue.");
    fgets(input, sizeof(input), stdin);
}

int signOf(int value) {
    if (value > 0) {
        return 1;
    }

    if (value < 0) {
        return -1;
    }

    return 0;
}

void formatTime(long long ms, char *buffer, size_t bufferSize) {
    if (ms < 0) {
        snprintf(buffer, bufferSize, "--:--.---");
        return;
    }

    long long minutes = ms / 60000LL;
    long long seconds = (ms % 60000LL) / 1000LL;
    long long millis = ms % 1000LL;

    snprintf(buffer, bufferSize, "%02lld:%02lld.%03lld", minutes, seconds, millis);
}

int isWallAt(int row, int col) {
    if (row < 0 || row >= HEIGHT || col < 0 || col >= WIDTH) {
        return 0;
    }

    return room[row][col] == '#';
}

void printWallSymbol(int row, int col) {
    int up = isWallAt(row - 1, col);
    int down = isWallAt(row + 1, col);
    int left = isWallAt(row, col - 1);
    int right = isWallAt(row, col + 1);

    if (up && down && left && right) {
        printf("╬");
    } else if (up && down && left) {
        printf("╣");
    } else if (up && down && right) {
        printf("╠");
    } else if (left && right && up) {
        printf("╩");
    } else if (left && right && down) {
        printf("╦");
    } else if (up && down) {
        printf("║");
    } else if (left && right) {
        printf("═");
    } else if (right && down) {
        printf("╔");
    } else if (left && down) {
        printf("╗");
    } else if (right && up) {
        printf("╚");
    } else if (left && up) {
        printf("╝");
    } else {
        printf("╬");
    }
}

void printDamagedWallSymbol(int row, int col) {
    int up = isWallAt(row - 1, col);
    int down = isWallAt(row + 1, col);
    int left = isWallAt(row, col - 1);
    int right = isWallAt(row, col + 1);

    if (up && down && left && right) {
        printf("┼");
    } else if (up && down && left) {
        printf("┤");
    } else if (up && down && right) {
        printf("├");
    } else if (left && right && up) {
        printf("┴");
    } else if (left && right && down) {
        printf("┬");
    } else if (up && down) {
        printf("│");
    } else if (left && right) {
        printf("─");
    } else if (right && down) {
        printf("╒");
    } else if (left && down) {
        printf("┐");
    } else if (right && up) {
        printf("└");
    } else if (left && up) {
        printf("┘");
    } else {
        printf("┼");
    }
}

const char *getPlayerSymbol(void) {
    if (playerDead) {
        return "ø";
    }

    if (lastMoveCommand == 'd') {
        return "ò";
    }

    if (lastMoveCommand == 'a') {
        return "ó";
    }

    if (selectedHat >= 0 && selectedHat < HAT_COUNT && unlockedHats[selectedHat]) {
        return hatSymbols[selectedHat];
    }

    return "o";
}

void updatePlayerPose(long long currentMs) {
    if (lastMoveCommand == '\0') {
        return;
    }

    if (currentMs - lastMovePoseMs >= MOVE_POSE_DURATION_MS) {
        lastMoveCommand = '\0';
    }
}

void killPlayer(const char *deathMessage) {
    playerCaught = 1;
    playerDead = 1;
    strncpy(message, deathMessage, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
}

void normalizePlayerName(char *name) {
    for (int i = 0; name[i] != '\0'; i++) {
        name[i] = toupper((unsigned char)name[i]);
    }

    if (strcmp(name, "LAURA") == 0) {
        strcpy(name, LAURA_HEART_NAME);
    }
}

int isValidPlayerName(const char *name) {
    int length = strlen(name);

    if (length < 1 || length > PLAYER_NAME_MAX_CHARS) {
        return 0;
    }

    for (int i = 0; i < length; i++) {
        if (!isalpha((unsigned char)name[i])) {
            return 0;
        }
    }

    return 1;
}

void createDefaultProfile(PlayerProfile *profile, const char *name) {
    strcpy(profile->name, name);
    profile->wins = 0;
    profile->deaths = 0;
    profile->bestTimeMs = -1;
    profile->selectedHat = -1;

    for (int i = 0; i < HAT_COUNT; i++) {
        profile->unlockedHats[i] = 0;
    }
}

void applyProfileToGame(PlayerProfile *profile) {
    strcpy(currentPlayerName, profile->name);
    hasCurrentPlayer = 1;

    selectedHat = profile->selectedHat;

    for (int i = 0; i < HAT_COUNT; i++) {
        unlockedHats[i] = profile->unlockedHats[i];
    }

    if (selectedHat < 0 || selectedHat >= HAT_COUNT || !unlockedHats[selectedHat]) {
        selectedHat = -1;
    }
}

void copyGameToProfile(PlayerProfile *profile) {
    strcpy(profile->name, currentPlayerName);
    profile->selectedHat = selectedHat;

    for (int i = 0; i < HAT_COUNT; i++) {
        profile->unlockedHats[i] = unlockedHats[i];
    }
}

int readProfiles(PlayerProfile profiles[], int *profileCount) {
    FILE *file = fopen(SAVE_FILE, "r");
    *profileCount = 0;

    if (file == NULL) {
        return 0;
    }

    while (*profileCount < MAX_PROFILES) {
        PlayerProfile profile;
        int valuesRead = fscanf(
            file,
            "%15s %d %d %lld %d %d %d %d",
            profile.name,
            &profile.wins,
            &profile.deaths,
            &profile.bestTimeMs,
            &profile.selectedHat,
            &profile.unlockedHats[0],
            &profile.unlockedHats[1],
            &profile.unlockedHats[2]
        );

        if (valuesRead != 8) {
            break;
        }

        profile.name[PLAYER_NAME_STORAGE - 1] = '\0';
        profiles[*profileCount] = profile;
        (*profileCount)++;
    }

    fclose(file);
    return 1;
}

int writeProfiles(PlayerProfile profiles[], int profileCount) {
    FILE *file = fopen(SAVE_FILE, "w");

    if (file == NULL) {
        return 0;
    }

    for (int i = 0; i < profileCount; i++) {
        fprintf(
            file,
            "%s %d %d %lld %d %d %d %d\n",
            profiles[i].name,
            profiles[i].wins,
            profiles[i].deaths,
            profiles[i].bestTimeMs,
            profiles[i].selectedHat,
            profiles[i].unlockedHats[0],
            profiles[i].unlockedHats[1],
            profiles[i].unlockedHats[2]
        );
    }

    fclose(file);
    return 1;
}

int findProfileIndex(PlayerProfile profiles[], int profileCount, const char *name) {
    for (int i = 0; i < profileCount; i++) {
        if (strcmp(profiles[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

void saveCurrentProfile(void) {
    if (!hasCurrentPlayer) {
        return;
    }

    PlayerProfile profiles[MAX_PROFILES];
    int profileCount;
    readProfiles(profiles, &profileCount);

    int index = findProfileIndex(profiles, profileCount, currentPlayerName);

    if (index == -1) {
        if (profileCount >= MAX_PROFILES) {
            return;
        }

        createDefaultProfile(&profiles[profileCount], currentPlayerName);
        index = profileCount;
        profileCount++;
    }

    copyGameToProfile(&profiles[index]);
    writeProfiles(profiles, profileCount);
}

void loadOrCreateProfile(const char *name) {
    PlayerProfile profiles[MAX_PROFILES];
    int profileCount;
    readProfiles(profiles, &profileCount);

    int index = findProfileIndex(profiles, profileCount, name);

    if (index == -1) {
        if (profileCount >= MAX_PROFILES) {
            return;
        }

        createDefaultProfile(&profiles[profileCount], name);
        index = profileCount;
        profileCount++;
        writeProfiles(profiles, profileCount);
    }

    applyProfileToGame(&profiles[index]);
}

void recordGameResult(int won, long long finishTimeMs) {
    if (!hasCurrentPlayer) {
        return;
    }

    PlayerProfile profiles[MAX_PROFILES];
    int profileCount;
    readProfiles(profiles, &profileCount);

    int index = findProfileIndex(profiles, profileCount, currentPlayerName);

    if (index == -1) {
        if (profileCount >= MAX_PROFILES) {
            return;
        }

        createDefaultProfile(&profiles[profileCount], currentPlayerName);
        index = profileCount;
        profileCount++;
    }

    if (won) {
        profiles[index].wins++;

        if (profiles[index].bestTimeMs < 0 || finishTimeMs < profiles[index].bestTimeMs) {
            profiles[index].bestTimeMs = finishTimeMs;
        }
    } else {
        profiles[index].deaths++;
    }

    copyGameToProfile(&profiles[index]);
    writeProfiles(profiles, profileCount);
}

void deleteCurrentProfile(void) {
    if (!hasCurrentPlayer) {
        return;
    }

    PlayerProfile profiles[MAX_PROFILES];
    int profileCount;
    readProfiles(profiles, &profileCount);

    int index = findProfileIndex(profiles, profileCount, currentPlayerName);

    if (index == -1) {
        return;
    }

    for (int i = index; i < profileCount - 1; i++) {
        profiles[i] = profiles[i + 1];
    }

    profileCount--;
    writeProfiles(profiles, profileCount);

    strcpy(currentPlayerName, "------");
    hasCurrentPlayer = 0;
    selectedHat = -1;

    for (int i = 0; i < HAT_COUNT; i++) {
        unlockedHats[i] = 0;
    }
}

void askForPlayerName(void) {
    char input[32];

    while (1) {
        clearScreen();
        printf("╔══════════════════════╗\n");
        printf("║     PLAYER NAME      ║\n");
        printf("╚══════════════════════╝\n\n");
        printf("Enter up to 6 letters, arcade style.\n");
        printf("Example: LUIS, LUF, LAURA\n\n");
        printf("Name: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return;
        }

        input[strcspn(input, "\n")] = '\0';

        if (!isValidPlayerName(input)) {
            continue;
        }

        normalizePlayerName(input);

        loadOrCreateProfile(input);
        return;
    }
}

void showProfileMenu(void) {
    char input[32];

    while (1) {
        clearScreen();
        printf("╔══════════════════════╗\n");
        printf("║    BLINK PROFILE     ║\n");
        printf("╚══════════════════════╝\n\n");
        printf("Current profile: %s\n\n", hasCurrentPlayer ? currentPlayerName : "NONE");
        printf("C - create/load profile\n");
        printf("D - delete current profile\n");
        printf("B - back\n\n");
        printf("Command: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return;
        }

        char command = (char)tolower((unsigned char)input[0]);

        if (command == 'c') {
            askForPlayerName();
        } else if (command == 'd') {
            deleteCurrentProfile();
        } else if (command == 'b') {
            return;
        }
    }
}

void showHatMenu(void) {
    char input[32];

    while (1) {
        clearScreen();
        printf("╔══════════════════════╗\n");
        printf("║      BLINK HATS      ║\n");
        printf("╚══════════════════════╝\n\n");
        printf("Current player: %s\n", getPlayerSymbol());
        printf("Current profile: %s\n\n", hasCurrentPlayer ? currentPlayerName : "NONE");
        printf("0 - no hat: o\n");

        for (int i = 0; i < HAT_COUNT; i++) {
            const char *power = "";
            if (i == 0) {
                power = "touch guards to delete them";
            } else if (i == 1) {
                power = "~ shot pierces guards";
            } else if (i == 2) {
                power = "double shot";
            }

            printf("%d - hat %d: %s [%s] - %s\n",
                   i + 1,
                   i + 1,
                   hatSymbols[i],
                   unlockedHats[i] ? "unlocked" : "locked",
                   power);
        }

        printf("\nPress 0-3 to choose.\n");
        printf("Press B to go back.\n\n");
        printf("Command: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return;
        }

        char command = (char)tolower((unsigned char)input[0]);

        if (command == 'b') {
            return;
        }

        if (command == '0') {
            selectedHat = -1;
            saveCurrentProfile();
            return;
        }

        if (command >= '1' && command <= '3') {
            int hatIndex = command - '1';

            if (unlockedHats[hatIndex]) {
                selectedHat = hatIndex;
                saveCurrentProfile();
                return;
            }
        }
    }
}

void unlockRandomHat(void) {
    int lockedHats[HAT_COUNT];
    int lockedCount = 0;

    for (int i = 0; i < HAT_COUNT; i++) {
        if (!unlockedHats[i]) {
            lockedHats[lockedCount] = i;
            lockedCount++;
        }
    }

    if (lockedCount == 0) {
        printf("\nYou already unlocked every hat.\n");
        return;
    }

    int randomSlot = rand() % lockedCount;
    int hatIndex = lockedHats[randomSlot];

    unlockedHats[hatIndex] = 1;
    selectedHat = hatIndex;
    saveCurrentProfile();

    printf("\nNew hat unlocked: %s\n", hatSymbols[hatIndex]);
    printf("It has been equipped for the next run.\n");
}

int compareProfilesForLeaderboard(const void *left, const void *right) {
    const PlayerProfile *a = (const PlayerProfile *)left;
    const PlayerProfile *b = (const PlayerProfile *)right;

    if (a->bestTimeMs < 0 && b->bestTimeMs >= 0) {
        return 1;
    }

    if (a->bestTimeMs >= 0 && b->bestTimeMs < 0) {
        return -1;
    }

    if (a->bestTimeMs >= 0 && b->bestTimeMs >= 0) {
        if (a->bestTimeMs < b->bestTimeMs) {
            return -1;
        }

        if (a->bestTimeMs > b->bestTimeMs) {
            return 1;
        }
    }

    if (a->wins != b->wins) {
        return b->wins - a->wins;
    }

    if (a->deaths != b->deaths) {
        return a->deaths - b->deaths;
    }

    return strcmp(a->name, b->name);
}

void showLeaderboard(void) {
    PlayerProfile profiles[MAX_PROFILES];
    int profileCount = 0;
    char formattedTime[32];

    readProfiles(profiles, &profileCount);
    qsort(profiles, (size_t)profileCount, sizeof(PlayerProfile), compareProfilesForLeaderboard);

    clearScreen();
    printf("╔══════════════════════════════════╗\n");
    printf("║            LEADERBOARD           ║\n");
    printf("╚══════════════════════════════════╝\n\n");

    printf("Ranked by fastest completed run.\n\n");
    printf("RK  ");
    printPaddedText("NAME", LEADERBOARD_NAME_WIDTH);
    printf("  BEST TIME  WINS  DEATHS\n");

    printf("--  ");
    printPaddedText("------", LEADERBOARD_NAME_WIDTH);
    printf("  ---------  ----  ------\n");

    for (int i = 0; i < profileCount && i < 10; i++) {
        formatTime(profiles[i].bestTimeMs, formattedTime, sizeof(formattedTime));
    printf("%2d  ", i + 1);
    printPaddedText(profiles[i].name, LEADERBOARD_NAME_WIDTH);
    printf("  %9s  %4d  %6d\n",
        formattedTime,
        profiles[i].wins,
        profiles[i].deaths);
    }

    if (profileCount == 0) {
        printf("No local profiles yet.\n");
    }

    waitForEnter();
}

int showTitleScreen(void) {
    char input[32];

    while (1) {
        clearScreen();
        printf("╔════════════════════════════════╗\n");
        printf("║  ██████╗ ██╗     ██╗███╗   ██╗██╗  ██╗ ║\n");
        printf("║  ██╔══██╗██║     ██║████╗  ██║██║ ██╔╝ ║\n");
        printf("║  ██████╔╝██║     ██║██╔██╗ ██║█████╔╝  ║\n");
        printf("║  ██╔══██╗██║     ██║██║╚██╗██║██╔═██╗  ║\n");
        printf("║  ██████╔╝███████╗██║██║ ╚████║██║  ██╗ ║\n");
        printf("║  ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ║\n");
        printf("╚════════════════════════════════╝\n\n");
        printf("Current player: %s\n", getPlayerSymbol());
        printf("Current profile: %s\n\n", hasCurrentPlayer ? currentPlayerName : "NONE");
        printf("N - new run\n");
        printf("P - profile\n");
        printf("H - hats\n");
        printf("L - leaderboard\n");
        printf("Q - quit\n\n");
        printf("Command: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }

        char command = (char)tolower((unsigned char)input[0]);

        if (command == 'q') {
            return 0;
        }

        if (command == 'n' || command == '\n') {
            return 1;
        }

        if (command == 'p') {
            showProfileMenu();
        } else if (command == 'h') {
            showHatMenu();
        } else if (command == 'l') {
            showLeaderboard();
        }
    }
}

int askRestart(void) {
    char input[32];

    while (1) {
        printf("\nR - restart | H - hats | L - leaderboard | Q - quit\n");
        printf("Command: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }

        char command = (char)tolower((unsigned char)input[0]);

        if (command == 'r') {
            return 1;
        }

        if (command == 'h') {
            showHatMenu();
        } else if (command == 'l') {
            showLeaderboard();
        } else if (command == 'q') {
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

char directionFromStep(int rowStep, int colStep) {
    if (rowStep < 0) {
        return '^';
    }

    if (rowStep > 0) {
        return 'v';
    }

    if (colStep < 0) {
        return '<';
    }

    if (colStep > 0) {
        return '>';
    }

    return '>';
}

void clearPlayerBullets(void) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = 0;
        playerBullets[i].lastMoveMs = 0;
    }
}

void clearEnemyBullets(void) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemyBullets[i].active = 0;
        enemyBullets[i].lastMoveMs = 0;
    }
}

void loadLevel(int levelIndex) {
    currentLevel = levelIndex;

    for (int row = 0; row < HEIGHT; row++) {
        strcpy(room[row], levels[levelIndex].layout[row]);

        for (int col = 0; col < WIDTH; col++) {
            damagedWalls[row][col] = 0;
        }
    }

    playerRow = levels[levelIndex].startRow;
    playerCol = levels[levelIndex].startCol;

    guardCount = levels[levelIndex].guardCount;
    for (int i = 0; i < guardCount; i++) {
        guards[i] = levels[levelIndex].startingGuards[i];
        guardAmmo[i] = GUARD_AMMO;
    }

    blinkActive = 0;
    playerCaught = 0;
    playerDead = 0;
    alertMode = 0;
    clearPlayerBullets();
    lastDirection = '>';
    lastMoveCommand = '\0';
    lastMovePoseMs = 0;
    clearEnemyBullets();
    strncpy(message, levels[levelIndex].introMessage, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
}

void resetGame(void) {
    signalPower = 3;
    runStartMs = nowMs();
    finalRunTimeMs = 0;
    lastPlayerMoveMs = 0;
    lastGuardMoveMs = 0;
    lastEnemyShootMs = 0;
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

int guardAtExcept(int row, int col, int exceptionIndex) {
    for (int i = 0; i < guardCount; i++) {
        if (i != exceptionIndex && guards[i].row == row && guards[i].col == col) {
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

int removeGuardAt(int row, int col);

int enemyBulletAt(int row, int col) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].active && enemyBullets[i].row == row && enemyBullets[i].col == col) {
            return 1;
        }
    }

    return 0;
}

int playerBulletAt(int row, int col) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active && playerBullets[i].row == row && playerBullets[i].col == col) {
            return 1;
        }
    }

    return 0;
}

int bulletAt(int row, int col) {
    return playerBulletAt(row, col) || enemyBulletAt(row, col);
}

int findPlayerBulletSlot(void) {
    int maxBullets = hatActive(2) ? MAX_PLAYER_BULLETS : 1;

    for (int i = 0; i < maxBullets; i++) {
        if (!playerBullets[i].active) {
            return i;
        }
    }

    return -1;
}

void deactivatePlayerBulletAt(int row, int col) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active && playerBullets[i].row == row && playerBullets[i].col == col) {
            playerBullets[i].active = 0;
            return;
        }
    }
}

int handleGuardHitByPlayerBulletAt(int row, int col) {
    if (!playerBulletAt(row, col)) {
        return 0;
    }

    if (!hatActive(1)) {
        deactivatePlayerBulletAt(row, col);
    }

    removeGuardAt(row, col);
    strcpy(message, hatActive(1) ? "The wave cuts through a guard." : "The shot deletes a guard.");
    return 1;
}

void removeGuardByIndex(int index) {
    for (int i = index; i < guardCount - 1; i++) {
        guards[i] = guards[i + 1];
        guardAmmo[i] = guardAmmo[i + 1];
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

int playerTouchesEnemyBullet(void) {
    return enemyBulletAt(playerRow, playerCol);
}

int tryMovePlayer(char command, long long currentMs) {
    if (currentMs - lastPlayerMoveMs < PLAYER_MOVE_DELAY_MS) {
        return 0;
    }

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

    lastPlayerMoveMs = currentMs;

    if (room[newRow][newCol] == '#') {
        lastMoveCommand = '\0';
        strcpy(message, "A wall blocks your movement.");
        return 0;
    }

    if (guardAt(newRow, newCol)) {
        if (hatActive(0)) {
            removeGuardAt(newRow, newCol);
            playerRow = newRow;
            playerCol = newCol;
            lastMoveCommand = command;
            lastMovePoseMs = currentMs;
            strcpy(message, "The hat deletes the guard on contact.");
            collectSignalIfNeeded();
            return 1;
        }

        lastMoveCommand = '\0';
        strcpy(message, "A guard blocks your movement.");
        return 0;
    }

    playerRow = newRow;
    playerCol = newCol;
    lastMoveCommand = command;
    lastMovePoseMs = currentMs;
    strcpy(message, "You move through the room.");
    collectSignalIfNeeded();

    if (playerTouchesEnemyBullet()) {
        killPlayer("You move into a hostile shot.");
    }

    return 1;
}

void activateBlink(long long currentMs) {
    if (blinkActive) {
        strcpy(message, "You are already blinking.");
        return;
    }

    if (signalPower <= 0) {
        strcpy(message, "No Signal left.");
        return;
    }

    signalPower--;
    blinkActive = 1;
    blinkEndMs = currentMs + BLINK_DURATION_MS;
    lastMoveCommand = '\0';
    strcpy(message, "You blink out of visible memory.");
}

void updateBlink(long long currentMs) {
    if (blinkActive && currentMs >= blinkEndMs) {
        blinkActive = 0;
        strcpy(message, "You blink back into sight.");
    }
}

void firePlayerBullet(long long currentMs) {
    int slot = findPlayerBulletSlot();

    if (slot == -1) {
        strcpy(message, hatActive(2) ? "Both shots are already active." : "Only one shot can exist at a time.");
        return;
    }

    if (signalPower <= 0) {
        strcpy(message, "No Signal left to fire.");
        return;
    }

    signalPower--;
    lastMoveCommand = '\0';

    playerBullets[slot].active = 1;
    playerBullets[slot].row = playerRow;
    playerBullets[slot].col = playerCol;
    playerBullets[slot].direction = lastDirection;
    playerBullets[slot].lastMoveMs = currentMs;

    if (hatActive(1)) {
        strcpy(message, "You fire a piercing wave.");
    } else if (hatActive(2)) {
        strcpy(message, "You fire one of two pulses.");
    } else {
        strcpy(message, "You spend 1 Signal and fire a thin pulse.");
    }
}

void movePlayerBullet(long long currentMs) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!playerBullets[i].active || currentMs - playerBullets[i].lastMoveMs < PLAYER_BULLET_DELAY_MS) {
            continue;
        }

        int rowDirection;
        int colDirection;
        getDirection(playerBullets[i].direction, &rowDirection, &colDirection);

        int newRow = playerBullets[i].row + rowDirection;
        int newCol = playerBullets[i].col + colDirection;
        playerBullets[i].lastMoveMs = currentMs;

        if (room[newRow][newCol] == '#') {
            damagedWalls[newRow][newCol] = 1;
            playerBullets[i].active = 0;
            strcpy(message, hatActive(1) ? "The wave breaks against a wall." : "The shot scars the wall.");
            continue;
        }

        playerBullets[i].row = newRow;
        playerBullets[i].col = newCol;

        if (removeGuardAt(playerBullets[i].row, playerBullets[i].col)) {
            if (!hatActive(1)) {
                playerBullets[i].active = 0;
                strcpy(message, "The shot deletes a guard.");
            } else {
                strcpy(message, "The wave pierces through a guard.");
            }
        }
    }
}

void moveEnemyBullets(long long currentMs) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active || currentMs - enemyBullets[i].lastMoveMs < ENEMY_BULLET_DELAY_MS) {
            continue;
        }

        int rowDirection;
        int colDirection;
        getDirection(enemyBullets[i].direction, &rowDirection, &colDirection);

        int newRow = enemyBullets[i].row + rowDirection;
        int newCol = enemyBullets[i].col + colDirection;
        enemyBullets[i].lastMoveMs = currentMs;

        if (room[newRow][newCol] == '#') {
            enemyBullets[i].active = 0;
            continue;
        }

        enemyBullets[i].row = newRow;
        enemyBullets[i].col = newCol;

        if (enemyBullets[i].row == playerRow && enemyBullets[i].col == playerCol) {
            killPlayer("A hostile shot hits the cursor.");
            return;
        }
    }
}

int guardSeesPlayer(void) {
    if (blinkActive) {
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

int guardHasLineOfSightToPlayer(int guardIndex, char *shotDirection) {
    int row = guards[guardIndex].row;
    int col = guards[guardIndex].col;

    if (row == playerRow) {
        int step = signOf(playerCol - col);
        if (step == 0) {
            return 0;
        }

        int checkCol = col + step;
        while (checkCol != playerCol) {
            if (room[row][checkCol] == '#') {
                return 0;
            }
            checkCol += step;
        }

        *shotDirection = step > 0 ? '>' : '<';
        return 1;
    }

    if (col == playerCol) {
        int step = signOf(playerRow - row);
        if (step == 0) {
            return 0;
        }

        int checkRow = row + step;
        while (checkRow != playerRow) {
            if (room[checkRow][col] == '#') {
                return 0;
            }
            checkRow += step;
        }

        *shotDirection = step > 0 ? 'v' : '^';
        return 1;
    }

    return 0;
}

int findEnemyBulletSlot(void) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) {
            return i;
        }
    }

    return -1;
}

void fireEnemyBulletFromGuard(int guardIndex, char direction, long long currentMs) {
    if (guardAmmo[guardIndex] <= 0) {
        return;
    }

    int slot = findEnemyBulletSlot();
    if (slot == -1) {
        return;
    }

    int rowDirection;
    int colDirection;
    getDirection(direction, &rowDirection, &colDirection);

    int startRow = guards[guardIndex].row + rowDirection;
    int startCol = guards[guardIndex].col + colDirection;

    if (room[startRow][startCol] == '#') {
        return;
    }

    guardAmmo[guardIndex]--;
    guards[guardIndex].direction = direction;

    if (startRow == playerRow && startCol == playerCol) {
        killPlayer("A guard fires point blank.");
        return;
    }

    enemyBullets[slot].active = 1;
    enemyBullets[slot].row = startRow;
    enemyBullets[slot].col = startCol;
    enemyBullets[slot].direction = direction;
    enemyBullets[slot].lastMoveMs = currentMs;

    strcpy(message, "A guard fires. The shot cuts through the room.");
}

void guardsShootIfPossible(long long currentMs) {
    if (currentMs - lastEnemyShootMs < ENEMY_SHOOT_DELAY_MS) {
        return;
    }

    int fired = 0;

    for (int i = 0; i < guardCount; i++) {
        char shotDirection = '>';

        if (guardHasLineOfSightToPlayer(i, &shotDirection)) {
            fireEnemyBulletFromGuard(i, shotDirection, currentMs);
            fired = 1;

            if (playerCaught) {
                return;
            }
        }
    }

    if (fired) {
        lastEnemyShootMs = currentMs;
    }
}

void movePatrolGuards(long long currentMs) {
    if (currentMs - lastGuardMoveMs < GUARD_PATROL_DELAY_MS) {
        return;
    }

    lastGuardMoveMs = currentMs;

    for (int i = 0; i < guardCount; i++) {
        int rowDirection;
        int colDirection;
        getDirection(guards[i].direction, &rowDirection, &colDirection);

        int newRow = guards[i].row + rowDirection;
        int newCol = guards[i].col + colDirection;

        if (newRow == playerRow && newCol == playerCol) {
            if (hatActive(0)) {
                removeGuardByIndex(i);
                i--;
                strcpy(message, "The hat deletes a guard on contact.");
                continue;
            }

            killPlayer("A guard touches the cursor.");
            return;
        }

        if (playerBulletAt(newRow, newCol)) {
            if (!hatActive(1)) {
                deactivatePlayerBulletAt(newRow, newCol);
            }

            removeGuardByIndex(i);
            i--;
            strcpy(message, hatActive(1) ? "A guard walks into the wave." : "A guard walks into your shot.");
            continue;
        }

        if (room[newRow][newCol] == '#' || room[newRow][newCol] == 'E' || room[newRow][newCol] == '*' || guardAt(newRow, newCol)) {
            guards[i].direction = reverseDirection(guards[i].direction);
        } else {
            guards[i].row = newRow;
            guards[i].col = newCol;
        }
    }
}

int tryMoveChasingGuard(int guardIndex, int rowStep, int colStep) {
    int newRow = guards[guardIndex].row + rowStep;
    int newCol = guards[guardIndex].col + colStep;

    if (newRow == playerRow && newCol == playerCol) {
        guards[guardIndex].direction = directionFromStep(rowStep, colStep);

        if (hatActive(0)) {
            removeGuardByIndex(guardIndex);
            strcpy(message, "The hat deletes a chasing guard.");
            return 1;
        }

        killPlayer("A guard reaches the cursor.");
        return 1;
    }

    if (playerBulletAt(newRow, newCol)) {
        if (!hatActive(1)) {
            deactivatePlayerBulletAt(newRow, newCol);
        }

        removeGuardByIndex(guardIndex);
        strcpy(message, hatActive(1) ? "A chasing guard hits the wave." : "A chasing guard hits your shot.");
        return 1;
    }

    if (room[newRow][newCol] == '#' || guardAtExcept(newRow, newCol, guardIndex)) {
        return 0;
    }

    guards[guardIndex].row = newRow;
    guards[guardIndex].col = newCol;
    guards[guardIndex].direction = directionFromStep(rowStep, colStep);

    return 1;
}

void moveChasingGuards(long long currentMs) {
    if (currentMs - lastGuardMoveMs < GUARD_ALERT_DELAY_MS) {
        return;
    }

    lastGuardMoveMs = currentMs;

    for (int i = 0; i < guardCount; i++) {
        int rowDiff = playerRow - guards[i].row;
        int colDiff = playerCol - guards[i].col;
        int rowStep = signOf(rowDiff);
        int colStep = signOf(colDiff);
        int moved = 0;

        if (abs(rowDiff) >= abs(colDiff)) {
            if (rowStep != 0) {
                moved = tryMoveChasingGuard(i, rowStep, 0);
            }

            if (!moved && colStep != 0) {
                moved = tryMoveChasingGuard(i, 0, colStep);
            }
        } else {
            if (colStep != 0) {
                moved = tryMoveChasingGuard(i, 0, colStep);
            }

            if (!moved && rowStep != 0) {
                moved = tryMoveChasingGuard(i, rowStep, 0);
            }
        }

        if (playerCaught) {
            return;
        }
    }
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
    strcpy(message, "Room loaded. Keep moving.");
    return 1;
}

void updateWorld(long long currentMs) {
    updateBlink(currentMs);
    movePlayerBullet(currentMs);

    if (playerCaught) {
        return;
    }

    moveEnemyBullets(currentMs);

    if (playerCaught) {
        return;
    }

    if (alertMode) {
        moveChasingGuards(currentMs);

        if (playerCaught) {
            return;
        }

        guardsShootIfPossible(currentMs);
    } else {
        movePatrolGuards(currentMs);

        if (!playerCaught && guardSeesPlayer()) {
            alertMode = 1;
            strcpy(message, "ALERT. Every guard locks onto your signal.");
        }
    }
}

void drawRoomRealtime(void) {
    char elapsedBuffer[32];
    long long elapsed = nowMs() - runStartMs;
    formatTime(elapsed, elapsedBuffer, sizeof(elapsedBuffer));

    clearScreen();
    printf("╔════════════════════════════════╗\n");
    printf("║             BLINK              ║\n");
    printf("╚════════════════════════════════╝\n\n");
    printf("Profile:    %s\n", hasCurrentPlayer ? currentPlayerName : "NONE");
    printf("Room:       %d/%d\n", currentLevel + 1, LEVEL_COUNT);
    printf("Time:       %s\n", elapsedBuffer);

    if (alertMode) {
        printf("Mode:       ALERT - THEY KNOW WHERE YOU ARE\n");
    } else {
        printf("Mode:       CALM\n");
    }

    printf("State:      %s\n", blinkActive ? "BLINKING" : "VISIBLE");
    printf("Signal:     %d/%d\n", signalPower, MAX_SIGNAL);

    if (hatActive(0)) {
        printf("Hat:        ô CONTACT DELETE\n");
    } else if (hatActive(1)) {
        printf("Hat:        õ PIERCING WAVE\n");
    } else if (hatActive(2)) {
        printf("Hat:        ö DOUBLE SHOT\n");
    } else {
        printf("Hat:        none\n");
    }

    printf("Facing:     %c\n\n", lastDirection);

    for (int row = 0; row < HEIGHT; row++) {
        for (int col = 0; col < WIDTH; col++) {
            if (row == playerRow && col == playerCol) {
                if (playerDead) {
                    printf("%s", getPlayerSymbol());
                } else if (blinkActive) {
                    printf(" ");
                } else {
                    printf("%s", getPlayerSymbol());
                }
            } else if (playerBulletAt(row, col)) {
                printf("%s", hatActive(1) ? "~" : ":");
            } else if (enemyBulletAt(row, col)) {
                printf(":");
            } else if (guardAt(row, col)) {
                printf("%c", guardSymbolAt(row, col));
            } else if (room[row][col] == 'E') {
                printf("[]");
            } else if (room[row][col] == '#') {
                if (damagedWalls[row][col]) {
                    printDamagedWallSymbol(row, col);
                } else {
                    printWallSymbol(row, col);
                }
            } else {
                printf("%c", room[row][col]);
            }
        }
        printf("\n");
    }

    printf("\nWASD move | SPACE blink | F shoot | Q quit run\n");
    printf("Message: %s\n", message);
    fflush(stdout);
}

void handleGameplayInput(long long currentMs, int *quitRun) {
    while (keyAvailable()) {
        int key = readKey();

        if (key == -1) {
            return;
        }

        char command = (char)tolower(key);

        if (command == 'q') {
            *quitRun = 1;
            return;
        }

        if (command == 'w' || command == 'a' || command == 's' || command == 'd') {
            tryMovePlayer(command, currentMs);
        } else if (key == ' ') {
            activateBlink(currentMs);
        } else if (command == 'f') {
            firePlayerBullet(currentMs);
        }
    }
}

int runRealtimeGame(void) {
    int quitRun = 0;
    int won = 0;
    long long lastDrawMs = 0;

    resetGame();
    enableRawMode();

    while (!quitRun && !playerCaught && !won) {
        long long currentMs = nowMs();

        handleGameplayInput(currentMs, &quitRun);
        updateWorld(currentMs);
        updatePlayerPose(currentMs);

        if (reachedExit()) {
            if (advanceLevelIfPossible()) {
                /* Continue the same timed run. */
            } else {
                won = 1;
                finalRunTimeMs = currentMs - runStartMs;
            }
        }

        if (currentMs - lastDrawMs >= FRAME_DELAY_MS) {
            drawRoomRealtime();
            lastDrawMs = currentMs;
        }

        sleepMs(5);
    }

    drawRoomRealtime();
    disableRawMode();

    if (quitRun) {
        printf("\nRun abandoned.\n");
        return 0;
    }

    if (!hasCurrentPlayer) {
        askForPlayerName();
    }

    if (won) {
        char finishBuffer[32];
        formatTime(finalRunTimeMs, finishBuffer, sizeof(finishBuffer));
        printf("\nYou reached the final [].\n");
        printf("Finish time: %s\n", finishBuffer);
        printf("The terminal loses your signal.\n");
        printf("You escaped.\n");
        printf("BLINK.\n");

        unlockRandomHat();
        recordGameResult(1, finalRunTimeMs);
        return 1;
    }

    printf("\nThe terminal deletes your position.\n");
    printf("GAME OVER.\n");
    recordGameResult(0, -1);
    return 0;
}

int main(void) {
    srand((unsigned int)time(NULL));
    atexit(disableRawMode);

    if (!showTitleScreen()) {
        printf("\nGame closed.\n");
        return 0;
    }

    while (1) {
        runRealtimeGame();

        if (!askRestart()) {
            break;
        }
    }

    printf("\nGame closed.\n");
    return 0;
}
