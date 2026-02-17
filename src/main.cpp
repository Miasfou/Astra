// ... includes remain same ...
#define NOMINMAX 
#ifdef _WIN32
#include <windows.h>
#endif
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <fstream> 
#include <algorithm>
#include <sstream>
#include <mutex>
#include <atomic>
#include <random>
#include <cmath> 

#if __has_include("Assets.h")
#include "Assets.h"
#define USE_EMBEDDED_ASSETS
#endif

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Chess/Board.h"
#include "Chess/Engine.h"

std::mutex boardMutex;
Chess::Board visBoard;
bool visGameOver = false;
bool whiteIsBot = false;
bool blackIsBot = true; 
int globalDepth = 6;
int playerColor = Chess::White; 
std::atomic<bool> botThinking{false};
std::atomic<bool> botReadyToMove{false};
Chess::Move botMove;
Chess::Engine astraEngine;
std::map<int, GLuint> pieceTextures;
int selectedSquare = -1;
Chess::Move lastIllegalMove;
bool showIllegalArrow = false;

// ... (printLogo, findAssetPath unchanged) ...

void printLogo() {
    std::cout << "╔═══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     ▄▄      ▄▄▄▄▄     ▄▄▄▄▄▄▄    ▄▄▄▄▄▄        ▄▄     ║" << std::endl;
    std::cout << "║   ▄█▀▀█▄   ██▀▀▀▀█▄  █▀▀██▀▀▀▀  █▀██▀▀▀█▄    ▄█▀▀█▄   ║" << std::endl;
    std::cout << "║   ██  ██   ▀██▄  ▄▀     ██        ██▄▄▄█▀    ██  ██   ║" << std::endl;
    std::cout << "║   ██▀▀██     ▀██▄▄      ██        ██▀▀█▄     ██▀▀██   ║" << std::endl;
    std::cout << "║ ▄ ██  ██   ▄   ▀██▄     ██      ▄ ██  ██   ▄ ██  ██   ║" << std::endl;
    std::cout << "║ ▀██▀  ▀█▄█ ▀██████▀     ▀██▄    ▀██▀  ▀██▀ ▀██▀  ▀█▄█ ║" << std::endl;
    std::cout << "║                                                       ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "ASTRA CHESS ENGINE - v1.0.4 Standalone" << std::endl;
    std::cout << "Type 'help' for commands." << std::endl;
}

std::string findAssetPath(const std::string& filename) {
    std::vector<std::string> prefixes = { "", "pieces/", "../pieces/", "../../pieces/", "src/pieces/", "../src/pieces/" };
    for (const auto& prefix : prefixes) {
        std::string fullPath = prefix + filename;
        std::ifstream f(fullPath.c_str());
        if (f.good()) return fullPath;
    }
    return ""; 
}

GLuint loadTexture(const std::string& baseName) {
    unsigned char* data = nullptr;
    int w, h, ch;

#ifdef USE_EMBEDDED_ASSETS
    auto& cache = Assets::GetCache();
    if (cache.count(baseName)) {
        auto& pngData = cache[baseName];
        // Use stb_image to decode the PNG bytes from memory
        data = stbi_load_from_memory(pngData.data(), (int)pngData.size(), &w, &h, &ch, 4);
    }
#endif

    if (!data) {
        std::string path = findAssetPath(baseName);
        if (!path.empty()) {
            data = stbi_load(path.c_str(), &w, &h, &ch, 4);
        }
    }

    if (!data) return 0;

    GLuint tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return tid;
}

void initTextures() {
#ifdef USE_EMBEDDED_ASSETS
    Assets::Load();
#endif
    std::string types[] = { "", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King" };
    for (int i = 1; i <= 6; ++i) {
        pieceTextures[Chess::White | i] = loadTexture("w" + types[i] + ".png");
        pieceTextures[Chess::Black | i] = loadTexture("b" + types[i] + ".png");
    }
}

// ... rest of main.cpp (draw functions, mouse_callback, loop) is unchanged ...
void drawSolidSquare(float x, float y, float w, float h, float r, float g, float b) {
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor3f(r, g, b);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

void drawTexturedSquare(float x, float y, float w, float h, GLuint textureID) {
    if (textureID == 0) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(1.0f, 1.0f, 1.0f); 
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y + h);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + h);
    glEnd();
    glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
}

void drawArrow(float x1, float y1, float x2, float y2, float r, float g, float b) {
    glDisable(GL_TEXTURE_2D);
    glLineWidth(4.0f);
    glColor3f(r, g, b);
    glBegin(GL_LINES); glVertex2f(x1, y1); glVertex2f(x2, y2); glEnd();
    float angle = std::atan2(y2 - y1, x2 - x1);
    float headLen = 0.05f;
    glBegin(GL_TRIANGLES);
    glVertex2f(x2, y2);
    glVertex2f(x2 - headLen * std::cos(angle - 0.5f), y2 - headLen * std::sin(angle - 0.5f));
    glVertex2f(x2 - headLen * std::cos(angle + 0.5f), y2 - headLen * std::sin(angle + 0.5f));
    glEnd();
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        showIllegalArrow = false;
        if (visGameOver || botThinking || botReadyToMove) return;
        std::lock_guard<std::mutex> lock(boardMutex);
        bool isHumanTurn = (visBoard.sideToMove == Chess::White && !whiteIsBot) || (visBoard.sideToMove == Chess::Black && !blackIsBot);
        if (!isHumanTurn) return;
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int w, h; glfwGetWindowSize(window, &w, &h);
        float dim = (float)std::min(w, h);
        float offX = (w - dim) / 2.0f, offY = (h - dim) / 2.0f;
        if (xpos < offX || xpos > offX + dim || ypos < offY || ypos > offY + dim) return;
        int c = (int)((xpos - offX) / (dim / 8.0));
        int r = 7 - (int)((ypos - offY) / (dim / 8.0));
        if (playerColor == Chess::Black) { r = 7 - r; c = 7 - c; }
        int sq = r * 8 + c;
        if (selectedSquare == -1) {
            if (visBoard.squares[sq] != Chess::Empty && (visBoard.squares[sq] & 24) == visBoard.sideToMove) selectedSquare = sq;
        } else {
            auto moves = visBoard.generateLegalMoves();
            bool moved = false;
            for (const auto& m : moves) {
                if (m.from == selectedSquare && m.to == sq) { visBoard.makeMove(m); moved = true; break; }
            }
            selectedSquare = moved ? -1 : (visBoard.squares[sq] != Chess::Empty && (visBoard.squares[sq] & 24) == visBoard.sideToMove ? sq : -1);
        }
    }
}

void renderBoard(int width, int height) {
    std::lock_guard<std::mutex> lock(boardMutex);
    float dim = (float)std::min(width, height);
    float sqX = (dim / 8.0f) / (width / 2.0f), sqY = (dim / 8.0f) / (height / 2.0f);
    float startX = -(dim / width), startY = -(dim / height);
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int drawR = (playerColor == Chess::White) ? r : 7 - r;
            int drawC = (playerColor == Chess::White) ? c : 7 - c;
            float x = startX + drawC * sqX, y = startY + drawR * sqY;
            if ((r * 8 + c) == selectedSquare) drawSolidSquare(x, y, sqX, sqY, 0.7f, 0.8f, 0.3f);
            else if ((r + c) % 2 == 0) drawSolidSquare(x, y, sqX, sqY, 0.45f, 0.55f, 0.65f);
            else drawSolidSquare(x, y, sqX, sqY, 0.9f, 0.9f, 0.95f);
            int p = visBoard.squares[r * 8 + c];
            if (p != Chess::Empty && pieceTextures.count(p)) drawTexturedSquare(x, y, sqX, sqY, pieceTextures[p]);
        }
    }
    if (showIllegalArrow && !lastIllegalMove.isNone()) {
        int f = lastIllegalMove.from; int t = lastIllegalMove.to;
        int r1 = f / 8, c1 = f % 8, r2 = t / 8, c2 = t % 8;
        int drawR1 = (playerColor == Chess::White) ? r1 : 7 - r1;
        int drawC1 = (playerColor == Chess::White) ? c1 : 7 - c1;
        int drawR2 = (playerColor == Chess::White) ? r2 : 7 - r2;
        int drawC2 = (playerColor == Chess::White) ? c2 : 7 - c2;
        float x1 = startX + drawC1 * sqX + sqX / 2.0f, y1 = startY + drawR1 * sqY + sqY / 2.0f;
        float x2 = startX + drawC2 * sqX + sqX / 2.0f, y2 = startY + drawR2 * sqY + sqY / 2.0f;
        drawArrow(x1, y1, x2, y2, 0.9f, 0.1f, 0.1f);
    }
}

void botTask(Chess::Board board, int depth) {
    if (board.halfMoveClock == 0 && board.history.size() < 2) astraEngine.clearTT();
    botMove = astraEngine.search(board, depth, 5000);
    botReadyToMove = true; botThinking = false;
}

void runConsoleSetup() {
    printLogo(); astraEngine.initBooks();
    std::string line;
    while (true) {
        std::cout << "Astra> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "start") break;
        if (line == "help") {
            std::cout << "Commands: set white human/bot, set black human/bot, set depth [1-7], set book [file/auto], status, start\n";
        } else if (line == "status") {
            std::cout << "Config: W=" << (whiteIsBot?"Bot":"User") << " B=" << (blackIsBot?"Bot":"User") << " D=" << globalDepth << " Book=" << astraEngine.activeBook << "\n";
        } else if (line.find("set white ") == 0) { whiteIsBot = (line.substr(10) == "bot"); if (!whiteIsBot) playerColor = Chess::White; }
        else if (line.find("set black ") == 0) { blackIsBot = (line.substr(10) == "bot"); if (!blackIsBot) playerColor = Chess::Black; }
        else if (line.find("set depth ") == 0) { int d = std::stoi(line.substr(10)); if (d >= 1 && d <= 7) globalDepth = d; }
        else if (line.find("set book ") == 0) { astraEngine.activeBook = line.substr(9); astraEngine.initBooks(); }
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001); 
#endif
    runConsoleSetup();
    if (!glfwInit()) {
        std::cout << "CRITICAL: Could not initialize GLFW" << std::endl;
        std::cin.get();
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(800, 800, "Astra Chess", NULL, NULL);
    if (!window) {
        std::cout << "CRITICAL: Could not create Window." << std::endl;
        glfwTerminate();
        std::cin.get();
        return -1;
    }
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
    glfwSetMouseButtonCallback(window, mouse_callback);
    initTextures();
    while (!glfwWindowShouldClose(window)) {
        {
            std::lock_guard<std::mutex> lock(boardMutex);
            if (botReadyToMove) { 
                if (!botMove.isNone()) { if (visBoard.isMoveLegal(botMove)) visBoard.makeMove(botMove); botMove = Chess::Move(); }
                botReadyToMove = false; selectedSquare = -1; 
            }
            visGameOver = visBoard.isDraw() || (visBoard.isInCheck() && visBoard.generateLegalMoves().empty());
            std::string title = "Astra - " + std::string(visBoard.sideToMove == Chess::White ? "White" : "Black");
            if (botThinking) title += " (Thinking...)";
            if (visGameOver) title = "GAME OVER";
            glfwSetWindowTitle(window, title.c_str());
        }
        {
            std::lock_guard<std::mutex> lock(boardMutex);
            bool isBotTurn = (visBoard.sideToMove == Chess::White && whiteIsBot) || (visBoard.sideToMove == Chess::Black && blackIsBot);
            if (!visGameOver && !botThinking && isBotTurn) { botThinking = true; std::thread(botTask, visBoard, globalDepth).detach(); }
        }
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h); glClearColor(0.12f, 0.12f, 0.12f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        renderBoard(w, h); glfwSwapBuffers(window); glfwPollEvents();
    }
    glfwTerminate(); return 0;
}