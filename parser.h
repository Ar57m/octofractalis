#ifndef AST_PARSER_H
#define AST_PARSER_H


#include <cstdio>
#include <functional>
#include <memory>
#include <algorithm>



#ifndef OCTO
    #include "custom_quaternion.h"
#else
    #include "custom_octonion.h"
#endif




struct ConstantNodeData {
    QuaternionOrOctonion value;
};

struct VariableNodeData {
    QuaternionOrOctonion* valuePtr;
};

struct ArrayAccessNodeData {
    double* array;
    uint32_t size;
    const struct ASTNode* indexNode;
};

struct UnaryFunctionNodeData {
    const struct ASTNode* operand;
    QuaternionOrOctonion (*func)(const QuaternionOrOctonion&);
};

struct BinaryFunctionNodeData {
    const struct ASTNode* operand1;
    const struct ASTNode* operand2;
    QuaternionOrOctonion (*func)(const QuaternionOrOctonion&, const QuaternionOrOctonion&);
};

struct TernaryFunctionNodeData {
    const struct ASTNode* operand1;
    const struct ASTNode* operand2;
    const struct ASTNode* operand3;
    QuaternionOrOctonion (*func)(const QuaternionOrOctonion&, const QuaternionOrOctonion&, const QuaternionOrOctonion&);
};

// AST Node structure with GPU-friendly qualifiers.
struct ASTNode {
    enum NodeType {
        CONSTANT,
        VARIABLE,
        ARRAY_ACCESS,
        UNARY_FUNCTION,
        BINARY_FUNCTION,
        TERNARY_FUNCTION
    } type;
    void* data;
    typedef QuaternionOrOctonion (*EvaluatorFunction)(const ASTNode* node);
    EvaluatorFunction evaluator;

    HOST_DEVICE ASTNode(NodeType t, void* d, EvaluatorFunction evalFunc)
        : type(t), data(d), evaluator(evalFunc) {}

    HOST_DEVICE QuaternionOrOctonion evaluate() const {
        return evaluator(this);
    }
};

HOST_DEVICE inline QuaternionOrOctonion evaluateConstantNode(const ASTNode* node) {
    ConstantNodeData* data = static_cast<ConstantNodeData*>(node->data);
    return data->value;
}

HOST_DEVICE inline QuaternionOrOctonion evaluateVariableNode(const ASTNode* node) {
    const VariableNodeData* data = static_cast<const VariableNodeData*>(node->data);
    return *(data->valuePtr);
} 

HOST_DEVICE inline QuaternionOrOctonion evaluateArrayAccessNode(const ASTNode* node) {
    ArrayAccessNodeData* data = static_cast<ArrayAccessNodeData*>(node->data);
    // Use fabs (which is device friendly) for floating point absolute value.
    uint32_t evaluatedIndex = static_cast<uint32_t>(fabs(data->indexNode->evaluate().real));
    return (evaluatedIndex < data->size) ? QuaternionOrOctonion(data->array[evaluatedIndex]) : QuaternionOrOctonion(0);
}

HOST_DEVICE inline QuaternionOrOctonion evaluateUnaryFunctionNode(const ASTNode* node) {
    UnaryFunctionNodeData* data = static_cast<UnaryFunctionNodeData*>(node->data);
    return data->func(data->operand->evaluate());
}

HOST_DEVICE inline QuaternionOrOctonion evaluateBinaryFunctionNode(const ASTNode* node) {
    BinaryFunctionNodeData* data = static_cast<BinaryFunctionNodeData*>(node->data);
    return data->func(data->operand1->evaluate(), data->operand2->evaluate());
}

HOST_DEVICE inline QuaternionOrOctonion evaluateTernaryFunctionNode(const ASTNode* node) {
    TernaryFunctionNodeData* data = static_cast<TernaryFunctionNodeData*>(node->data);
    return data->func(data->operand1->evaluate(), data->operand2->evaluate(), data->operand3->evaluate());
}




constexpr int MAX_EXPR_SIZE = 384;
constexpr size_t AST_NODE_BUFFER_SIZE = 5120;
constexpr size_t GENERIC_NODE_DATA_BUFFER_SIZE = 12288;

// ------------------- Fixed-Size Stack Allocator Template -------------------
template <typename T, size_t BufferSize>
struct StackAllocator {
    alignas(alignof(T)) unsigned char buffer[BufferSize];
    size_t offset;

    HOST_DEVICE StackAllocator() : offset(0) {}

    HOST_DEVICE T* allocate() {
        const size_t needed_size = sizeof(T);
        const size_t alignment   = alignof(T);

        uintptr_t base = reinterpret_cast<uintptr_t>(buffer);
        size_t current = offset;

        // Using bitmask (bit faster than %) addr & alignment
        uintptr_t mis  = (base + current) & (alignment - 1);
        if (mis != 0) current += (alignment - mis);

        if (current + needed_size > BufferSize) {
            return nullptr;
        }

        void* ptr = buffer + current;
        offset    = current + needed_size;
        return reinterpret_cast<T*>(ptr);
    }

    HOST_DEVICE void reset() { offset = 0; }
};


struct AlignedNodeData {
    alignas(16) union {
        ConstantNodeData       constant;
        VariableNodeData       variable;
        ArrayAccessNodeData    arrayAccess;
        UnaryFunctionNodeData  unary;
        BinaryFunctionNodeData binary;
        TernaryFunctionNodeData ternary;
    } data;
};

struct ASTAllocators {
    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE> nodeAllocator;
    StackAllocator<AlignedNodeData, GENERIC_NODE_DATA_BUFFER_SIZE> genericDataAllocator;
};


HOST_DEVICE ASTNode* createConstantNode(ASTAllocators& allocs, const QuaternionOrOctonion& val) {
    AlignedNodeData* slot = allocs.genericDataAllocator.allocate();
    if (!slot) return nullptr;
    slot->data.constant.value = val;

    ASTNode* node = allocs.nodeAllocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::CONSTANT, &slot->data.constant, evaluateConstantNode);
}

HOST_DEVICE ASTNode* createVariableNode(ASTAllocators& allocs, QuaternionOrOctonion* valuePtr) {
    AlignedNodeData* slot = allocs.genericDataAllocator.allocate();
    if (!slot) return nullptr;
    slot->data.variable.valuePtr = valuePtr;

    ASTNode* node = allocs.nodeAllocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::VARIABLE, &slot->data.variable, evaluateVariableNode);
}

HOST_DEVICE ASTNode* createArrayAccessNode(ASTAllocators& allocs, double* array, uint32_t size, ASTNode* indexNode) {
    AlignedNodeData* slot = allocs.genericDataAllocator.allocate();
    if (!slot) return nullptr;
    slot->data.arrayAccess.array = array;
    slot->data.arrayAccess.size = size;
    slot->data.arrayAccess.indexNode = indexNode;

    ASTNode* node = allocs.nodeAllocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::ARRAY_ACCESS, &slot->data.arrayAccess, evaluateArrayAccessNode);
}

HOST_DEVICE ASTNode* createUnaryFunctionNode(ASTAllocators& allocs, ASTNode* operand, QuaternionOrOctonion (*func)(const QuaternionOrOctonion&)) {
    AlignedNodeData* slot = allocs.genericDataAllocator.allocate();
    if (!slot) return nullptr;
    slot->data.unary.operand = operand;
    slot->data.unary.func = func;

    ASTNode* node = allocs.nodeAllocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::UNARY_FUNCTION, &slot->data.unary, evaluateUnaryFunctionNode);
}

HOST_DEVICE ASTNode* createBinaryFunctionNode(ASTAllocators& allocs, ASTNode* operand1, ASTNode* operand2, QuaternionOrOctonion (*func)(const QuaternionOrOctonion&, const QuaternionOrOctonion&)) {
    AlignedNodeData* slot = allocs.genericDataAllocator.allocate();
    if (!slot) return nullptr;
    slot->data.binary.operand1 = operand1;
    slot->data.binary.operand2 = operand2;
    slot->data.binary.func = func;

    ASTNode* node = allocs.nodeAllocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::BINARY_FUNCTION, &slot->data.binary, evaluateBinaryFunctionNode);
}

HOST_DEVICE ASTNode* createTernaryFunctionNode(ASTAllocators& allocs, ASTNode* operand1, ASTNode* operand2, ASTNode* operand3, QuaternionOrOctonion (*func)(const QuaternionOrOctonion&, const QuaternionOrOctonion&, const QuaternionOrOctonion&)) {
    AlignedNodeData* slot = allocs.genericDataAllocator.allocate();
    if (!slot) return nullptr;
    slot->data.ternary.operand1 = operand1;
    slot->data.ternary.operand2 = operand2;
    slot->data.ternary.operand3 = operand3;
    slot->data.ternary.func = func;

    ASTNode* node = allocs.nodeAllocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::TERNARY_FUNCTION, &slot->data.ternary, evaluateTernaryFunctionNode);
}

// ------------------- GPU-Friendly String Comparison -------------------
class ExprTools {
public:
    HOST_DEVICE inline static constexpr bool isImaginaryChar(const char c) {
    #ifdef OCTO
        return (c > 'h' && c < 'p');
    #else
        return (c > 'h' && c < 'l');
    #endif
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


// VariableEntry and ArrayEntry structures.
struct VariableEntry {
    const char* name;
    QuaternionOrOctonion* value;
};

struct ArrayEntry {
    const char* name;
    double* array;
    uint32_t size;
};

HOST_DEVICE const VariableEntry* findVariable(const char* name, const VariableEntry* varEntries, size_t numVars) {
    for (size_t i = 0; i < numVars; ++i) {
        if (ExprTools::my_strcmp(varEntries[i].name, name) == 0) {
            return &varEntries[i];
        }
    }
    return nullptr;
}

HOST_DEVICE const ArrayEntry* findArray(const char* name, const ArrayEntry* arrEntries, size_t numArrays) {
    for (size_t i = 0; i < numArrays; ++i) {
        if (ExprTools::my_strcmp(arrEntries[i].name, name) == 0) {
            return &arrEntries[i];
        }
    }
    return nullptr;
}




class Parser {
private:
    char expr[MAX_EXPR_SIZE];
    size_t expr_size;
    size_t pos;
    VariableEntry* varEntries;
    size_t numVars;
    ArrayEntry* arrEntries;
    size_t numArrays;

    ASTAllocators allocators;
    
    ASTNode* error_zero = createConstantNode(allocators, QuaternionOrOctonion(0.0));


public:
    HOST_DEVICE Parser(const char* expr, size_t expr_size,
        VariableEntry* vars, size_t numVars,
        ArrayEntry* arrays, size_t numArrays
        )
        : expr_size(expr_size), pos(0), varEntries(vars), numVars(numVars),
        arrEntries(arrays), numArrays(numArrays)
    {
        size_t i = 0;
        for (; i < expr_size && i < MAX_EXPR_SIZE - 1; i++) {
            this->expr[i] = expr[i];
        }

        this->expr[i] = '\0';
    }

    HOST_DEVICE ASTNode* parse() {
        return parseExpression();
    }

private:


    const char* varNames[10] = {"z", "c", "it", "v", "p", "f", "dif", "dx", "dy", "dz"};
    const char* oVarNames[5] = {"pi", "e", "phi", "x", "y"};
    
    
    
    
    HOST_DEVICE bool shiftRight(int posi, int shift) {
        int len = ExprTools::my_strlen(this->expr);
        if (len + shift >= MAX_EXPR_SIZE)
            return false;
        for (int i = len; i > posi - 1; i--) {
            this->expr[i + shift] = this->expr[i];
        }
        return true;
    }

    HOST_DEVICE void shiftLeft(int posi, int shift) {
        int len = ExprTools::my_strlen(this->expr);
        for (int i = posi; i < len + 1; i++) {
            this->expr[i - shift] = this->expr[i];
        }
    }

    HOST_DEVICE void eraseBeforePos(int posi) {
        if (posi < 1) return;
        int len = ExprTools::my_strlen(this->expr);
        int newIndex = 0;
        for (int i = posi; i < len + 1; i++) {
            this->expr[newIndex++] = this->expr[i];
        }
    }

    HOST_DEVICE void replaceTokens(const char* tokens[], int numTokens) {
        for (int t = 0; t < numTokens; t++) {
            const char* token = tokens[t];
            char rep[128];
            int rep_index = 0;
            rep[rep_index++] = '(';
            for (int i = 0; token[i] != '\0' && rep_index < 127; i++) {
                rep[rep_index++] = token[i];
            }
            const char* suffix = "+0.000001";
            for (int i = 0; suffix[i] != '\0' && rep_index < 127; i++) {
                rep[rep_index++] = suffix[i];
            }
            if (rep_index < 127) rep[rep_index++] = ')';
            rep[rep_index] = '\0';

            int tokenLen = 0;
            while (token[tokenLen] != '\0') {
                tokenLen++;
            }
            int repLen = rep_index;

            char temp[MAX_EXPR_SIZE];
            int dst = 0;
            int posi = 0;

            while (expr[posi] != '\0' && dst < MAX_EXPR_SIZE - 1) {
                if (ExprTools::startsWith(expr, posi, token) && ExprTools::isTokenBoundary(expr, posi, tokenLen)) {
                    if (dst + repLen >= MAX_EXPR_SIZE - 1) {
                        break;
                    }
                    for (int j = 0; rep[j] != '\0'; j++) {
                        temp[dst++] = rep[j];
                    }
                    posi += tokenLen;
                } else {
                    temp[dst++] = expr[posi++];
                }
            }
            temp[dst] = '\0';

            for (int i = 0; i < MAX_EXPR_SIZE - 1; i++) {
                this->expr[i] = temp[i];
                if (temp[i] == '\0') break;
            }
            this->expr[MAX_EXPR_SIZE - 1] = '\0';
        }
    }

    HOST_DEVICE ASTNode* parseExpression() {
        ASTNode* node = parseTerm();
        while (pos < expr_size) {
            switch (this->expr[pos]) {
                case '+': {
                    ++pos;
                    ASTNode* right = parseTerm();
                    node = createBinaryFunctionNode(
                        allocators,
                        node, right,
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a + b; }
                    );
                    break;
                }
                case '-': {
                    ++pos;
                    ASTNode* right = parseTerm();
                    node = createBinaryFunctionNode(
                        allocators,
                        node, right,
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a - b; }
                    );
                    break;
                }
                default: return node;
            }
        }
        return node;
    }
    HOST_DEVICE ASTNode* parseTerm() {
        ASTNode* node = parseFactor();
    
        while (pos < expr_size) {
            switch (this->expr[pos]) {
                case '*':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a * b; });
                    break;
    
                case '/':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a / b; });
                    break;
    
                case '%':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a % b; });
                    break;
    
                case '^':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.pow(b); });
                    break;

                case '=':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return QuaternionOrOctonion(a == b); });
                    break;
                    
                case '&':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return QuaternionOrOctonion(a.cosSim(b)); });
                    break;
                    
                case '>':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return QuaternionOrOctonion(a > b); });
                    break;
                    
                case '<':
                    ++pos;
                    node = createBinaryFunctionNode(
                        allocators,
                        node, parseFactor(),
                        [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return QuaternionOrOctonion(a < b); });
                    break;
                
                default:
                    return node;
            }
        }
        return node;
    }


    HOST_DEVICE ASTNode* parseFactor() {
        const char exprPosBefore = this->expr[pos];
        size_t tempPos = pos;
        
        switch (exprPosBefore) {
            case '+':
                ++pos;
                return parseFactor();
                break;
            case '-':
                ++pos;
                return createUnaryFunctionNode(allocators, parseFactor(), 
                [](const QuaternionOrOctonion& a) { return -a; });
                break;
            case '(':
                ++pos;
                ASTNode* node = parseExpression();
                if (expr[pos] != ')') return error_zero;
                ++pos;
                return node;
                break;
        }
        
        char name[32] = {0};
        size_t nameIndex = 0;

        while (tempPos < expr_size && ExprTools::isIdentifierChar(expr[tempPos]) && nameIndex < 31) {
            name[nameIndex++] = this->expr[tempPos++];
        }
        name[nameIndex] = '\0';
        const char exprPos = this->expr[tempPos];
        
        if ( tempPos < expr_size && exprPos == '(' ) {
            pos = tempPos;
            return parseFunction(name);
        } else if ( ExprTools::containsToken(oVarNames, 5, name) || ExprTools::containsToken(varNames, 10, name) || exprPos == '[' ) {
            pos = tempPos;
            return parseVarOrArray(name);
        } else if (ExprTools::my_isdigit(exprPosBefore) || exprPosBefore == '.' ||  ExprTools::isImaginaryChar(exprPosBefore) ) {
            return parseNumber();
        }
        
        return error_zero;
    } 
    
    HOST_DEVICE ASTNode* parseNumber() {
        const int MAX_NUM_LEN = 63; // leave space for '\0'
        char number[MAX_NUM_LEN + 1];
        int num_index = 0;
        DefaultType realPart = 0.0, imagPart = 0.0, jPart = 0.0, kPart = 0.0; 
    #ifdef OCTO
        DefaultType lPart = 0.0, mPart = 0.0, nPart = 0.0, oPart = 0.0;
    #endif
    
        char identifier = '\0';
        char exprpos = this->expr[pos];
        bool imag = ExprTools::isImaginaryChar(exprpos);
    
        while (pos < expr_size && (ExprTools::my_isdigit(exprpos) || exprpos == '.' || imag || exprpos == 'e' || exprpos == 'E')) {
            if (imag) {
                identifier = this->expr[pos++];
                break;
            } else if (exprpos == 'e' || exprpos == 'E') {
                if (num_index < MAX_NUM_LEN) number[num_index++] = this->expr[pos++];
                if (pos < expr_size && (this->expr[pos] == '+' || this->expr[pos] == '-')) {
                    if (num_index < MAX_NUM_LEN) number[num_index++] = this->expr[pos++];
                }
            } else {
                if (num_index < MAX_NUM_LEN) number[num_index++] = this->expr[pos++];
                else pos++; // skip excess to avoid overflow
            }
            exprpos = this->expr[pos];
            imag = ExprTools::isImaginaryChar(exprpos);
        }
        number[num_index] = '\0';
    
        if (num_index == 0) {
            const char* defaultStr = "0.0";
            int i = 0;
            while (defaultStr[i] != '\0' && i < MAX_NUM_LEN) {
                number[i] = defaultStr[i];
                ++i;
            }

            number[i] = '\0';
        }
    
        const DefaultType parsedValue =
            (identifier != '\0' && ExprTools::my_strcmp(number, "0.0") == 0)
            ? 1.0
            : ExprTools::my_atof(number);
    
        switch (identifier) {
            case 'i': imagPart = parsedValue; break;
            case 'j': jPart = parsedValue; break;
            case 'k': kPart = parsedValue; break;
    #ifdef OCTO
            case 'l': lPart = parsedValue; break;
            case 'm': mPart = parsedValue; break;
            case 'n': nPart = parsedValue; break;
            case 'o': oPart = parsedValue; break;
    #endif
            default: realPart = parsedValue; break;
        }
    
    #ifdef OCTO
        return createConstantNode(allocators, QuaternionOrOctonion(realPart, imagPart, jPart, kPart, lPart, mPart, nPart, oPart));
    #else
        return createConstantNode(allocators, QuaternionOrOctonion(realPart, imagPart, jPart, kPart));
    #endif
    }

    HOST_DEVICE ASTNode* parseVarOrArray(const char* name) {

        const VariableEntry* varEntry = findVariable(name, varEntries, numVars);
        if (varEntry != nullptr) {
            return createVariableNode(allocators, varEntry->value);
        }
        
        // If not a plain variable, check if it is an array access.
        if (pos < expr_size && this->expr[pos] == '[') {
            const ArrayEntry* arrEntry = findArray(name, arrEntries, numArrays);
            if (arrEntry != nullptr) {
                ++pos; // Skip the '[' character.
                ASTNode* indexNode = parseExpression();
                if (pos < expr_size && this->expr[pos] == ']') {
                    ++pos; // Skip the ']' character.
                    return createArrayAccessNode(allocators, arrEntry->array, arrEntry->size, indexNode);
                }
            }
        }
        
        return error_zero;
    }

    // Helper for unary functions:
    HOST_DEVICE ASTNode* parseUnaryFunction(unsigned int hash, ASTNode* arg, const size_t old_pos) {
        switch (hash) {
            case ExprTools::str2int("sqrt"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.sqrt(); });
            case ExprTools::str2int("log"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.log(); });
            case ExprTools::str2int("sin"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.sin(); });
            case ExprTools::str2int("cos"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.cos(); });
            case ExprTools::str2int("tan"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.tan(); });
            case ExprTools::str2int("sinh"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.sinh(); });
            case ExprTools::str2int("cosh"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.cosh(); });
            case ExprTools::str2int("tanh"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.tanh(); });
            case ExprTools::str2int("arg"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.arg()); });
            case ExprTools::str2int("conj"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.conj()); });
            case ExprTools::str2int("mag"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.mag()); });
            case ExprTools::str2int("abs"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.abs(); });
            case ExprTools::str2int("exp"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.exp(); });
            case ExprTools::str2int("re"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.real); });
            case ExprTools::str2int("im"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(0.0, a.imag, a.j, a.k); });
            case ExprTools::str2int("i"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.imag); });
            case ExprTools::str2int("j"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.j); });
            case ExprTools::str2int("k"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.k); });
            #ifdef OCTO
            case ExprTools::str2int("l"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.l); });
            case ExprTools::str2int("m"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.m); });
            case ExprTools::str2int("n"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.n); });
            case ExprTools::str2int("o"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return QuaternionOrOctonion(a.o); });
            #endif
            case ExprTools::str2int("round"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.round(); });
            case ExprTools::str2int("sign"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.sign(); });
            case ExprTools::str2int("gamma"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.gamma(); });
            case ExprTools::str2int("zeta"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.zeta(); });
            case ExprTools::str2int("airy"):
                return createUnaryFunctionNode(allocators, arg,
                    [](const QuaternionOrOctonion& a) { return a.airy(); });
            case ExprTools::str2int("diff"):
            {
                // Backup the current expression into old_expr using a simple loop.
                char old_expr[MAX_EXPR_SIZE];
                for (int i = 0; i < MAX_EXPR_SIZE; ++i) {
                    old_expr[i] = this->expr[i];
                }
                
                size_t n_pos = pos;
                size_t old_expr_size = expr_size;
                
                pos = old_pos;
                eraseBeforePos(pos);
                replaceTokens(varNames, 10);
                expr_size = ExprTools::my_strlen(this->expr);
                pos = 0;
                ASTNode* arg2 = parseExpression();
                
                // Restore the original expression from the backup.
                for (int i = 0; i < MAX_EXPR_SIZE; ++i) {
                    this->expr[i] = old_expr[i];
                }
                
                pos = n_pos;
                expr_size = old_expr_size;
                
                return createBinaryFunctionNode(
                    allocators,
                    arg2, arg,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return (a - b) / 1e-6; }
                );
            }

            default:
                return nullptr;
        }
    }
    
    // Helper for binary functions:
    HOST_DEVICE ASTNode* parseBinaryFunction(unsigned int hash, ASTNode* arg1, ASTNode* arg2) {
        switch (hash) {
            case ExprTools::str2int("logn"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.logn(b); });
            case ExprTools::str2int("pow"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.pow(b); });
            case ExprTools::str2int("root"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.root(b); });
            case ExprTools::str2int("max"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.maximum(b); });
            case ExprTools::str2int("min"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.minimum(b); });
            case ExprTools::str2int("circle"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.circle(b); });
            case ExprTools::str2int("rect"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.rect(b); });
            case ExprTools::str2int("triangle"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.triangle(b); });
            case ExprTools::str2int("dot"):
                return createBinaryFunctionNode(allocators,
                    arg1, arg2, [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b) { return a.c_dot(b); });
            default:
                return nullptr;
        }
    }
    
    // Helper for ternary functions:
    HOST_DEVICE ASTNode* parseTernaryFunction(unsigned int hash, ASTNode* arg1, ASTNode* arg2, ASTNode* arg3) {
        switch (hash) {
            case ExprTools::str2int("if"):
                return createTernaryFunctionNode(allocators,
                    arg1, arg2, arg3,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b, const QuaternionOrOctonion& c) {
                        return abs(a.real) > 1e-9 ? b : c;
                    });
            #ifndef USE_CUDA
            case ExprTools::str2int("rand"):
                return createTernaryFunctionNode(allocators,
                    arg1, arg2, arg3,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b, const QuaternionOrOctonion& c) {
                        return a.generateRandom(b,c);
                    });
            #endif
            case ExprTools::str2int("spin"):
                return createTernaryFunctionNode(allocators,
                    arg1, arg2, arg3,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b, const QuaternionOrOctonion& c) {
                        return a.rotate_in_circle(b, c);
                    });
            case ExprTools::str2int("rotation"):
                return createTernaryFunctionNode(allocators,
                    arg1, arg2, arg3,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b, const QuaternionOrOctonion& c) {
                        return a.rotation(b, c);
                    });
            case ExprTools::str2int("ellipsoid"):
                return createTernaryFunctionNode(allocators,
                    arg1, arg2, arg3,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b, const QuaternionOrOctonion& c) {
                        return a.ellipsoid(b,c);
                    });
            case ExprTools::str2int("line"):
                return createTernaryFunctionNode(allocators,
                    arg1, arg2, arg3,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b, const QuaternionOrOctonion& c) {
                        return a.line(b,c);
                    });
            case ExprTools::str2int("lerp"):
                return createTernaryFunctionNode(allocators,
                    arg1, arg2, arg3,
                    [](const QuaternionOrOctonion& a, const QuaternionOrOctonion& b, const QuaternionOrOctonion& c) {
                        return a.lerp(b,c);
                    });
            default:
                return nullptr;
        }
    }
        
    // Main parseFunction, now much shorter:
    HOST_DEVICE ASTNode* parseFunction(const char* func) {
        const unsigned int functionHash = ExprTools::str2int(func);
        ++pos;

        size_t old_pos = pos;
    
        ASTNode* arg1 = parseExpression();

        ASTNode* node = parseUnaryFunction(functionHash, arg1, old_pos);
        if (node) {
            if (this->expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
    
        // If not unary, expect a comma for a binary function:
        if (this->expr[pos] != ',') return error_zero;
        ++pos;
        ASTNode* arg2 = parseExpression();
        node = parseBinaryFunction(functionHash, arg1, arg2);
        if (node) {
            if (this->expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
    
        // Otherwise, for ternary functions:
        if (this->expr[pos] != ',') return error_zero;
        ++pos;
        ASTNode* arg3 = parseExpression();
        node = parseTernaryFunction(functionHash, arg1, arg2, arg3);
        if (node) {
            if (this->expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
        return error_zero;
    }
    

    
};




#endif // AST_PARSER_H