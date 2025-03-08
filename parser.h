#ifndef AST_PARSER_H
#define AST_PARSER_H


#include <cstdio>
#include <functional>
#include <memory>
#include <algorithm>
#include "custom_quaternion.h"





struct ConstantNodeData {
    Quaternion value;
};

struct VariableNodeData {
    Quaternion* valuePtr;
};

struct ArrayAccessNodeData {
    double* array;
    uint32_t size;
    const struct ASTNode* indexNode;
};

struct UnaryFunctionNodeData {
    const struct ASTNode* operand;
    Quaternion (*func)(const Quaternion&);
};

struct BinaryFunctionNodeData {
    const struct ASTNode* operand1;
    const struct ASTNode* operand2;
    Quaternion (*func)(const Quaternion&, const Quaternion&);
};

struct TernaryFunctionNodeData {
    const struct ASTNode* operand1;
    const struct ASTNode* operand2;
    const struct ASTNode* operand3;
    Quaternion (*func)(const Quaternion&, const Quaternion&, const Quaternion&);
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
    typedef Quaternion (*EvaluatorFunction)(const ASTNode* node);
    EvaluatorFunction evaluator;

    HOST_DEVICE ASTNode(NodeType t, void* d, EvaluatorFunction evalFunc)
        : type(t), data(d), evaluator(evalFunc) {}

    HOST_DEVICE Quaternion evaluate() const {
        return evaluator(this);
    }
};

// Evaluation Functions marked as __host__ __device__
HOST_DEVICE Quaternion evaluateConstantNode(const ASTNode* node) {
    ConstantNodeData* data = static_cast<ConstantNodeData*>(node->data);
    return data->value;
}

HOST_DEVICE Quaternion evaluateVariableNode(const ASTNode* node) {
    const VariableNodeData* data = static_cast<const VariableNodeData*>(node->data);
    return *(data->valuePtr);
} 

HOST_DEVICE Quaternion evaluateArrayAccessNode(const ASTNode* node) {
    ArrayAccessNodeData* data = static_cast<ArrayAccessNodeData*>(node->data);
    // Use fabs (which is device friendly) for floating point absolute value.
    uint32_t evaluatedIndex = static_cast<uint32_t>(fabs(data->indexNode->evaluate().real));
    return (evaluatedIndex < data->size) ? Quaternion(data->array[evaluatedIndex]) : Quaternion(0);
}

HOST_DEVICE Quaternion evaluateUnaryFunctionNode(const ASTNode* node) {
    UnaryFunctionNodeData* data = static_cast<UnaryFunctionNodeData*>(node->data);
    return data->func(data->operand->evaluate());
}

HOST_DEVICE Quaternion evaluateBinaryFunctionNode(const ASTNode* node) {
    BinaryFunctionNodeData* data = static_cast<BinaryFunctionNodeData*>(node->data);
    return data->func(data->operand1->evaluate(), data->operand2->evaluate());
}

HOST_DEVICE Quaternion evaluateTernaryFunctionNode(const ASTNode* node) {
    TernaryFunctionNodeData* data = static_cast<TernaryFunctionNodeData*>(node->data);
    return data->func(data->operand1->evaluate(), data->operand2->evaluate(), data->operand3->evaluate());
}


// ------------------- Fixed-Size Stack Allocator Template -------------------
template <typename T, size_t BufferSize>
struct StackAllocator {
    unsigned char buffer[BufferSize];
    size_t offset;

    HOST_DEVICE StackAllocator() : offset(0) {}

    HOST_DEVICE T* allocate() {
        size_t needed_size = sizeof(T);
        if (offset + needed_size > BufferSize) {
            return nullptr; // Allocation failed
        }
        void* ptr = buffer + offset;
        offset += needed_size;
        return reinterpret_cast<T*>(ptr);
    }

    HOST_DEVICE void reset() {
        offset = 0;
    }
};



// Define buffer sizes for each allocator
constexpr size_t AST_NODE_BUFFER_SIZE                   = 16384;
constexpr size_t CONSTANT_NODE_DATA_BUFFER_SIZE         = 4096;
constexpr size_t VARIABLE_NODE_DATA_BUFFER_SIZE         = 4096;
constexpr size_t ARRAY_ACCESS_NODE_DATA_BUFFER_SIZE     = 4096;
constexpr size_t UNARY_FUNCTION_NODE_DATA_BUFFER_SIZE   = 4096;
constexpr size_t BINARY_FUNCTION_NODE_DATA_BUFFER_SIZE  = 4096;
constexpr size_t TERNARY_FUNCTION_NODE_DATA_BUFFER_SIZE = 4096;

constexpr int MAX_EXPR_SIZE = 512;



// ------------------- Node Creation Functions -------------------
HOST_DEVICE ASTNode* createConstantNode(
    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE>& allocator, 
    StackAllocator<ConstantNodeData, CONSTANT_NODE_DATA_BUFFER_SIZE>& dataAllocator, 
    const Quaternion& val
) {
    ConstantNodeData* data = dataAllocator.allocate();
    if (!data) return nullptr;
    data->value = val;
    ASTNode* node = allocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::CONSTANT, data, evaluateConstantNode);
}

HOST_DEVICE ASTNode* createVariableNode(
    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE>& allocator, 
    StackAllocator<VariableNodeData, VARIABLE_NODE_DATA_BUFFER_SIZE>& dataAllocator, 
    Quaternion* valuePtr
) { 
    VariableNodeData* data = dataAllocator.allocate();
    if (!data) return nullptr;
    data->valuePtr = valuePtr;
    ASTNode* node = allocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::VARIABLE, data, evaluateVariableNode);
}

HOST_DEVICE ASTNode* createArrayAccessNode(
    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE>& allocator, 
    StackAllocator<ArrayAccessNodeData, ARRAY_ACCESS_NODE_DATA_BUFFER_SIZE>& dataAllocator, 
    double* array, uint32_t size, ASTNode* indexNode
) {
    ArrayAccessNodeData* data = dataAllocator.allocate();
    if (!data) return nullptr;
    data->array = array;
    data->size = size;
    data->indexNode = indexNode;
    ASTNode* node = allocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::ARRAY_ACCESS, data, evaluateArrayAccessNode);
}

HOST_DEVICE ASTNode* createUnaryFunctionNode(
    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE>& allocator, 
    StackAllocator<UnaryFunctionNodeData, UNARY_FUNCTION_NODE_DATA_BUFFER_SIZE>& dataAllocator, 
    ASTNode* operand, Quaternion (*func)(const Quaternion&)
) {
    UnaryFunctionNodeData* data = dataAllocator.allocate();
    if (!data) return nullptr;
    data->operand = operand;
    data->func = func;
    ASTNode* node = allocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::UNARY_FUNCTION, data, evaluateUnaryFunctionNode);
}

HOST_DEVICE ASTNode* createBinaryFunctionNode(
    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE>& allocator, 
    StackAllocator<BinaryFunctionNodeData, BINARY_FUNCTION_NODE_DATA_BUFFER_SIZE>& dataAllocator, 
    ASTNode* operand1, ASTNode* operand2, Quaternion (*func)(const Quaternion&, const Quaternion&)
) {
    BinaryFunctionNodeData* data = dataAllocator.allocate();
    if (!data) return nullptr;
    data->operand1 = operand1;
    data->operand2 = operand2;
    data->func = func;
    ASTNode* node = allocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::BINARY_FUNCTION, data, evaluateBinaryFunctionNode);
}

HOST_DEVICE ASTNode* createTernaryFunctionNode(
    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE>& allocator, 
    StackAllocator<TernaryFunctionNodeData, TERNARY_FUNCTION_NODE_DATA_BUFFER_SIZE>& dataAllocator, 
    ASTNode* operand1, ASTNode* operand2, ASTNode* operand3, 
    Quaternion (*func)(const Quaternion&, const Quaternion&, const Quaternion&)
) {
    TernaryFunctionNodeData* data = dataAllocator.allocate();
    if (!data) return nullptr;
    data->operand1 = operand1;
    data->operand2 = operand2;
    data->operand3 = operand3;
    data->func = func;
    ASTNode* node = allocator.allocate();
    if (!node) return nullptr;
    return new (node) ASTNode(ASTNode::TERNARY_FUNCTION, data, evaluateTernaryFunctionNode);
}

// ------------------- GPU-Friendly String Comparison -------------------
// A simple __host__ __device__ string comparison function.
HOST_DEVICE int my_strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Example VariableEntry and ArrayEntry structures.
struct VariableEntry {
    const char* name;
    Quaternion* value;
};

struct ArrayEntry {
    const char* name;
    double* array;
    uint32_t size;
};

// Find functions now accept the entry arrays and their counts.
HOST_DEVICE const VariableEntry* findVariable(const char* name, const VariableEntry* varEntries, size_t numVars) {
    for (size_t i = 0; i < numVars; ++i) {
        if (my_strcmp(varEntries[i].name, name) == 0) {
            return &varEntries[i];
        }
    }
    return nullptr;
}

HOST_DEVICE const ArrayEntry* findArray(const char* name, const ArrayEntry* arrEntries, size_t numArrays) {
    for (size_t i = 0; i < numArrays; ++i) {
        if (my_strcmp(arrEntries[i].name, name) == 0) {
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

    StackAllocator<ASTNode, AST_NODE_BUFFER_SIZE> nodeAllocator;
    StackAllocator<ConstantNodeData, CONSTANT_NODE_DATA_BUFFER_SIZE> constantDataAllocator;
    StackAllocator<VariableNodeData, VARIABLE_NODE_DATA_BUFFER_SIZE> variableDataAllocator;
    StackAllocator<ArrayAccessNodeData, ARRAY_ACCESS_NODE_DATA_BUFFER_SIZE> arrayDataAllocator;
    StackAllocator<UnaryFunctionNodeData, UNARY_FUNCTION_NODE_DATA_BUFFER_SIZE> unaryDataAllocator;
    StackAllocator<BinaryFunctionNodeData, BINARY_FUNCTION_NODE_DATA_BUFFER_SIZE> binaryDataAllocator;
    StackAllocator<TernaryFunctionNodeData, TERNARY_FUNCTION_NODE_DATA_BUFFER_SIZE> ternaryDataAllocator;



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


    HOST_DEVICE static constexpr bool isImaginaryChar(const char c) {
        return (c > 'h' && c < 'l');
    }

    const char* tokens[10] = {"z", "c", "It", "v", "l", "f", "dif", "dx", "dy", "dz"};
    
    ASTNode* error_zero = createConstantNode(nodeAllocator, constantDataAllocator, Quaternion(0.0));

    HOST_DEVICE int my_strlen(const char* s) {
        int len = 0;
        while (s[len] != '\0') {
            len++;
        }
        return len;
    }
    
    HOST_DEVICE bool startsWith(int posi, const char* token) {
        int i = 0;
        while (token[i] != '\0') {
            if (expr[posi + i] == '\0' || expr[posi + i] != token[i])
                return false;
            i++;
        }
        return true;
    }

    HOST_DEVICE bool my_isdigit(char c) {
        return (c > 47 && c < 58);
    }


    HOST_DEVICE double my_atof(const char* str) {
        // Skip any leading whitespace.
        while (*str == ' ' || *str == '\t' || *str == '\n') {
            str++;
        }
        
        // Process optional sign.
        int sign = 1;
        if (*str == '-') {
            sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }
        
        double result = 0.0;
        
        // Process the integer part.
        while (*str >= '0' && *str <= '9') {
            result = result * 10.0 + (*str - '0');
            str++;
        }
        
        // Process the fractional part.
        if (*str == '.') {
            str++;
            double fraction = 0.0;
            double divisor = 10.0;
            while (*str >= '0' && *str <= '9') {
                fraction += (*str - '0') / divisor;
                divisor *= 10.0;
                str++;
            }
            result += fraction;
        }
        
        // Process the exponent part if present (e or E).
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
            double expMultiplier = 1.0;
            // Compute 10^exponent by simple multiplication.
            for (int i = 0; i < exponent; ++i) {
                expMultiplier *= 10.0;
            }
            if (expSign == -1) {
                result /= expMultiplier;
            } else {
                result *= expMultiplier;
            }
        }
        
        return sign * result;
    }

    // Check if character is valid in an identifier (letter, digit, or underscore).
    HOST_DEVICE bool isIdentifierChar(char c) {
        return ((c > 64 && c < 91 ) || //A-Z
                (c > 96 && c < 123 ) ); //a-z
    }

    // Check that the token found at position pos is a standalone token.
    HOST_DEVICE bool isTokenBoundary(int posi, int tokenLen) {
        // Left boundary: either pos==0 or previous char is not identifier.
        if (posi > 0 && isIdentifierChar(expr[posi - 1])) {
            return false;
        }
        // Right boundary: either token ends at '\0' or the next char is not identifier.
        if (expr[posi + tokenLen] != '\0' && isIdentifierChar(expr[posi + tokenLen])) {
            return false;
        }
        return true;
    }

    // Shift the expression to the right by 'shift' characters starting from pos.
    // Returns false if there isn’t enough space.
    HOST_DEVICE bool shiftRight(int posi, int shift) {
        int len = my_strlen(expr);
        if (len + shift >= MAX_EXPR_SIZE)
            return false;
        // Shift from the end (including '\0') to pos.
        for (int i = len; i >= posi; i--) {
            expr[i + shift] = expr[i];
        }
        return true;
    }

    // Shift the expression to the left by 'shift' characters starting from pos.
    HOST_DEVICE void shiftLeft(int posi, int shift) {
        int len = my_strlen(expr);
        for (int i = posi; i <= len; i++) {
            expr[i - shift] = expr[i];
        }
    }

    HOST_DEVICE void eraseBeforePos(int posi) {
        if (posi <= 0) return; // Nothing to erase
    
        int len = my_strlen(expr);
        int newIndex = 0;
    
        // Shift everything from `pos` forward to the beginning of `expr`
        for (int i = posi; i <= len; i++) { // Include `\0` at the end
            expr[newIndex++] = expr[i];
        }
    }
    
    
    
    HOST_DEVICE void replaceTokens(const char* tokens[], int numTokens) {
        for (int t = 0; t < numTokens; t++) {
            const char* token = tokens[t];
            // Build the replacement string into rep.
            char rep[64];  // Sufficient for "(" + token + "+0.000001)"
            int rep_index = 0;
            rep[rep_index++] = '(';
            for (int i = 0; token[i] != '\0' && rep_index < 63; i++) {
                rep[rep_index++] = token[i];
            }
            const char* suffix = "+0.000001";
            for (int i = 0; suffix[i] != '\0' && rep_index < 63; i++) {
                rep[rep_index++] = suffix[i];
            }
            rep[rep_index++] = ')';
            rep[rep_index] = '\0';
            
            // Calculate token length.
            int tokenLen = 0;
            while (token[tokenLen] != '\0') {
                tokenLen++;
            }
            int repLen = rep_index;
            
            // Create a temporary buffer to build the new expression.
            char temp[MAX_EXPR_SIZE];
            int dst = 0; // destination index for temp
            int posi = 0; // source index for expr
            
            while (expr[posi] != '\0' && dst < MAX_EXPR_SIZE - 1) {
                if (startsWith(posi, token) && isTokenBoundary(posi, tokenLen)) {
                    // Check if there's enough space in temp.
                    if (dst + repLen >= MAX_EXPR_SIZE - 1) {
                        // Not enough space: break out or handle the error.
                        break;
                    }
                    // Copy the replacement string into temp.
                    for (int j = 0; rep[j] != '\0'; j++) {
                        temp[dst++] = rep[j];
                    }
                    posi += tokenLen;
                } else {
                    temp[dst++] = expr[posi++];
                }
            }
            temp[dst] = '\0';
            
            // Copy the temporary result back to expr using a simple loop.
            for (int i = 0; i < MAX_EXPR_SIZE - 1; i++) {
                expr[i] = temp[i];
                if (temp[i] == '\0') break;
            }
            expr[MAX_EXPR_SIZE - 1] = '\0'; // Ensure null termination.
        }
    }
    
    


    HOST_DEVICE static constexpr unsigned int str2int(const char* str, int h = 0) {
        return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ static_cast<unsigned int>(str[h]);
    }

    HOST_DEVICE ASTNode* parseExpression() {
        ASTNode* node = parseTerm();
        while (pos < expr_size) {
            switch (expr[pos]) {
                case '+': {
                    ++pos;
                    ASTNode* right = parseTerm();
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, right,
                        [](const Quaternion& a, const Quaternion& b) { return a + b; }
                    );
                    break;
                }
                case '-': {
                    ++pos;
                    ASTNode* right = parseTerm();
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, right,
                        [](const Quaternion& a, const Quaternion& b) { return a - b; }
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
            switch (expr[pos]) {
                case '*':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a * b; });
                    break;
    
                case '/':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a / b; });
                    break;
    
                case '%':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a % b; });
                    break;
    
                case '^':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a.pow(b); });
                    break;

                case '=':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return Quaternion(a == b); });
                    break;
                    
                case '&':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return Quaternion(a.cosSim(b)); });
                    break;
                    
                case '>':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return Quaternion(a > b); });
                    break;
                    
                case '<':
                    ++pos;
                    node = createBinaryFunctionNode(
                        nodeAllocator, binaryDataAllocator,
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return Quaternion(a < b); });
                    break;
                
                default:
                    return node;
            }
        }
        return node;
    }

    HOST_DEVICE ASTNode* parseFactor() {
        switch (expr[pos]) {
            case '+':
                ++pos;
                return parseFactor();
                break;
            case '-':
                ++pos;
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, parseFactor(), 
                [](const Quaternion& a) { return -a; });
                break;
        }
        
        const char exprpos = expr[pos];
        if ( isIdentifierChar(exprpos) && !isImaginaryChar(exprpos) ) {
            return parseVariableOrFunction();
        } else if (my_isdigit(exprpos) || exprpos == '.' ||  isImaginaryChar(exprpos) ) {
            return parseNumber();
        } else if (exprpos == '(') {
            ++pos;
            ASTNode* node = parseExpression();
            if (expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
        return error_zero;
    }


    HOST_DEVICE ASTNode* parseNumber() {
        char number[256];
        int num_index = 0;
        double realPart = 0.0, imagPart = 0.0, jPart = 0.0, kPart = 0.0;
        char identifier = '\0';
        char exprpos = expr[pos];
        bool imag = isImaginaryChar(exprpos);
    
        while (pos < expr_size && (my_isdigit(exprpos) || exprpos == '.' || imag)) {
            if (imag) {
                identifier = expr[pos++];
                break;
            } else {
                number[num_index++] = expr[pos++];
            }
            exprpos = expr[pos];
            imag = isImaginaryChar(exprpos);
        }
        number[num_index] = '\0';
    
        // If no number was found, replace with "0.0" using a simple loop.
        if (num_index == 0) {
            const char* defaultStr = "0.0";
            int i = 0;
            while (defaultStr[i] != '\0' && i < (int)sizeof(number) - 1) {
                number[i] = defaultStr[i];
                i++;
            }
            number[i] = '\0';
        }
        
        const double parsedValue = (identifier != '\0' && my_strcmp(number, "0.0") == 0) ? 1.0 : my_atof(number);
        
        switch (identifier) {
            case 'i':
                imagPart = parsedValue;
                break;
            case 'j':
                jPart = parsedValue;
                break;
            case 'k':
                kPart = parsedValue;
                break;
            default:
                realPart = parsedValue;
                break;
        }
        
        return createConstantNode(nodeAllocator, constantDataAllocator, Quaternion(realPart, imagPart, jPart, kPart));
    }

    HOST_DEVICE ASTNode* parseVariableOrFunction() {
        char name[128] = {};
        size_t nameIndex = 0;
    
        // Collect alphabetic characters to form the identifier.
        while (pos < expr_size && isIdentifierChar(expr[pos]) && nameIndex < sizeof(name) - 1) {
            name[nameIndex++] = expr[pos++];
        }
        name[nameIndex] = '\0'; // Null terminate the identifier
    
        // First try to find a variable with the given name.
        const VariableEntry* varEntry = findVariable(name,varEntries,numVars);
        if (varEntry != nullptr) {
            return createVariableNode(nodeAllocator, variableDataAllocator, varEntry->value);
        }
        
        // If not a plain variable, check if it might be a function or an array.
        if (pos < expr_size) {
            if (expr[pos] == '(') {
                // It’s a function call, so parse as such.
                return parseFunction(name);
            } else if (expr[pos] == '[') {
                // Check if it is a defined array.
                const ArrayEntry* arrEntry = findArray(name,arrEntries,numArrays);
                if (arrEntry != nullptr) {
                    ++pos; // Skip the '[' character.
                    ASTNode* idx = parseExpression();
                    if (pos < expr_size && expr[pos] == ']') {
                        ++pos; // Skip the ']' character.
                        return createArrayAccessNode(nodeAllocator, arrayDataAllocator, arrEntry->array, arrEntry->size, idx);
                    }
                }
            }
        }
        return error_zero;
    }


    // Helper for unary functions:
    HOST_DEVICE ASTNode* parseUnaryFunction(unsigned int hash, ASTNode* arg, const size_t old_pos) {
        switch (hash) {
            case str2int("sqrt"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.sqrt(); });
            case str2int("log"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.log(); });
            case str2int("sin"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.sin(); });
            case str2int("cos"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.cos(); });
            case str2int("tan"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.tan(); });
            case str2int("sinh"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.sinh(); });
            case str2int("cosh"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.cosh(); });
            case str2int("tanh"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.tanh(); });
            case str2int("arg"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(a.arg()); });
            case str2int("conj"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(a.conj()); });
            case str2int("mag"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(a.mag()); });
            case str2int("abs"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.abs(); });
            case str2int("exp"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.exp(); });
            case str2int("re"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(a.real); });
            case str2int("Im"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(0.0, a.imag, a.j, a.k); });
            case str2int("I"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(a.imag); });
            case str2int("J"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(a.j); });
            case str2int("K"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return Quaternion(a.k); });
            case str2int("round"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.round(); });
            case str2int("sign"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.sign(); });
            case str2int("gamma"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.gamma(); });
            case str2int("zeta"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.zeta(); });
            case str2int("airy"):
                return createUnaryFunctionNode(nodeAllocator, unaryDataAllocator, arg,
                    [](const Quaternion& a) { return a.airy(); });
            case str2int("diff"):
            {
                // Backup the current expression into old_expr using a simple loop.
                char old_expr[MAX_EXPR_SIZE];
                for (int i = 0; i < MAX_EXPR_SIZE; ++i) {
                    old_expr[i] = expr[i];
                }
                
                size_t n_pos = pos;
                size_t old_expr_size = expr_size;
                
                pos = old_pos;
                eraseBeforePos(pos);
                replaceTokens(tokens, 3);
                expr_size = my_strlen(expr);
                pos = 0;
                ASTNode* arg2 = parseExpression();
                
                // Restore the original expression from the backup.
                for (int i = 0; i < MAX_EXPR_SIZE; ++i) {
                    expr[i] = old_expr[i];
                }
                
                pos = n_pos;
                expr_size = old_expr_size;
                
                return createBinaryFunctionNode(
                    nodeAllocator, binaryDataAllocator,
                    arg2, arg,
                    [](const Quaternion& a, const Quaternion& b) { return (a - b) / 1e-6; }
                );
            }

            default:
                return nullptr;
        }
    }
    
    // Helper for binary functions:
    HOST_DEVICE ASTNode* parseBinaryFunction(unsigned int hash, ASTNode* arg1, ASTNode* arg2) {
        switch (hash) {
            case str2int("logn"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.logn(b); });
            case str2int("pow"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.pow(b); });
            case str2int("root"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.root(b); });
            case str2int("max"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.maximum(b); });
            case str2int("min"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.minimum(b); });
            case str2int("circle"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.circle(b); });
            case str2int("square"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.square(b); });
            case str2int("triangle"):
                return createBinaryFunctionNode(nodeAllocator, binaryDataAllocator,
                    arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.triangle(b); });
            default:
                return nullptr;
        }
    }
    
    // Helper for ternary functions:
    HOST_DEVICE ASTNode* parseTernaryFunction(unsigned int hash, ASTNode* arg1, ASTNode* arg2, ASTNode* arg3) {
        switch (hash) {
            case str2int("If"):
                return createTernaryFunctionNode(nodeAllocator, ternaryDataAllocator,
                    arg1, arg2, arg3,
                    [](const Quaternion& a, const Quaternion& b, const Quaternion& c) {
                        return abs(a.real) > 1e-9 ? b : c;
                    });
            #ifndef USE_CUDA
            case str2int("rand"):
                return createTernaryFunctionNode(nodeAllocator, ternaryDataAllocator,
                    arg1, arg2, arg3,
                    [](const Quaternion& a, const Quaternion& b, const Quaternion& c) {
                        return a.generateRandom(b,c);
                    });
            #endif
            case str2int("rotate"):
                return createTernaryFunctionNode(nodeAllocator, ternaryDataAllocator,
                    arg1, arg2, arg3,
                    [](const Quaternion& a, const Quaternion& b, const Quaternion& c) {
                        return a.rotate_in_circle(b, c);
                    });
            case str2int("rotation"):
                return createTernaryFunctionNode(nodeAllocator, ternaryDataAllocator,
                    arg1, arg2, arg3,
                    [](const Quaternion& a, const Quaternion& b, const Quaternion& c) {
                        return a.rotation(b, c);
                    });
            case str2int("ellipsoid"):
                return createTernaryFunctionNode(nodeAllocator, ternaryDataAllocator,
                    arg1, arg2, arg3,
                    [](const Quaternion& a, const Quaternion& b, const Quaternion& c) {
                        return a.ellipsoid(b,c);
                    });
            default:
                return nullptr;
        }
    }
        
    // Main parseFunction, now much shorter:
    HOST_DEVICE ASTNode* parseFunction(const char* func) {
        const unsigned int functionHash = str2int(func);
        ++pos;

        size_t old_pos = pos;
    
        ASTNode* arg1 = parseExpression();

        ASTNode* node = parseUnaryFunction(functionHash, arg1, old_pos);
        if (node) {
            if (expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
    
        // If not unary, expect a comma for a binary function:
        if (expr[pos] != ',') return error_zero;
        ++pos;
        ASTNode* arg2 = parseExpression();
        node = parseBinaryFunction(functionHash, arg1, arg2);
        if (node) {
            if (expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
    
        // Otherwise, for ternary functions:
        if (expr[pos] != ',') return error_zero;
        ++pos;
        ASTNode* arg3 = parseExpression();
        node = parseTernaryFunction(functionHash, arg1, arg2, arg3);
        if (node) {
            if (expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
        return error_zero;
    }
    

    
};




#endif // AST_PARSER_H