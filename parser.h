#ifndef AST_PARSER_H
#define AST_PARSER_H

#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <iostream>
#include "custom_complex.h"




// ASTNode base class
class ASTNode {
public:
    virtual Complex evaluate() const = 0;
    virtual ~ASTNode() = default;
};

// ConstantNode class
class ConstantNode : public ASTNode {
    Complex value;
public:
    ConstantNode(const Complex& val) : value(val) {}
    Complex evaluate() const override { return value; }
};

// VariableNode class
class VariableNode : public ASTNode {
    std::function<Complex()> getter;
public:
    VariableNode(const std::function<Complex()>& getter) : getter(getter) {}
    Complex evaluate() const override { return getter(); }
};

// BinaryOpNode class
class BinaryOpNode : public ASTNode {
    std::shared_ptr<ASTNode> left, right;
    std::function<Complex(const Complex&, const Complex&)> op;
public:
    BinaryOpNode(const std::shared_ptr<ASTNode>& left, const std::shared_ptr<ASTNode>& right, const std::function<Complex(const Complex&, const Complex&)>& op)
        : left(left), right(right), op(op) {}

    Complex evaluate() const override {
        return op(left->evaluate(), right->evaluate());
    }
};

// UnaryFunctionNode class
class UnaryFunctionNode : public ASTNode {
    std::shared_ptr<ASTNode> operand;
    std::function<Complex(const Complex&)> func;
public:
    UnaryFunctionNode(const std::shared_ptr<ASTNode>& operand, std::function<Complex(const Complex&)> func)
        : operand(operand), func(func) {}

    Complex evaluate() const override {
        return func(operand->evaluate());
    }
};

// BinaryFunctionNode class
class BinaryFunctionNode : public ASTNode {
    std::shared_ptr<ASTNode> operand1, operand2;
    std::function<Complex(const Complex&, const Complex&)> func;
public:
    BinaryFunctionNode(const std::shared_ptr<ASTNode>& operand1, const std::shared_ptr<ASTNode>& operand2, 
                       std::function<Complex(const Complex&, const Complex&)> func)
        : operand1(operand1), operand2(operand2), func(func) {}

    Complex evaluate() const override {
        return func(operand1->evaluate(), operand2->evaluate());
    }
};

// TernaryFunctionNode class
class TernaryFunctionNode : public ASTNode {
    std::shared_ptr<ASTNode> operand1, operand2, operand3;
    std::function<Complex(const Complex&, const Complex&, const Complex&)> func;
public:
    TernaryFunctionNode(const std::shared_ptr<ASTNode>& operand1,
                        const std::shared_ptr<ASTNode>& operand2,
                        const std::shared_ptr<ASTNode>& operand3,
                        std::function<Complex(const Complex&, const Complex&, const Complex&)> func)
        : operand1(operand1), operand2(operand2), operand3(operand3), func(func) {}

    Complex evaluate() const override {
        return func(operand1->evaluate(), operand2->evaluate(), operand3->evaluate());
    }
};



static const std::unordered_map<std::string, std::function<std::shared_ptr<ASTNode>(std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>)>> error_zero = {
    {"zero", [](std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<ConstantNode>(Complex(0.0, 0.0)); }}
};

static const std::unordered_map<std::string, std::function<std::shared_ptr<ASTNode>(std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>)>> functionMap = {
    {"sin", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sin(); }); }},
    {"cos", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.cos(); }); }},
    {"tan", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.tan(); }); }},
    {"sqrt", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sqrt(); }); }},
    {"log", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.log(); }); }},
    {"logten", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.log10(); }); }},
    {"sinh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sinh(); }); }},
    {"cosh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.cosh(); }); }},
    {"tanh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.tanh(); }); }},
    {"arg", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.arg(); }); }},
    {"conj", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.conj(); }); }},
    {"abs", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.c_abs(); }); }},
    {"round", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.round(); }); }},
    {"logn", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.logn(b); }); }},
    {"pow", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.pow(b); }); }},
    {"root", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.root(b); }); }},
    {"max", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.maximum(b); }); }},
    {"min", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.minimum(b); }); }},
    {"gamma", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.gamma(); }); }},
    {"zeta", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.zeta(); }); }},
    {"airy", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.airy(); }); }},
    {"ellipsoid", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Complex& a, const Complex& b, const Complex& c) { return a.ellipsoid(b,c); }); }},
    {"circle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.circle(b); }); }},
    {"square", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.square(b); }); }},
    {"triangle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.triangle(b); }); }},
};

bool printerror = false; // to not fill the cli with errors


// Parser class
class Parser {
public:
    Parser(const std::string& expr, const std::map<std::string, std::function<Complex()>>& vars) 
        : expr(expr), pos(0), variables(vars) {}

    std::shared_ptr<ASTNode> parse() {
        return parseExpression();
    }


private:
    std::string expr;
    size_t pos;
    std::map<std::string, std::function<Complex()>> variables;





    void print_error(bool& value, const std::string& message) const {
        if (!value) {
            std::cout << message << std::endl;
            value = true;
        }
    }

    const std::shared_ptr<ASTNode> parseExpression() {
        auto node = parseTerm();
        while (pos < expr.size()) {
            if (expr[pos] == '+') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseTerm(), [](const Complex& a, const Complex& b) { return (a + b); });
            } else if (expr[pos] == '-') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseTerm(), [](const Complex& a, const Complex& b) { return (a - b); });
            } else {
                break;
            }
        }
        return node;
    }

    const std::shared_ptr<ASTNode> parseTerm() {
        auto node = parseFactor();
        while (pos < expr.size()) {
            if (expr[pos] == '*') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseFactor(), [](const Complex& a, const Complex& b) { return (a * b); });
            } else if (expr[pos] == '/') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseFactor(), [](const Complex& a, const Complex& b) { return (a / b); });
            } else {
                break;
            }
        }
        return node;
    }

    const std::shared_ptr<ASTNode> parseFactor() {

        if (expr[pos] == '+') {
            ++pos;
            return parseFactor();
        } else if (expr[pos] == '-') {
            ++pos;

            return std::make_shared<UnaryFunctionNode>(parseFactor(), [](const Complex& a) { return -a; });
        }

        if (isalpha(expr[pos])) {
            return parseVariableOrFunction();
        } else if (isdigit(expr[pos]) || expr[pos] == '.' || expr[pos] == 'i') {
            return parseNumber();
        } else if (expr[pos] == '(') {
            ++pos;
            auto node = parseExpression();
            if (expr[pos] != ')') {
                print_error(printerror,"Expected ')'" "\n");
                return std::shared_ptr<ASTNode> (error_zero.find("zero")->second(nullptr, nullptr, nullptr));
            }
            ++pos;
            return node;
        }

        print_error(printerror,"Unexpected character in expression" "\n");
        return std::shared_ptr<ASTNode> (error_zero.find("zero")->second(nullptr, nullptr, nullptr));
    }


    const std::shared_ptr<ASTNode> parseVariableOrFunction() {
        std::string name;
        while (pos < expr.size() && isalpha(expr[pos])) {
            name += expr[pos++];
        }

        if (pos < expr.size() && expr[pos] == '(') {
            return parseFunction(name);
        }

        if (variables.find(name) != variables.end()) {
            return std::make_shared<VariableNode>(variables.at(name));
        } else {
            print_error(printerror,"Unknown variable: " + name + "\n");
            return std::shared_ptr<ASTNode> (error_zero.find("zero")->second(nullptr, nullptr, nullptr));
        }
    }

    const std::shared_ptr<ASTNode> parseNumber() {
        std::string number;
        bool hasImaginaryPart = false;

        while (pos < expr.size() && (isdigit(expr[pos]) || expr[pos] == '.' || expr[pos] == 'i')) {
            if (expr[pos] == 'i') {
                hasImaginaryPart = true;
            }
            number += expr[pos++];
        }

        double realPart = 0.0, imagPart = 0.0;
        if (hasImaginaryPart) {
            imagPart = std::stod(number);
        } else {
            realPart = std::stod(number);
        }

        return std::make_shared<ConstantNode>(Complex(realPart, imagPart));
    }

    const std::shared_ptr<ASTNode> parseFunction(const std::string& func) {
        ++pos;  // Skip '('

        const auto arg1 = parseExpression(); // Parse the first argument

        std::shared_ptr<ASTNode> arg2 = nullptr; // Optional second argument
        std::shared_ptr<ASTNode> arg3 = nullptr; // Optional third argument


        // Parse binary functions (expecting a second argument)
        if (func == "logn" || func == "pow" || func == "root" || func == "max" || func == "min" || func == "square" || func == "triangle" || func == "circle") {
            if (expr[pos] == ',') {
                ++pos; // Skip ','
                arg2 = parseExpression();  // Parse the second argument for binary functions
            } else {
                print_error(printerror,"Expected ',' between arguments for " + func + "\n");
                return std::shared_ptr<ASTNode> (error_zero.find("zero")->second(nullptr, nullptr, nullptr));
            }
        }
  

        // Parse functions that expect a third argument
        if (func == "ellipsoid") {

            if (expr[pos] == ',') {
                ++pos;
  
                arg2 = parseExpression();
                ++pos;

                arg3 = parseExpression();
            } else {
                print_error(printerror,"Expected ',' between arguments for " + func + "\n");
                return std::shared_ptr<ASTNode> (error_zero.find("zero")->second(nullptr, nullptr, nullptr));
            }
        }


        if (expr[pos] != ')') {
            print_error(printerror,"Expected ')' to close the function " + func + "\n");
            return std::shared_ptr<ASTNode> (error_zero.find("zero")->second(nullptr, nullptr, nullptr));
        }
        ++pos;  // Skip ')'


        const auto it = functionMap.find(func);
        if (it != functionMap.end()) {
            return it->second(arg1, arg2, arg3);  // Call the mapped function
        } else {
            print_error(printerror,"Function not recognized: " + func + "\n");
            return std::shared_ptr<ASTNode> (error_zero.find("zero")->second(nullptr, nullptr, nullptr));
        }
    }
    // void skipWhitespace() {
    //     while (pos < expr.size() && isspace(expr[pos])) {
    //         ++pos;
    //     }
    // }
};




#endif // AST_PARSER_H
