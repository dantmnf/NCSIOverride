typedef void (*FUNCSHIM)();
void shim_prepare();


/* system detection */
#if defined(__i386__) || defined(_M_IX86)
#define SHIM_ARCH_X86
#elif defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
#define SHIM_ARCH_AMD64
#elif defined(__aarch64__) || defined(_M_ARM64)
#define SHIM_ARCH_AARCH64
#endif

#ifdef _MSC_VER
#define SHIM_COMPILER_MSVC_FLAVORED
#endif

#if defined(__GNUC__) || defined(__clang__)
#define SHIM_COMPILER_GNU_FLAVORED
#endif


#if defined(SHIM_ARCH_AMD64)
#define MANGLE(x) x
#elif defined(SHIM_ARCH_X86)
#define MANGLE(x) "_" x
#elif defined(SHIM_ARCH_AARCH64)
#error aarch64 not supported yet
#endif

#if !defined(SHIM_USE_EXTERNAL_ENTRY) && defined(SHIM_COMPILER_MSVC_FLAVORED) && defined(SHIM_ARCH_X86)

    #define DECLARE_SHIM(name) \
        FUNCSHIM p##name; \
        __declspec(naked) void Shim##name() { \
        __asm call shim_prepare \
        __asm jmp [p##name] \
        }

#elif !defined(SHIM_USE_EXTERNAL_ENTRY) && defined(SHIM_COMPILER_GNU_FLAVORED)

    #define BEGIN_NAKED_FUNCTION(name) __asm__(".align 8"); \
                                               __asm__(MANGLE(#name) ":"); \
                                               __asm__(".globl " MANGLE(#name));
    #define END_NAKED_FUNCTION()

    #if defined(SHIM_ARCH_AMD64)

        BEGIN_NAKED_FUNCTION(shim_prepare2)
            __asm__("cmpl $0, " MANGLE("shim_prepared") "(%rip)");
            __asm__("jz 114514f");
            __asm__("ret");
            __asm__("114514:");
            __asm__("push %r9");
            __asm__("push %r8");
            __asm__("push %rdx");
            __asm__("push %rcx");
            __asm__("call " MANGLE("shim_prepare"));
            __asm__("pop %rcx");
            __asm__("pop %rdx");
            __asm__("pop %r8");
            __asm__("pop %r9");
            __asm__("ret");
        END_NAKED_FUNCTION()

        #define DECLARE_SHIM(name) \
                FUNCSHIM p##name; \
                BEGIN_NAKED_FUNCTION( Shim##name ) \
                    __asm__ ("call " MANGLE("shim_prepare2")); \
                    __asm__ ("jmp *" MANGLE("p" #name) "(%rip)"); \
                END_NAKED_FUNCTION()

    #elif defined(SHIM_ARCH_X86)

        BEGIN_NAKED_FUNCTION(shim_prepare2)
            __asm__("cmpl $0, " MANGLE("shim_prepared"));
            __asm__("jz 114514f");
            __asm__("ret");
            __asm__("114514:");
            __asm__("jmp " MANGLE("shim_prepare"));
        END_NAKED_FUNCTION()

        #define DECLARE_SHIM(name) \
                FUNCSHIM p##name; \
                BEGIN_NAKED_FUNCTION( Shim##name ) \
                    __asm__ ("call " MANGLE("shim_prepare2")); \
                    __asm__ ("jmp *" MANGLE("p" #name)); \
                END_NAKED_FUNCTION()

    #endif
#else
    #define DECLARE_SHIM(name) \
        FUNCSHIM p##name; \
        extern void Shim##name();
#endif

#define FILL_SHIM(name) do {p##name = (FUNCSHIM)GetProcAddress(hDll, #name);} while(0);



#include <windows.h>

static HMODULE hDll;
volatile LONG shim_preparing = 0, shim_prepared = 0;

__declspec(noinline) static HMODULE LoadTargetLibrary();
__declspec(noinline) static void FillShims();

#define SHIM_DECLARE
#define SHIM DECLARE_SHIM
#include "shim_declare.h"
#undef SHIM
#undef SHIM_DECLARE

static void FillShims() {
#define SHIM_FILL
#define SHIM FILL_SHIM
#include "shim_declare.h"
#undef SHIM
#undef SHIM_FILL
}

void shim_prepare() {
    if (shim_prepared) return;
    if (InterlockedCompareExchange(&shim_preparing, 1, 0) == 1) {
        // yield if another thread is preparing
        while (!shim_prepared) SwitchToThread();
        // prepared by other thread
        return;
    }
    hDll = LoadTargetLibrary();
    if (hDll) {
        FillShims();
        shim_prepared = 1;
    }
    InterlockedCompareExchange(&shim_preparing, 0, 1);
}
