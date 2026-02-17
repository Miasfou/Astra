#include "Evaluator.h"
#include <algorithm>
#include <cmath>

namespace Chess {

    // Helper for distance calc
    int dist(int a, int b) {
        int r1 = a / 8, c1 = a % 8;
        int r2 = b / 8, c2 = b % 8;
        return std::abs(r1 - r2) + std::abs(c1 - c2);
    }

    int Evaluator::evaluate(const Board& board) {
        // --- OPTIMIZATION 2: Return Incremental Score ---
        // The board now maintains `currentEval` which represents White's advantage (White Material - Black Material).
        // If SideToMove is Black, we negate it.
        
        int score = board.currentEval;
        int finalScore = (board.sideToMove == White) ? score : -score;

        // --- MOP UP HEURISTIC (Endgame) ---
        // Fast endgame logic to force mate when material is high
        
        if (std::abs(score) > 1000) { // If one side has +1000 (~Queen) advantage
             int whiteKingSq = -1, blackKingSq = -1;
             for (int i=0; i<64; ++i) {
                 if (board.squares[i] == (White|King)) whiteKingSq = i;
                 if (board.squares[i] == (Black|King)) blackKingSq = i;
             }

             if (whiteKingSq != -1 && blackKingSq != -1) {
                 int myKing = (board.sideToMove == White) ? whiteKingSq : blackKingSq;
                 int oppKing = (board.sideToMove == White) ? blackKingSq : whiteKingSq;
                 
                 int r = oppKing / 8;
                 int c = oppKing % 8;
                 int distCenter = std::max(3 - r, r - 4) + std::max(3 - c, c - 4);
                 int distKings = dist(myKing, oppKing);
                 
                 // Push winning side's advantage
                 int mopUp = (distCenter * 40) + (14 - distKings) * 10;
                 
                 // Simplified Mop-up application
                 if (finalScore > 0) finalScore += mopUp;
             }
        }

        return finalScore;
    }
}