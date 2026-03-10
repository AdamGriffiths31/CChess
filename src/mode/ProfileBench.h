#ifndef CCHESS_PROFILEBENCH_H
#define CCHESS_PROFILEBENCH_H

namespace cchess {

// Non-interactive benchmark mode for profiling.
// Usage: cchess --bench [time_ms]
//   time_ms  Search time per position in milliseconds (default: 30000)
//
// Runs the first position from STS1.epd (or Kiwipete as fallback) for the
// specified duration and prints NPS/depth stats. Designed to be driven
// directly from the command line without interactive prompts so it works
// cleanly with LD_PRELOAD profilers.
class ProfileBench {
public:
    static void run(int searchTimeMs = 30000);
};

}  // namespace cchess

#endif  // CCHESS_PROFILEBENCH_H
