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
    {"qaim", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,a.imag,a.j,a.k); }); }},
    {"qiim", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,a.imag); }); }},
    {"qjim", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,0,a.j); }); }},
    {"qkim", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return Quaternion(0,0,0,a.k); }); }},
    {"round", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.round(); }); }},
    {"gamma", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.gamma(); }); }},
    {"zeta", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.zeta(); }); }},
    {"airy", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Quaternion& a) { return a.airy(); }); }},
    {"ellipsoid", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Quaternion& a, const Quaternion& b, const Quaternion& c) { return a.ellipsoid(b,c); }); }},
    {"rotation", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Quaternion& a, const Quaternion& b, const Quaternion& c) { return a.rotation(b,c); }); }},
    {"rotate", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Quaternion& a, const Quaternion& b, const Quaternion& c) { return a.rotate_in_circle(b,c); }); }},
    {"circle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.circle(b); }); }},
    {"square", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.square(b); }); }},
    {"triangle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Quaternion& a, const Quaternion& b) { return a.triangle(b); }); }},
};



// Parser class

class Parser {
public:
    Parser(const std::string& expr, const std::map<std::string, std::function<Quaternion()>>& vars)
        : expr(expr), pos(0), variables(vars) {}

    std::shared_ptr<ASTNode> parse() {
        return parseExpression();
    }

private:
    std::string expr;
    size_t pos;
    std::map<std::string, std::function<Quaternion()>> variables;

   
    

    bool isTwoArgFunction(const std::string& func) {
        static const std::unordered_set<std::string> validFunctions = {
            "logn", "pow", "root", "max", "min", "square", "triangle", "circle"
        };
        return validFunctions.find(func) != validFunctions.end();
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
        if (isalpha(exprpos)) {
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
        const bool parse_fun = pos < expr.size() && expr[pos] == '(';
        switch ( parse_fun + 3*(variables.find(name) != variables.end()) ) {
            case 0:
                return error_zero;
                break;
            case 1:
                return parseFunction(name);
                break;
            case 3:
                return std::make_shared<VariableNode>(variables.at(name));
                break;
            case 4:
                return parseFunction(name);
                break; 
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

        const double parsedValue = number.empty() ? 0.0 : std::stod(number);

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

    const std::shared_ptr<ASTNode> parseFunction(const std::string& func) {
        ++pos;  // Skip '('
        const std::shared_ptr<ASTNode> arg1 = parseExpression();
    
        std::shared_ptr<ASTNode> arg2 = nullptr; // Optional second argument
        std::shared_ptr<ASTNode> arg3 = nullptr; // Optional third argument
    
        // Parsing functions with two or three arguments
        const bool treeArgs = func == "ellipsoid" || func == "rotation" || func == "rotate";
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
