#pragma once

#include <cstdint>

namespace nsm {

/**
 * @brief A single point-in-time network speed measurement.
 *
 * All values are in bytes per second (B/s).  The renderer converts to
 * Kbps / Mbps / Gbps as needed.
 */
struct SpeedSample
{
    double uploadBps   { 0.0 };   ///< Upload speed   in bytes/second
    double downloadBps { 0.0 };   ///< Download speed in bytes/second

    // ── Convenience helpers ───────────────────────────────────────────────────

    /// Convert to kilobytes per second
    static double toKBps(double bps) noexcept { return bps / 1024.0; }

    /// Convert to megabytes per second
    static double toMBps(double bps) noexcept { return bps / (1024.0 * 1024.0); }

    /// Convert to megabits per second (Mbps, the common ISP unit)
    static double toMbps(double bps) noexcept { return (bps * 8.0) / (1024.0 * 1024.0); }
};

} // namespace nsm
