#pragma once
#include "Board.h"
#include "Move.h"
#include "Evaluator.h"
#include "TranspositionTable.h"
#include <vector>
#include <chrono>
#include <string>

namespace Chess {

    class Engine {
    public:
        Engine();
        
        // Call this on startup to verify books
        void initBooks();

        Move search(Board& board, int maxDepth, int maxTimeMs);
        void clearTT() { tt.clear(); }
        
        Move readBook(const Board& board);

        std::string activeBook = "auto"; 

    private:
        int minimax(Board& board, int depth, int alpha, int beta, bool allowNullMove);
        int quiescence(Board& board, int alpha, int beta);
        void orderMoves(MoveList& moves, const Board& board, const Move& ttMove, int depth);

        std::string resolveBookPath(const std::string& filename);

        TranspositionTable tt;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        int stopTimeMs;
        bool stopSearch;
        uint64_t nodesVisited;
        
        Move killerMoves[64][2];
        int historyMoves[13][64];

        void checkTime();
    };
}