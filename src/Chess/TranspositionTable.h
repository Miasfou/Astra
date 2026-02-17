#pragma once
#include "Move.h"
#include <vector>
#include <cstdint>

namespace Chess {

    // 0=Exact, 1=LowerBound (Alpha), 2=UpperBound (Beta)
    enum TTFlag { TT_EXACT, TT_ALPHA, TT_BETA };

    struct TTEntry {
        uint64_t key;
        Move bestMove;
        int score;
        int depth;
        TTFlag flag;
    };

    class TranspositionTable {
    public:
        // 2 Million entries ~ 48MB RAM. Very fast lookup.
        static const int SIZE = 2000000; 
        std::vector<TTEntry> table;

        TranspositionTable() {
            table.resize(SIZE);
            clear();
        }

        void clear() {
            for (auto& entry : table) {
                entry.key = 0;
                entry.depth = -1;
            }
        }

        void store(uint64_t key, int depth, int score, TTFlag flag, Move move) {
            int index = key % SIZE;
            // Always replace if depth is higher, or if same position (update)
            // Simple replacement scheme (Always Replace) is surprisingly effective for simple engines
            table[index] = { key, move, score, depth, flag };
        }

        bool probe(uint64_t key, int depth, int alpha, int beta, int& score, Move& bestMove) {
            int index = key % SIZE;
            const TTEntry& entry = table[index];

            if (entry.key == key) {
                // Return bestMove for ordering even if depth is lower
                bestMove = entry.bestMove; 
                
                if (entry.depth >= depth) {
                    if (entry.flag == TT_EXACT) {
                        score = entry.score;
                        return true;
                    }
                    if (entry.flag == TT_ALPHA && entry.score <= alpha) {
                        score = entry.score;
                        return true;
                    }
                    if (entry.flag == TT_BETA && entry.score >= beta) {
                        score = entry.score;
                        return true;
                    }
                }
            }
            return false;
        }
        
        Move getStoredMove(uint64_t key) {
            int index = key % SIZE;
            if (table[index].key == key) return table[index].bestMove;
            return Move();
        }
    };
}