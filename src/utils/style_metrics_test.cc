// Basic unit tests for style metrics (Phase 0).

#include "gtest/gtest.h"

#include "chess/position.h"
#include "utils/style_metrics.h"

using namespace lczero;

namespace {

Position PositionFromFen(const std::string& fen) {
  Position pos;
  pos.SetFromFen(fen);
  return pos;
}

TEST(StyleMetricsTest, InitialPosition) {
  auto pos = PositionFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  auto m = ComputeStyleRawMetrics(pos);
  EXPECT_EQ(m.central_pawn_occupation, 0);
  EXPECT_EQ(m.locked_pawn_pairs, 0);
  EXPECT_EQ(m.fianchetto_developed, 0);
}

TEST(StyleMetricsTest, SimpleCentralOccupation) {
  // After 1.d4 e5 2.e4 (illegal sequence? We'll craft FEN directly with pawns on d4,e5,e4)
  auto pos = PositionFromFen("rnbqkbnr/pppp1ppp/8/4p3/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 3");
  auto m = ComputeStyleRawMetrics(pos);
  EXPECT_EQ(m.central_pawn_occupation, 2) << "White pawns on d4,e4 count";
}

TEST(StyleMetricsTest, LockedPawns) {
  // Locked structure: white pawn d4 black pawn d5.
  auto pos = PositionFromFen("rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 3");
  auto m = ComputeStyleRawMetrics(pos);
  EXPECT_EQ(m.locked_pawn_pairs, 1);
}

}  // namespace
