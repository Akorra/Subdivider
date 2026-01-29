#pragma once

#include "config.hpp"

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace Subdiv::Diagnostics
{
/**
 * @brief Error severity levels.
 */
enum class ErrorSeverity
{
    WARNING,  // Non-critical issue, operation may continue
    ERROR,    // Critical issue, operation failed
    FATAL     // Unrecoverable error
};

/**
 * @brief Error information structure.
 */
struct ErrorInfo
{
    ErrorSeverity severity;
    std::string   code;        // Machine-readable error code (e.g., "INVALID_VERTEX_INDEX")
    std::string   message;     // Human-readable message
    std::string   context;     // Additional context (e.g., "at face index 42")
    
    ErrorInfo(ErrorSeverity sev, const std::string& c, const std::string& msg, const std::string& ctx = "")
        : severity(sev), code(c), message(msg), context(ctx) {}
};

/**
 * @brief Timing information for a profiled operation.
 */
struct TimingInfo
{
    std::string name;
    double      durationMs;
    size_t      callCount;
    double      minMs;
    double      maxMs;
    double      avgMs;
    
    TimingInfo(const std::string& n = "")
        : name(n), durationMs(0.0), callCount(0), minMs(1e9), maxMs(0.0), avgMs(0.0) {}
    
    void addSample(double ms) {
        durationMs += ms;
        callCount++;
        minMs = std::min(minMs, ms);
        maxMs = std::max(maxMs, ms);
        avgMs = durationMs / callCount;
    }
};

/**
 * @brief Memory tracking information.
 */
struct MemoryInfo
{
    std::string name;
    size_t      allocatedBytes;
    size_t      peakBytes;
    size_t      allocationCount;
    
    MemoryInfo(const std::string& n = "")
        : name(n), allocatedBytes(0), peakBytes(0), allocationCount(0) {}
    
    void recordAllocation(size_t bytes) {
        allocatedBytes += bytes;
        peakBytes = std::max(peakBytes, allocatedBytes);
        allocationCount++;
    }
    
    void recordDeallocation(size_t bytes) {
        allocatedBytes = (bytes > allocatedBytes) ? 0 : allocatedBytes - bytes;
    }
};

/**
 * @brief Scoped timer for automatic profiling.
 */
class ScopedTimer
{
public:

#if SUBDIV_PROFILING_ENABLED
    ScopedTimer(const std::string& name);
    ~ScopedTimer();
#else
    // No-op in non-profiling builds
    ScopedTimer(const std::string&) {}
    ~ScopedTimer() {}
#endif
    
    // Disable copy/move
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    
private:

#if SUBDIV_PROFILING_ENABLED
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
#endif
};

/**
 * @brief Diagnostic context - thread-safe global singleton.
 * 
 * Usage:
 *   // Enable Diagnostics
 *   Context::enable(Context::Mode::ERRORS_AND_PROFILING);
 *   
 *   // Operations automatically log to global context
 *   mesh.addFace(vertices);
 *   
 *   // Get results
 *   if (Context::hasErrors()) {
 *       std::cout << Context::getErrorSummary();
 *   }
 *   
 *   // Disable when done
 *   Context::disable();
 */
class Context
{
public:
    enum class Mode
    {
        DISABLED,              // No diagnostics (default, zero overhead)
        ERRORS_ONLY,           // Only track errors
        ERRORS_AND_PROFILING,  // Track errors and timing
        FULL_DIAGNOSTICS       // Track everything (errors, timing, memory)
    };

    // --- Singleton access ---
    static Context& instance();
    
    // --- Configuration ---
    static void enable(Mode mode = Mode::ERRORS_ONLY);
    static void disable();
    static bool isEnabled();
    static Mode getMode();
    
     // --- Error tracking ---
    static void addError(ErrorSeverity severity, const std::string& code, 
                        const std::string& message, const std::string& context = "");
    
    static bool hasErrors();
    static bool hasWarnings();
    static bool hasFatalErrors();
    
    static const std::vector<ErrorInfo>& getErrors();
    static const ErrorInfo* getLastError();
    
    static std::string getErrorSummary();
    
     // --- Profiling ---
#if SUBDIV_PROFILING_ENABLED
    static void startTimer(const std::string& name);
    static void stopTimer(const std::string& name);
    static void recordTiming(const std::string& name, double durationMs);
    
    static const std::unordered_map<std::string, TimingInfo>& getTimings();
    static std::string getProfilingSummary();
#else
    static void startTimer(const std::string&) {}
    static void stopTimer(const std::string&) {}
    static void recordTiming(const std::string&, double) {}
    
    static std::string getProfilingSummary() { return "Profiling disabled"; }
#endif
    
    // --- Memory tracking ---
#if SUBDIV_MEMORY_TRACKING_ENABLED
    static void recordAllocation(const std::string& category, size_t bytes);
    static void recordDeallocation(const std::string& category, size_t bytes);
    
    static const std::unordered_map<std::string, MemoryInfo>& getMemoryInfo();
    static std::string getMemorySummary();
#else
    static void recordAllocation(const std::string&, size_t) {}
    static void recordDeallocation(const std::string&, size_t) {}
    
    static std::string getMemorySummary() { return "Memory tracking disabled"; }
#endif
    
    // --- General ---
    static void clear();
    static std::string getFullReport();
    
    static constexpr bool isProfilingSupported() { return SUBDIV_PROFILING_ENABLED; }
    static constexpr bool isMemoryTrackingSupported() { return SUBDIV_MEMORY_TRACKING_ENABLED; }
    
    // Delete copy/move
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;
    
private:
    Context() : m_mode(Mode::DISABLED) {}
    ~Context() = default;
    
    Mode m_mode;
    std::mutex m_mutex;  // Thread safety
    
    // Error tracking
    std::vector<ErrorInfo> m_errors;
    
    // Profiling
#if SUBDIV_PROFILING_ENABLED
    std::unordered_map<std::string, TimingInfo> m_timings;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> m_activeTimers;
#endif
    
    // Memory tracking
#if SUBDIV_MEMORY_TRACKING_ENABLED
    std::unordered_map<std::string, MemoryInfo> m_memoryTracking;
#endif
};

/**
 * @brief Result type for operations that can fail.
 */
template<typename T>
class Result
{
public:
    static Result success(T value) {
        Result r;
        r.m_value = std::move(value);
        return r;
    }
    
    static Result error(ErrorSeverity severity, const std::string& code, 
                       const std::string& message, const std::string& context = "") {
        Result r;
        r.m_error = ErrorInfo(severity, code, message, context);
        return r;
    }
    
    bool isOk() const { return m_value.has_value(); }
    bool isError() const { return m_error.has_value(); }
    
    const T& value() const { return m_value.value(); }
    T& value() { return m_value.value(); }
    
    const ErrorInfo& error() const { return m_error.value(); }
    
    T valueOr(const T& defaultValue) const {
        return m_value.value_or(defaultValue);
    }
    
private:
    Result() = default;
    std::optional<T> m_value;
    std::optional<ErrorInfo> m_error;
};

}