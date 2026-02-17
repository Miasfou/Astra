#pragma once

#include <cstdint>
#include <string>

namespace Chess {

    enum MoveFlag : uint8_t {
        None = 0,
        EnPassant = 1,
        Castling = 2,
        PromoteToQueen = 3,
        PromoteToRook = 4,
        PromoteToBishop = 5,
        PromoteToKnight = 6,
        PawnDoublePush = 7
    };

    struct Move {
        uint8_t from;
        uint8_t to;
        uint8_t flags;
        uint8_t capturedPiece; 

        Move() : from(0), to(0), flags(None), capturedPiece(0) {}
        Move(uint8_t f, uint8_t t, uint8_t fl = None, uint8_t cp = 0) 
            : from(f), to(t), flags(fl), capturedPiece(cp) {}

        bool isNone() const { return from == 0 && to == 0; }
        
        static std::string indexToString(int idx) {
            std::string s = "";
            s += (char)('a' + (idx % 8));
            s += (char)('1' + (idx / 8));
            return s;
        }

        std::string toUci() const {
            std::string s = indexToString(from) + indexToString(to);
            if (flags == PromoteToQueen) s += 'q';
            else if (flags == PromoteToRook) s += 'r';
            else if (flags == PromoteToBishop) s += 'b';
            else if (flags == PromoteToKnight) s += 'n';
            return s;
        }
    };

    // --- OPTIMIZATION 1: Fixed Size Move List (No malloc/free) ---
    struct MoveList {
        static const int CAPACITY = 512; // Increased to prevent overflow
        Move moves[CAPACITY]; 
        int count = 0;
        
        void push_back(const Move& m) { 
            if (count < CAPACITY) moves[count++] = m; 
        }
        
        // Emplace back support
        template<typename... Args>
        void emplace_back(Args&&... args) {
            if (count < CAPACITY) moves[count++] = Move(std::forward<Args>(args)...);
        }

        void clear() { count = 0; }
        bool empty() const { return count == 0; }
        size_t size() const { return count; }

        Move* begin() { return moves; }
        Move* end() { return moves + count; }
        const Move* begin() const { return moves; }
        const Move* end() const { return moves + count; }
        
        Move& operator[](int index) { return moves[index]; }
        const Move& operator[](int index) const { return moves[index]; }
    };
}