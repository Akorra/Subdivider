#include "diagnostics/context.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Subdiv::Diagnostics
{

// --- ScopedTimer ---

#if SUBDIV_PROFILING_ENABLED
ScopedTimer::ScopedTimer(const std::string& name)
    : m_name(name)
{
    if (Context::isEnabled() && Context::getMode() != Context::Mode::ERRORS_ONLY) {
        m_start = std::chrono::high_resolution_clock::now();
    }
}

ScopedTimer::~ScopedTimer()
{
    if (Context::isEnabled() && Context::getMode() != Context::Mode::ERRORS_ONLY) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - m_start).count();
        Context::recordTiming(m_name, duration);
    }
}
#endif

// --- Context Singleton ---

Context& Context::instance()
{
    static Context instance;
    return instance;
}

void Context::enable(Mode mode)
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    inst.m_mode = mode;
}

void Context::disable()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    inst.m_mode = Mode::DISABLED;
}

bool Context::isEnabled()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    return inst.m_mode != Mode::DISABLED;
}

Context::Mode Context::getMode()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    return inst.m_mode;
}

// --- Error tracking ---

void Context::addError(ErrorSeverity severity, const std::string& code,
                           const std::string& message, const std::string& context)
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_mode == Mode::DISABLED) return;
    
    inst.m_errors.emplace_back(severity, code, message, context);
}

bool Context::hasErrors()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    return !inst.m_errors.empty();
}

bool Context::hasWarnings()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    for (const auto& err : inst.m_errors) {
        if (err.severity == ErrorSeverity::WARNING) {
            return true;
        }
    }
    return false;
}

bool Context::hasFatalErrors()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    for (const auto& err : inst.m_errors) {
        if (err.severity == ErrorSeverity::FATAL) {
            return true;
        }
    }
    return false;
}

const std::vector<ErrorInfo>& Context::getErrors()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    return inst.m_errors;
}

const ErrorInfo* Context::getLastError()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_errors.empty()) {
        return nullptr;
    }
    return &inst.m_errors.back();
}

std::string Context::getErrorSummary()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_errors.empty()) {
        return "No errors";
    }
    
    std::ostringstream oss;
    int warnings = 0;
    int errors = 0;
    int fatal = 0;
    
    for (const auto& err : inst.m_errors) {
        switch (err.severity) {
            case ErrorSeverity::WARNING: warnings++; break;
            case ErrorSeverity::ERROR: errors++; break;
            case ErrorSeverity::FATAL: fatal++; break;
        }
    }
    
    oss << "=== Error Summary ===\n";
    oss << "Errors: " << errors;
    if (fatal > 0) oss << " (Fatal: " << fatal << ")";
    if (warnings > 0) oss << ", Warnings: " << warnings;
    oss << "\n\n";
    
    for (const auto& err : inst.m_errors) {
        std::string severityStr;
        switch (err.severity) {
            case ErrorSeverity::WARNING: severityStr = "WARNING"; break;
            case ErrorSeverity::ERROR:   severityStr = "ERROR"; break;
            case ErrorSeverity::FATAL:   severityStr = "FATAL"; break;
        }
        
        oss << "[" << severityStr << "] " << err.code << ": " << err.message;
        if (!err.context.empty()) {
            oss << " (" << err.context << ")";
        }
        oss << "\n";
    }
    
    return oss.str();
}

// --- Profiling ---

#if SUBDIV_PROFILING_ENABLED
void Context::startTimer(const std::string& name)
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_mode != Mode::ERRORS_AND_PROFILING && inst.m_mode != Mode::FULL_DIAGNOSTICS) {
        return;
    }
    
    inst.m_activeTimers[name] = std::chrono::high_resolution_clock::now();
}

void Context::stopTimer(const std::string& name)
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_mode != Mode::ERRORS_AND_PROFILING && inst.m_mode != Mode::FULL_DIAGNOSTICS) {
        return;
    }
    
    auto it = inst.m_activeTimers.find(name);
    if (it != inst.m_activeTimers.end()) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - it->second).count();
        recordTiming(name, duration);
        inst.m_activeTimers.erase(it);
    }
}

void Context::recordTiming(const std::string& name, double durationMs)
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_mode != Mode::ERRORS_AND_PROFILING && inst.m_mode != Mode::FULL_DIAGNOSTICS) {
        return;
    }
    
    auto& timing = inst.m_timings[name];
    if (timing.name.empty()) {
        timing.name = name;
    }
    timing.addSample(durationMs);
}

const std::unordered_map<std::string, TimingInfo>& Context::getTimings()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    return inst.m_timings;
}

std::string Context::getProfilingSummary()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_timings.empty()) {
        return "No profiling data";
    }
    
    std::ostringstream oss;
    oss << "=== Profiling Summary ===\n";
    oss << std::fixed << std::setprecision(3);
    
    std::vector<std::pair<std::string, TimingInfo>> sorted;
    for (const auto& [name, timing] : inst.m_timings) {
        sorted.emplace_back(name, timing);
    }
    std::sort(sorted.begin(), sorted.end(), 
              [](const auto& a, const auto& b) { return a.second.durationMs > b.second.durationMs; });
    
    oss << std::left << std::setw(30) << "Operation"
        << std::right << std::setw(10) << "Total (ms)"
        << std::setw(10) << "Avg (ms)"
        << std::setw(10) << "Min (ms)"
        << std::setw(10) << "Max (ms)"
        << std::setw(10) << "Calls"
        << "\n";
    oss << std::string(80, '-') << "\n";
    
    for (const auto& [name, timing] : sorted) {
        oss << std::left << std::setw(30) << name
            << std::right << std::setw(10) << timing.durationMs
            << std::setw(10) << timing.avgMs
            << std::setw(10) << timing.minMs
            << std::setw(10) << timing.maxMs
            << std::setw(10) << timing.callCount
            << "\n";
    }
    
    return oss.str();
}
#endif

// --- Memory tracking ---

#if SUBDIV_MEMORY_TRACKING_ENABLED
void Context::recordAllocation(const std::string& category, size_t bytes)
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_mode != Mode::FULL_DIAGNOSTICS) {
        return;
    }
    
    auto& mem = inst.m_memoryTracking[category];
    if (mem.name.empty()) {
        mem.name = category;
    }
    mem.recordAllocation(bytes);
}

void Context::recordDeallocation(const std::string& category, size_t bytes)
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_mode != Mode::FULL_DIAGNOSTICS) {
        return;
    }
    
    auto it = inst.m_memoryTracking.find(category);
    if (it != inst.m_memoryTracking.end()) {
        it->second.recordDeallocation(bytes);
    }
}

const std::unordered_map<std::string, MemoryInfo>& Context::getMemoryInfo()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    return inst.m_memoryTracking;
}

std::string Context::getMemorySummary()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    if (inst.m_memoryTracking.empty()) {
        return "No memory tracking data";
    }
    
    std::ostringstream oss;
    oss << "=== Memory Summary ===\n";
    
    size_t totalCurrent = 0;
    size_t totalPeak = 0;
    size_t totalAllocs = 0;
    
    for (const auto& [name, mem] : inst.m_memoryTracking) {
        totalCurrent += mem.allocatedBytes;
        totalPeak += mem.peakBytes;
        totalAllocs += mem.allocationCount;
    }
    
    oss << "Total Current: " << (totalCurrent / 1024.0 / 1024.0) << " MB\n";
    oss << "Total Peak:    " << (totalPeak / 1024.0 / 1024.0) << " MB\n";
    oss << "Total Allocs:  " << totalAllocs << "\n\n";
    
    oss << std::left << std::setw(20) << "Category"
        << std::right << std::setw(15) << "Current (KB)"
        << std::setw(15) << "Peak (KB)"
        << std::setw(12) << "Allocs"
        << "\n";
    oss << std::string(62, '-') << "\n";
    
    for (const auto& [name, mem] : inst.m_memoryTracking) {
        oss << std::left << std::setw(20) << name
            << std::right << std::setw(15) << (mem.allocatedBytes / 1024.0)
            << std::setw(15) << (mem.peakBytes / 1024.0)
            << std::setw(12) << mem.allocationCount
            << "\n";
    }
    
    return oss.str();
}
#endif

// --- General ---

void Context::clear()
{
    auto& inst = instance();
    std::lock_guard<std::mutex> lock(inst.m_mutex);
    
    inst.m_errors.clear();
    
#if SUBDIV_PROFILING_ENABLED
    inst.m_timings.clear();
    inst.m_activeTimers.clear();
#endif
    
#if SUBDIV_MEMORY_TRACKING_ENABLED
    inst.m_memoryTracking.clear();
#endif
}

std::string Context::getFullReport()
{
    std::ostringstream oss;
    
    oss << getErrorSummary() << "\n\n";
    
#if SUBDIV_PROFILING_ENABLED
    if (getMode() == Mode::ERRORS_AND_PROFILING || getMode() == Mode::FULL_DIAGNOSTICS) {
        oss << getProfilingSummary() << "\n\n";
    }
#endif
    
#if SUBDIV_MEMORY_TRACKING_ENABLED
    if (getMode() == Mode::FULL_DIAGNOSTICS) {
        oss << getMemorySummary() << "\n";
    }
#endif
    
    return oss.str();
}

} // namespace Subdiv