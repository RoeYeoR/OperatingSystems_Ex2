#include <iostream>
#include <algorithm>
#include <cctype>
#include "TicTacToe.hpp"

using namespace std;

TicTacToe::TicTacToe(const string& strategy) {
    if (!validateStrategy(strategy)) {
        cout << "Error\n";
        exit(1);
    }
    this->strategy = strategy;
    board = vector<char>(9, ' ');
}

void TicTacToe::playGame() {
    while (true) {
        int move = makeMove();
        board[move - 1] = 'X';
        cout << move << "\n";
        if (checkWin('X')) {
            cout << "I Won\n";
            break;
        }
        if (isBoardFull()) {
            cout << "DRAW\n";
            break;
        }

        int playerMove;
        cin >> playerMove;
        if (!isValidMove(playerMove)) {
            cout << "Error\n";
            exit(1);
        }
        board[playerMove - 1] = 'O';
        if (checkWin('O')) {
            cout << "I Lost\n";
            break;
        }
        if (isBoardFull()) {
            cout << "DRAW\n";
            break;
        }
    }
}

bool TicTacToe::validateStrategy(const string& strategy) {
    if (strategy.size() != 9) return false;
    vector<int> count(10, 0);
    for (char c : strategy) {
        if (!isdigit(c) || c == '0') return false;
        count[c - '0']++;
    }
    return all_of(count.begin() + 1, count.end(), [](int i) { return i == 1; });
}

bool TicTacToe::isValidMove(int move) {
    return move >= 1 && move <= 9 && board[move - 1] == ' ';
}

int TicTacToe::makeMove() {
    for (char c : strategy) {
        int move = c - '0';
        if (board[move - 1] == ' ') {
            return move;
        }
    }
    return -1; 
}

bool TicTacToe::checkWin(char player) {
    const vector<vector<int>> winPositions = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // columns
        {0, 4, 8}, {2, 4, 6}             // diagonals
    };
    for (const auto& pos : winPositions) {
        if (board[pos[0]] == player && board[pos[1]] == player && board[pos[2]] == player) {
            return true;
        }
    }
    return false;
}

bool TicTacToe::isBoardFull() {
    return all_of(board.begin(), board.end(), [](char c) { return c != ' '; });
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Error\n";
        return 1;
    }
    std::string strategy = argv[1];
    TicTacToe game(strategy);
    game.playGame();
    return 0;
}