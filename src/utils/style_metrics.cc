// Implementation of interim Phase 0 metrics (minimal compile-safe version).

#include "utils/style_metrics.h"

#include <array>
#include <string>
#include <vector>

namespace lczero {

namespace {

// Utility: Convert board (side-to-move oriented) to a canonical absolute FEN layer string (piece placement only).
// Simpler: use PositionToFen and take substring until first space.
std::string BoardPlacementFen(const Position& pos) {
  std::string full = PositionToFen(pos);
  auto sp = full.find(' ');
  return full.substr(0, sp);
}

// Returns piece char at file f (0..7), rank r (0..7) from FEN placement (r8 displayed first).
// We map ranks: FEN rank '8' is r=7 down to '1' r=0.
char CharAt(const std::string& placement, int file, int rank) {
  int current_rank = 7;
  size_t i = 0;
  int f = 0;
  while (i < placement.size()) {
    char c = placement[i++];
    if (c == '/') { current_rank--; f = 0; continue; }
    if (isdigit(static_cast<unsigned char>(c))) {
      f += c - '0';
      continue;
    }
    if (current_rank == rank) {
      if (f == file) return c;
    }
    f++;
  }
  return ' '; // empty fallback
}

inline bool IsWhitePawn(char c) { return c == 'P'; }
inline bool IsBlackPawn(char c) { return c == 'p'; }

} // namespace

int ComputeCentralPawnOccupation(const Position& pos) {
  // White pawns on d4/e4 (files 3,4 rank 3), Black pawns on d5/e5 (rank 4)
  auto placement = BoardPlacementFen(pos);
  int count = 0;
  // d4 (file=3, rank=3), e4 (4,3)
  if (IsWhitePawn(CharAt(placement, 3, 3))) ++count;
  if (IsWhitePawn(CharAt(placement, 4, 3))) ++count;
  // d5/e5 black (file 3,4 rank 4)
  if (IsBlackPawn(CharAt(placement, 3, 4))) ++count;
  if (IsBlackPawn(CharAt(placement, 4, 4))) ++count;
  return count;
}

int CountLockedPawnPairs(const Position& pos) {
  // Locked: white pawn on (file f, rank r) and black pawn on (f, r+1)
  auto placement = BoardPlacementFen(pos);
  int locked = 0;
  for (int file = 0; file < 8; ++file) {
    for (int rank = 1; rank < 7; ++rank) { // ranks 1..6 can have a white pawn with a black in front
      char w = CharAt(placement, file, rank);
      char b = CharAt(placement, file, rank + 1);
      if (IsWhitePawn(w) && IsBlackPawn(b)) locked++;
    }
  }
  return locked;
}

int ComputeDistantCentralControl(const Position& /*pos*/) { return 0; }
int ComputeFianchettoDeveloped(const Position& /*pos*/) { return 0; }
int ComputeSpaceImbalance(const Position& /*pos*/) { return 0; }
int ComputePawnBreakDelayStub() { return 0; }

StyleRawMetrics ComputeStyleRawMetrics(const Position& pos) {
  StyleRawMetrics m;
  m.central_pawn_occupation = ComputeCentralPawnOccupation(pos);
  m.locked_pawn_pairs = CountLockedPawnPairs(pos);
  m.distant_central_control = ComputeDistantCentralControl(pos);
  m.fianchetto_developed = ComputeFianchettoDeveloped(pos);
  m.pawn_break_delay = ComputePawnBreakDelayStub();
  m.pawn_break_censored = true; // placeholder until PB delay logic implemented
  m.space_imbalance = ComputeSpaceImbalance(pos);
  return m;
}

} // namespace lczero
