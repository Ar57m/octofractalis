#ifndef AST_PARSER_H
#define AST_PARSER_H


#include <cstdio>
#include <functional>
#include <memory>
#include <algorithm>



#include "core/hypercomplex.h"











// ------------------- GPU-Friendly String Comparison -------------------
class ExprTools {
public:
    template <int Dim>
    HOST_DEVICE static constexpr bool isImaginaryChar(char c) {
        if constexpr (Dim == 2) return c == 'i';
        else if constexpr (Dim == 4) return c == 'i' || c == 'j' || c == 'k';
        else if constexpr (Dim == 8) return c >= 'i' && c <= 'o';
        else return false;
    }
    
    HOST_DEVICE inline static int my_strcmp(const char* s1, const char* s2) {
        while(*s1 && (*s1 == *s2)) {
            ++s1;
            ++s2;
        }
        return *(unsigned char*)s1 - *(unsigned char*)s2;
    }

    HOST_DEVICE inline static int my_strlen(const char* s) {
        int len = 0;
        while (s[len] != '\0') {
            len++;
        }
        return len;
    }

    HOST_DEVICE inline static constexpr bool startsWith(const char* expr, const int posi, const char* token) {
        int i = 0;
        while (token[i] != '\0') {
            if (expr[posi + i] == '\0' || expr[posi + i] != token[i])
                return false;
            i++;
        }
        return true;
    }

    HOST_DEVICE inline static constexpr bool my_isdigit(const char c) {
        return (c > 47 && c < 58);
    }

    HOST_DEVICE static DefaultType my_atof(const char* str) {
        int sign = 1;
        if (*str == '-') {
            sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }

        DefaultType result = 0.0;

        while (*str >= '0' && *str <= '9') {
            result = result * 10.0 + (*str - '0');
            str++;
        }

        if (*str == '.') {
            str++;
            DefaultType fraction = 0.0;
            DefaultType divisor = 10.0;
            while (*str >= '0' && *str <= '9') {
                fraction += (*str - '0') / divisor;
                divisor *= 10.0;
                str++;
            }
            result += fraction;
        }

        if (*str == 'e' || *str == 'E') {
            str++;
            int expSign = 1;
            if (*str == '-') {
                expSign = -1;
                str++;
            } else if (*str == '+') {
                str++;
            }
            int exponent = 0;
            while (*str >= '0' && *str <= '9') {
                exponent = exponent * 10 + (*str - '0');
                str++;
            }
            DefaultType expMultiplier = 1.0;
            for (int i = 0; i < exponent; i++) {
                expMultiplier *= 10.0;
            }
            if (expSign == -1) {
                result = result / expMultiplier;
            } else {
                result = result * expMultiplier;
            }
        }

        return sign * result;
    }

    HOST_DEVICE inline static constexpr bool isIdentifierChar(const char c) {
        return ((c > 64 && c < 91) || (c > 96 && c < 123));
    }

    HOST_DEVICE inline static bool containsToken(const char* tokens[], int tokenCount, const char* target) {
        for (int i = 0; i < tokenCount; i++) {
            if (my_strcmp(tokens[i], target) == 0)
                return true;
        }
        return false;
    }

    HOST_DEVICE inline static bool isTokenBoundary(const char* expr, const int posi, const int tokenLen) {
        if (posi > 0 && isIdentifierChar(expr[posi - 1])) {
            return false;
        }
        if (expr[posi + tokenLen] != '\0' && isIdentifierChar(expr[posi + tokenLen])) {
            return false;
        }
        return true;
    }

    HOST_DEVICE inline static constexpr unsigned int str2int(const char* str, const int h = 0) {
        return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ static_cast<unsigned int>(str[h]);
    }
};

constexpr unsigned int H_SQRT  = ExprTools::str2int("sqrt");
constexpr unsigned int H_LOG   = ExprTools::str2int("log");
constexpr unsigned int H_LOGN  = ExprTools::str2int("logn");
constexpr unsigned int H_EXP   = ExprTools::str2int("exp");

constexpr unsigned int H_SIN   = ExprTools::str2int("sin");
constexpr unsigned int H_COS   = ExprTools::str2int("cos");
constexpr unsigned int H_TAN   = ExprTools::str2int("tan");

constexpr unsigned int H_SINH  = ExprTools::str2int("sinh");
constexpr unsigned int H_COSH  = ExprTools::str2int("cosh");
constexpr unsigned int H_TANH  = ExprTools::str2int("tanh");

constexpr unsigned int H_ARG   = ExprTools::str2int("arg");
constexpr unsigned int H_CONJ  = ExprTools::str2int("conj");
constexpr unsigned int H_MAG   = ExprTools::str2int("mag");
constexpr unsigned int H_ABS   = ExprTools::str2int("abs");

constexpr unsigned int H_RE    = ExprTools::str2int("re");
constexpr unsigned int H_IM    = ExprTools::str2int("im");
constexpr unsigned int H_I     = ExprTools::str2int("i");
constexpr unsigned int H_J     = ExprTools::str2int("j");
constexpr unsigned int H_K     = ExprTools::str2int("k");

constexpr unsigned int H_L     = ExprTools::str2int("l");
constexpr unsigned int H_M     = ExprTools::str2int("m");
constexpr unsigned int H_N     = ExprTools::str2int("n");
constexpr unsigned int H_O     = ExprTools::str2int("o");

constexpr unsigned int H_ROUND = ExprTools::str2int("round");
constexpr unsigned int H_SIGN  = ExprTools::str2int("sign");
constexpr unsigned int H_GAMMA = ExprTools::str2int("gamma");
constexpr unsigned int H_ZETA  = ExprTools::str2int("zeta");
constexpr unsigned int H_AIRY  = ExprTools::str2int("airy");

constexpr unsigned int H_POW   = ExprTools::str2int("pow");
constexpr unsigned int H_ROOT  = ExprTools::str2int("root");
constexpr unsigned int H_MAX   = ExprTools::str2int("max");
constexpr unsigned int H_MIN   = ExprTools::str2int("min");
constexpr unsigned int H_DOT   = ExprTools::str2int("dot");

// constexpr unsigned int H_CIRCLE   = ExprTools::str2int("circle");
// constexpr unsigned int H_RECT     = ExprTools::str2int("rect");
// constexpr unsigned int H_TRIANGLE = ExprTools::str2int("triangle");

constexpr unsigned int H_IF    = ExprTools::str2int("if");
constexpr unsigned int H_RAND  = ExprTools::str2int("rand");
constexpr unsigned int H_SPIN  = ExprTools::str2int("spin");
// constexpr unsigned int H_ROT   = ExprTools::str2int("rotation");
// constexpr unsigned int H_ELL   = ExprTools::str2int("ellipsoid");
// constexpr unsigned int H_LINE  = ExprTools::str2int("line");
constexpr unsigned int H_LERP  = ExprTools::str2int("lerp");


// -------------------------------------------------------------------------
// BYTECODE AND INSTRUCTION DEFINITIONS
// -------------------------------------------------------------------------

constexpr int MAX_EXPR_SIZE = 384;
constexpr int MAX_REGISTERS = 16;

enum OpCode : uint8_t {
    // values / memory
    OP_CONSTANT,       // load constant
    OP_LOAD_VAR,       // load variable
    OP_ARRAY_ACCESS,   // array[index]

    // arithmetic
    OP_ADD,            // a + b
    OP_SUB,            // a - b
    OP_MUL,            // a * b
    OP_DIV,            // a / b
    OP_MOD,            // a % b
    OP_POW,            // a ^ b

    OP_MUL_ADD,

    // comparisons
    OP_EQ,             // a == b
    OP_COS_SIM,        // cosine similarity (custom)
    OP_GT,             // a > b
    OP_LT,             // a < b

    // unary basic
    OP_NEG,            // -a

    // elementary functions
    OP_SQRT,           // sqrt(a)
    OP_LOG,      // unary
    OP_LOGN,     // binary
    OP_EXP,            // exp(a)

    // trig
    OP_SIN,            // sin(a)
    OP_COS,            // cos(a)
    OP_TAN,            // tan(a)

    // hyperbolic
    OP_SINH,           // sinh(a)
    OP_COSH,           // cosh(a)
    OP_TANH,           // tanh(a)

    // complex / quaternion ops
    OP_ARG,            // arg(a)
    OP_CONJ,           // conjugate(a)
    OP_MAG,            // magnitude(a)
    OP_ABS,            // abs(a)

    // component extraction
    OP_REAL,           // real(a)
    OP_IMAG,           // imaginary vector (i,j,k)
    OP_I,              // i component
    OP_J,              // j component
    OP_K,              // k component
    OP_L,              // l component
    OP_M,              // m component
    OP_N,              // n component
    OP_O,              // o component


    // special functions
    OP_ROUND,          // round(a)
    OP_SIGN,           // sign(a)
    OP_GAMMA,          // gamma(a)
    OP_ZETA,           // zeta(a)
    OP_AIRY,           // airy(a)

    // binary ops (extra)
    OP_ROOT,        // a.root(b)
    OP_MAX,         // max(a,b)
    OP_MIN,         // min(a,b)
    OP_DOT,         // dot(a,b)

    OP_CIRCLE,      // shape ops
    OP_RECT,
    OP_TRIANGLE,

    // control
    OP_IF,             // if(cond, a, b)
    OP_SPIN,
    OP_ROTATION,
    OP_ELLIPSOID,
    OP_LINE,
    OP_LERP,
    OP_RAND,

    OP_ERROR
};

template <int Dim>
struct alignas(16) Instruction {
    OpCode op;
    uint8_t out;
    uint8_t in1;
    uint8_t in2;
    uint8_t in3;

    union Data {
        Hypercomplex<Dim> constant;
        uint32_t varIndex;
        uint32_t arrayIndex;

        HOST_DEVICE Data() {}
        HOST_DEVICE ~Data() {}
    } data;

    HOST_DEVICE Instruction() : op(OP_ERROR), out(0), in1(0), in2(0), in3(0), data() {}

    HOST_DEVICE Instruction(const Instruction& other)
        : op(other.op), out(other.out), in1(other.in1), in2(other.in2), in3(other.in3)
    {
        if (op == OP_CONSTANT) data.constant = other.data.constant;
        else data.varIndex = other.data.varIndex;
    }

    HOST_DEVICE Instruction& operator=(const Instruction& other) {
        if (this != &other) {
            op = other.op;
            out = other.out;
            in1 = other.in1;
            in2 = other.in2;
            in3 = other.in3;

            if (op == OP_CONSTANT) data.constant = other.data.constant;
            else data.varIndex = other.data.varIndex;
        }
        return *this;
    }
};

template <int Dim>
struct VariableEntry {
    const char* name;
    Hypercomplex<Dim>* value;
};

struct ArrayEntry {
    const char* name;
    double* array;
    uint32_t size;
};

template <int Dim>
HOST_DEVICE inline int findVariableIndex(const char* name,
                                  const VariableEntry<Dim>* varEntries,
                                  size_t numVars) {
    for (size_t i = 0; i < numVars; ++i) {
        if (ExprTools::my_strcmp(varEntries[i].name, name) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

HOST_DEVICE inline int findArrayIndex(const char* name, const ArrayEntry* arrEntries, size_t numArrays) {
    for (size_t i = 0; i < numArrays; ++i) {
        if (ExprTools::my_strcmp(arrEntries[i].name, name) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

template <int Dim>
class Parser {
private:
    char expr[MAX_EXPR_SIZE];
    size_t expr_size;
    size_t pos;
    bool hasError = false;
    const VariableEntry<Dim>* varEntries;
    size_t numVars;

    const ArrayEntry* arrEntries;
    size_t numArrays;

    Instruction<Dim> bytecode[MAX_EXPR_SIZE];
    size_t bytecode_size;
    uint8_t vsp;

    const char* varNames[10] = {"z","c","it","v","p","f","dif","dx","dy","dz"};
    const char* oVarNames[5] = {"pi","e","phi","x","y"};

    static constexpr unsigned int H_SQRT  = ExprTools::str2int("sqrt");
    static constexpr unsigned int H_LOG   = ExprTools::str2int("log");
    static constexpr unsigned int H_LOGN  = ExprTools::str2int("logn");
    static constexpr unsigned int H_EXP   = ExprTools::str2int("exp");

    static constexpr unsigned int H_SIN   = ExprTools::str2int("sin");
    static constexpr unsigned int H_COS   = ExprTools::str2int("cos");
    static constexpr unsigned int H_TAN   = ExprTools::str2int("tan");

    static constexpr unsigned int H_SINH  = ExprTools::str2int("sinh");
    static constexpr unsigned int H_COSH  = ExprTools::str2int("cosh");
    static constexpr unsigned int H_TANH  = ExprTools::str2int("tanh");

    static constexpr unsigned int H_ARG   = ExprTools::str2int("arg");
    static constexpr unsigned int H_CONJ  = ExprTools::str2int("conj");
    static constexpr unsigned int H_MAG   = ExprTools::str2int("mag");
    static constexpr unsigned int H_ABS   = ExprTools::str2int("abs");

    static constexpr unsigned int H_RE    = ExprTools::str2int("re");
    static constexpr unsigned int H_IM    = ExprTools::str2int("im");
    static constexpr unsigned int H_I     = ExprTools::str2int("i");
    static constexpr unsigned int H_J     = ExprTools::str2int("j");
    static constexpr unsigned int H_K     = ExprTools::str2int("k");

    static constexpr unsigned int H_L     = ExprTools::str2int("l");
    static constexpr unsigned int H_M     = ExprTools::str2int("m");
    static constexpr unsigned int H_N     = ExprTools::str2int("n");
    static constexpr unsigned int H_O     = ExprTools::str2int("o");

    static constexpr unsigned int H_ROUND = ExprTools::str2int("round");
    static constexpr unsigned int H_SIGN  = ExprTools::str2int("sign");
    static constexpr unsigned int H_GAMMA = ExprTools::str2int("gamma");
    static constexpr unsigned int H_ZETA  = ExprTools::str2int("zeta");
    static constexpr unsigned int H_AIRY  = ExprTools::str2int("airy");

    static constexpr unsigned int H_POW   = ExprTools::str2int("pow");
    static constexpr unsigned int H_ROOT  = ExprTools::str2int("root");
    static constexpr unsigned int H_MAX   = ExprTools::str2int("max");
    static constexpr unsigned int H_MIN   = ExprTools::str2int("min");
    static constexpr unsigned int H_DOT   = ExprTools::str2int("dot");

    // static constexpr unsigned int H_ELL   = ExprTools::str2int("ellipsoid");
    // static constexpr unsigned int H_CIRCLE   = ExprTools::str2int("circle");
    // static constexpr unsigned int H_RECT     = ExprTools::str2int("rect");
    // static constexpr unsigned int H_TRIANGLE = ExprTools::str2int("triangle");
    // static constexpr unsigned int H_LINE  = ExprTools::str2int("line");

    static constexpr unsigned int H_IF    = ExprTools::str2int("if");
    static constexpr unsigned int H_RAND  = ExprTools::str2int("rand");
    static constexpr unsigned int H_SPIN  = ExprTools::str2int("spin");
    static constexpr unsigned int H_ROT   = ExprTools::str2int("rotation");
    static constexpr unsigned int H_LERP  = ExprTools::str2int("lerp");




    // 1. Helper to ensure we don't write past bytecode capacity
    HOST_DEVICE bool canEmit() {
        return !hasError && bytecode_size < MAX_EXPR_SIZE;
    }

    HOST_DEVICE void emit1(OpCode op, uint8_t out, uint8_t in1) {
        if (canEmit() && out < MAX_REGISTERS && in1 < MAX_REGISTERS) {
            auto& b = bytecode[bytecode_size++];
            b.op = op; b.out = out; b.in1 = in1;
        } else { hasError = true; }
    }

    HOST_DEVICE void emit2(OpCode op, uint8_t out, uint8_t in1, uint8_t in2) {
        if (canEmit() && out < MAX_REGISTERS && in1 < MAX_REGISTERS && in2 < MAX_REGISTERS) {
            auto& b = bytecode[bytecode_size++];
            b.op = op; b.out = out; b.in1 = in1; b.in2 = in2;
        } else { hasError = true; }
    }

    HOST_DEVICE void emit3(OpCode op, uint8_t out, uint8_t in1, uint8_t in2, uint8_t in3) {
        if (canEmit() && out < MAX_REGISTERS && in1 < MAX_REGISTERS && in2 < MAX_REGISTERS && in3 < MAX_REGISTERS) {
            auto& b = bytecode[bytecode_size++];
            b.op = op; b.out = out; b.in1 = in1; b.in2 = in2; b.in3 = in3;
        } else { hasError = true; }
    }

    HOST_DEVICE void emitUnary(OpCode op) {
        if (vsp >= 1) {
            emit1(op, (uint8_t)(vsp - 1), (uint8_t)(vsp - 1));
        } else { hasError = true; }
    }

    HOST_DEVICE void emitBinary(OpCode op) {
        if (vsp >= 2) {
            // Pop 2, Push 1: Result goes into the lower register
            emit2(op, (uint8_t)(vsp - 2), (uint8_t)(vsp - 2), (uint8_t)(vsp - 1));
            vsp--;
        } else { hasError = true; }
    }

    HOST_DEVICE void emitTernary(OpCode op) {
        if (vsp >= 3) {
            // Pop 3, Push 1
            emit3(op, (uint8_t)(vsp - 3), (uint8_t)(vsp - 3), (uint8_t)(vsp - 2), (uint8_t)(vsp - 1));
            vsp -= 2;
        } else { hasError = true; }
    }

    HOST_DEVICE void emitConstant(const Hypercomplex<Dim>& v) {
        if (canEmit() && vsp < MAX_REGISTERS) {
            auto& b = bytecode[bytecode_size++];
            b.op = OP_CONSTANT;
            b.out = (uint8_t)vsp;
            b.data.constant = v;
            vsp++;
        } else { hasError = true; }
    }

    HOST_DEVICE void emitLoadVar(uint32_t idx) {
        if (canEmit() && vsp < MAX_REGISTERS) {
            auto& b = bytecode[bytecode_size++];
            b.op = OP_LOAD_VAR;
            b.out = (uint8_t)vsp;
            b.data.varIndex = idx;
            vsp++;
        } else { hasError = true; }
    }

    HOST_DEVICE void emitArrayAccess(uint32_t idx, uint8_t target) {
        // Note: ensure the target register actually exists
        if (canEmit() && target < MAX_REGISTERS) {
            auto& b = bytecode[bytecode_size++];
            b.op = OP_ARRAY_ACCESS;
            b.out = target;
            b.in1 = target;
            b.data.arrayIndex = idx;
        } else { hasError = true; }
    }

    HOST_DEVICE void optimize() {
        if (bytecode_size < 2) return;
    
        for (size_t i = 0; i < bytecode_size - 1; ++i) {
            Instruction<Dim>& current = bytecode[i];
    
            // MUL + ADD → MUL_ADD
            if (current.op == OP_MUL && i + 1 < bytecode_size &&
                bytecode[i + 1].op == OP_ADD)
            {
                Instruction<Dim>& next = bytecode[i + 1];
    
                uint8_t regC = 0;
                bool canCombine = false;
    
                if (next.in1 == current.out) {
                    regC = next.in2;
                    canCombine = true;
                } else if (next.in2 == current.out) {
                    regC = next.in1;
                    canCombine = true;
                }
    
                if (canCombine) {
                    current.op  = OP_MUL_ADD;
                    current.in3 = regC;
                    current.out = next.out;
    
                    for (size_t j = i + 1; j < bytecode_size - 1; ++j)
                        bytecode[j] = bytecode[j + 1];
    
                    bytecode_size--;
                    i--;
                    continue;
                }
            }
    
            // MUL + LOAD_VAR + ADD → MUL_ADD
            if (current.op == OP_MUL && i + 2 < bytecode_size) {
                Instruction<Dim>& load = bytecode[i + 1];
                Instruction<Dim>& add  = bytecode[i + 2];
    
                if (load.op == OP_LOAD_VAR && add.op == OP_ADD) {
                    bool canCombine =
                        (add.in1 == current.out && add.in2 == load.out) ||
                        (add.in2 == current.out && add.in1 == load.out);
    
                    if (canCombine) {
                        uint8_t safeReg =
                            (load.out == current.in1 || load.out == current.in2)
                            ? load.out + 1
                            : load.out;
    
                        load.out = safeReg;
    
                        add.op  = OP_MUL_ADD;
                        add.in1 = current.in1;
                        add.in2 = current.in2;
                        add.in3 = safeReg;
    
                        for (size_t j = i; j < bytecode_size - 1; ++j)
                            bytecode[j] = bytecode[j + 1];
    
                        bytecode_size--;
                        i--;
                    }
                }
            }
        }
    }

public:
    HOST_DEVICE Parser(const char* expr, size_t expr_size,
        const VariableEntry<Dim>* vars, size_t numVars,
        const ArrayEntry* arrays, size_t numArrays)
        : expr_size(expr_size), pos(0),
          varEntries(vars), numVars(numVars),
          arrEntries(arrays), numArrays(numArrays),
          bytecode_size(0), vsp(0)
    {
        size_t i = 0;
        for (; i < expr_size && i < MAX_EXPR_SIZE - 1; i++)
            this->expr[i] = expr[i];
        this->expr[i] = '\0';
    }

    HOST_DEVICE void parse() { parseExpression(); optimize(); }

    HOST_DEVICE const Instruction<Dim>* getBytecode() const { return bytecode; }
    HOST_DEVICE size_t getBytecodeSize() const { return bytecode_size; }

private:
    HOST_DEVICE void parseExpression() {
        parseCompare();
        while (pos < expr_size) {
            switch (expr[pos]) {
                case '+': ++pos; parseCompare(); emitBinary(OP_ADD); break;
                case '-': ++pos; parseCompare(); emitBinary(OP_SUB); break;
                default: return;
            }
        }
    }
    HOST_DEVICE void parseCompare() {
        parseEquality();
        while (pos < expr_size) {
            switch (expr[pos]) {
                case '>': ++pos; parseEquality(); emitBinary(OP_GT); break;
                case '<': ++pos; parseEquality(); emitBinary(OP_LT); break;
                default: return;
            }
        }
    }
    HOST_DEVICE void parseEquality() {
        parseCosSim();
        while (pos < expr_size) {
            if (expr[pos] == '=') {
                ++pos;
                parseCosSim();
                emitBinary(OP_EQ);
            } else return;
        }
    }
    HOST_DEVICE void parseCosSim() {
        parseTerm();
        while (pos < expr_size) {
            if (expr[pos] == '&') {
                ++pos;
                parseTerm();
                emitBinary(OP_COS_SIM);
            } else return;
        }
    }
    HOST_DEVICE void parseTerm() {
        parsePower();
        while (pos < expr_size) {
            switch (expr[pos]) {
                case '*': ++pos; parsePower(); emitBinary(OP_MUL); break;
                case '/': ++pos; parsePower(); emitBinary(OP_DIV); break;
                case '%': ++pos; parsePower(); emitBinary(OP_MOD); break;
                default: return;
            }
        }
    }
    HOST_DEVICE void parsePower() {
        parseFactor();

        // IMPORTANT: right-associative
        if (pos < expr_size && expr[pos] == '^') {
            ++pos;
            parsePower();  // recursion makes it right-associative
            emitBinary(OP_POW);
        }
    }
    HOST_DEVICE void parseFactor() {
        if (pos >= expr_size) return;
        char c = expr[pos];
    
        // 1. Handle Unary Plus (just skip it)
        if (c == '+') {
            ++pos;
            parseFactor();
            return;
        }
    
        // 2. Handle Unary Minus / Negative Numbers
        if (c == '-') {
            ++pos;
            if (pos < expr_size) {
                char next = expr[pos];
                
                // Check if it's a digit or decimal point
                bool isDigit = ExprTools::my_isdigit(next) || next == '.';
                
                // Check if it's an imaginary unit (i, j, k...) 
                // BUT ensure it's not the start of a function call like -j(c)
                bool isStandaloneUnit = ExprTools::isImaginaryChar<Dim>(next) && 
                                        (pos + 1 >= expr_size || expr[pos+1] != '(');
    
                if (isDigit || isStandaloneUnit) {
                    parseNumber(true); // Bake the negative sign
                    return;
                }
            }
            
            // If it's -j(c) or -(expression), we parse it normally and negate the result
            parseFactor();
            emitUnary(OP_NEG);
            return;
        }
    
    
        // 3. Parentheses
        if (c == '(') {
            ++pos;
            parseExpression();
            if (pos < expr_size && expr[pos] == ')') ++pos;
            return;
        }
    
        // 4. Numbers (Positive)
        if (ExprTools::my_isdigit(c) || c == '.') {
            parseNumber(false);
            return;
        }
    
        // 5. Identifiers (Variables, Functions, or Fallbacks)
        size_t start = pos;
        char name[32] = {0};
        size_t len = 0;
    
        while (pos < expr_size && ExprTools::isIdentifierChar(expr[pos]) && len < 31) {
            name[len++] = expr[pos++];
        }
        name[len] = '\0';
    
        if (len > 0) {
            // Function call
            if (pos < expr_size && expr[pos] == '(') {
                parseFunction(name);
                return;
            }
    
            // Known variable or array
            if (ExprTools::containsToken(oVarNames, 5, name) ||
                ExprTools::containsToken(varNames, 10, name) ||
                (pos < expr_size && expr[pos] == '[')) 
            {
                parseVarOrArray(name);
                return;
            }
    
            // Lone imaginary unit (i, j, k...)
            if (len == 1 && ExprTools::isImaginaryChar<Dim>(name[0])) {
                pos = start; // Backtrack to let parseNumber handle the unit
                parseNumber(false);
                return;
            }
    
            // FALLBACK: Unknown identifier (e.g., 'q')
            // We already advanced 'pos' in the while loop, so we just emit 0 and move on.
            emitConstant(Hypercomplex<Dim>(0.0));
            return;
        }
    
        // Ultimate fallback for stray characters
        if (pos < expr_size) ++pos;
        emitConstant(Hypercomplex<Dim>(0.0));
    }

    HOST_DEVICE void parseNumber(bool isNegative) {
        const int MAX_NUM_LEN = 63;
        char number[MAX_NUM_LEN + 1];
        int idx = 0;

        Hypercomplex<Dim> constVal;          // all components zero
        char ident = '\0';                   // imaginary unit, if any

        while (pos < expr_size) {
            char c = expr[pos];

            // --- Check for a valid imaginary unit for this Dim ---
            bool isImagUnit = false;
            if constexpr (Dim == 2) {
                isImagUnit = (c == 'i');
            } else if constexpr (Dim == 4) {
                isImagUnit = (c == 'i' || c == 'j' || c == 'k');
            } else if constexpr (Dim == 8) {
                isImagUnit = (c >= 'i' && c <= 'o');  // i,j,k,l,m,n,o
            }
            if (isImagUnit) {
                ident = c;
                ++pos;
                break;
            }

            // --- Parse number (digits, decimal point, scientific notation) ---
            if (ExprTools::my_isdigit(c) || c == '.' || c == 'e' || c == 'E') {
                if (idx < MAX_NUM_LEN) number[idx++] = c;
                ++pos;

                // consume optional sign after 'e'/'E'
                if ((c == 'e' || c == 'E') && pos < expr_size) {
                    char next = expr[pos];
                    if (next == '+' || next == '-') {
                        if (pos + 1 < expr_size && ExprTools::my_isdigit(expr[pos+1])) {
                            if (idx < MAX_NUM_LEN) number[idx++] = next;
                            ++pos;
                        }
                    }
                }
                continue;
            }

            break;   // not part of a number or imaginary unit
        }

        number[idx] = '\0';
        DefaultType val = (idx == 0) ? 1.0 : ExprTools::my_atof(number);
        if (isNegative) val = -val;

        // --- Assign value to the appropriate component ---
        switch (ident) {
            case '\0':               // real part
                constVal.v[0] = val;
                break;
            case 'i':
                if constexpr (Dim >= 2) constVal.v[1] = val;
                else { hasError = true; return; }
                break;
            case 'j':
                if constexpr (Dim >= 4) constVal.v[2] = val;
                else { hasError = true; return; }
                break;
            case 'k':
                if constexpr (Dim >= 4) constVal.v[3] = val;
                else { hasError = true; return; }
                break;
            case 'l':
                if constexpr (Dim >= 8) constVal.v[4] = val;
                else { hasError = true; return; }
                break;
            case 'm':
                if constexpr (Dim >= 8) constVal.v[5] = val;
                else { hasError = true; return; }
                break;
            case 'n':
                if constexpr (Dim >= 8) constVal.v[6] = val;
                else { hasError = true; return; }
                break;
            case 'o':
                if constexpr (Dim >= 8) constVal.v[7] = val;
                else { hasError = true; return; }
                break;
            default:
                hasError = true;
                return;
        }

        emitConstant(constVal);
    }

    HOST_DEVICE void parseVarOrArray(const char* name) {
        int v = findVariableIndex<Dim>(name,varEntries,numVars);
        if (v != -1) {
            emitLoadVar((uint32_t)v);
            return;
        }

        if (pos < expr_size && expr[pos] == '[') {
            int a = findArrayIndex(name,arrEntries,numArrays);
            if (a != -1) {
                ++pos;
                parseExpression();
                if (expr[pos] == ']') {
                    ++pos;
                    emitArrayAccess((uint32_t)a, vsp - 1);
                    return;
                }
            }
        }

        emitConstant(Hypercomplex<Dim>(0.0));
    }

    HOST_DEVICE bool parseUnaryFunction(unsigned int h) {
        // common functions (always available)
        if (h == H_SQRT)  { emitUnary(OP_SQRT); return true; }
        if (h == H_LOG)   { emitUnary(OP_LOG);  return true; }
        if (h == H_EXP)   { emitUnary(OP_EXP);  return true; }
        if (h == H_SIN)   { emitUnary(OP_SIN);  return true; }
        if (h == H_COS)   { emitUnary(OP_COS);  return true; }
        if (h == H_TAN)   { emitUnary(OP_TAN);  return true; }
        if (h == H_SINH)  { emitUnary(OP_SINH); return true; }
        if (h == H_COSH)  { emitUnary(OP_COSH); return true; }
        if (h == H_TANH)  { emitUnary(OP_TANH); return true; }
        if (h == H_ARG)   { emitUnary(OP_ARG);  return true; }
        if (h == H_CONJ)  { emitUnary(OP_CONJ); return true; }
        if (h == H_MAG)   { emitUnary(OP_MAG);  return true; }
        if (h == H_ABS)   { emitUnary(OP_ABS);  return true; }
        if (h == H_RE)    { emitUnary(OP_REAL); return true; }
        if (h == H_IM)    { emitUnary(OP_IMAG); return true; }
        if (h == H_I)     { emitUnary(OP_I);    return true; }

        // dimension‑dependent imaginary unit extraction
        if constexpr (Dim >= 4) {
            if (h == H_J) { emitUnary(OP_J); return true; }
            if (h == H_K) { emitUnary(OP_K); return true; }
        }
        if constexpr (Dim >= 8) {
            if (h == H_L) { emitUnary(OP_L); return true; }
            if (h == H_M) { emitUnary(OP_M); return true; }
            if (h == H_N) { emitUnary(OP_N); return true; }
            if (h == H_O) { emitUnary(OP_O); return true; }
        }

        // other common functions
        if (h == H_ROUND) { emitUnary(OP_ROUND); return true; }
        if (h == H_SIGN)  { emitUnary(OP_SIGN);  return true; }
        if (h == H_GAMMA) { emitUnary(OP_GAMMA); return true; }
        if (h == H_ZETA)  { emitUnary(OP_ZETA);  return true; }
        if (h == H_AIRY)  { emitUnary(OP_AIRY);  return true; }

        return false;
    }

    HOST_DEVICE bool parseBinaryFunction(unsigned int h) {
        switch (h) {
            case H_LOGN:     emitBinary(OP_LOGN); return true;
            case H_POW:      emitBinary(OP_POW);  return true;
            case H_ROOT:     emitBinary(OP_ROOT); return true;
            case H_MAX:      emitBinary(OP_MAX);  return true;
            case H_MIN:      emitBinary(OP_MIN);  return true;
            case H_DOT:      emitBinary(OP_DOT);  return true;

            // case H_CIRCLE:   emitBinary(OP_CIRCLE);   return true;
            // case H_RECT:     emitBinary(OP_RECT);     return true;
            // case H_TRIANGLE: emitBinary(OP_TRIANGLE); return true;
        }
        return false;
    }

    HOST_DEVICE bool parseTernaryFunction(unsigned int h) {
        switch (h) {
            case H_IF:    emitTernary(OP_IF); return true;
            case H_SPIN:  emitTernary(OP_SPIN); return true;
            case H_ROT:   emitTernary(OP_ROTATION); return true;
            // case H_ELL:   emitTernary(OP_ELLIPSOID); return true;
            // case H_LINE:  emitTernary(OP_LINE); return true;
            case H_LERP:  emitTernary(OP_LERP); return true;

    #ifndef USE_CUDA
            case H_RAND:  emitTernary(OP_RAND); return true;
    #endif
        }
        return false;
    }

    HOST_DEVICE void parseFunction(const char* func) {
        unsigned int h = ExprTools::str2int(func);
        ++pos;

        // first argument (always exists)
        parseExpression();

        // try unary
        if (parseUnaryFunction(h)) {
            if (expr[pos] == ')') {
                ++pos;
                return;
            }
            emitConstant(Hypercomplex<Dim>(0.0));
            return;
        }

        // expect comma → binary or ternary
        if (pos >= expr_size || expr[pos] != ',') {
            emitConstant(Hypercomplex<Dim>(0.0));
            return;
        }
        ++pos;

        parseExpression();

        // try binary
        if (parseBinaryFunction(h)) {
            if (expr[pos] == ')') {
                ++pos;
                return;
            }
            emitConstant(Hypercomplex<Dim>(0.0));
            return;
        }

        // expect second comma → ternary
        if (pos >= expr_size || expr[pos] != ',') {
            emitConstant(Hypercomplex<Dim>(0.0));
            return;
        }
        ++pos;

        parseExpression();

        // try ternary
        if (parseTernaryFunction(h)) {
            if (expr[pos] == ')') {
                ++pos;
                return;
            }
        }

        // fallback
        emitConstant(Hypercomplex<Dim>(0.0));
    }
};


template <int Dim>
HOST_DEVICE inline void evaluateBytecode(
    Hypercomplex<Dim>& out,
    const Instruction<Dim>* code,
    size_t size,
    const VariableEntry<Dim>* vars,
    const ArrayEntry* arrays)
{
    alignas(16) Hypercomplex<Dim> R[MAX_REGISTERS];
    // bool hasError = false;

    for (size_t i = 0; i < size; ++i) {
        const Instruction<Dim>& inst = code[i];
        switch (inst.op) {

            case OP_CONSTANT:
                R[inst.out] = inst.data.constant;
                break;

            case OP_LOAD_VAR:
                R[inst.out] = *(vars[inst.data.varIndex].value);
                break;

            // --- arithmetic ---
            case OP_ADD: Hypercomplex<Dim>::sum(R[inst.out], R[inst.in1], R[inst.in2]); break;
            case OP_SUB: Hypercomplex<Dim>::sub(R[inst.out], R[inst.in1], R[inst.in2]); break;
            case OP_MUL: Hypercomplex<Dim>::mul(R[inst.out], R[inst.in1], R[inst.in2]); break;
            case OP_DIV: Hypercomplex<Dim>::div(R[inst.out], R[inst.in1], R[inst.in2]); break;
            case OP_POW: Hypercomplex<Dim>::pow(R[inst.out], R[inst.in1], R[inst.in2]); break;
            case OP_MUL_ADD: Hypercomplex<Dim>::mul_sum(R[inst.out], R[inst.in1], R[inst.in2], R[inst.in3]);
            break;

            case OP_MOD: R[inst.out] = R[inst.in1] % R[inst.in2]; break;


            // --- comparisons ---
            case OP_EQ:  R[inst.out] = Hypercomplex<Dim>((DefaultType)(R[inst.in1] == R[inst.in2])); break;
            case OP_GT:  R[inst.out] = Hypercomplex<Dim>((DefaultType)(R[inst.in1] > R[inst.in2])); break;
            case OP_LT:  R[inst.out] = Hypercomplex<Dim>((DefaultType)(R[inst.in1] < R[inst.in2])); break;
            case OP_COS_SIM: Hypercomplex<Dim>::cosSim(R[inst.out],R[inst.in1],R[inst.in2]); break;

            // --- unary ---
            case OP_NEG:  R[inst.out] = -R[inst.in1]; break;

            case OP_SQRT: Hypercomplex<Dim>::sqrt(R[inst.out],R[inst.in1]); break;
            case OP_LOG:  Hypercomplex<Dim>::log(R[inst.out], R[inst.in1]); break;
            case OP_EXP:  Hypercomplex<Dim>::exp(R[inst.out], R[inst.in1]); break;

            case OP_SIN:  Hypercomplex<Dim>::sin(R[inst.out], R[inst.in1]); break;
            case OP_COS: Hypercomplex<Dim>::cos(R[inst.out], R[inst.in1]); break;
            case OP_TAN:  Hypercomplex<Dim>::tan(R[inst.out], R[inst.in1]); break;

            case OP_SINH: Hypercomplex<Dim>::sinh(R[inst.out], R[inst.in1]); break;
            case OP_COSH: Hypercomplex<Dim>::cosh(R[inst.out], R[inst.in1]); break;
            case OP_TANH: Hypercomplex<Dim>::tanh(R[inst.out], R[inst.in1]); break;

            case OP_ARG:  Hypercomplex<Dim>::hc_arg(R[inst.out],R[inst.in1]); break;
            case OP_CONJ: R[inst.out]=R[inst.in1].conj(); break;
            case OP_MAG:  Hypercomplex<Dim>::hc_mag(R[inst.out],R[inst.in1]); break;
            case OP_ABS:  Hypercomplex<Dim>::abs(R[inst.out],R[inst.in1]); break;

            case OP_REAL: R[inst.out] = Hypercomplex<Dim>(R[inst.in1].v[0]); break;
            case OP_IMAG: Hypercomplex<Dim>::get_imag(R[inst.out], R[inst.in1]); break;

            case OP_I:
                R[inst.out] = Hypercomplex<Dim>(R[inst.in1].v[1]); // i component
                break;
            case OP_J:
                if constexpr (Dim >= 4)
                    R[inst.out] = Hypercomplex<Dim>(R[inst.in1].v[2]);
                // else
                //     hasError = true;
                break;
            case OP_K:
                if constexpr (Dim >= 4)
                    R[inst.out] = Hypercomplex<Dim>(R[inst.in1].v[3]);
                // else
                //     hasError = true;
                break;
            case OP_L:
                if constexpr (Dim >= 8)
                    R[inst.out] = Hypercomplex<Dim>(R[inst.in1].v[4]);
                // else
                //     hasError = true;
                break;

            case OP_ROUND: Hypercomplex<Dim>::round(R[inst.out],R[inst.in1]); break;
            case OP_SIGN:  Hypercomplex<Dim>::sign(R[inst.out],R[inst.in1]); break;
            case OP_GAMMA: Hypercomplex<Dim>::gamma(R[inst.out],R[inst.in1]); break;
            case OP_ZETA:  Hypercomplex<Dim>::zeta(R[inst.out],R[inst.in1]); break;
            case OP_AIRY:  Hypercomplex<Dim>::airy(R[inst.out],R[inst.in1]); break;

            // --- binary functions ---
            case OP_LOGN: Hypercomplex<Dim>::logn(R[inst.out],R[inst.in1],R[inst.in2]); break;
            case OP_ROOT: Hypercomplex<Dim>::root(R[inst.out],R[inst.in1],R[inst.in2]); break;
            case OP_MAX:  Hypercomplex<Dim>::maximum(R[inst.out],R[inst.in1],R[inst.in2]);break;
            case OP_MIN:  Hypercomplex<Dim>::minimum(R[inst.out],R[inst.in1],R[inst.in2]); break;
            case OP_DOT:  Hypercomplex<Dim>::hc_dot(R[inst.out],R[inst.in1],R[inst.in2]); break;

            // case OP_CIRCLE:   R[inst.out] = R[inst.in1].circle(R[inst.in2]); break;
            // case OP_RECT:     R[inst.out] = R[inst.in1].rect(R[inst.in2]); break;
            // case OP_TRIANGLE: R[inst.out] = R[inst.in1].triangle(R[inst.in2]); break;

            // --- ternary ---
            case OP_IF:
                R[inst.out] = (fabs(R[inst.in1].v[0]) > EPS)
                    ? R[inst.in2]
                    : R[inst.in3];
                break;

#ifndef USE_CUDA
            case OP_RAND:
                Hypercomplex<Dim>::random(R[inst.out],R[inst.in1],R[inst.in2], R[inst.in3]);
                break;
#endif

            case OP_SPIN:
                Hypercomplex<Dim>::rotate_in_circle(R[inst.out], R[inst.in1], R[inst.in2], R[inst.in3]);
                break;

            case OP_ROTATION:
                Hypercomplex<Dim>::rotation(R[inst.out], R[inst.in1], R[inst.in2], R[inst.in3]);
                break;

            // case OP_ELLIPSOID:
            //     R[inst.out] = R[inst.in1].ellipsoid(R[inst.in2], R[inst.in3]);
            //     break;

            // case OP_LINE:
            //     R[inst.out] = R[inst.in1].line(R[inst.in2], R[inst.in3]);
            //     break;

            case OP_LERP:
                Hypercomplex<Dim>::lerp(R[inst.out], R[inst.in1], R[inst.in2], R[inst.in3]);
                break;
                
            case OP_ARRAY_ACCESS: {
                uint32_t idx = (uint32_t)fabs(R[inst.in1].v[0]);
                const ArrayEntry& a = arrays[inst.data.arrayIndex];
                R[inst.out] = (idx < a.size)
                    ? Hypercomplex<Dim>(a.array[idx])
                    : Hypercomplex<Dim>(0.0);
                break;
            }

            default:
                break;
        }
    }
    out = R[0];
    return;
}





#endif // AST_PARSER_H