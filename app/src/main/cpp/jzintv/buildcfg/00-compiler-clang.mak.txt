##############################################################################
## Clang configuration.
##############################################################################

# Compile with "LTO=" to make a one-off compile w/out link time optimization.
LTO ?= -flto

# Clang-specific sanitizer flags.
# Add USE_SANI=1 to enable a sanitizer build.  Recommend 'make clean' first.
ifeq ($(USE_SANI),1)
 SANI  = -fsanitize=address
 SANI += -fsanitize-address-use-after-scope
 SANI += -fsanitize=undefined
 # These don't seem to be supported in the version that comes with XCode.
#SANI += -fsanitize=cfi
#SANI += -fsanitize=leak
#SANI += -fsanitize=memory
#SANI += -fsanitize=safe-stack
endif

# Use whatever 'clang' is in the current path.
CC       = clang -std=gnu11
CXX      = clang++ -std=gnu++14
CXXFLAGS = -fvisibility=hidden

# Aggressive optimization flags, unless you compile with DEBUG=1.
ifeq ($(DEBUG),1)
OPT_FLAGS = -ggdb3
else
OPT_FLAGS  = -O3 -fomit-frame-pointer -fstrict-aliasing
endif

CFLAGS   += -ffunction-sections -fdata-sections -fno-common
CXXFLAGS += -ffunction-sections -fdata-sections -fno-common

# Clang-specific warnings that we'll add to WARN and WARNXX below.
WCLANG  = -Warc-maybe-repeated-use-of-weak
WCLANG += -Warc-repeated-use-of-weak
WCLANG += -Warray-bounds-pointer-arithmetic
WCLANG += -Wassign-enum
WCLANG += -Watomic-properties
WCLANG += -Wauto-import
WCLANG += -Wbind-to-temporary-copy
WCLANG += -Wbitfield-enum-conversion
WCLANG += -Wc++11-compat -Wc++14-compat -Wc++17-compat
WCLANG += -Wc++11-compat-pedantic -Wc++14-compat-pedantic
WCLANG += -Wc++17-compat-pedantic
WCLANG += -Wc++11-compat-reserved-user-defined-literal
WCLANG += -Wc++11-extensions
WCLANG += -Wc++11-narrowing
WCLANG += -Wc++2a-compat -Wc++2a-compat-pedantic
WCLANG += -Wc11-extensions
WCLANG += -Wcast-align
WCLANG += -Wchar-subscripts
WCLANG += -Wcomma
WCLANG += -Wconditional-uninitialized
WCLANG += -Wconsumed
WCLANG += -Wdeprecated
WCLANG += -Wdeprecated-writable-strings
WCLANG += -Wdirect-ivar-access
WCLANG += -Wdisabled-macro-expansion
WCLANG += -Wdivision-by-zero
WCLANG += -Wduplicate-enum
WCLANG += -Weffc++
WCLANG += -Wembedded-directive
WCLANG += -Wempty-translation-unit
WCLANG += -Wendif-labels
WCLANG += -Wflexible-array-extensions
WCLANG += -Wformat-non-iso
WCLANG += -Wfour-char-constants
WCLANG += -Wheader-hygiene
WCLANG += -Widiomatic-parentheses
WCLANG += -Wignored-qualifiers
WCLANG += -Wimplicit
WCLANG += -Wkeyword-macro
WCLANG += -Wlanguage-extension-token
WCLANG += -Wloop-analysis
WCLANG += -Wmain
WCLANG += -Wmethod-signatures
WCLANG += -Wmicrosoft
WCLANG += -Wmismatched-tags
WCLANG += -Wmissing-method-return-type
WCLANG += -Wmissing-prototypes
WCLANG += -Wmost
WCLANG += -Wnested-anon-types
WCLANG += -Wnewline-eof
WCLANG += -Wnonportable-system-include-path
WCLANG += -Wnull-pointer-arithmetic
WCLANG += -Wnullability-extension
WCLANG += -Wnullable-to-nonnull-conversion
WCLANG += -Wold-style-cast
WCLANG += -Woverlength-strings
WCLANG += -Woverloaded-virtual
WCLANG += -Woverriding-method-mismatch
WCLANG += -pedantic
WCLANG += -Wpedantic
WCLANG += -Wpessimizing-move
WCLANG += -Wpointer-arith
WCLANG += -Wprofile-instr-missing
WCLANG += -Wreceiver-forward-class
WCLANG += -Wredundant-move
WCLANG += -Wredundant-parens
WCLANG += -Wreorder
WCLANG += -Wretained-language-linkage
WCLANG += -Wself-move
WCLANG += -Wsemicolon-before-method-body
WCLANG += -Wsequence-point
WCLANG += -Wshadow-all
WCLANG += -Wshift-sign-overflow
WCLANG += -Wsign-compare
WCLANG += -Wsigned-enum-bitfield
WCLANG += -Wsometimes-uninitialized
WCLANG += -Wstrict-prototypes
WCLANG += -Wsuper-class-method-mismatch
WCLANG += -Wthread-safety
WCLANG += -Wundefined-func-template
WCLANG += -Wundefined-internal-type
WCLANG += -Wundefined-reinterpret-cast
WCLANG += -Wunguarded-availability
WCLANG += -Wunreachable-code-aggressive
WCLANG += -Wunneeded-member-function
WCLANG += -Wunused
WCLANG += -Wused-but-marked-unused
WCLANG += -Wvector-conversion
WCLANG += -Wvla
WCLANG += -Wweak-template-vtables
WCLANG += -Wweak-vtables
WCLANG += -Wzero-as-null-pointer-constant
WCLANG += -Wzero-length-array

# Used when building the SDL1 plat/SDL1_osx_main.m file on Macintosh only.
WARN_M  = -Wall -W -Wextra -Wshadow -Wpointer-arith
WARN_M += -Wbad-function-cast -Wcast-qual -Wc++-compat
WARN_M += -Wmissing-declarations -Wmissing-prototypes
WARN_M += -Wstrict-prototypes
WARN_M += -Werror

# WARN is used when compiling C files.
WARN    = -Wall -W -Wextra -Wshadow -Wpointer-arith
WARN   += -Wbad-function-cast -Wcast-qual -Wc++-compat
WARN   += -Wmissing-declarations -Wmissing-prototypes
WARN   += -Wstrict-prototypes -Wstrict-aliasing
WARN   += -Werror
WARN   += $(WCLANG)

# WARN is used when compiling C++ files.
WARNXX  = -Wall -W -Wextra -Wshadow -Wpointer-arith
WARNXX += -Wcast-qual -Wsequence-point -Wstrict-aliasing
WARNXX += -Wc++11-compat -Wc++14-compat -Wc++1z-compat
WARNXX += -Werror
WARNXX += $(WCLANG)
