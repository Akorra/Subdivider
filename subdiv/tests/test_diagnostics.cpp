#include <catch2/catch_test_macros.hpp>
#include "diagnostics/context.hpp"

using namespace Subdiv::Diagnostics;

TEST_CASE("Diagnostics Context - Error Tracking", "[diagnostics]")
{
    Context diag;
    
    SECTION("Empty context has no errors")
    {
        REQUIRE(!diag.hasErrors());
        REQUIRE(!diag.hasWarnings());
        REQUIRE(!diag.hasFatalErrors());
    }
    
    SECTION("Add single error")
    {
        diag.addError(ErrorSeverity::ERROR, "TEST_ERROR", "Test error message");
        
        REQUIRE(diag.hasErrors());
        REQUIRE(!diag.hasWarnings());
        
        const auto* err = diag.getLastError();
        REQUIRE(err != nullptr);
        REQUIRE(err->code == "TEST_ERROR");
        REQUIRE(err->message == "Test error message");
    }
    
    SECTION("Add warning")
    {
        diag.addError(ErrorSeverity::WARNING, "TEST_WARN", "Test warning");
        
        REQUIRE(diag.hasWarnings());
        REQUIRE(diag.hasErrors()); // Warnings still count as errors
    }
    
    SECTION("Add fatal error")
    {
        diag.addError(ErrorSeverity::FATAL, "TEST_FATAL", "Fatal error");
        
        REQUIRE(diag.hasFatalErrors());
        REQUIRE(diag.hasErrors());
    }
    
    SECTION("Multiple errors")
    {
        diag.addError(ErrorSeverity::WARNING, "WARN1", "Warning 1");
        diag.addError(ErrorSeverity::ERROR, "ERR1", "Error 1");
        diag.addError(ErrorSeverity::ERROR, "ERR2", "Error 2");
        
        REQUIRE(diag.getErrors().size() == 3);
    }
    
    SECTION("Clear errors")
    {
        diag.addError(ErrorSeverity::ERROR, "ERR", "Error");
        REQUIRE(diag.hasErrors());
        
        diag.clear();
        REQUIRE(!diag.hasErrors());
    }
}

TEST_CASE("Diagnostics Context - Error Summary", "[diagnostics]")
{
    Context diag;
    
    diag.addError(ErrorSeverity::WARNING, "WARN1", "Warning message");
    diag.addError(ErrorSeverity::ERROR, "ERR1", "Error message", "context info");
    
    std::string summary = diag.getErrorSummary();
    
    REQUIRE(!summary.empty());
    REQUIRE(summary.find("WARN1") != std::string::npos);
    REQUIRE(summary.find("ERR1") != std::string::npos);
    REQUIRE(summary.find("context info") != std::string::npos);
}

#if SUBDIV_PROFILING_ENABLED
TEST_CASE("Diagnostics Context - Profiling", "[diagnostics][profiling]")
{
    Context diag(Context::Mode::ERRORS_AND_PROFILING);
    
    SECTION("Manual timing")
    {
        diag.startTimer("TestOperation");
        // Simulate work
        for (volatile int i = 0; i < 1000; ++i) {}
        diag.stopTimer("TestOperation");
        
        const auto& timings = diag.getTimings();
        REQUIRE(timings.find("TestOperation") != timings.end());
        
        const auto& timing = timings.at("TestOperation");
        REQUIRE(timing.callCount == 1);
        REQUIRE(timing.durationMs > 0.0);
    }
    
    SECTION("Scoped timing")
    {
        {
            ScopedTimer timer(&diag, "ScopedOperation");
            for (volatile int i = 0; i < 1000; ++i) {}
        }
        
        const auto& timings = diag.getTimings();
        REQUIRE(timings.find("ScopedOperation") != timings.end());
    }
    
    SECTION("Multiple calls accumulate")
    {
        for (int i = 0; i < 5; ++i) {
            diag.startTimer("Repeated");
            diag.stopTimer("Repeated");
        }
        
        const auto& timings = diag.getTimings();
        REQUIRE(timings.at("Repeated").callCount == 5);
    }
    
    SECTION("Profiling summary")
    {
        diag.startTimer("Op1");
        diag.stopTimer("Op1");
        
        std::string summary = diag.getProfilingSummary();
        REQUIRE(!summary.empty());
        REQUIRE(summary.find("Op1") != std::string::npos);
    }
}
#endif

#if SUBDIV_MEMORY_TRACKING_ENABLED
TEST_CASE("Diagnostics Context - Memory Tracking", "[diagnostics][memory]")
{
    Context diag(Context::Mode::FULL_DIAGNOSTICS);
    
    SECTION("Track allocations")
    {
        diag.recordAllocation("TestCategory", 1024);
        diag.recordAllocation("TestCategory", 2048);
        
        const auto& memInfo = diag.getMemoryInfo();
        REQUIRE(memInfo.find("TestCategory") != memInfo.end());
        
        const auto& info = memInfo.at("TestCategory");
        REQUIRE(info.allocatedBytes == 3072);
        REQUIRE(info.allocationCount == 2);
        REQUIRE(info.peakBytes == 3072);
    }
    
    SECTION("Track deallocations")
    {
        diag.recordAllocation("TestCategory", 2048);
        diag.recordDeallocation("TestCategory", 1024);
        
        const auto& info = diag.getMemoryInfo().at("TestCategory");
        REQUIRE(info.allocatedBytes == 1024);
    }
    
    SECTION("Peak tracking")
    {
        diag.recordAllocation("TestCategory", 4096);
        diag.recordAllocation("TestCategory", 4096);
        diag.recordDeallocation("TestCategory", 4096);
        
        const auto& info = diag.getMemoryInfo().at("TestCategory");
        REQUIRE(info.allocatedBytes == 4096);
        REQUIRE(info.peakBytes == 8192); // Peak was higher
    }
    
    SECTION("Memory summary")
    {
        diag.recordAllocation("Cat1", 1024);
        diag.recordAllocation("Cat2", 2048);
        
        std::string summary = diag.getMemorySummary();
        REQUIRE(!summary.empty());
        REQUIRE(summary.find("Cat1") != std::string::npos);
        REQUIRE(summary.find("Cat2") != std::string::npos);
    }
}
#endif

TEST_CASE("Diagnostics Context - Modes", "[diagnostics]")
{
    SECTION("Errors only mode")
    {
        Context diag(Context::Mode::ERRORS_ONLY);
        
        REQUIRE(!diag.isProfilingEnabled());
        REQUIRE(!diag.isMemoryTrackingEnabled());
    }
    
#if SUBDIV_PROFILING_ENABLED
    SECTION("Errors and profiling mode")
    {
        Context diag(Context::Mode::ERRORS_AND_PROFILING);
        
        REQUIRE(diag.isProfilingEnabled());
        REQUIRE(!diag.isMemoryTrackingEnabled());
    }
#endif
    
#if SUBDIV_MEMORY_TRACKING_ENABLED
    SECTION("Full diagnostics mode")
    {
        Context diag(Context::Mode::FULL_DIAGNOSTICS);
        
        REQUIRE(diag.isProfilingEnabled());
        REQUIRE(diag.isMemoryTrackingEnabled());
    }
#endif
}