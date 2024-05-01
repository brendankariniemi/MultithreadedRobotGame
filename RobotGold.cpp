#include <iostream>
#include <vector>
#include <set>
#include <thread>
#include <unistd.h>
#include <random>

using namespace std;

const int NUM_THREADS = 2;
const int BOARD_SIZE = 4;
const int SLEEP_TIME = 2;
const std::pair<int,int> removedGoldLocation = std::make_pair(-1, -1);

typedef struct Thread {
    thread thread;
    char character;
    pair<int,int> position;
    bool gameOverCopy;
} ThreadData;

struct mutex_shared {
    pair<int,int> robotPosition;
    pair<int,int> bombPosition;
    pair<int,int> gold1Position;
    pair<int,int> gold2Position;
    char PREV_MOVE_CHARACTER;
    char WINNER;
    int ROBOT_MOVE_COUNT;
    int TOTAL_MOVE_NUMBER;
    bool GAME_OVER;
} SHARED;

mutex boardMutex;

void printBoard() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            cout << "----";
        }
        cout << "-" << endl;

        for (int j = 0; j < BOARD_SIZE; j++) {
            pair<int,int> curPos(i,j);
            cout << "| ";
            if (curPos == SHARED.gold1Position || curPos == SHARED.gold2Position) {
                cout << "G ";
            } else if (curPos == SHARED.robotPosition) {
                cout << "R ";
            } else if (curPos == SHARED.bombPosition) {
                cout << "B ";
            } else {
                cout << "- ";
            }
        }
        cout << "|" << endl;
    }

    for (int j = 0; j < BOARD_SIZE; j++) {
        cout << "----";
    }
    cout << "-" << endl << endl;
}

bool areAdjacent(const pair<int, int>& pos1, const pair<int, int>& pos2) {
    int dx = abs(pos1.first - pos2.first);
    int dy = abs(pos1.second - pos2.second);
    return (dx <= 1 && dy <= 1);
}

pair<int, int> getRandomAdjacentPosition(const pair<int, int>& currentPosition) {
    std::random_device rd;
    std::mt19937 randomNumberGenerator(rd());
    std::uniform_int_distribution<> dist(-1, 1);

    // Loop until location within board bounds is found
    int x, y;
    do {
        int xShift = dist(randomNumberGenerator);
        int yShift = dist(randomNumberGenerator);
        x = currentPosition.first + xShift;
        y = currentPosition.second + yShift;
    } while (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE);

    return make_pair(x, y);
}

pair<int, int> getNextPosition(ThreadData * player) {
    if (player->character == 'R') {
        // Check if there is gold nearby
        if (areAdjacent(player->position, SHARED.gold1Position) || areAdjacent(player->position, SHARED.gold2Position)) {
            // Move towards the gold
            if (areAdjacent(player->position, SHARED.gold1Position)) {
                return SHARED.gold1Position;
            } else {
                return SHARED.gold2Position;
            }
        } // Check if there is a bomb nearby
        else if (areAdjacent(player->position, SHARED.bombPosition)) {
            // Avoid the bomb, move randomly
            pair<int,int> position = getRandomAdjacentPosition(player->position);
            while (position == SHARED.bombPosition) {
                position = getRandomAdjacentPosition(player->position);
            }
            return position;
        }

        return getRandomAdjacentPosition(player->position);
    } else if (player->character == 'B') {
        // Check if the robot is nearby
        if (areAdjacent(player->position, SHARED.robotPosition)) {
            // Move towards the robot
            return SHARED.robotPosition;
        } // Check if there is gold nearby
        else if (areAdjacent(player->position, SHARED.gold1Position) || areAdjacent(player->position, SHARED.gold2Position)) {
            // Avoid the gold, move randomly
            pair<int,int> position = getRandomAdjacentPosition(player->position);
            while (position == SHARED.gold1Position || position == SHARED.gold2Position) {
                position = getRandomAdjacentPosition(player->position);
            }
            return position;
        }

        return getRandomAdjacentPosition(player->position);
    }
    // Default to no movement
    return player->position;
}


void playerMove(ThreadData * player) {
    boardMutex.lock(); // Lock the mutex before editing board

    // Make sure game did not complete while waiting on mutex
    // And ensure one player does not go twice
    if (!SHARED.GAME_OVER && player->character != SHARED.PREV_MOVE_CHARACTER) {
        if (player->character == 'R') {
            if (++SHARED.ROBOT_MOVE_COUNT >= 2) {
                SHARED.PREV_MOVE_CHARACTER = player->character;
                SHARED.ROBOT_MOVE_COUNT = 0;
            }
        } else {
            SHARED.PREV_MOVE_CHARACTER = player->character;
        }

        // Get location to play
        pair<int,int> location = getNextPosition(player);
        player->position = location;

        if (player->character == 'R') {
            SHARED.robotPosition = location;
            // Collect the gold if needed
            if (SHARED.robotPosition == SHARED.gold1Position) {
                SHARED.gold1Position = make_pair(-1, -1);
            }
            if (SHARED.robotPosition == SHARED.gold2Position) {
                SHARED.gold2Position = make_pair(-1, -1);
            }
            if (SHARED.gold1Position == removedGoldLocation && SHARED.gold2Position == removedGoldLocation) {
                SHARED.GAME_OVER = true;
                SHARED.WINNER = 'R';
            }
        } else if (player->character == 'B') {
            SHARED.bombPosition = location;
            if (SHARED.bombPosition == SHARED.robotPosition) {
                SHARED.robotPosition = make_pair(-1, -1);
                SHARED.GAME_OVER = true;
                SHARED.WINNER = 'B';
            }
        }

        if (!SHARED.GAME_OVER) {
            sleep(SLEEP_TIME);
        }

        cout << "Move " << ++SHARED.TOTAL_MOVE_NUMBER << ": " << endl;
        printBoard();
    }

    player->gameOverCopy = SHARED.GAME_OVER;
    boardMutex.unlock(); // Unlock the mutex
}

void runAPI(void * thread) {
    ThreadData * player = (ThreadData*) thread;

    // Loop until game ends
    while (!player->gameOverCopy) {
        playerMove(player);
    }
}

pair<int, int> getInitialPosition(set<pair<int, int>>& usedPositions) {
    std::random_device rd;
    std::mt19937 randomNumberGenerator(rd());
    std::uniform_int_distribution<> dist(0, BOARD_SIZE - 1);

    pair<int, int> position;
    position = make_pair(dist(randomNumberGenerator), dist(randomNumberGenerator));

    while (usedPositions.contains(position)) {
        position = make_pair(dist(randomNumberGenerator), dist(randomNumberGenerator));
    }

    usedPositions.insert(position);
    return position;
}

void initData(ThreadData *threads) {
    // Ensure no duplicate initial positions
    set<pair<int, int>> usedPositions;

    // Initialize shared data
    SHARED.robotPosition = getInitialPosition(usedPositions);
    SHARED.bombPosition = getInitialPosition(usedPositions);
    SHARED.gold1Position = getInitialPosition(usedPositions);
    SHARED.gold2Position = getInitialPosition(usedPositions);
    SHARED.GAME_OVER = false;
    SHARED.ROBOT_MOVE_COUNT = 0;
    SHARED.TOTAL_MOVE_NUMBER = 0;
    SHARED.PREV_MOVE_CHARACTER = 'B';

    // Set the thread characters
    threads[0].character = 'R';
    threads[0].position = SHARED.robotPosition;
    threads[0].gameOverCopy = SHARED.GAME_OVER;
    threads[1].character = 'B';
    threads[1].position = SHARED.bombPosition;
    threads[1].gameOverCopy = SHARED.GAME_OVER;
}

int main() {

    // Create array of ThreadData and initialize it
    ThreadData threads[NUM_THREADS];
    initData(threads);
    cout << "Beginning Board:" << endl;
    printBoard();

    // Start the threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].thread = std::thread(runAPI, (void *)(&threads[i]));
    }

    // Wait for all to join before continuing
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].thread.join();
    }

    // Find the WINNER
    if (SHARED.WINNER == 'R') {
        cout << "Robot Won!" << endl;
    } else if (SHARED.WINNER == 'B' ){
        cout << "Bomb Won!" << endl;
    }

    return 0;
}