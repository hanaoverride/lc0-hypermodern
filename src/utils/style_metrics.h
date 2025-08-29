// Style metrics computation (Phase 0) - initial skeleton.
// Calculates hypermodern & closed-style related raw features for a given Position.
// These are raw (unnormalized) values; aggregation & normalization handled in tooling.

#pragma once

#include <cstdint>
#include <string>

#include "chess/position.h"
#include "chess/types.h"

namespace lczero {

struct StyleRawMetrics {
  int ply = 0;                   // current ply in game context (if provided)
  int central_pawn_occupation = 0; // count of own pawns on (e4,d4) or (e5,d5) depending on side to move perspective? Here absolute across both colors.
  int distant_central_control = 0; // aggregated attack weight into {d4,e4,d5,e5}
  int locked_pawn_pairs = 0;       // opposing pawns directly blocking each other
  int fianchetto_developed = 0;    // number of bishops currently on g2,b2,g7,b7 with home pawn (g/h or b/a) still present forming structure
  int early_pawn_contact_created = 0; // flag (0/1) if current position has first central pawn tension already.
};

// Compute raw metrics for a single position. Lightweight: no heap alloc.
StyleRawMetrics ComputeStyleRawMetrics(const Position& pos);

// Helper: returns number of locked pawn pairs (simple file+rank adjacency blocking: pawn directly in front of another of opposite color).
int CountLockedPawnPairs(const Position& pos);

// Helper: central pawn occupation (# of white pawns on e4/d4 + black pawns on e5/d5).
int ComputeCentralPawnOccupation(const Position& pos);

// Helper: distant central control (sum of piece attacks into central squares {d4,e4,d5,e5}). Pawns count 1, knights/bishops 2, rooks 2, queens 3, kings 0.
int ComputeDistantCentralControl(const Position& pos);

// Helper: bishops on fianchetto squares with supporting pawn still on its original square (b-pawn for b2 bishop, g-pawn for g2 bishop, etc.).
int ComputeFianchettoDeveloped(const Position& pos);

// Detect if any central pawn tension (white pawn adjacent forward to black pawn on d/e files) exists.
bool HasEarlyCentralPawnContact(const Position& pos);

}  // namespace lczero
