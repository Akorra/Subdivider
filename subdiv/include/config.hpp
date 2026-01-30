#pragma once

// Build configuration - automatically set by CMake
#ifdef SUBDIV_ENABLE_PROFILING
    #define SUBDIV_PROFILING_ENABLED 1
#else
    #define SUBDIV_PROFILING_ENABLED 0
#endif

#ifdef SUBDIV_ENABLE_MEMORY_TRACKING
    #define SUBDIV_MEMORY_TRACKING_ENABLED 1
#else
    #define SUBDIV_MEMORY_TRACKING_ENABLED 0
#endif

#ifdef SUBDIV_ENABLE_VALIDATION
    #define SUBDIV_VALIDATION_ENABLED 1
#else
    #define SUBDIV_VALIDATION_ENABLED 0
#endif

#ifdef SUBDIV_ENABLE_ASSERTS
    #define SUBDIV_ASSERTS_ENABLED 1
#else
    #define SUBDIV_ASSERTS_ENABLED 0
#endif

// Custom assert macro
#if SUBDIV_ASSERTS_ENABLED
    #include <cassert>
    #define SUBDIV_ASSERT(condition, message) assert((condition) && (message))
#else
    #define SUBDIV_ASSERT(condition, message) ((void)0)
#endif

// Forward declare Diagnostics namespace for macros
// The actual class definition is in diagnosticscontext.h
namespace Subdiv { class Context; class ScopedTimer; }

// Profiling macros (now uses global singleton)
#if SUBDIV_PROFILING_ENABLED
    #define SUBDIV_PROFILE(name) \
        ::Subdiv::ScopedTimer _subdiv_timer_##__LINE__((name))
    
    #define SUBDIV_PROFILE_FUNCTION() \
        ::Subdiv::ScopedTimer _subdiv_timer_##__LINE__(__FUNCTION__)
    
    #define SUBDIV_PROFILE_SCOPE(name) \
        ::Subdiv::ScopedTimer _subdiv_timer_##__LINE__((name))
#else
    #define SUBDIV_PROFILE(name) ((void)0)
    #define SUBDIV_PROFILE_FUNCTION() ((void)0)
    #define SUBDIV_PROFILE_SCOPE(name) ((void)0)
#endif

// Memory tracking macros (now uses global singleton)
// Note: These require diagnosticscontext.h to be included
#if SUBDIV_MEMORY_TRACKING_ENABLED
    #define SUBDIV_TRACK_ALLOC(category, bytes) \
        do { ::Subdiv::Diagnostics::Context::recordAllocation((category), (bytes)); } while(0)
    
    #define SUBDIV_TRACK_DEALLOC(category, bytes) \
        do { ::Subdiv::Diagnostics::Context::recordDeallocation((category), (bytes)); } while(0)
#else
    #define SUBDIV_TRACK_ALLOC(category, bytes) ((void)0)
    #define SUBDIV_TRACK_DEALLOC(category, bytes) ((void)0)
#endif

// Validation macros (now uses global singleton)
// Note: These require context.h to be included
#if SUBDIV_VALIDATION_ENABLED
    #define SUBDIV_ADD_ERROR(severity, code, message, context) \
        do { ::Subdiv::Diagnostics::Context::addError((severity), (code), (message), (context)); } while(0)
#else
    #define SUBDIV_ADD_ERROR(severity, code, message, context) ((void)0)
#endif

// Build info
#define SUBDIV_VERSION_MAJOR 0
#define SUBDIV_VERSION_MINOR 1
#define SUBDIV_VERSION_PATCH 0

// Helper to convert integer to string at compile time
#define SUBDIV_STRINGIFY(x) #x
#define SUBDIV_TOSTRING(x) SUBDIV_STRINGIFY(x)

namespace Subdiv
{
    /**
     * @brief Compile-time build information.
     */
    struct BuildInfo
    {
        static constexpr bool profilingEnabled = SUBDIV_PROFILING_ENABLED;
        static constexpr bool memoryTrackingEnabled = SUBDIV_MEMORY_TRACKING_ENABLED;
        static constexpr bool validationEnabled = SUBDIV_VALIDATION_ENABLED;
        static constexpr bool assertsEnabled = SUBDIV_ASSERTS_ENABLED;
        
        static constexpr int versionMajor = SUBDIV_VERSION_MAJOR;
        static constexpr int versionMinor = SUBDIV_VERSION_MINOR;
        static constexpr int versionPatch = SUBDIV_VERSION_PATCH;
        
        /**
         * @brief Get version string.
         */
        static constexpr const char* getVersionString()
        {
            return SUBDIV_TOSTRING(SUBDIV_VERSION_MAJOR) "." 
                   SUBDIV_TOSTRING(SUBDIV_VERSION_MINOR) "." 
                   SUBDIV_TOSTRING(SUBDIV_VERSION_PATCH);
        }
        
        /**
         * @brief Get a human-readable configuration string.
         */
        static const char* getConfigString()
        {
            static const char* config = 
                "Subdiv Library v" SUBDIV_TOSTRING(SUBDIV_VERSION_MAJOR) "." 
                                    SUBDIV_TOSTRING(SUBDIV_VERSION_MINOR) "." 
                                    SUBDIV_TOSTRING(SUBDIV_VERSION_PATCH)
#if SUBDIV_PROFILING_ENABLED
                " [PROFILING]"
#endif
#if SUBDIV_MEMORY_TRACKING_ENABLED
                " [MEMORY_TRACKING]"
#endif
#if SUBDIV_VALIDATION_ENABLED
                " [VALIDATION]"
#endif
#if SUBDIV_ASSERTS_ENABLED
                " [ASSERTS]"
#endif
            ;
            return config;
        }
        
        /**
         * @brief Get build type string.
         */
        static const char* getBuildType()
        {
#if defined(NDEBUG)
    #if SUBDIV_PROFILING_ENABLED
            return "Profile";
    #else
            return "Release";
    #endif
#else
            return "Debug";
#endif
        }
    };
}