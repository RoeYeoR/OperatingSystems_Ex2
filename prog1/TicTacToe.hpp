#ifndef TICTACTOE_H
#define TICTACTOE_H

#include <vector>
#include <string>

class TicTacToe {
public:
    TicTacToe(const std::string& strategy);
    void playGame();

private:
    std::vector<char> board;
    std::string strategy;

    bool validateStrategy(const std::string& strategy);
    bool isValidMove(int move);
    int makeMove();
    bool checkWin(char player);
    bool isBoardFull();
};

#endif // TICTACTOE_H
