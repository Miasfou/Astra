#pragma once

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include "Move.h"

namespace Chess {

    enum PieceType {
        Empty = 0,
        Pawn = 1, Knight = 2, Bishop = 3, Rook = 4, Queen = 5, King = 6
    };

    enum PieceColor {
        White = 8,
        Black = 16
    };

    class Board {
    public:
        std::array<int, 64> squares;
        int sideToMove; 
        uint8_t castlingRights; 
        int enPassantSquare; 
        
        int halfMoveClock; 
        std::vector<uint64_t> positionHistory; 
        uint64_t currentZobristKey;
        
        int currentEval; 

        struct GameState {
            uint8_t castlingRights;
            int enPassantSquare;
            int halfMoveClock; 
            uint8_t capturedPiece;
            uint64_t zobristKey; 
        };
        std::vector<GameState> history;

        Board();

        void reset();
        void setFen(const std::string& fen);
        std::string getFen() const;

        void makeMove(const Move& move);
        void unmakeMove(const Move& move);
        
        void makeNullMove();
        void unmakeNullMove();
        bool hasNonPawnMaterial(int color) const; 

        MoveList generateLegalMoves();
        
        bool isDraw() const;
        bool isThreeFoldRepetition() const;
        bool isFiftyMoveRule() const;
        bool isInsufficientMaterial() const;

        bool isMoveLegal(const Move& move);

        bool isSquareAttacked(int square, int attackerColor) const;
        bool isInCheck() const;

        // --- NEW: Polyglot Compatibility ---
        uint64_t getPolyglotKey() const;

    private:
        void generatePseudoLegalMoves(MoveList& moves);
        void genPawnMoves(MoveList& moves, int square);
        void genKnightMoves(MoveList& moves, int square);
        void genKingMoves(MoveList& moves, int square);
        void genSlidingMoves(MoveList& moves, int square, int type);

        void initZobrist();
        uint64_t generateZobristKey() const;
        void initEvaluation();
    };
}