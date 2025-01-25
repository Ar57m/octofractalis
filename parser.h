#ifndef AST_PARSER_H
#define AST_PARSER_H

#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iostream>
#include "custom_quaternion.h"




// ASTNode base class
class ASTNode {
public:
    virtual Quaternion evaluate() const = 0;
    virtual ~ASTNode() = default;
};

// ConstantNode class
class ConstantNode : public ASTNode {
    Quaternion value;
public:
    ConstantNode(const Quaternion& val) : value(val) {}
    Quaternion evaluate() const override { return value; }
};

// VariableNode class
class VariableNode : public ASTNode {
    std::function<Quaternion()> getter;
public:
    VariableNode(const std::function<Quaternion()>& getter) : getter(getter) {}
    Quaternion evaluate() const override { return getter(); }
};


class ArrayAccessNode : public ASTNode {
    double* array;
    uint32_t size;
    std::shared_ptr<ASTNode> indexNode;

public:
    ArrayAccessNode(double* array, uint32_t size, std::shared_ptr<ASTNode> indexNode)
        : array(array), size(size), indexNode(indexNode) {}

    Quaternion evaluate() const override {
        uint32_t evaluatedIndex = static_cast<uint32_t>(std::abs(indexNode->evaluate().real));

        if (evaluatedIndex >= size) {
            return Quaternion(0);
        }

        return Quaternion(array[evaluatedIndex]);
    }
};

// UnaryFunctionNode class
class UnaryFunctionNode : public ASTNode {
    std::shared_ptr<ASTNode> operand;
    std::function<Quaternion(const Quaternion&)> func;
public:
    UnaryFunctionNode(const std::shared_ptr<ASTNode>& operand, std::function<Quaternion(const Quaternion&)> func)
        : operand(operand), func(func) {}

    Quaternion evaluate() const override {
        return func(operand->evaluate());
    }
};

// BinaryFunctionNode class
class BinaryFunctionNode : public ASTNode {
    std::shared_ptr<ASTNode> operand1, operand2;
    std::function<Quaternion(const Quaternion&, const Quaternion&)> func;
public:
    BinaryFunctionNode(const std::shared_ptr<ASTNode>& operand1, const std::shared_ptr<ASTNode>& operand2, 
                       std::function<Quaternion(const Quaternion&, const Quaternion&)> func)
        : operand1(operand1), operand2(operand2), func(func) {}

    Quaternion evaluate() const override {
        return func(operand1->evaluate(), operand2->evaluate());
    }
};


// TernaryFunctionNode class
class TernaryFunctionNode : public ASTNode {
    std::shared_ptr<ASTNode> operand1, operand2, operand3;
    std::function<Quaternion(const Quaternion&, const Quaternion&, const Quaternion&)> func;
public:
    TernaryFunctionNode(const std::shared_ptr<ASTNode>& operand1,
                        const std::shared_ptr<ASTNode>& operand2,
                        const std::shared_ptr<ASTNode>& operand3,
                        std::function<Quaternion(const Quaternion&, const Quaternion&, const Quaternion&)> func)
        : operand1(operand1), operand2(operand2), operand3(operand3), func(func) {}

    Quaternion evaluate() const override {
        return func(operand1->evaluate(), operand2->evaluate(), operand3->evaluate());
    }
};


static const std::shared_ptr<ASTNode> error_zero = std::make_shared<ConstantNode>(Quaternion(0));

static const std::unordered_map<std::string, std::function<std::shared_ptr<ASTNode>(std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>)>> functionMap = {
    {"logn", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.logn(b); }); }},
    {"pow", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.pow(b); }); }},
    {"root", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.root(b); }); }},
    {"max", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.maximum(b); }); }},
    {"min", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.minimum(b); }); }},
    {"sqrt", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.sqrt(); }); }},
    {"log", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.log(); }); }},
    {"sin", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.sin(); }); }},
    {"cos", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.cos(); }); }},
    {"tan", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.tan(); }); }},
    {"logten", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.log10(); }); }},
    {"sinh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.sinh(); }); }},
    {"cosh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.cosh(); }); }},
    {"tanh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.tanh(); }); }},
    {"arg", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.arg(); }); }},
    {"conj", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.conj(); }); }},
    {"mag", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.c_mag(); }); }},
    {"abs", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.abs(); }); }},
    {"exp", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.exp(); }); }},
    {"re", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(a.real); }); }},
    {"Im", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,a.imag,a.j,a.k); }); }},
    {"I", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,a.imag); }); }},
    {"J", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,0,a.j); }); }},
    {"K", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,0,0,a.k); }); }},
    {"round", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.round(); }); }},
    {"gamma", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.gamma(); }); }},
    {"zeta", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.zeta(); }); }},
    {"airy", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.airy(); }); }},
    {"ellipsoid", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Quaternion& a, const Quaternion& b, const Quaternion& c) { return a.ellipsoid(b,c); }); }},
    {"rotation", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Quaternion& a, const Quaternion& b, const Quaternion& c) { return a.rotation(b,c); }); }},
    {"rotate", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Quaternion& a, const Quaternion& b, const Quaternion& c) { return a.rotate_in_circle(b,c); }); }},
    {"rand", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Quaternion& a, const Quaternion& b, const Quaternion& c) { return a.generateRandom(b,c); }); }},
    {"circle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.circle(b); }); }},
    {"square", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.square(b); }); }},
    {"triangle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.triangle(b); }); }},
};



// Parser class

class Parser {
public:
    Parser(const std::string& expr, 
           const std::map<std::string, std::function<Quaternion()>>& vars, 
           const std::map<std::string, std::pair<double*, uint32_t>>& arrays)
        : expr(expr), pos(0), variables(vars), arrayVariables(arrays) {}

    std::shared_ptr<ASTNode> parse() {
        return parseExpression();
    }

private:
    std::string expr;
    size_t pos;
    const std::map<std::string, std::function<Quaternion()>>& variables;
    const std::map<std::string, std::pair<double*, uint32_t>>& arrayVariables;



   

    bool isTwoArgFunction(const std::string& func) {
        static const std::unordered_set<std::string> validFunctions = {
            "logn", "pow", "root", "max", "min", "square", "triangle", "circle"
        };
        return validFunctions.find(func) != validFunctions.end();
    }
    


    std::string replaceChar(std::string input, const char char_find, const std::string& replacement) {
        size_t i = 0; // Start at the beginning of the string
        
        while (i < input.size()) {
            if (input[i] == char_find) {
                // Check if characters to the left and right are letters
                bool left_is_letter = (i > 0 && std::isalpha(input[i - 1]));
                bool right_is_letter = (i < input.size() - 1 && std::isalpha(input[i + 1]));
                
                if (!left_is_letter && !right_is_letter) {
                    input.replace(i, 1, replacement);
                    i += replacement.size(); // Move the index past the replacement
                } else {
                    ++i; // Move to the next character
                }
            } else {
                ++i; // Move to the next character
            }
        }
        return input;
    }
    
    

    const std::shared_ptr<ASTNode> parseExpression() {
        std::shared_ptr<ASTNode> node = parseTerm();
        while (pos < expr.size()) {
            switch (expr[pos]) {
                case '+':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseTerm(), 
                        [](const Quaternion& a, const Quaternion& b) { return (a + b); });
                    break;
                case '-':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseTerm(), 
                        [](const Quaternion& a, const Quaternion& b) { return (a - b); });
                    break;
                default:
                    return node;
            }
        }
        return node;
    }

    const std::shared_ptr<ASTNode> parseTerm() {
        std::shared_ptr<ASTNode> node = parseFactor();
    
        while (pos < expr.size()) {
            switch (expr[pos]) {
                case '*':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a * b; });
                    break;
    
                case '/':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a / b; });
                    break;
    
                case '%':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a % b; });
                    break;
    
                case '^':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a.pow(b); });
                    break;

                case '=':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a.mseScore(b); });
                    break;
                    
                case '&':
                    ++pos;
                    node = std::make_shared<BinaryFunctionNode>(
                        node, parseFactor(),
                        [](const Quaternion& a, const Quaternion& b) { return a.cosSim(b); });
                    break;
                
                default:
                    return node;
            }
        }
        return node;
    }

    const std::shared_ptr<ASTNode> parseFactor() {
        switch (expr[pos]) {
                case '+':
                    ++pos;
                    return parseFactor();
                    break;
                case '-':
                    ++pos;
                    return std::make_shared<UnaryFunctionNode>(
                        parseFactor(), 
                        [](const Quaternion& a) { return -a; });
                    break;
        }
        
        const char exprpos = expr[pos];
        if ( isalpha(exprpos) && !(exprpos > 'h' && exprpos < 'l') ) {
            return parseVariableOrFunction();
        } else if (isdigit(exprpos) || exprpos == '.' ||  (exprpos > 'h' && exprpos < 'l') ) {
            return parseNumber();
        } else if (exprpos == '(') {
            ++pos;
            std::shared_ptr<ASTNode> node = parseExpression();
            if (expr[pos] != ')') return error_zero;
            ++pos;
            return node;
        }
        return error_zero;
    }
    
    const std::shared_ptr<ASTNode> parseVariableOrFunction() {
        std::string name;
        while (pos < expr.size() && isalpha(expr[pos])) {
            name += expr[pos++];
        }
    
        if (variables.find(name) != variables.end()) {
            return std::make_shared<VariableNode>(variables.at(name));
        }
    
        if (pos < expr.size()) {
            if ( expr[pos] == '(') {
                return parseFunction(name);
                
            } else if ( arrayVariables.find(name) != arrayVariables.end() && expr[pos] == '[') {
                ++pos; // '['
                std::shared_ptr<ASTNode> idx = parseExpression();
                if (pos < expr.size() && expr[pos] == ']') {
                    ++pos; // ']'
                    auto arrayInfo = arrayVariables.at(name);
                    return std::make_shared<ArrayAccessNode>(arrayInfo.first, arrayInfo.second, idx);
                }
            }
        }
        return error_zero;
    }

    const std::shared_ptr<ASTNode> parseNumber() {
        std::string number;
        double realPart = 0.0, imagPart = 0.0, jPart = 0.0, kPart = 0.0;
        char identifier = '\0';
        char exprpos = expr[pos];
        bool imag = (exprpos > 'h' && exprpos < 'l'); // i, j, k

        while (pos < expr.size() && (isdigit(exprpos) || exprpos == '.' || imag)) {
            if (imag) {
                identifier = expr[pos++];
                break;
            } else {
                number += expr[pos++];
            }
            exprpos = expr[pos];
            imag = (exprpos > 'h' && exprpos < 'l');
        }

        if (number.empty()) number = "0.0";
        const double parsedValue = (identifier != '\0' && number == "0.0") ? 1.0 : std::stod(number);


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

        return std::make_shared<ConstantNode>(Quaternion(realPart, imagPart, jPart, kPart));
    }
    
    
    
    std::shared_ptr<ASTNode> parseFunction(const std::string& func) {
        ++pos;  // Skip '('
        std::shared_ptr<ASTNode> arg1 = nullptr;
        
        
        if ( func != "diff" ) {
            arg1 = parseExpression();
        } else {
            std::string old_expr = expr;
            size_t old_pos = pos;

            arg1 = parseExpression();

            expr = replaceChar(old_expr.erase(0, old_pos), 'z' , "(z+0.000001)");

            pos = 0;

            arg1 = std::make_shared<BinaryFunctionNode>(
                parseExpression(), arg1,
                [](const Quaternion& a, const Quaternion& b) { 
                    return (a-b)/1e-6; });

            if (expr[pos] != ')') return error_zero;
            ++pos;  // Skip ')'
            return arg1;
        }



        std::shared_ptr<ASTNode> arg2 = nullptr; // Optional second argument
        std::shared_ptr<ASTNode> arg3 = nullptr; // Optional third argument
    
        // Parsing functions with two or three arguments
        const bool treeArgs = func == "ellipsoid" || func == "rotation" || func == "rotate" || func == "rand";
        if (isTwoArgFunction(func) || treeArgs) {
            if (expr[pos] == ',') {
                ++pos; // Skip ','
                arg2 = parseExpression();
            } else {
                return error_zero;
            }
            if (treeArgs) {
                if ( (expr[pos] != ',') ) return error_zero;
                ++pos; // Skip ','
                arg3 = parseExpression();
            }
        }
    
        if (expr[pos] != ')') return error_zero;
        ++pos;  // Skip ')'
        

    
        const auto it = functionMap.find(func);
        if (it != functionMap.end()) {
           return it->second(arg1, arg2, arg3);  // Call the mapped function
        }
        return error_zero;
    }
};





#endif // AST_PARSER_H
