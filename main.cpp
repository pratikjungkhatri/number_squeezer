#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <conio.h>
#include <windows.h>
#include <string>
#include <climits>

using namespace std;

class GameBoard {
private:
    static const int SIZE = 5;
    vector<vector<int>> board;
    int score;
    int nextNumber;
    int launcherNumber;
    int selectedColumn;
    int lastDropCol;
    HANDLE hConsole;

public:
    GameBoard() : board(SIZE, vector<int>(SIZE, 0)), score(0), nextNumber(2), 
                 launcherNumber(2), selectedColumn(0), lastDropCol(-1) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    void setColor(int color) {
        SetConsoleTextAttribute(hConsole, color & 0xFF);
    }

    void hideCursor(bool hide) {
        CONSOLE_CURSOR_INFO curInfo;
        GetConsoleCursorInfo(hConsole, &curInfo);
        curInfo.bVisible = hide ? FALSE : TRUE;
        SetConsoleCursorInfo(hConsole, &curInfo);
    }

    void clearConsole() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD written;
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
        DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
        COORD home = {0, 0};
        FillConsoleOutputCharacter(hConsole, ' ', consoleSize, home, &written);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, home, &written);
        SetConsoleCursorPosition(hConsole, home);
    }

    int colorForValue(int v) {
        if (v <= 0) return 7;
        int t = v, idx = 0;
        while (t > 1) { t >>= 1; idx++; }
        static const int palette[] = {
            7, 11, 10, 13, 14, 12, 9, 15, 6, 5, 3, 8, 11, 10, 13, 14
        };
        int maxIdx = (int)(sizeof(palette) / sizeof(palette[0])) - 1;
        if (idx < 1) idx = 1;
        if (idx > maxIdx) idx = ((idx - 1) % maxIdx) + 1;
        return palette[idx];
    }

    void printColoredCellNumber(int value) {
        if (value == 0) {
            cout << setw(5) << " ";
            return;
        }
        int col = colorForValue(value);
        setColor(col);
        cout << setw(5) << value;
        setColor(7);
    }

    string launcherCellLabel(int colIndex, bool isSelected) {
        if (isSelected) {
            string s = "[" + to_string(launcherNumber) + "]";
            if ((int)s.size() > 6) s = s.substr(0, 6);
            return s;
        }
        return "  v  ";
    }

    void saveScore() {
        ofstream file("score_history.txt", ios::app);
        file << "Score: " << score << endl;
        file.close();
    }

    int getHighScore() {
        ifstream file("score_history.txt");
        int highScore = 0, currentScore;
        string line;
        while (getline(file, line)) {
            size_t pos = line.find("Score: ");
            if (pos != string::npos) {
                try {
                    currentScore = stoi(line.substr(pos + 7));
                    if (currentScore > highScore) highScore = currentScore;
                } catch (...) {}
            }
        }
        file.close();
        return highScore;
    }

    void showHighScoreInline() {
        cout << "\nHighest Score so far: " << getHighScore() << endl;
    }

    void printBoard() {
        clearConsole();
        cout << "\n\t\t====== Number Squeezer ======" << endl;
        cout << "\t\t\tScore: " << score << endl;
        cout << "\t\t=============================" << endl;
        cout << "\n\t\tNext Number: ";
        setColor(colorForValue(nextNumber));
        cout << nextNumber << endl;
        setColor(7);
        cout << "\t\tSelect Column (LEFT/RIGHT) - DOWN to Drop | Q to Quit\n";

        cout << "\n\t\t";
        for (int j = 0; j < SIZE; j++) {
            bool sel = (j == selectedColumn);
            string label = launcherCellLabel(j, sel);
            if (sel) setColor(colorForValue(launcherNumber));
            cout << setw(6) << label;
            setColor(7);
        }
        cout << endl;

        for (int i = 0; i < SIZE; i++) {
            cout << "\t\t";
            for (int j = 0; j < SIZE; j++) {
                cout << "+-----";
            }
            cout << "+" << endl;
            cout << "\t\t";
            for (int j = 0; j < SIZE; j++) {
                cout << "|";
                if (board[i][j] != 0) {
                    printColoredCellNumber(board[i][j]);
                } else {
                    cout << setw(5) << " ";
                }
            }
            cout << "|" << endl;
        }
        cout << "\t\t";
        for (int j = 0; j < SIZE; j++) {
            cout << "+-----";
        }
        cout << "+" << endl;

        cout << "\t\t";
        for (int j = 0; j < SIZE; j++) {
            if (j == selectedColumn) {
                setColor(colorForValue(launcherNumber));
                cout << setw(6) << "^";
                setColor(7);
            } else {
                cout << setw(6) << ".";
            }
        }
        cout << endl;
    }

    bool isGameOver() {
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                if (board[i][j] == 0)
                    return false;

        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE - 1; j++)
                if (board[i][j] == board[i][j + 1])
                    return false;

        for (int i = 0; i < SIZE - 1; i++)
            for (int j = 0; j < SIZE; j++)
                if (board[i][j] == board[i + 1][j])
                    return false;

        return true;
    }

    bool checkTShapeMerge(int row, int col, int val) {
        bool merged = false;
        if (row > 0 && col > 0 && col < SIZE - 1) {
            if (board[row-1][col] == val && board[row][col-1] == val && board[row][col+1] == val) {
                board[row-1][col] = 0;
                board[row][col-1] = 0;
                board[row][col+1] = 0;
                board[row][col] = val * 4;
                score += val * 4;
                setColor(14);
                cout << "\nBONUS! T-shape merge of " << val << " found! Next number set to " << val << "!\n";
                setColor(7);
                nextNumber = val;
                merged = true;
            }
        }
        if (row < SIZE - 1 && col > 0 && col < SIZE - 1) {
            if (board[row+1][col] == val && board[row][col-1] == val && board[row][col+1] == val) {
                board[row+1][col] = 0;
                board[row][col-1] = 0;
                board[row][col+1] = 0;
                board[row][col] = val * 4;
                score += val * 4;
                setColor(14);
                cout << "\nBONUS! T-shape merge of " << val << " found! Next number set to " << val << "!\n";
                setColor(7);
                nextNumber = val;
                merged = true;
            }
        }
        if (!merged) {
            if (row < SIZE - 1 && col > 0) {
                if (board[row][col-1] == val && board[row+1][col] == val) {
                    board[row][col-1] = 0;
                    board[row+1][col] = 0;
                    board[row][col] = val * 4;
                    score += val * 4;
                    setColor(14);
                    cout << "\nBONUS! T-shape merge of " << val << " found! Next number set to " << val << "!\n";
                    setColor(7);
                    nextNumber = val;
                    return true;
                }
            }
            if (row < SIZE - 1 && col < SIZE - 1) {
                if (board[row][col+1] == val && board[row+1][col] == val) {
                    board[row][col+1] = 0;
                    board[row+1][col] = 0;
                    board[row][col] = val * 4;
                    score += val * 4;
                    setColor(14);
                    cout << "\nBONUS! T-shape merge of " << val << " found! Next number set to " << val << "!\n";
                    setColor(7);
                    nextNumber = val;
                    return true;
                }
            }
        }
        return merged;
    }

    bool checkSideTopMerge(int row, int col, int val) {
        if (row > 0 && col > 0) {
            if (board[row-1][col] == val && board[row][col-1] == val) {
                board[row-1][col] = 0;
                board[row][col-1] = 0;
                board[row][col] = val * 4;
                score += val * 4;
                setColor(14);
                cout << "\nBONUS! Side-Top merge of " << val << " found! Next number set to " << val << "!\n";
                setColor(7);
                nextNumber = val;
                return true;
            }
        }
        if (row > 0 && col < SIZE - 1) {
            if (board[row-1][col] == val && board[row][col+1] == val) {
                board[row-1][col] = 0;
                board[row][col+1] = 0;
                board[row][col] = val * 4;
                score += val * 4;
                setColor(14);
                cout << "\nBONUS! Side-Top merge of " << val << " found! Next number set to " << val << "!\n";
                setColor(7);
                nextNumber = val;
                return true;
            }
        }
        return false;
    }

    bool mergeOnce() {
        bool changed = false;

        // Check for horizontal four
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j <= SIZE - 4; j++) {
                int a = board[i][j];
                int b = board[i][j + 1];
                int c = board[i][j + 2];
                int d = board[i][j + 3];
                if (a != 0 && a == b && b == c && c == d) {
                    int targetCol = (lastDropCol >= j && lastDropCol <= j + 3) ? lastDropCol : j + 1;
                    for (int k = j; k <= j + 3; k++) {
                        if (k != targetCol) board[i][k] = 0;
                    }
                    board[i][targetCol] = a * 8;
                    score += a * 8;
                    changed = true;
                }
            }
        }

        // Check for vertical four
        for (int j = 0; j < SIZE; j++) {
            for (int i = 0; i <= SIZE - 4; i++) {
                int a = board[i][j];
                int b = board[i + 1][j];
                int c = board[i + 2][j];
                int d = board[i + 3][j];
                if (a != 0 && a == b && b == c && c == d) {
                    int targetRow = (lastDropCol >= i && lastDropCol <= i + 3) ? lastDropCol : i + 2;
                    for (int k = i; k <= i + 3; k++) {
                        if (k != targetRow) board[k][j] = 0;
                    }
                    board[targetRow][j] = a * 8;
                    score += a * 8;
                    changed = true;
                }
            }
        }

        // Check for side-top merges
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] != 0) {
                    if (checkSideTopMerge(i, j, board[i][j])) {
                        changed = true;
                    }
                }
            }
        }

        // Check for T-shape merges
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] != 0) {
                    if (checkTShapeMerge(i, j, board[i][j])) {
                        changed = true;
                    }
                }
            }
        }

        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j <= SIZE - 3; j++) {
                int a = board[i][j];
                int b = board[i][j + 1];
                int c = board[i][j + 2];
                if (a != 0 && a == b && b == c) {
                    int val = a;
                    int targetCol = j + 1;
                    if (lastDropCol >= j && lastDropCol <= j + 2) targetCol = lastDropCol;
                    board[i][targetCol] = val * 4;
                    for (int k = j; k <= j + 2; k++) if (k != targetCol) board[i][k] = 0;
                    score += board[i][targetCol];
                    changed = true;
                    setColor(14);
                    cout << "\nBONUS! Horizontal three merge of " << val << " found! Next number set to " << val << "!\n";
                    setColor(7);
                    nextNumber = val;
                }
            }
        }

        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE - 1; j++) {
                if (board[i][j] != 0 && board[i][j] == board[i][j + 1]) {
                    if (lastDropCol >= 0 && (j == lastDropCol || j + 1 == lastDropCol)) {
                        int targetCol = (j == lastDropCol) ? j : j + 1;
                        int otherCol = (targetCol == j) ? j + 1 : j;
                        board[i][targetCol] *= 2;
                        score += board[i][targetCol];
                        board[i][otherCol] = 0;
                    } else {
                        board[i][j] *= 2;
                        score += board[i][j];
                        board[i][j + 1] = 0;
                    }
                    changed = true;
                }
            }
        }

        for (int j = 0; j < SIZE; j++) {
            for (int i = 0; i <= SIZE - 3; i++) {
                int a = board[i][j];
                int b = board[i + 1][j];
                int c = board[i + 2][j];
                if (a != 0 && a == b && b == c) {
                    int val = a;
                    int targetRow = i + 2;
                    board[targetRow][j] = val * 4;
                    for (int k = i; k <= i + 2; k++) if (k != targetRow) board[k][j] = 0;
                    score += board[targetRow][j];
                    changed = true;
                    setColor(14);
                    cout << "\nBONUS! Vertical three merge of " << val << " found! Next number set to " << val << "!\n";
                    setColor(7);
                    nextNumber = val;
                }
            }
        }

        for (int j = 0; j < SIZE; j++) {
            for (int i = SIZE - 1; i > 0; i--) {
                if (board[i][j] != 0 && board[i][j] == board[i - 1][j]) {
                    board[i][j] *= 2;
                    score += board[i][j];
                    board[i - 1][j] = 0;
                    changed = true;
                }
            }
        }

        return changed;
    }

    void settle() {
        for (int col = 0; col < SIZE; col++) {
            vector<int> tmp;
            for (int row = SIZE - 1; row >= 0; row--) {
                if (board[row][col] != 0)
                    tmp.push_back(board[row][col]);
            }
            int rowIdx = SIZE - 1;
            for (int val : tmp) {
                board[rowIdx--][col] = val;
            }
            while (rowIdx >= 0) {
                board[rowIdx--][col] = 0;
            }
        }
    }

    void autoMerge() {
        bool changed;
        do {
            settle();
            changed = mergeOnce();
            settle();
        } while (changed);
    }

    void checkTriangles() {
        bool bonusGiven = false;
        for (int i = 0; i < SIZE - 1 && !bonusGiven; i++) {
            for (int j = 0; j < SIZE - 1 && !bonusGiven; j++) {
                int val = board[i][j];
                if (val == 0) continue;
                if (board[i][j + 1] == val && board[i + 1][j] == val) {
                    setColor(14);
                    cout << "\nBONUS! Upper Triangle of " << val << " found! Next number set to " << val << "!\n";
                    setColor(7);
                    nextNumber = val;
                    bonusGiven = true;
                }
                else if (board[i + 1][j] == val && board[i + 1][j + 1] == val) {
                    setColor(14);
                    cout << "\nBONUS! Lower Triangle of " << val << " found! Next number set to " << val << "!\n";
                    setColor(7);
                    nextNumber = val;
                    bonusGiven = true;
                }
            }
        }
    }

    void showGameOverScreen(bool fromTopMismatch) {
        printBoard();
        setColor(12);
        cout << "\nGame Over!";
        if (fromTopMismatch) {
            cout << " (Top cell blocked by a different number)";
        }
        setColor(7);
        cout << "\nFinal Score: " << score << endl;
        int high = getHighScore();
        cout << "Highest Score so far: " << high << endl;
        if (score > high) {
            cout << "NEW HIGH SCORE! !!" << endl;
        }
        saveScore();
        cout << "Press any key to return to menu...";
        _getch();
    }

    int lowestEmptyInColumn(int col) {
        for (int i = SIZE - 1; i >= 0; --i) {
            if (board[i][col] == 0) return i;
        }
        return -1;
    }

    static int rollRandomTile() {
        int r = (rand() % 5 + 1);
        return (1 << r);
    }

    void playGame() {
        score = 0;
        board = vector<vector<int>>(SIZE, vector<int>(SIZE, 0));
        srand((unsigned)time(0));
        launcherNumber = rollRandomTile();
        nextNumber = rollRandomTile();
        selectedColumn = 0;
        lastDropCol = -1;
        hideCursor(true);

        while (true) {
            printBoard();
            if (isGameOver()) {
                showGameOverScreen(false);
                break;
            }

            int input = _getch();
            if (input == 224) {
                input = _getch();
                switch (input) {
                case 75: // LEFT
                    if (selectedColumn > 0) selectedColumn--;
                    break;
                case 77: // RIGHT
                    if (selectedColumn < SIZE - 1) selectedColumn++;
                    break;
                case 80: { // DOWN
                    int topVal = board[0][selectedColumn];
                    if (topVal != 0 && topVal != launcherNumber) {
                        showGameOverScreen(true);
                        return;
                    }

                    int insertRow = lowestEmptyInColumn(selectedColumn);
                    if (insertRow != -1) {
                        board[insertRow][selectedColumn] = launcherNumber;
                        lastDropCol = selectedColumn;
                        autoMerge();
                        launcherNumber = nextNumber;
                        nextNumber = rollRandomTile();
                    } else {
                        if (topVal == launcherNumber) {
                            board[0][selectedColumn] *= 2;
                            score += board[0][selectedColumn];
                            lastDropCol = selectedColumn;
                            autoMerge();
                            launcherNumber = nextNumber;
                            nextNumber = rollRandomTile();
                        } else {
                            showGameOverScreen(true);
                            return;
                        }
                    }
                    checkTriangles();
                    break;
                }
                }
            }
            else if (input == 'Q' || input == 'q') {
                break;
            }
        }
        hideCursor(false);
    }
};

class GameMenu {
private:
    GameBoard& game;

public:
    GameMenu(GameBoard& g) : game(g) {}

    void instructions() {
        game.clearConsole();
        cout << "-------- Instructions --------" << endl;
        cout << "1. Use LEFT/RIGHT Arrow Keys to choose column." << endl;
        cout << "2. Press DOWN Arrow to drop the number (the launcher number is used)." << endl;
        cout << "3. After each drop, the staged 'Next Number' moves to the launcher, and a new random number is staged." << endl;
        cout << "4. Same numbers merge automatically after each drop." << endl;
        cout << "5. Horizontal merges keep result in the column you dropped into." << endl;
        cout << "6. If you form a triangle (upper/lower), you get a bonus Next Number!" << endl;
        cout << "7. Three identical numbers in a straight line merge into one cell with value = 4 * original." << endl;
        cout << "8. T-shape merges: one above + two sides OR one below + two sides merge to 8 * value with bonus; for three in T, merge to 4 * value with bonus." << endl;
        cout << "9. Side-Top merges: numbers on side and top merge into 4x value with bonus." << endl;
        cout << "10. Four identical in line merge to 8 * value, no bonus." << endl;
        cout << "11. If the top cell has a DIFFERENT number than your launcher, shooting there ends the game." << endl;
        cout << "12. If the top cell has the SAME number as your launcher and the column is FULL, the top cell doubles." << endl;
        cout << "13. Press 'Q' during the game to quit." << endl;
        cout << "-------------------------------" << endl;
        cout << "Press any key to return to menu...";
        _getch();
    }

    void about() {
        game.clearConsole();
        cout << "-------- About --------" << endl;
        cout << "Number Squeezer " << endl;
        cout << "Made by: Pratik Jung Khatri \n"; 
        cout << "         Januka Jirel" << endl;
        cout << "Console-based number merging game." << endl;
        cout << "Made for bca second semester project." << endl;
        cout << "-------------------------" << endl;
        cout << "Press any key to return to menu...";
        _getch();
    }

    void showScoreHistory() {
        game.clearConsole();
        cout << "-------- Score History --------" << endl;
        ifstream file("score_history.txt");
        if (!file) {
            cout << "No score history found." << endl;
        } else {
            string line;
            while (getline(file, line)) {
                cout << line << endl;
            }
        }
        game.showHighScoreInline();
        cout << "-------------------------------" << endl;
        cout << "Press any key to return to menu...";
        _getch();
    }

    void welcomeScreen() {
        game.clearConsole();
        game.hideCursor(true);
        vector<vector<int>> welcomeBoard(5, vector<int>(5, 0));
        
        game.setColor(14);
        cout << "\n\n\n\t\t    W E L C O M E   T O\n";
        game.setColor(11);
        cout << "\t\t   N U M B E R   S Q U E E Z E R\n";
        game.setColor(7);
        cout << "\t\t   ===========================\n\n";
        
        for (int step = 0; step < 15; step++) {
            game.clearConsole();
            game.setColor(14);
            cout << "\n\n\t\t    W E L C O M E   T O\n";
            game.setColor(11);
            cout << "\t\t   N U M B E R   S Q U E E Z E R\n";
            game.setColor(7);
            cout << "\t\t   ===========================\n\n";
            
            if (step < 10) {
                for (int i = 0; i < 5; i++) {
                    for (int j = 0; j < 5; j++) {
                        if (welcomeBoard[i][j] == 0 && rand() % 3 == 0) {
                            welcomeBoard[i][j] = (step % 5 + 1) * 2;
                        }
                    }
                }
            } else if (step == 10) {
                for (int i = 0; i < 5; i++) {
                    for (int j = 0; j < 4; j++) {
                        if (welcomeBoard[i][j] == welcomeBoard[i][j+1] && welcomeBoard[i][j] > 0) {
                            welcomeBoard[i][j] *= 2;
                            welcomeBoard[i][j+1] = 0;
                        }
                    }
                }
            } else if (step == 11) {
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 5; j++) {
                        if (welcomeBoard[i][j] == welcomeBoard[i+1][j] && welcomeBoard[i][j] > 0) {
                            welcomeBoard[i][j] *= 2;
                            welcomeBoard[i+1][j] = 0;
                        }
                    }
                }
            } else if (step == 12) {
                welcomeBoard[2][2] = 64;
                welcomeBoard[1][2] = 0;
                welcomeBoard[2][1] = 0;
                welcomeBoard[2][3] = 0;
                welcomeBoard[3][2] = 0;
            }
            
            cout << "\t\t      ";
            for (int j = 0; j < 5; j++) cout << "+-----";
            cout << "+\n";
            
            for (int i = 0; i < 5; i++) {
                cout << "\t\t      ";
                for (int j = 0; j < 5; j++) {
                    cout << "|";
                    if (welcomeBoard[i][j] != 0) {
                        game.setColor(game.colorForValue(welcomeBoard[i][j]));
                        cout << setw(5) << welcomeBoard[i][j];
                        game.setColor(7);
                    } else {
                        cout << setw(5) << " ";
                    }
                }
                cout << "|\n";
                cout << "\t\t      ";
                for (int j = 0; j < 5; j++) cout << "+-----";
                cout << "+\n";
            }
            
            cout << "\n\t\t      ";
            game.setColor(10);
            cout << "L O A D I N G";
            for (int i = 0; i <= step % 4; i++) cout << ".";
            for (int i = step % 4; i < 3; i++) cout << " ";
            game.setColor(7);
            
            Sleep(300);
        }
        
        game.clearConsole();
        game.setColor(14);
        cout << "\n\n\n\t\t    W E L C O M E   T O\n";
        game.setColor(11);
        cout << "\t\t   N U M B E R   S Q U E E Z E R\n";
        game.setColor(7);
        cout << "\t\t   ===========================\n\n";
        
        game.setColor(13);
        cout << "\t\t      Match numbers strategically!\n";
        cout << "\t\t      Create merges and get highscore.\n\n";
        game.setColor(7);
        
        cout << "\t\t      Press any key to continue...";
        _getch();
        game.hideCursor(false);
    }

    void run() {
        welcomeScreen();
        while (true) {
            game.clearConsole();
            game.setColor(11);
            cout << "===== Number Squeezer Menu =====" << endl;
             game.setColor(7);
            cout << "1. Play Game" << endl;
            cout << "2. Instructions" << endl;
            cout << "3. About" << endl;
            cout << "4. Score History" << endl;
            cout << "5. Exit" << endl;
            cout << "====================================" << endl;
            cout << "Enter your choice: ";

            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(INT_MAX, '\n');
                cout << "Invalid input. Try again..." << endl;
                Sleep(800);
                continue;
            }

            switch (choice) {
            case 1:
                game.playGame();
                break;
            case 2:
                instructions();
                break;
            case 3:
                about();
                break;
            case 4:
                showScoreHistory();
                break;
            case 5:
                return;
            default:
                cout << "Invalid choice. Try again!" << endl;
                Sleep(800);
            }
        }
    }
};

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return 1;
    
    GameBoard gameBoard;
    GameMenu menu(gameBoard);
    menu.run();
    
    return 0;
}
