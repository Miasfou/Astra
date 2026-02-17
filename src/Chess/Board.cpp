#include "Board.h"
#include "Evaluator.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace Chess {

    // Piece IDs for Hashing: 
    // EMPTY=0, W_P=1, W_N=2, W_B=3, W_R=4, W_Q=5, W_K=6, B_P=7, B_N=8, B_B=9, B_R=10, B_Q=11, B_K=12
    uint8_t getGeneratorPiece(int p) {
        if (p == Empty) return 0;
        int type = p & 7;
        int color = p & 24;
        if (color == White) return (uint8_t)type;
        return (uint8_t)(type + 6);
    }

    uint64_t Board::generateZobristKey() const {
        uint64_t h = 14695981039346656037ULL; // FNV Offset Basis
        const uint64_t prime = 1099511628211ULL;

        // 1. Hash 64 Squares
        for (int i = 0; i < 64; i++) {
            h ^= (uint64_t)getGeneratorPiece(squares[i]);
            h *= prime;
        }

        // 2. Hash side to move (White=1, Black=0)
        h ^= (uint64_t)(sideToMove == White ? 1 : 0);
        h *= prime;

        // 3. Hash castling rights (Strict Order: WK, WQ, BK, BQ)
        h ^= (uint64_t)(castlingRights & 1 ? 1 : 0); h *= prime;
        h ^= (uint64_t)(castlingRights & 2 ? 1 : 0); h *= prime;
        h ^= (uint64_t)(castlingRights & 4 ? 1 : 0); h *= prime;
        h ^= (uint64_t)(castlingRights & 8 ? 1 : 0); h *= prime;

        return h;
    }

    Board::Board() { reset(); }
    void Board::initEvaluation() {
        currentEval = 0;
        for (int i = 0; i < 64; ++i) {
            int p = squares[i];
            if (p != Empty) {
                int type = p & 7, color = p & 24;
                int val = Evaluator::getPieceValue(type);
                int pst = Evaluator::getPSTValue(type, color, i);
                if (color == White) currentEval += (val + pst); else currentEval -= (val + pst);
            }
        }
    }
    void Board::reset() {
        squares.fill(Empty);
        const int backRank[] = { Rook, Knight, Bishop, Queen, King, Bishop, Knight, Rook };
        for (int i = 0; i < 8; ++i) {
            squares[i] = White | backRank[i]; squares[i + 8] = White | Pawn;
            squares[i + 48] = Black | Pawn; squares[i + 56] = Black | backRank[i];
        }
        sideToMove = White; castlingRights = 0b1111; enPassantSquare = -1; halfMoveClock = 0; 
        currentZobristKey = generateZobristKey();
        positionHistory.clear(); positionHistory.push_back(currentZobristKey);
        history.clear(); initEvaluation(); 
    }
    void Board::makeMove(const Move& m) { GameState s; s.castlingRights = castlingRights; s.enPassantSquare = enPassantSquare; s.halfMoveClock = halfMoveClock; s.zobristKey = currentZobristKey; int p = squares[m.from], t = p & 7, c = p & 24; if (m.flags == EnPassant) s.capturedPiece = squares[(c == White) ? m.to - 8 : m.to + 8]; else s.capturedPiece = squares[m.to]; history.push_back(s); if (c == White) currentEval -= (Evaluator::getPieceValue(t) + Evaluator::getPSTValue(t, White, m.from)); else currentEval += (Evaluator::getPieceValue(t) + Evaluator::getPSTValue(t, Black, m.from)); if (s.capturedPiece != Empty && m.flags != EnPassant) { int ct = s.capturedPiece & 7, cc = s.capturedPiece & 24; if (cc == White) currentEval -= (Evaluator::getPieceValue(ct) + Evaluator::getPSTValue(ct, White, m.to)); else currentEval += (Evaluator::getPieceValue(ct) + Evaluator::getPSTValue(ct, Black, m.to)); } if (t == Pawn || s.capturedPiece != Empty) halfMoveClock = 0; else halfMoveClock++; squares[m.to] = p; squares[m.from] = Empty; if (m.flags == EnPassant) { int cp = (c == White) ? m.to - 8 : m.to + 8; squares[cp] = Empty; halfMoveClock = 0; int cc = (c == White) ? Black : White; if (cc == White) currentEval -= (Evaluator::getPieceValue(Pawn) + Evaluator::getPSTValue(Pawn, White, cp)); else currentEval += (Evaluator::getPieceValue(Pawn) + Evaluator::getPSTValue(Pawn, Black, cp)); } int ft = t; if (m.flags >= PromoteToQueen && m.flags <= PromoteToKnight) { switch (m.flags) { case PromoteToQueen: ft = Queen; break; case PromoteToRook: ft = Rook; break; case PromoteToBishop: ft = Bishop; break; case PromoteToKnight: ft = Knight; break; } squares[m.to] = c | ft; } if (c == White) currentEval += (Evaluator::getPieceValue(ft) + Evaluator::getPSTValue(ft, White, m.to)); else currentEval -= (Evaluator::getPieceValue(ft) + Evaluator::getPSTValue(ft, Black, m.to)); if (m.flags == Castling) { int rf = -1, rt = -1; if (m.to == 6) { rf = 7; rt = 5; } else if (m.to == 2) { rf = 0; rt = 3; } else if (m.to == 62) { rf = 63; rt = 61; } else if (m.to == 58) { rf = 56; rt = 59; } if (rf != -1) { squares[rt] = squares[rf]; squares[rf] = Empty; if (c == White) { currentEval -= (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, White, rf)); currentEval += (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, White, rt)); } else { currentEval += (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, Black, rf)); currentEval -= (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, Black, rt)); } } } if (t == King) { if (c == White) castlingRights &= ~3; else castlingRights &= ~12; } if (m.from == 0 || m.to == 0) castlingRights &= ~2; if (m.from == 7 || m.to == 7) castlingRights &= ~1; if (m.from == 56 || m.to == 56) castlingRights &= ~8; if (m.from == 63 || m.to == 63) castlingRights &= ~4; if (m.flags == PawnDoublePush) enPassantSquare = (m.from + m.to) / 2; else enPassantSquare = -1; sideToMove = (sideToMove == White) ? Black : White; currentZobristKey = generateZobristKey(); positionHistory.push_back(currentZobristKey); }
    void Board::unmakeMove(const Move& m) { sideToMove = (sideToMove == White) ? Black : White; GameState s = history.back(); history.pop_back(); castlingRights = s.castlingRights; enPassantSquare = s.enPassantSquare; halfMoveClock = s.halfMoveClock; currentZobristKey = s.zobristKey; positionHistory.pop_back(); int mt = squares[m.to] & 7, c = sideToMove; if (c == White) currentEval -= (Evaluator::getPieceValue(mt) + Evaluator::getPSTValue(mt, White, m.to)); else currentEval += (Evaluator::getPieceValue(mt) + Evaluator::getPSTValue(mt, Black, m.to)); int ot = (m.flags >= PromoteToQueen && m.flags <= PromoteToKnight) ? Pawn : mt; squares[m.from] = c | ot; if (m.flags == EnPassant) squares[m.to] = Empty; else squares[m.to] = s.capturedPiece; if (c == White) currentEval += (Evaluator::getPieceValue(ot) + Evaluator::getPSTValue(ot, White, m.from)); else currentEval -= (Evaluator::getPieceValue(ot) + Evaluator::getPSTValue(ot, Black, m.from)); if (m.flags != EnPassant && s.capturedPiece != Empty) { int ct = s.capturedPiece & 7, cc = s.capturedPiece & 24; if (cc == White) currentEval += (Evaluator::getPieceValue(ct) + Evaluator::getPSTValue(ct, White, m.to)); else currentEval -= (Evaluator::getPieceValue(ct) + Evaluator::getPSTValue(ct, Black, m.to)); } if (m.flags == EnPassant) { int cp = (c == White) ? m.to - 8 : m.to + 8; squares[cp] = ((c == White) ? Black : White) | Pawn; if ((c == White ? Black : White) == White) currentEval += (Evaluator::getPieceValue(Pawn) + Evaluator::getPSTValue(Pawn, White, cp)); else currentEval -= (Evaluator::getPieceValue(Pawn) + Evaluator::getPSTValue(Pawn, Black, cp)); } if (m.flags == Castling) { int rf = -1, rt = -1; if (m.to == 6) { rf = 7; rt = 5; } else if (m.to == 2) { rf = 0; rt = 3; } else if (m.to == 62) { rf = 63; rt = 61; } else if (m.to == 58) { rf = 56; rt = 59; } if (rf != -1) { squares[rf] = squares[rt]; squares[rt] = Empty; if (c == White) { currentEval += (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, White, rf)); currentEval -= (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, White, rt)); } else { currentEval -= (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, Black, rf)); currentEval += (Evaluator::getPieceValue(Rook) + Evaluator::getPSTValue(Rook, Black, rt)); } } } }
    bool Board::isSquareAttacked(int sq, int ac) const { int p1 = sq - (ac == White ? 7 : -7), p2 = sq - (ac == White ? 9 : -9); if (p1 >= 0 && p1 < 64 && std::abs((p1 % 8) - (sq % 8)) == 1 && squares[p1] == (ac | Pawn)) return true; if (p2 >= 0 && p2 < 64 && std::abs((p2 % 8) - (sq % 8)) == 1 && squares[p2] == (ac | Pawn)) return true; const int ko[] = { 15, 17, 6, 10, -15, -17, -6, -10 }; for (int o : ko) { int t = sq + o; if (t >= 0 && t < 64 && std::abs((sq % 8) - (t % 8)) <= 2) if (squares[t] == (ac | Knight)) return true; } int km[] = { 8, -8, 1, -1, 9, 7, -9, -7 }; for (int d : km) { int t = sq + d; if (t >= 0 && t < 64 && std::abs((sq % 8) - (t % 8)) <= 1) if (squares[t] == (ac | King)) return true; } int rd[] = { 8, -8, 1, -1 }; for (int d : rd) { int c = sq; while (true) { if ((d == 1 && c % 8 == 7) || (d == -1 && c % 8 == 0)) break; c += d; if (c < 0 || c >= 64) break; if (squares[c] != Empty) { if ((squares[c] & 24) == ac && ((squares[c] & 7) == Rook || (squares[c] & 7) == Queen)) return true; break; } } } int bd[] = { 9, 7, -9, -7 }; for (int d : bd) { int c = sq; while (true) { if ((d == 9 || d == -7) && c % 8 == 7) break; if ((d == 7 || d == -9) && c % 8 == 0) break; c += d; if (c < 0 || c >= 64) break; if (squares[c] != Empty) { if ((squares[c] & 24) == ac && ((squares[c] & 7) == Bishop || (squares[c] & 7) == Queen)) return true; break; } } } return false; }
    bool Board::isInCheck() const { int ks = -1; for (int i = 0; i < 64; ++i) if (squares[i] == (sideToMove|King)) { ks = i; break; } return (ks == -1) ? false : isSquareAttacked(ks, (sideToMove == White) ? Black : White); }
    void Board::generatePseudoLegalMoves(MoveList& moves) { for (int i = 0; i < 64; ++i) { if (squares[i] == Empty || (squares[i] & 24) != sideToMove) continue; switch (squares[i] & 7) { case Pawn: genPawnMoves(moves, i); break; case Knight: genKnightMoves(moves, i); break; case Bishop: genSlidingMoves(moves, i, Bishop); break; case Rook: genSlidingMoves(moves, i, Rook); break; case Queen: genSlidingMoves(moves, i, Queen); break; case King: genKingMoves(moves, i); break; } } }
    MoveList Board::generateLegalMoves() { MoveList pseudo; generatePseudoLegalMoves(pseudo); MoveList legal; for (const auto& m : pseudo) { makeMove(m); int kc = (sideToMove == White) ? Black : White, at = sideToMove, ks = -1; for(int i=0; i<64; ++i) if (squares[i] == (kc | King)) { ks = i; break; } if (ks != -1 && !isSquareAttacked(ks, at)) legal.push_back(m); unmakeMove(m); } return legal; }
    void Board::genPawnMoves(MoveList& moves, int idx) { int fw = (sideToMove == White) ? 8 : -8, dst = idx + fw; if (dst >= 0 && dst < 64 && squares[dst] == Empty) { if (dst / 8 == (sideToMove == White ? 7 : 0)) { moves.emplace_back(idx, dst, PromoteToQueen, 0); moves.emplace_back(idx, dst, PromoteToRook, 0); moves.emplace_back(idx, dst, PromoteToBishop, 0); moves.emplace_back(idx, dst, PromoteToKnight, 0); } else { moves.emplace_back(idx, dst, None, 0); if (idx / 8 == (sideToMove == White ? 1 : 6) && squares[idx + fw * 2] == Empty) moves.emplace_back(idx, idx + fw * 2, PawnDoublePush, 0); } } int cs[] = { (sideToMove == White ? 9 : -9), (sideToMove == White ? 7 : -7) }; for (int o : cs) { int d = idx + o; if (d >= 0 && d < 64 && std::abs((idx % 8) - (d % 8)) == 1) { if (squares[d] != Empty && (squares[d] & 24) != sideToMove) { if (d / 8 == (sideToMove == White ? 7 : 0)) moves.emplace_back(idx, d, PromoteToQueen, squares[d]); else moves.emplace_back(idx, d, None, squares[d]); } else if (d == enPassantSquare) moves.emplace_back(idx, d, EnPassant, squares[d]); } } }
    void Board::genKnightMoves(MoveList& m, int i) { const int o[] = { 15, 17, 6, 10, -15, -17, -6, -10 }; for (int x : o) { int d = i + x; if (d >= 0 && d < 64 && std::abs((i % 8) - (d % 8)) <= 2) if (squares[d] == Empty || (squares[d] & 24) != sideToMove) m.emplace_back(i, d, None, squares[d]); } }
    void Board::genKingMoves(MoveList& m, int i) { int o[] = { 1, -1, 8, -8, 9, -9, 7, -7 }; for (int x : o) { int d = i + x; if (d >= 0 && d < 64 && std::abs((i % 8) - (d % 8)) <= 1) if (squares[d] == Empty || (squares[d] & 24) != sideToMove) m.emplace_back(i, d, None, squares[d]); } if (isInCheck()) return; int en = (sideToMove == White) ? Black : White; if (sideToMove == White) { if ((castlingRights & 1) && squares[5] == Empty && squares[6] == Empty && !isSquareAttacked(5, en) && !isSquareAttacked(6, en)) m.emplace_back(4, 6, Castling, 0); if ((castlingRights & 2) && squares[3] == Empty && squares[2] == Empty && squares[1] == Empty && !isSquareAttacked(3, en) && !isSquareAttacked(2, en)) m.emplace_back(4, 2, Castling, 0); } else { if ((castlingRights & 4) && squares[61] == Empty && squares[62] == Empty && !isSquareAttacked(61, en) && !isSquareAttacked(62, en)) m.emplace_back(60, 62, Castling, 0); if ((castlingRights & 8) && squares[59] == Empty && squares[58] == Empty && squares[57] == Empty && !isSquareAttacked(59, en) && !isSquareAttacked(58, en)) m.emplace_back(60, 58, Castling, 0); } }
    void Board::genSlidingMoves(MoveList& m, int i, int t) { int s = (t == Bishop) ? 4 : 0, e = (t == Rook) ? 4 : 8, os[] = { 8, -8, 1, -1, 9, 7, -9, -7 }; for (int j = s; j < e; ++j) { int o = os[j], c = i; while (true) { if ((o == 1 || o == 9 || o == -7) && c % 8 == 7) break; if ((o == -1 || o == 7 || o == -9) && c % 8 == 0) break; c += o; if (c < 0 || c >= 64) break; if (squares[c] == Empty) m.emplace_back(i, c, None, 0); else { if ((squares[c] & 24) != sideToMove) m.emplace_back(i, c, None, squares[c]); break; } } } }
    bool Board::isMoveLegal(const Move& m) { MoveList ms = generateLegalMoves(); for (const auto& x : ms) if (x.from == m.from && x.to == m.to) return true; return false; }
    bool Board::isDraw() const { return halfMoveClock >= 100 || isThreeFoldRepetition() || isInsufficientMaterial(); }
    bool Board::isThreeFoldRepetition() const { int c = 0; for (int i = (int)positionHistory.size() - 1; i >= 0; i -= 2) if (positionHistory[i] == currentZobristKey) if (++c >= 3) return true; return false; }
    bool Board::isInsufficientMaterial() const { int p = 0; bool wB = false, wN = false, bB = false, bN = false; for (int s : squares) { if (s == Empty) continue; int t = s & 7; if (t == Queen || t == Rook || t == Pawn) return false; p++; int c = s & 24; if (c == White) { if (t == Bishop) wB = true; if (t == Knight) wN = true; } else { if (t == Bishop) bB = true; if (t == Knight) bN = true; } } return p == 2 || (p == 3 && (wB || wN || bB || bN)); }
    void Board::makeNullMove() { GameState s; s.castlingRights = castlingRights; s.enPassantSquare = enPassantSquare; s.halfMoveClock = halfMoveClock; s.capturedPiece = Empty; s.zobristKey = currentZobristKey; history.push_back(s); sideToMove = (sideToMove == White) ? Black : White; enPassantSquare = -1; currentZobristKey = generateZobristKey(); positionHistory.push_back(currentZobristKey); }
    void Board::unmakeNullMove() { GameState s = history.back(); history.pop_back(); castlingRights = s.castlingRights; enPassantSquare = s.enPassantSquare; halfMoveClock = s.halfMoveClock; currentZobristKey = s.zobristKey; positionHistory.pop_back(); sideToMove = (sideToMove == White) ? Black : White; }
    bool Board::hasNonPawnMaterial(int c) const { for (int p : squares) if (p != Empty && (p & 24) == c && (p & 7) != King && (p & 7) != Pawn) return true; return false; }
    std::string Board::getFen() const { std::stringstream ss; for (int r = 7; r >= 0; --r) { int e = 0; for (int c = 0; c < 8; ++c) { int p = squares[r * 8 + c]; if (p == Empty) e++; else { if (e > 0) { ss << e; e = 0; } char ch = " PNBRQK" [p & 7]; if ((p & 24) == Black) ch = tolower(ch); ss << ch; } } if (e > 0) ss << e; if (r > 0) ss << '/'; } ss << (sideToMove == White ? " w " : " b "); std::string cr = ""; if (castlingRights & 1) cr += "K"; if (castlingRights & 2) cr += "Q"; if (castlingRights & 4) cr += "k"; if (castlingRights & 8) cr += "q"; ss << (cr == "" ? "-" : cr) << " "; if (enPassantSquare != -1) ss << Move::indexToString(enPassantSquare); else ss << "-"; ss << " " << halfMoveClock << " 1"; return ss.str(); }
    void Board::setFen(const std::string& f) { squares.fill(Empty); int s = 56, i = 0; for (; i < (int)f.length(); ++i) { char c = f[i]; if (c == ' ') break; if (c == '/') { s -= 16; continue; } if (isdigit(c)) { s += (c - '0'); continue; } int clr = isupper(c) ? White : Black, t = Empty; switch (tolower(c)) { case 'p': t = Pawn; break; case 'n': t = Knight; break; case 'b': t = Bishop; break; case 'r': t = Rook; break; case 'q': t = Queen; break; case 'k': t = King; break; } squares[s++] = clr | t; } i++; sideToMove = (f[i] == 'w') ? White : Black; castlingRights = 0b1111; enPassantSquare = -1; halfMoveClock = 0; currentZobristKey = generateZobristKey(); positionHistory.clear(); positionHistory.push_back(currentZobristKey); initEvaluation(); }
}