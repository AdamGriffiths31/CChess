# Search Improvement Plan

## Current State

The engine has a basic but functional search:

- **Negamax + alpha-beta** with iterative deepening (`Search.cpp:70-113`)
- **Time control** with periodic check every 1024 nodes (`Search.cpp:71-73`)
- **Move ordering**: captures-first via `stable_partition` (`Search.cpp:119-121`)
- **Evaluation**: material + piece-square tables, midgame only (`Eval.cpp`)
- **No transposition table** -- identical positions re-searched millions of times
- **No quiescence search** -- vulnerable to horizon effect (evaluates mid-tactic)
- **No advanced pruning** -- searches many irrelevant branches to full depth

---

## Phase 1: Foundations (Critical -- each item is transformational)

### 1.1 Quiescence Search

**Problem**: Without qsearch the engine evaluates positions mid-capture-sequence. It might think it won a queen when the recapture is one ply beyond the horizon.

**Design**:
- At depth 0, call `quiescence(alpha, beta)` instead of `eval::evaluate()`
- Stand-pat: evaluate statically; if `stand_pat >= beta`, return beta
- Generate captures only and search them recursively
- Requires a `generateCaptures()` method on `MoveGenerator`

**Files**: `Search.cpp`, `MoveGenerator.{h,cpp}`
**Elo**: +200-400 | **Complexity**: Simple

### 1.2 Transposition Table (Zobrist Hashing)

**Problem**: The same position is reached via different move orders. Without a TT, identical subtrees are re-searched, wasting the majority of search effort.

**Design**:
- **ZobristHash**: Random 64-bit values for each `(piece, square)`, side-to-move, castling bits, en-passant file. Position hash = XOR of all. Incremental update in `makeMove`/`unmakeMove`.
- **TTEntry**: `{ uint16_t hashVerify, int16_t score, int16_t staticEval, uint8_t depth, uint8_t bound (EXACT/UPPER/LOWER), Move bestMove }`
- **TranspositionTable**: Fixed-size array, index = `hash % size`. Always-replace or depth-preferred replacement.
- **Probe**: At each node, check TT. If `stored_depth >= depth` and bound is compatible, return stored score (cutoff). Otherwise use stored bestMove for ordering.
- **Store**: After searching a node, write result to TT.

**Files**: New `Zobrist.{h,cpp}`, `TranspositionTable.{h,cpp}`. Modify `Position` or `Board` to maintain incremental hash. Modify `Search.cpp` to probe/store.
**Elo**: +150-300 | **Complexity**: Moderate

### 1.3 MVV-LVA Move Ordering

**Problem**: Current ordering only separates captures from quiets. Within captures, QxP is tried at the same priority as PxQ.

**Design**:
- Score each capture: `victim_value * 10 - attacker_value`
- Sort captures descending by this score
- Ordering priority: **TT move > captures (MVV-LVA) > quiets**

**Files**: `Search.cpp` (replace `orderMoves`)
**Elo**: +50-100 | **Complexity**: Simple

---

## Phase 2: Standard Toolkit (High-impact, builds on Phase 1)

### 2.1 Principal Variation Search (PVS)

**Problem**: With good move ordering, the first move is usually best. Full-window search on later moves is wasteful.

**Design**:
- Search first move with full window `(-beta, -alpha)`
- Search remaining moves with null window `(-alpha-1, -alpha)`
- If null-window search fails high (score > alpha), re-search with full window

**Depends on**: Phase 1 move ordering (TT move + MVV-LVA)
**Files**: `Search.cpp` (modify negamax loop)
**Elo**: +50-100 | **Complexity**: Simple

### 2.2 Killer Moves

**Problem**: Quiet moves have no ordering signal. A move that refuted a sibling node often refutes the current node too.

**Design**:
- `Move killers[MAX_PLY][2]` -- two killer slots per ply
- On beta cutoff by a quiet move, store as killer (shift existing)
- Ordering: TT move > captures (MVV-LVA) > killers > other quiets
- Killers must be verified as pseudo-legal before trying

**Files**: `Search.{h,cpp}`
**Elo**: +40-80 | **Complexity**: Simple

### 2.3 History Heuristic

**Problem**: Beyond killers, quiet moves still lack a general ordering signal across the tree.

**Design**:
- `int history[2][64][64]` indexed by `[color][from][to]`
- On cutoff by quiet move: `history[side][from][to] += depth * depth`
- On searched-but-not-cutoff quiet: apply a malus (negative bonus)
- Sort remaining quiet moves by history score
- Age/decay values periodically to prevent overflow (halve on new search)

**Files**: `Search.{h,cpp}`
**Elo**: +40-80 | **Complexity**: Simple

### 2.4 Null Move Pruning

**Problem**: Many positions are so good that even passing (giving the opponent a free move) still results in a fail-high. Detecting this cheaply avoids searching huge subtrees.

**Design**:
```
if (depth >= 3 && !inCheck && !isPvNode && hasNonPawnMaterial) {
    R = 3 + depth / 4;
    makeNullMove();   // flip side, clear en passant
    score = -negamax(depth - 1 - R, -beta, -beta + 1, ply + 1);
    unmakeNullMove();
    if (score >= beta) return beta;
}
```
- Guard: require non-pawn material (avoids zugzwang in K+P endings)
- Guard: don't allow consecutive null moves (track with bool flag)
- Don't apply in PV nodes

**Depends on**: Reliable eval, `makeNullMove()`/`unmakeNullMove()` on Board
**Files**: `Search.cpp`, `Board.{h,cpp}` (add null move support)
**Elo**: +50-150 | **Complexity**: Moderate

### 2.5 Aspiration Windows

**Problem**: Root search uses full window `(-INF, +INF)`. Previous iteration's score is a good estimate -- narrowing the window causes more cutoffs.

**Design**:
- After depth 1, set `alpha = prevScore - 25`, `beta = prevScore + 25`
- On fail-low or fail-high, widen window (double) and re-search
- Fall back to full window after 2-3 failures

**Files**: `Search.cpp` (`findBestMove` root loop)
**Elo**: +30-60 | **Complexity**: Simple

### 2.6 Check Extensions

**Problem**: Cutting search short while in check misses obvious tactical continuations.

**Design**:
- When the side to move is in check, extend by 1 ply: `depth - 1 + 1`
- Simple and safe; can optionally cap total extensions

**Files**: `Search.cpp`
**Elo**: +30-60 | **Complexity**: Simple

---

## Phase 3: Advanced Pruning (Major Elo, needs solid foundation)

### 3.1 Late Move Reductions (LMR)

**Problem**: Late moves in a well-ordered list are unlikely to be good. Searching them to full depth wastes most of the engine's time.

**Design**:
```
if (moveIndex >= 3 && depth >= 3 && !inCheck && !isCapture && !isPromotion) {
    R = 0.75 + ln(depth) * ln(moveIndex) / 2.25;
    // Reduce less for: killers, good history, PV nodes
    // Reduce more for: bad history, cut nodes
    score = -negamax(depth - 1 - R, -alpha - 1, -alpha, ply + 1);
    if (score > alpha)
        score = -negamax(depth - 1, -beta, -alpha, ply + 1);  // re-search
}
```
- Pre-compute reduction table `LMR[depth][moveIndex]` at startup
- Adjust reductions based on history scores: `R -= history[move] / 8192`

**Depends on**: Good move ordering (TT, MVV-LVA, killers, history). Without it, LMR reduces good moves and weakens play.
**Files**: `Search.{h,cpp}`
**Elo**: +100-200 | **Complexity**: Moderate

### 3.2 Reverse Futility Pruning (Static Null Move Pruning)

**Problem**: At shallow depths, if the static eval is far above beta, searching further is pointless.

**Design**:
```
if (depth <= 6 && !inCheck && !isPvNode) {
    int margin = depth * 80;
    if (staticEval - margin >= beta)
        return staticEval - margin;
}
```

**Files**: `Search.cpp`
**Elo**: +20-40 | **Complexity**: Simple

### 3.3 Futility Pruning

**Problem**: At shallow depths, if static eval + a margin can't reach alpha, quiet moves are futile.

**Design**:
```
if (depth <= 2 && !inCheck) {
    int margin = depth * 150;
    if (staticEval + margin <= alpha && !move.isCapture())
        continue;  // skip this move
}
```

**Files**: `Search.cpp`
**Elo**: +20-40 | **Complexity**: Simple

### 3.4 Delta Pruning (in Quiescence)

**Problem**: In qsearch, searching captures that can't possibly raise alpha wastes time.

**Design**:
```
int delta = pieceValue[capturedPiece] + 200;
if (standPat + delta < alpha) continue;
```

**Files**: `Search.cpp` (qsearch)
**Elo**: +10-25 | **Complexity**: Simple

---

## Phase 4: Refinements (Diminishing returns, polish)

### 4.1 Static Exchange Evaluation (SEE)

Simulates the full sequence of captures on one square to determine net material outcome. Used to prune losing captures in qsearch (don't search QxP if the queen gets recaptured) and to improve move ordering (winning captures before killers before losing captures).

**Elo**: +20-40 | **Complexity**: Moderate-High

### 4.2 Countermove Heuristic

Track which response refuted each opponent move. Table: `countermove[piece][toSquare]`. Use as an ordering hint after killers.

**Elo**: +15-30 | **Complexity**: Simple

### 4.3 Internal Iterative Reductions (IIR)

When no TT move exists at a node, reduce depth by 1. Modern replacement for the more complex IID technique.

**Elo**: +10-20 | **Complexity**: Simple

### 4.4 Razoring

At low depths, if static eval is far below alpha, drop directly into qsearch.

**Elo**: +10-25 | **Complexity**: Simple

### 4.5 Singular Extensions

When the TT move is clearly the only good move (no alternative comes close at reduced depth), extend it by 1 ply. Requires TT with stored scores.

**Elo**: +10-30 | **Complexity**: Moderate

---

## Implementation Order Summary

| #  | Item                         | Phase | Elo       | Effort   | Depends On          |
|----|------------------------------|-------|-----------|----------|---------------------|
| 1  | Quiescence search            | 1     | +200-400  | Simple   | generateCaptures()  |
| 2  | Zobrist hashing + TT         | 1     | +150-300  | Moderate | --                  |
| 3  | MVV-LVA + TT move ordering   | 1     | +50-100   | Simple   | TT                  |
| 4  | PVS                          | 2     | +50-100   | Simple   | move ordering       |
| 5  | Killer moves                 | 2     | +40-80    | Simple   | --                  |
| 6  | History heuristic            | 2     | +40-80    | Simple   | --                  |
| 7  | Null move pruning            | 2     | +50-150   | Moderate | eval, null move API |
| 8  | Aspiration windows           | 2     | +30-60    | Simple   | iterative deepening |
| 9  | Check extensions             | 2     | +30-60    | Simple   | check detection     |
| 10 | LMR                          | 3     | +100-200  | Moderate | #3-6 (ordering)     |
| 11 | Reverse futility pruning     | 3     | +20-40    | Simple   | static eval         |
| 12 | Futility pruning             | 3     | +20-40    | Simple   | static eval         |
| 13 | Delta pruning                | 3     | +10-25    | Simple   | qsearch             |
| 14 | SEE                          | 4     | +20-40    | High     | bitboard attackers  |
| 15 | Countermove heuristic        | 4     | +15-30    | Simple   | --                  |
| 16 | IIR                          | 4     | +10-20    | Simple   | TT                  |
| 17 | Razoring                     | 4     | +10-25    | Simple   | qsearch             |
| 18 | Singular extensions          | 4     | +10-30    | Moderate | TT                  |

**Estimated cumulative Elo gain**: +800-1800 across all phases (depending on tuning and interaction effects).

---

## Benchmarking Strategy

Use the existing STS runner (`StsRunner.cpp`) to measure progress after each item. Record results in `results/sts.md` with a note on which feature was just added. This gives a concrete before/after comparison.

Additionally, consider adding **perft-based regression tests** after any change to move generation, and **self-play matches** (e.g., run the old version vs. the new version at fixed time controls) once the engine is strong enough for meaningful games.

---

## References

- [Chess Programming Wiki](https://www.chessprogramming.org) -- definitive reference
- [Stockfish source](https://github.com/official-stockfish/Stockfish) -- gold standard
- [Ethereal](https://github.com/AndyGrant/Ethereal) -- clean, well-documented
- [Fruit 2.1](https://chessprogramming.org/Fruit) -- popularized LMR ("history pruning")
