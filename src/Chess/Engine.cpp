#include "Engine.h"
#include <algorithm>
#include <iostream>
#include <cstring> 
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iomanip>
#include <random>

namespace fs = std::filesystem;

namespace Chess {

    const int INF = 50000;
    const int MATE_SCORE = 49000;

    Engine::Engine() : nodesVisited(0), stopSearch(false) {
        std::memset(historyMoves, 0, sizeof(historyMoves));
        for(int i=0; i<64; i++) { killerMoves[i][0] = Move(); killerMoves[i][1] = Move(); }
    }

    std::string Engine::resolveBookPath(const std::string& filename) {
        if (fs::exists(filename)) return fs::absolute(filename).string();
        std::vector<std::string> searchPaths = { ".", "build/Release", "build", "src", ".." };
        for (const auto& path : searchPaths) {
            try {
                fs::path p = fs::path(path) / filename;
                if (fs::exists(p)) return fs::absolute(p).string();
            } catch (...) {}
        }
        return ""; 
    }

    void Engine::initBooks() {
        std::cout << "[Book] Verifying configuration..." << std::endl;
        
        std::vector<std::string> checkList;
        if (activeBook == "auto") {
            checkList = { "book_pro.bin", "book.bin", "book_light.bin", "book_flash.bin", "book_flash_light.bin" };
        } else {
            checkList = { activeBook };
        }

        bool found = false;
        for (const auto& name : checkList) {
            std::string path = resolveBookPath(name);
            if (!path.empty()) {
                std::ifstream file(path, std::ios::binary);
                if (file.is_open()) {
                    file.seekg(0, std::ios::end);
                    long long totalEntries = (long long)file.tellg() / 10;
                    
                    if (activeBook == "auto") {
                        std::cout << "[Book] Auto-selected: " << name << " (" << totalEntries << " positions)" << std::endl;
                    } else {
                        std::cout << "[Book] Manual load: " << name << " (" << totalEntries << " positions) - OK" << std::endl;
                    }
                    found = true;
                    break; 
                }
            }
        }

        if (!found) {
            if (activeBook == "auto") 
                std::cout << "[Book] No standard opening books found in directory." << std::endl;
            else 
                std::cout << "[Book] Error: Could not find file '" << activeBook << "'" << std::endl;
        }
    }

    Move Engine::readBook(const Board& board) {
        std::vector<std::string> searchOrder;
        if (activeBook == "auto") {
            searchOrder = { "book_pro.bin", "book.bin", "book_light.bin", "book_flash.bin", "book_flash_light.bin" };
        } else {
            searchOrder = { activeBook };
        }

        uint64_t targetHash = board.currentZobristKey;

        for (const std::string& rawName : searchOrder) {
            std::string fullPath = resolveBookPath(rawName);
            if (fullPath.empty()) continue; 

            std::ifstream file(fullPath, std::ios::binary);
            if (!file.is_open()) continue;

            file.seekg(0, std::ios::end);
            long long fileSize = (long long)file.tellg();
            long long totalEntries = fileSize / 10;
            
            long long low = 0, high = totalEntries - 1;
            while (low <= high) {
                long long mid = low + (high - low) / 2;
                file.seekg(mid * 10, std::ios::beg);
                
                uint64_t entryHash; 
                file.read((char*)&entryHash, 8); 

                if (targetHash == entryHash) {
                    // Match found, scan backward to start of block
                    long long start = mid;
                    while (start > 0) {
                        file.seekg((start - 1) * 10, std::ios::beg);
                        uint64_t prevH;
                        file.read((char*)&prevH, 8);
                        if (prevH != targetHash) break;
                        start--;
                    }

                    Board temp = board;
                    MoveList legalMoves = temp.generateLegalMoves();
                    std::vector<Move> candidates;

                    long long curr = start;
                    while (curr < totalEntries) {
                        file.seekg(curr * 10, std::ios::beg);
                        uint64_t h;
                        file.read((char*)&h, 8);
                        if (h != targetHash) break;

                        uint16_t rawMove;
                        file.read((char*)&rawMove, 2);

                        int b_from = rawMove & 0x3F;
                        int b_to = (rawMove >> 6) & 0x3F;
                        int b_promo = (rawMove >> 12) & 0xF;

                        for(const auto& lm : legalMoves) {
                            if (lm.from == b_from && lm.to == b_to) {
                                if (b_promo > 0) {
                                    int pType = 0;
                                    if (lm.flags == PromoteToQueen) pType = 4;
                                    else if (lm.flags == PromoteToRook) pType = 3;
                                    else if (lm.flags == PromoteToBishop) pType = 2;
                                    else if (lm.flags == PromoteToKnight) pType = 1;
                                    if (pType != b_promo) continue;
                                }
                                candidates.push_back(lm);
                                break; 
                            }
                        }
                        curr++;
                    }

                    if (!candidates.empty()) {
                        static std::random_device rd;
                        static std::mt19937 gen(rd());
                        std::uniform_int_distribution<> dis(0, candidates.size() - 1);
                        return candidates[dis(gen)];
                    }
                    break; 
                }
                else if (targetHash < entryHash) high = mid - 1; 
                else low = mid + 1;
            }
        }
        return Move(); 
    }

    void Engine::checkTime() { if ((nodesVisited & 2047) == 0) { auto now = std::chrono::high_resolution_clock::now(); long long el = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count(); if (el >= (long long)stopTimeMs) stopSearch = true; } }

    void Engine::orderMoves(MoveList& moves, const Board& board, const Move& ttMove, int depth) {
        int scores[256]; int count = (int)moves.size();
        for (int i = 0; i < count; ++i) {
            const Move& m = moves[i]; int s = 0;
            if (!ttMove.isNone() && m.from == ttMove.from && m.to == ttMove.to) s = 2000000;
            else if (board.squares[m.to] != Empty) { int v = board.squares[m.to] & 7, a = board.squares[m.from] & 7; s = 100000 + (v * 10 - a); }
            else if (m.from == killerMoves[depth][0].from && m.to == killerMoves[depth][0].to) s = 90000;
            else if (m.from == killerMoves[depth][1].from && m.to == killerMoves[depth][1].to) s = 80000;
            else { int p = board.squares[m.from], pi = ((p & 24) == White) ? (p & 7) : (p & 7) + 6; if (pi > 0 && pi < 13) s = historyMoves[pi][m.to]; }
            scores[i] = s;
        }
        for (int i = 0; i < count; ++i) { for (int j = i + 1; j < count; ++j) { if (scores[j] > scores[i]) { std::swap(scores[i], scores[j]); std::swap(moves[i], moves[j]); } } }
    }

    int Engine::quiescence(Board& b, int a, int be) {
        checkTime(); if (stopSearch) return 0; nodesVisited++; bool in = b.isInCheck();
        if (!in) { int sp = Evaluator::evaluate(b); if (sp >= be) return be; if (a < sp) a = sp; }
        auto ms = b.generateLegalMoves(); orderMoves(ms, b, Move(), 0); bool fl = false;
        for (const auto& m : ms) { if (!in) if (b.squares[m.to] == Empty && m.flags != EnPassant && m.flags < PromoteToQueen) continue; b.makeMove(m); int s = -quiescence(b, -be, -a); b.unmakeMove(m); if (stopSearch) return 0; fl = true; if (s >= be) return be; if (s > a) a = s; }
        if (in && !fl) return -MATE_SCORE; return a;
    }

    int Engine::minimax(Board& b, int d, int a, int be, bool an) {
        if (stopSearch) return 0; if (b.isDraw()) return 0;
        int s = 0; Move bm; if (tt.probe(b.currentZobristKey, d, a, be, s, bm)) if (!bm.isNone()) return s; 
        if (d <= 0) return quiescence(b, a, be);
        nodesVisited++; bool in = b.isInCheck(); if (in) d++;
        if (!in && d <= 3) { int se = Evaluator::evaluate(b), rm = 300 * d; if (se + rm < a) { int qs = quiescence(b, a, be); if (qs < a) return qs; } }
        if (an && !in && d >= 3 && b.hasNonPawnMaterial(b.sideToMove)) { int R = (d > 6) ? 3 : 2; b.makeNullMove(); int v = -minimax(b, d - 1 - R, -be, -be + 1, false); b.unmakeNullMove(); if (stopSearch) return 0; if (v >= be) return be; }
        auto ms = b.generateLegalMoves(); if (ms.empty()) return in ? -MATE_SCORE + (100 - d) : 0;
        Move tm = tt.getStoredMove(b.currentZobristKey); orderMoves(ms, b, tm, d);
        int bs = -INF; Move fbm; TTFlag fl = TT_ALPHA; int msr = 0;
        for (const auto& m : ms) {
            b.makeMove(m); msr++; int v;
            if (msr > 1 && d > 2 && !in && b.squares[m.to] == Empty && m.flags < PromoteToQueen) { int r = (msr > 10) ? 3 : (msr > 4 ? 2 : 1); v = -minimax(b, d - 1 - r, -a - 1, -a, true); if (v > a) v = -minimax(b, d - 1, -be, -a, true); }
            else { if (msr == 1) v = -minimax(b, d - 1, -be, -a, true); else { v = -minimax(b, d - 1, -a - 1, -a, true); if (v > a) v = -minimax(b, d - 1, -be, -a, true); } }
            b.unmakeMove(m); if (stopSearch) return 0;
            if (v > bs) { bs = v; fbm = m; if (v > a) { a = v; fl = TT_EXACT; } }
            if (a >= be) { fl = TT_BETA; if (b.squares[m.to] == Empty) { killerMoves[d][1] = killerMoves[d][0]; killerMoves[d][0] = m; int p = b.squares[m.from], pi = ((p & 24) == White) ? (p & 7) : (p & 7) + 6; if(pi > 0 && pi < 13) { historyMoves[pi][m.to] += d * d; if (historyMoves[pi][m.to] > 1000000) for(int k=0; k<13; k++) for(int st=0; st<64; st++) historyMoves[k][st] /= 2; } } break; }
        }
        if (!stopSearch) tt.store(b.currentZobristKey, d, bs, fl, fbm); return bs;
    }

    Move Engine::search(Board& b, int md, int mt) {
        Move bm = readBook(b); 
        if (!bm.isNone()) {
            std::cout << "info string book move " << bm.toUci() << " played" << std::endl;
            return bm;
        }

        nodesVisited = 0; stopSearch = false; startTime = std::chrono::high_resolution_clock::now(); stopTimeMs = mt;
        Move gbm = Move(); MoveList lm = b.generateLegalMoves(); if (lm.empty()) return Move();
        for (int d = 1; d <= md; ++d) {
            int s = minimax(b, d, -INF, INF, true); if (stopSearch) break;
            Move tm = tt.getStoredMove(b.currentZobristKey); bool iv = false;
            for (const auto& x : lm) if (x.from == tm.from && x.to == tm.to) { iv = true; gbm = x; break; }
            if (!iv && gbm.isNone()) gbm = lm[0];
            if (iv) std::cout << "info depth " << d << " score cp " << s << " nodes " << nodesVisited << " pv " << gbm.toUci() << std::endl;
            if (std::abs(s) > MATE_SCORE - 1000) break;
        }
        if (gbm.isNone() && !lm.empty()) gbm = lm[0]; return gbm;
    }
}