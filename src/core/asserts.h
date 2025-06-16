
#define KASSERTIONS_ENABLED

// If not setup, try the old way.
#if !defined(kdebug_break)
#    if defined(__clang__) || defined(__gcc__)
/** @brief Causes a debug breakpoint to be hit. */
#        define kdebug_break() __builtin_trap()
#    elif defined(_MSC_VER)
#        include <intrin.h>
/** @brief Causes a debug breakpoint to be hit. */
#        define kdebug_break() __debugbreak()
#    else
// Fall back to x86/x86_64
#        define kdebug_break() asm { int 3 }
#    endif
#endif

#ifdef KASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

#include <stdint.h>

void report_assertion_failure(const char* expression, const char* message, const char* file, int32_t line);


#define KASSERT(expr, message)                                                \
        {                                                                \
            if (expr) {                                                  \
            } else {                                                     \
                report_assertion_failure(#expr, message, __FILE__, __LINE__); \
                kdebug_break();                                          \
            }                                                            \
        }


#ifdef _DEBUG
#define KASSERT_DEBUG(expr)                                                \
        {                                                                \
            if (expr) {                                                  \
            } else {                                                     \
                report_assertion_failure(#expr, "", __FILE__, __LINE__); \
                kdebug_break();                                          \
            }                                                            \
        }
#else
#define KASSERT_DEBUG(expr)
#endif

#else
#define KASSERT(expr)
#define KASSERT_MSG(expr, message)
#define KASSERT_DEBUG(expr)
#endif