// Implementation of style metrics (Phase 0 skeleton).

#include "utils/style_metrics.h"

#include <array>
#include <vector>

#include "chess/position.h"

namespace lczero {

namespace {

// Central squares.
constexpr Square kCentralSquares[4] = {SQ_D4, SQ_E4, SQ_D5, SQ_E5};

int PieceAttackWeight(PieceType pt) {
  switch (pt) {
    case PT_PAWN: return 1;
    case PT_KNIGHT:
    case PT_BISHOP: return 2;
    case PT_ROOK: return 2;
    case PT_QUEEN: return 3;
    case PT_KING: return 0; // ignore king for distant control
    default: return 0;
  }
}

bool IsFianchettoSquare(Square sq) {
  return sq == SQ_G2 || sq == SQ_B2 || sq == SQ_G7 || sq == SQ_B7;
}

bool HasHomePawnForFianchetto(const Position& pos, Square bishop_sq) {
  // For g2 bishop need pawn at g2? Actually typical structure is pawn at g2 and bishop on g2 after pawn moved from g2 to g3? Simpler early heuristic:
  // We'll require bishop on g2 with pawn still on g3 OR g2? Too complex; Phase 0 skeleton: just count bishop on g2/b2/g7/b7.
  (void)pos; (void)bishop_sq;
  return true; // placeholder refinement Phase 1.
}

}  // namespace

int ComputeCentralPawnOccupation(const Position& pos) {
  int count = 0;
  for (Square sq : {SQ_E4, SQ_D4}) {
    if (pos.GetPieceOnSquare(sq) == MakePiece(WHITE, PT_PAWN)) count++;
  }
  for (Square sq : {SQ_E5, SQ_D5}) {
    if (pos.GetPieceOnSquare(sq) == MakePiece(BLACK, PT_PAWN)) count++;
  }
  return count;
}

int CountLockedPawnPairs(const Position& pos) {
  int locked = 0;
  // Iterate all files, ranks where a white pawn has a black pawn directly in front.
  for (int sq = SQ_A2; sq <= SQ_H7; ++sq) { // iterate plausible white pawn origins.
    Piece p = pos.GetPieceOnSquare(static_cast<Square>(sq));
    if (p == MakePiece(WHITE, PT_PAWN)) {
      Square forward = static_cast<Square>(sq + 8); // one rank up (towards black)
      if (forward <= SQ_H8 && pos.GetPieceOnSquare(forward) == MakePiece(BLACK, PT_PAWN)) {
        locked++;
      }
    }
  }
  return locked;
}

int ComputeDistantCentralControl(const Position& pos) {
  int total = 0;
  for (Color c : {WHITE, BLACK}) {
    for (Square sq = SQ_A1; sq <= SQ_H8; sq = static_cast<Square>(sq + 1)) {
      Piece piece = pos.GetPieceOnSquare(sq);
      if (piece == PIECE_EMPTY) continue;
      if (GetColor(piece) != c) continue;
      PieceType pt = GetPieceType(piece);
      int w = PieceAttackWeight(pt);
      if (w == 0) continue;
      // For simplicity Phase 0: naive check using position's PseudoLegalMoves? Instead we approximate: if piece could move like its pattern.
      // Placeholder: count presence if piece is itself on a central square OR (pawn) controlling central square by rule.
      // TODO Phase 1: replace with true attack map (bitboards) for accuracy.
      for (Square cs : kCentralSquares) {
        if (pt == PT_PAWN) {
          int dir = (c == WHITE) ? 8 : -8; // forward increment.
          // Pawn attacks differ by diagonal; approximate: white pawn on file +/-1 rank -1 from cs
          if (c == WHITE) {
            if (sq == cs - 7 || sq == cs - 9) total += w; // from which it attacks cs
          } else {
            if (sq == cs + 7 || sq == cs + 9) total += w;
          }
        } else {
          // Very rough heuristic: if taxicab distance <=2 treat as controlling central square.
          int file_sq = sq % 8; int rank_sq = sq / 8;
          int file_cs = cs % 8; int rank_cs = cs / 8;
          int manhattan = (file_sq > file_cs ? file_sq - file_cs : file_cs - file_sq) +
                          (rank_sq > rank_cs ? rank_sq - rank_cs : rank_cs - rank_sq);
          if (manhattan <= 2) total += w; // placeholder
        }
      }
    }
  }
  return total;
}

int ComputeFianchettoDeveloped(const Position& pos) {
  int count = 0;
  for (Square sq = SQ_A1; sq <= SQ_H8; sq = static_cast<Square>(sq + 1)) {
    if (!IsFianchettoSquare(sq)) continue;
    Piece p = pos.GetPieceOnSquare(sq);
    if (p == PIECE_EMPTY) continue;
    if (GetPieceType(p) == PT_BISHOP && HasHomePawnForFianchetto(pos, sq)) count++;
  }
  return count;
}

bool HasEarlyCentralPawnContact(const Position& pos) {
  // Detect white pawn on d/e file directly facing black pawn.
  for (Square w_sq = SQ_D2; w_sq <= SQ_E7; w_sq = static_cast<Square>(w_sq + 1)) {
    Piece pw = pos.GetPieceOnSquare(w_sq);
    if (pw != MakePiece(WHITE, PT_PAWN)) continue;
    Square forward = static_cast<Square>(w_sq + 8);
    if (forward <= SQ_H8 && (w_sq % 8 == 3 || w_sq % 8 == 4)) { // file d=3, e=4
      if (pos.GetPieceOnSquare(forward) == MakePiece(BLACK, PT_PAWN)) return true;
    }
  }
  return false;
}

StyleRawMetrics ComputeStyleRawMetrics(const Position& pos) {
  StyleRawMetrics m;
  m.central_pawn_occupation = ComputeCentralPawnOccupation(pos);
  m.locked_pawn_pairs = CountLockedPawnPairs(pos);
  m.distant_central_control = ComputeDistantCentralControl(pos);
  m.fianchetto_developed = ComputeFianchettoDeveloped(pos);
  m.early_pawn_contact_created = HasEarlyCentralPawnContact(pos) ? 1 : 0;
  return m;
}

}  // namespace lczero
