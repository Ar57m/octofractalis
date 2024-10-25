// fract.cpp
#include <omp.h>
#include <math.h>
#include <cstdint>
#include <cmath>
#include <vector>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <memory>
#include <functional>
#include <stdexcept>
#include <sstream>
#include "complex.h"
#include <chrono>


void signal_handler(int signal) {
    std::cout << "(Ctrl+C)" << std::endl;
    std::exit(signal);
}


double pi = 3.1415926535897932384626433832795028841971693993751;
double phi =1.6180339887498948482045868343656381177203091798057;
double e =  2.7182818284590452353602874713526624977572470937000;

double noNan(double value) {
    return (std::abs(value) > 1e-13 && std::abs(value) < 1e300) ? value : 0;
}




int current = 0;

void display_progress( int &current, const int total, const int iteration_interval) {
    static auto start_time = std::chrono::steady_clock::now();
    static int avg_it_per_sec = 0;
    static auto last_update_time = start_time;

    if (current % iteration_interval == 0 || current == total) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_since_last_update = now - last_update_time;

        if (elapsed_since_last_update.count() >= 0.5 || current == total) {
            last_update_time = now;

            double progress = static_cast<double>(current) / total * 100.0;

            std::chrono::duration<double> elapsed = now - start_time;
            avg_it_per_sec = (avg_it_per_sec == 0) ? static_cast<int>(current / elapsed.count()) : static_cast<int>((avg_it_per_sec + current / elapsed.count()) / 2.0);

            int bar_width = 50;
            int pos = static_cast<int>(bar_width * progress / 100.0);

            std::cout << "[";
            for (int i = 0; i < bar_width; ++i) {
                if (i <= pos) std::cout << std::string(1,(char)254u);
                else std::cout << " ";
            }
            std::cout << "] " << int(progress) << "%  [ "
                      << avg_it_per_sec << " it/s; " << int((total - current) / avg_it_per_sec) << "s left ] \r";
            std::cout.flush();
        }
    }
    current++;
}



// ASTNode base class
class ASTNode {
public:
    virtual Complex evaluate() const = 0;
    virtual ~ASTNode() = default;
};


class ConstantNode : public ASTNode {
    Complex value;
public:
    ConstantNode(const Complex& val) : value(val) {}
    Complex evaluate() const override { return value; }
};


class VariableNode : public ASTNode {
    std::function<Complex()> getter;
public:
    VariableNode(const std::function<Complex()>& getter) : getter(getter) {}
    Complex evaluate() const override { return getter(); }
};


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


// Zeroing if something is wrong to not crash
static const std::unordered_map<std::string, std::function<std::shared_ptr<ASTNode>(std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>)>> error_zero = {
    {"zero", [](std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<ConstantNode>(Complex(0.0, 0.0)); }}
};

bool printerror = false;


// Parser
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

    void print_error(bool& value,  const std::string& message) {
        if (!value) {
            std::cout << message << std::endl;
            value = true;
        }
    }

    std::shared_ptr<ASTNode> parseExpression() {
        auto node = parseTerm();
        while (pos < expr.size()) {
            if (expr[pos] == '+') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseTerm(), [](const Complex& a, const Complex& b) { return (a + b).noNan(); });
            } else if (expr[pos] == '-') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseTerm(), [](const Complex& a, const Complex& b) { return (a - b).noNan(); });
            } else {
                break;
            }
        }
        return node;
    }

    std::shared_ptr<ASTNode> parseTerm() {
        auto node = parseFactor();
        while (pos < expr.size()) {
            if (expr[pos] == '*') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseFactor(), [](const Complex& a, const Complex& b) { return (a * b).noNan(); });
            } else if (expr[pos] == '/') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseFactor(), [](const Complex& a, const Complex& b) { return (a / b).noNan(); });
            } else {
                break;
            }
        }
        return node;
    }

    std::shared_ptr<ASTNode> parseFactor() {

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


    std::shared_ptr<ASTNode> parseVariableOrFunction() {
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

    std::shared_ptr<ASTNode> parseNumber() {
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

    std::shared_ptr<ASTNode> parseFunction(const std::string& func) {
        ++pos;  // Skip '('

        auto arg1 = parseExpression(); // Parse the first argument

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

        // Mapping functions to nodes (unary, binary, and ternary)
        static const std::unordered_map<std::string, std::function<std::shared_ptr<ASTNode>(std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>)>> functionMap = {
            {"sin", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sin(); }); }},
            {"cos", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.cos(); }); }},
            {"tan", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.tan().noNan(); }); }},
            {"sqrt", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sqrt(); }); }},
            {"log", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.log().noNan(); }); }},
            {"logten", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.log10().noNan(); }); }},
            {"sinh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sinh(); }); }},
            {"cosh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.cosh(); }); }},
            {"tanh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.tanh(); }); }},
            {"arg", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return noNan(a.arg()); }); }},
            {"conj", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.conj(); }); }},
            {"abs", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.abs(); }); }},
            {"round", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.round(); }); }},
            {"logn", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.logn(b).noNan(); }); }},
            {"pow", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.pow(b).noNan(); }); }},
            {"root", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.root(b); }); }},
            {"max", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.maximum(b); }); }},
            {"min", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.minimum(b); }); }},
            {"gamma", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.gamma().noNan(); }); }},
            {"zeta", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.zeta().noNan(); }); }},
            {"airy", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.airy().noNan(); }); }},
            {"ellipsoid", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode> arg3) { return std::make_shared<TernaryFunctionNode>(arg1, arg2, arg3, [](const Complex& a, const Complex& b, const Complex& c) { return a.ellipsoid(b,c).noNan(); }); }},
            {"circle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.circle(b).noNan(); }); }},
            {"square", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.square(b).noNan(); }); }},
            {"triangle", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2, std::shared_ptr<ASTNode>) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.triangle(b).noNan(); }); }},
        };

        auto it = functionMap.find(func);
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


void update_output(uint16_t* output, const double temp, const uint16_t max_iter, const uint16_t width, const uint16_t iteration, const uint16_t x, const uint16_t y, const bool lake, const bool lya) {
    if ((temp < 2 && lake)) {
        output[y * width + x] = static_cast<uint16_t>(std::round((temp / (temp + 1.0)) * max_iter)) + max_iter;
    } else if ( lya) {
        output[y * width + x] = static_cast<uint16_t>((std::round((temp / (temp + max_iter/10.0)) * max_iter)));
    } else {
        output[y * width + x] = iteration;
    }
}
    




extern "C" {
    void scale(const float* input_tensor, float* scaled_tensor, const int input_size, const float new_min, const float new_max) {
        std::signal(SIGINT, signal_handler);
        float current_min = *std::min_element(input_tensor, input_tensor + input_size);
        float current_max = *std::max_element(input_tensor, input_tensor + input_size);

        if (current_min == current_max){
            current_min -= 1;
        } 

        float scale_factor = (new_max - new_min) / (current_max - current_min);
        
        for (int i = 0; i < input_size; ++i) {
            scaled_tensor[i] = (input_tensor[i] - current_min) * scale_factor + new_min;
        }

    }



    void fractal(uint16_t* output, double* failed_gen, const char* exp, const uint16_t width, const uint16_t height, const uint16_t max_iter, const double xmin, const double xmax, const double ymin, const double ymax, const double c_real, const double c_imag, const bool juliaset, const bool lake, const double quaternion_j, const double quaternion_k) {

        std::signal(SIGINT, signal_handler);

        const double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;
        const bool quatern = (quaternion_j != 0.0 || quaternion_k != 0.0);
        *failed_gen = *failed_gen == 0 ? 1 : 1;
        current = 0;

        const std::string expression = std::string(exp);

        if ( (expression == "rt*rt+rw" || expression == "pow(rt,2)+rw") && ( !quatern ) ) {
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {

                for (int y = 0; y < height; ++y) {
                    
                    double r_part = xmin + x * dx;
                    double i_part = ymin + y * dy;
                    
                    Complex c, z;
                    if (juliaset) {
                        c = Complex (c_real, c_imag);
                        z = Complex (r_part, i_part);
                    } else {
                        c = Complex (r_part, i_part);
                        z = Complex (0.0, 0.0);
                    }

                    uint16_t iteration = 0;
                    
    
                    while (z.abs() < 2 && iteration < max_iter) {
                        z = ((z*z)+c).noNan();
                        ++iteration;
                    }
                    update_output( output, z.abs(), max_iter, width, iteration, x, y, lake, false);
                }
                display_progress( current, width, 100);
            }
            std::cout << "\n";


        } else if ( quatern ) {


            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {

                for (int y = 0; y < height; ++y) {
                    
                    double r_part = xmin + x * dx;
                    double i_part = ymin + y * dy;
                    
                    Quaternion c, z;
                    if (juliaset) {
                        c = Quaternion (c_real, c_imag);
                        z = Quaternion (r_part, i_part, quaternion_j, quaternion_k);
                    } else {
                        c = Quaternion (r_part, i_part);
                        z = Quaternion (0.0, 0.0, quaternion_j, quaternion_k);
                    }

                    uint16_t iteration = 0;
                    
    
                    while (z.abs() < 2 && iteration < max_iter) {
    
                        z = ((z*z)+c).noNan();
                        ++iteration;
                    }
                    update_output( output, z.abs(), max_iter, width, iteration, x, y, lake, false);
                }
                display_progress( current, width, 100);
            }
            std::cout << "\n";


        } else {
            
            int y;
            *failed_gen = 0.0;
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                Complex z,c;
    
                std::map<std::string, std::function<Complex()>> variables = {
                    {"rt", [&z]() { return z; }},
                    {"rw", [&c]() { return c; }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"y", [&]() { return Complex (y, 0.0); }},
                    {"x", [&]() { return Complex (x, 0.0); }}
                };
    
                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables);
                auto ast = parser.parse();
    
    
                for (int y = 0; y < height; ++y) {
                    double r_part = xmin + x * dx;
                    double i_part = ymin + y * dy;
    

                    if (juliaset) {
                        c = Complex (c_real, c_imag);
                        z = Complex (r_part, i_part);
                    } else {
                        c = Complex (r_part, i_part);
                        z = Complex (0.0, 0.0);
                    }

                    uint16_t iteration = 0;
                    double temp = z.abs();
    
                    while (temp < 2 && iteration < max_iter) {
                        z = (ast->evaluate()).noNan();
                        temp = z.abs();
                        ++iteration;
                    }
                    *failed_gen = temp > *failed_gen ? temp : *failed_gen;
                    update_output( output, temp, max_iter, width, iteration, x, y, lake, false);
                }
                display_progress( current, width, 100);
            }
            std::cout << "\n";
       } 
    }



    void lyapunov(uint16_t* output, double* failed_gen,const char* exp, const uint16_t width, const uint16_t height, const uint16_t max_iter, const double xmin, const double xmax, const double ymin, const double ymax, double complex_a, double complex_b, const bool use_complex_ab, const double quaternion_j, const double quaternion_k) {
        
        std::signal(SIGINT, signal_handler);
        
        const double dx = (xmax - xmin) / width;
        const double dy = (ymax - ymin) / height;
        const bool quatern = (quaternion_j != 0.0 || quaternion_k != 0.0);
        complex_a = !use_complex_ab ? 0.0 : complex_a;
        complex_b = !use_complex_ab ? 0.0 : complex_b;
        *failed_gen = *failed_gen == 0 ? 1 : 1;
        current = 0;

        const std::string expression = std::string(exp);

        if ( (expression == "rt*rt+rw" || expression == "pow(rt,2)+rw") && ( !quatern ) ) {

            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    double x = xmin + i * dx;
                    double y = ymin + j * dy;
                    Complex a(0.5 + x * 0.5, complex_a);
                    Complex b(0.5 + y * 0.5, complex_b);
                    Complex l(0.0, 0.0);
                    Complex v(0.5, 0.0);

                    for (int k = 0; k < max_iter; ++k) {
                        if (k % 12 < 6) {
                            v = b * v * (1.0 - v);
                            l += (((b * (1.0 - 2.0 * v)).c_abs()).log()).noNan();
                        } else {
                            v = a * v * (1.0 - v);
                            l += (((a * (1.0 - 2.0 * v)).c_abs()).log()).noNan();
                        }
                    }
                    update_output( output, l.abs(), max_iter, width, 0, i, j, false, true);
                    
                }
                display_progress( current, width, 100);
            }
            std::cout << "\n";

        } else if (quatern) {
            
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    double x = xmin + i * dx;
                    double y = ymin + j * dy;
                    Quaternion a(0.5 + x * 0.5, complex_a, quaternion_j, quaternion_k);
                    Quaternion b(0.5 + y * 0.5, complex_b, quaternion_j, quaternion_k);
                    Quaternion l(0.0, 0.0);
                    Quaternion v(0.5, 0.0);

                    for (int k = 0; k < max_iter; ++k) {
                        if (k % 12 < 6) {
                            v = b * v * (1.0 - v);
                            l += (((b * (1.0 - 2.0 * v)).q_abs()).log()).noNan();
                        } else {
                            v = a * v * (1.0 - v);
                            l += (((a * (1.0 - 2.0 * v)).q_abs()).log()).noNan();
                        }
                    }
                    update_output( output, l.abs(), max_iter, width, 0, i, j, false, true);
                    
                }
                display_progress( current, width, 100);
            }
            std::cout << "\n";

        } else {

            
            *failed_gen = 0.0;
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < width; ++i) {
                Complex l, v, temp;
                std::map<std::string, std::function<Complex()>> variables = {
                    {"rt", [&v]() { return v; }},
                    {"rw", [&l]() { return l; }},
                    {"rk", [&temp]() { return temp; }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                };

                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables);
                auto ast = parser.parse();
                for (int j = 0; j < height; ++j) {
                    double x = xmin + i * dx;
                    double y = ymin + j * dy;
                    Complex a(0.5 + x * 0.5, complex_a);
                    Complex b(0.5 + y * 0.5, complex_b);
                    l = Complex (0.0, 0.0);
                    v = Complex (0.5, 0.0);


                    for (int k = 0; k < max_iter; ++k) {
                        if (k % 12 < 6) {
                            v = b * v * (1.0 - v);
                            temp = b;
                            l += (ast->evaluate()).noNan();
                        } else {
                            v = a * v * (1.0 - v);
                            temp = a;
                            l += (ast->evaluate()).noNan();
                        }
                    }
                    double labs = l.abs();
                    *failed_gen = labs > *failed_gen ? labs : *failed_gen;
                    update_output( output, labs, max_iter, width, 0, i, j, false, true);
                    
                }
                display_progress( current, width, 100);
            }
            std::cout << "\n";
        }
    }



    void sandpile(uint8_t* output, const uint16_t width, const uint16_t height, const uint32_t n_grains, const uint16_t max_grains=3) {
            std::signal(SIGINT, signal_handler);
            std::vector<std::vector<uint32_t>> sandpile(height, std::vector<uint32_t>(width, 0));
        
            // Add grains to the center of the sandpile
            sandpile[height / 2][width / 2] = n_grains;
        
            bool unstable = true;
            while (unstable) {
                unstable = false;
        
                // Process each cell in the sandpile
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        // Distribute grains if the number of grains in the cell is greater than max_grains
                        uint32_t grains_to_distribute = sandpile[y][x]; 
                        if (grains_to_distribute > max_grains &&  grains_to_distribute > 3) {
                            grains_to_distribute /= 4;
                            // Distribute grains to neighboring cells
                            if (y > 0) sandpile[y-1][x] += grains_to_distribute;
                            if (y < height-1) sandpile[y+1][x] += grains_to_distribute;
                            if (x > 0) sandpile[y][x-1] += grains_to_distribute;
                            if (x < width-1) sandpile[y][x+1] += grains_to_distribute;
        
                            // Remove grains from current cell
                            sandpile[y][x] %= 4;
                            unstable = true;
                        }
                        output[y * width + x] = static_cast<uint8_t>(sandpile[y][x]);
                    }
                }
            }
        }





    void process_array(uint32_t* input_array, uint8_t* output_array, const uint16_t width, const uint16_t height, const double max_value, const uint16_t batch_size, const double npmax) {
        std::signal(SIGINT, signal_handler);
        // Iterate over each batch
        
        #pragma omp parallel for schedule(dynamic)
        for(int i = 0; i < width * height; i += batch_size) {
            // Iterate over each value in the batch
            for(int j = i; j < i + batch_size && j < width * height; j++) {
                // Convert the value to double and scale it
                double value = (static_cast<double>((input_array[j])) / npmax) * max_value;

                // Round to nearest integer
                uint32_t rounded_value = static_cast<uint32_t>(std::round(value));
                
                // Separate RGB channels
                output_array[j * 3] = (rounded_value >> 16) & 0xFF;
                output_array[j * 3 + 1] = (rounded_value >> 8) & 0xFF;
                output_array[j * 3 + 2] = rounded_value & 0xFF;
            }
        }
    }
}



