// Style metrics computation (Phase 0 interim implementation).
// NOTE: This header purposefully keeps API minimal until Phase 1 expansion.
// Raw (unnormalized) metrics only; normalization & weighting handled elsewhere.

#pragma once

#include <cstdint>
#include <string>

#include "chess/position.h"
#include "chess/types.h"

namespace lczero {

struct StyleRawMetrics {
  int ply = 0;                       // (optional) game ply if provided externally
  int central_pawn_occupation = 0;    // CO
  int locked_pawn_pairs = 0;          // CL (partial component)
  int distant_central_control = 0;    // DCC (stub Phase0)
  int fianchetto_developed = 0;       // FCH (stub Phase0)
  int pawn_break_delay = 0;           // PB_delay (stub Phase0; horizon+1 when unknown)
  bool pawn_break_censored = true;    // censor flag for PB_delay
  int space_imbalance = 0;            // SI (stub Phase0)
};

// Compute raw metrics for a single position. Lightweight: no heap alloc.
// Computes raw metrics (Phase 0). Only CO & locked are fully implemented; others stubbed.
StyleRawMetrics ComputeStyleRawMetrics(const Position& pos);

// Exposed for unit tests (implemented metrics only):
int ComputeCentralPawnOccupation(const Position& pos);
int CountLockedPawnPairs(const Position& pos);

// Phase 0 stubs (return 0; replaced later)
int ComputeDistantCentralControl(const Position& pos);
int ComputeFianchettoDeveloped(const Position& pos);
int ComputeSpaceImbalance(const Position& pos);
// Pawn break delay placeholder (returns horizon+1, sets censored=true in aggregate call).
int ComputePawnBreakDelayStub();

}  // namespace lczero
