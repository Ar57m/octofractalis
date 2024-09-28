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

void signal_handler(int signal) {
    std::cout << "(Ctrl+C)" << std::endl;
    std::exit(signal);
}


double pi = 3.1415926535897932384626433832795028841971693993751;
double e =  2.7182818284590452353602874713526624977572470937000;

double noNan(double value) {
    if (std::isnan(value) || std::isinf(value)) {
        return 0;
    } else {
        return value;
    }
}





struct Complex {
    double real;
    double imag;

    Complex(double r = 0.0, double i = 0.0) : real(r), imag(i) {}


    Complex noNan() const {
        double realPart = (std::abs(real) > 1e-13 && !std::isnan(real) && !std::isinf(real)) ? real : (std::isinf(real) ? 3 : 0);
        double imagPart = (std::abs(imag) > 1e-13 && !std::isnan(imag) && !std::isinf(imag)) ? imag : 0;
        return Complex(realPart, imagPart);
    }




    // Sum
    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }

    Complex operator+(double value) const {
        return Complex(real + value, imag);
    }

    Complex operator+() const {
    return Complex(+real, +imag);
    }
    
    Complex& operator+=(const Complex& other) {
        real += other.real;
        imag += other.imag;
        return *this;
    }

    Complex& operator+=(double value) {
        real += value;
        return *this;
    }
    friend Complex operator+(double value, const Complex& c) {
        return Complex(c.real + value, c.imag);
    }

    // Sub
    Complex operator-(const Complex& other) const {
        return Complex(real - other.real, imag - other.imag);
    }
    Complex operator-(double value) const {
        return Complex(real - value, imag);
    }

    Complex operator-() const {
    return Complex(-real, -imag);
    }

    Complex& operator-=(const Complex& other) {
        real -= other.real;
        imag -= other.imag;
        return *this;
    }

    Complex& operator-=(double value) {
        real -= value;
        return *this;
    }

    friend Complex operator-(double value, const Complex& c) {
        return Complex(value - c.real, c.imag);
    } 

    // Mul
    Complex operator*(const Complex& other) const {
        return Complex(real * other.real - imag * other.imag, real * other.imag + imag * other.real);
    }

    Complex operator*(double scalar) const {
        return Complex(real * scalar, imag * scalar);
    }

    Complex& operator*=(const Complex& other) {
        double new_real = real * other.real - imag * other.imag;
        double new_imag = real * other.imag + imag * other.real;

        real = new_real;
        imag = new_imag;
        return *this;
    }

    Complex& operator*=(double value) {
        real *= value;
        imag *= value;
        return *this;
    }

    friend Complex operator*(double scalar, const Complex& c) {
        return c * scalar;
    }

    // Division
    Complex operator/(const Complex& other) const {
        double denom = other.real * other.real + other.imag * other.imag;
        return Complex((real * other.real + imag * other.imag) / denom,
                       (imag * other.real - real * other.imag) / denom);
    }

    Complex operator/(double value) const {
        return Complex(real / value, imag / value);
    }

    friend Complex operator/(double value, const Complex& c) {
        double denom = c.real * c.real + c.imag * c.imag;
        return Complex((value * c.real) / denom, (-value * c.imag) / denom);
    }

    // Conj
    Complex conj() const {
        return Complex(real, -imag);
    }

    // abs
    double abs() const {
        return std::sqrt(real * real + imag * imag);
    }

    // sqrt
    Complex sqrt() const {
        double magnitude = std::sqrt(abs());
        double angle = std::atan2(imag, real) / 2.0;
        return Complex(magnitude * std::cos(angle), magnitude * std::sin(angle));
    }
    
    // arg
    double arg() const {
        return std::atan2(imag, real);
    }

    // root
    Complex root(const Complex& n1) const {
        if (n1.real == 0 && n1.imag == 0) {
            return Complex(0,0);
        } else {
            Complex result = (this->log())/n1;
            double magnitude = std::exp(result.real);
            return Complex(magnitude * std::cos(result.imag), magnitude * std::sin(result.imag));
        }
    }

    // log
    Complex log() const {
        if (real == 0 && imag == 0) {
            return Complex(0,0);
        } else {
            return Complex(std::log(abs()), std::atan2(imag, real));
        }
    }

    // pow
    Complex pow(const Complex& exponent) const {
        Complex log_z = this->log();
        Complex result = exponent * log_z;
        double magnitude = std::exp(result.real);
        return Complex(magnitude * std::cos(result.imag), magnitude * std::sin(result.imag));
    }

    // pow
    Complex pow(double exponent) const {
        double magnitude = std::pow(abs(), exponent);
        double angle = std::atan2(imag, real) * exponent;
        return Complex(magnitude * std::cos(angle), magnitude * std::sin(angle));
    }



    // Sin
    Complex sin() const {
        return Complex(std::sin(real) * std::cosh(imag), std::cos(real) * std::sinh(imag));
    }

    // Cos
    Complex cos() const {
        return Complex(std::cos(real) * std::cosh(imag), -std::sin(real) * std::sinh(imag));
    }

    // Tanh
    Complex tanh() const {
        return this->sinh() / this->cosh(); 
    }

    // Sinh
    Complex sinh() const {
        return Complex(std::sinh(real) * std::cos(imag), std::cosh(real) * std::sin(imag));
    }

    // Cosh
    Complex cosh() const {
        return Complex(std::cosh(real) * std::cos(imag), std::sinh(real) * std::sin(imag));
    }

    // Log10
    Complex log10() const {
        return log() / std::log(10.0);
    }

    // Log nthing
    Complex logn(const Complex& base) const {
        return this->log() / base.log();
    }

    Complex maximum(const Complex& arg2) const {
        return this->abs() < arg2.abs() ? arg2 : *this;
    }

    Complex minimum(const Complex& arg2) const {
        return this->abs() > arg2.abs() ? arg2 : *this;
    }

    Complex round() const {
        return Complex(std::round(real), std::round(imag));
    }

    // Complex gamma() const {
    //     // Using Stirling's approximation for Gamma function
    //     Complex z = *this;
    //     Complex stirling_approx = (2.0 * (pi / z)).sqrt() * (z / e).pow(z);
    //     return stirling_approx;
    // }
    
    
    Complex gamma() const {
        const double sqrt_2_pi = std::sqrt(2 * 3.1415926535897932384626433832795028841971693993751);
        Complex z = *this;
        
        if (z.real <= 0 && z.imag == 0) {
            return Complex(0,0);
        }
        Complex e(2.7182818284590452353602874713526624977572470937000,0);
        return sqrt_2_pi * e.pow(z * (z.log() - 1.0));
    }

    Complex zeta() const {
        Complex sum(0, 0);
        const int N = 100;  // Number of terms for approximation
        for (int n = 1; n <= N; ++n) {
            sum = sum + Complex(n, 0).pow(-(*this));
        }
        return sum;
    }
    friend std::ostream& operator<<(std::ostream& os, const Complex& c) {
        os << "(" << c.real << " + " << c.imag << "i)";
        return os;
    }
};




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

    std::shared_ptr<ASTNode> parseExpression() {
        auto node = parseTerm();
        while (pos < expr.size()) {
            if (expr[pos] == '+') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseTerm(), [](const Complex& a, const Complex& b) { return a + b; });
            } else if (expr[pos] == '-') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseTerm(), [](const Complex& a, const Complex& b) { return a - b; });
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
                node = std::make_shared<BinaryOpNode>(node, parseFactor(), [](const Complex& a, const Complex& b) { return a * b; });
            } else if (expr[pos] == '/') {
                ++pos;
                node = std::make_shared<BinaryOpNode>(node, parseFactor(), [](const Complex& a, const Complex& b) { return a / b; });
            } else {
                break;
            }
        }
        return node;
    }

    std::shared_ptr<ASTNode> parseFactor() {
        skipWhitespace();

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
                throw std::runtime_error("Expected ')'");
            }
            ++pos;
            return node;
        }

        throw std::runtime_error("Unexpected character in expression");
    }


    std::shared_ptr<ASTNode> parseVariableOrFunction() {
        std::string name;
        while (pos < expr.size() && isalpha(expr[pos])) {
            name += expr[pos++];
        }

        skipWhitespace();
        if (pos < expr.size() && expr[pos] == '(') {
            return parseFunction(name);
        }

        if (variables.find(name) != variables.end()) {
            return std::make_shared<VariableNode>(variables.at(name));
        } else {
            throw std::runtime_error("Unknown variable: " + name);
        }
    }

    std::shared_ptr<ASTNode> parseNumber() {
        skipWhitespace();
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
        
        skipWhitespace(); // Skip any leading whitespace before parsing expressions
        auto arg1 = parseExpression(); // Parse the first argument
        
        std::shared_ptr<ASTNode> arg2 = nullptr; // Optional second argument
        
        skipWhitespace(); // Skip any whitespace after the first argument

        // Parse binary functions (expecting a second argument)
        if (func == "logn" || func == "pow" || func == "root"|| func == "max"|| func == "min") {
            if (expr[pos] == ',') {
                ++pos; // Skip ','
                skipWhitespace(); // Skip any whitespace before the second argument
                arg2 = parseExpression();  // Parse the second argument for binary functions
            } else {
                throw std::runtime_error("Expected ',' between arguments for " + func);
            }
        }

        skipWhitespace(); // Skip any trailing whitespace before closing the parentheses

        if (expr[pos] != ')') {
            throw std::runtime_error("Expected ')' to close the function " + func);
        }
        ++pos;  // Skip ')'

        // Mapping functions to nodes (unary and binary)
        static const std::unordered_map<std::string, std::function<std::shared_ptr<ASTNode>(std::shared_ptr<ASTNode>, std::shared_ptr<ASTNode>)>> functionMap = {
            {"sin", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sin(); }); }},
            {"cos", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.cos(); }); }},
            {"sqrt", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sqrt(); }); }},
            {"log", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.log(); }); }},
            {"logten", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.log10(); }); }},
            {"sinh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.sinh(); }); }},
            {"cosh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.cosh(); }); }},
            {"tanh", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.tanh(); }); }},
            {"arg", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.arg(); }); }},
            {"conj", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.conj(); }); }},
            {"abs", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.abs(); }); }},
            {"round", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.round(); }); }},
            {"logn", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.logn(b); }); }},
            {"pow", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.pow(b); }); }},
            {"root", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.root(b); }); }},
            {"max", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.maximum(b); }); }},
            {"min", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode> arg2) { return std::make_shared<BinaryFunctionNode>(arg1, arg2, [](const Complex& a, const Complex& b) { return a.minimum(b); }); }},
            {"gamma", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.gamma(); }); }},
            {"zeta", [](std::shared_ptr<ASTNode> arg1, std::shared_ptr<ASTNode>) { return std::make_shared<UnaryFunctionNode>(arg1, [](const Complex& a) { return a.zeta(); }); }},
        };

        auto it = functionMap.find(func);
        if (it != functionMap.end()) {
            return it->second(arg1, arg2);  // Call the mapped function
        } else {
            throw std::runtime_error("Unknown function: " + func);
        }
    }

    void skipWhitespace() {
        while (pos < expr.size() && isspace(expr[pos])) {
            ++pos;
        }
    }
};


void update_output(uint16_t* output, const double temp, const uint16_t max_iter, const uint16_t width, const uint16_t iteration, const uint16_t x, const uint16_t y, const bool lake, const bool lya) {
    if ((temp < 2 && lake)) {
        output[y * width + x] = static_cast<uint16_t>(std::round((temp / (temp + 1)) * max_iter)) + max_iter;
    } else if ( lya) {
        output[y * width + x] = static_cast<uint16_t>((std::round((temp / (temp + 1)) * max_iter)));
    } else {
        output[y * width + x] = iteration;
    }
}
    




extern "C" {
    void scale(const float* input_tensor, float* scaled_tensor, int input_size, float new_min, float new_max) {
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



    void fractal(uint16_t* output, const char* exp,const uint16_t width, const uint16_t height, const uint16_t max_iter, const double xmin, const double xmax, const double ymin, const double ymax, const double c_real, const double c_imag, const bool juliaset, const bool lake) {

        std::signal(SIGINT, signal_handler);

        const double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;


        const std::string expression = std::string(exp);

        if (expression == "rt*rt+rw" || expression == "pow(rt,2)+rw" ) {

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
    
                        z = (z*z)+c;
                        ++iteration;
                    }
                    update_output( output, z.abs(), max_iter, width, iteration, x, y, lake, false);
                }
            }
        } else {
            
            int y;
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                Complex z, c;
    
                std::map<std::string, std::function<Complex()>> variables = {
                    {"rt", [&z]() { return z; }},
                    {"rw", [&c]() { return c; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"y", [&]() { return static_cast<double>(y); }},
                    {"x", [&]() { return static_cast<double>(x); }}
                };
    
                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables);
                auto ast = parser.parse();
    
    
                for (int y = 0; y < height; ++y) {
                    double r_part = xmin + x * dx;
                    double i_part = ymin + y * dy;
    

                    if (juliaset) {
                        c = Complex (-0.8, 0.16);
                        z = Complex (r_part, i_part);
                    } else {
                        c = Complex (r_part, i_part);
                        z = Complex (0.0, 0.0);
                    }

    
                    double temp = z.abs();
                    uint16_t iteration = 0;
    
                    while (temp < 2 && iteration < max_iter) {
                        z = (ast->evaluate()).noNan();
                        temp = z.abs();
                        ++iteration;
                    }
                    update_output( output, temp, max_iter, width, iteration, x, y, lake, false);
                }
            }
       } 
    }



    void lyapunov(uint16_t* output, const uint16_t width, const uint16_t height, const uint16_t max_iter, const double xmin=3.4, const double xmax=4.0, const double ymin=2.5, const double ymax=3.4) {
        
        std::signal(SIGINT, signal_handler);
        
        const double dx = (xmax - xmin) / width;
        const double dy = (ymax - ymin) / height;

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                double x = xmin + i * dx;
                double y = ymin + j * dy;
                Complex a(0.5 + x * 0.5, 0.0);
                Complex b(0.5 + y * 0.5, 0.0);
                Complex l(0.0, 0.0);
                Complex v(0.5, 0.0);

                for (int k = 0; k < max_iter; ++k) {
                    if (k % 12 < 6) {
                        v = b * v * (1 - v);
                        l += noNan(log((b * (1 - 2 * v)).abs()));
                    } else {
                        v = a * v * (1 - v);
                        l += noNan(log((a * (1 - 2 * v)).abs()));
                    }
                }
                update_output( output, l.abs(), max_iter, width, 0, i, j, false, true);
                
            }
        }
    }



    void sandpile(uint8_t* output, uint16_t width, uint16_t height, uint32_t n_grains, uint16_t max_grains=3) {
        // Create a 2D array to store the sandpile
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
                    if (sandpile[y][x] > max_grains) {
                        // Distribute grains to neighboring cells
                        if (y > 0) sandpile[y-1][x] += sandpile[y][x] / 4;
                        if (y < height-1) sandpile[y+1][x] += sandpile[y][x] / 4;
                        if (x > 0) sandpile[y][x-1] += sandpile[y][x] / 4;
                        if (x < width-1) sandpile[y][x+1] += sandpile[y][x] / 4;

                        // Remove grains from current cell
                        sandpile[y][x] %= 4;
                        unstable = true;
                    }
                    output[y * width + x] = static_cast<uint8_t>(sandpile[y][x]);
                }
            }
        }
    }



    void process_array(uint32_t* input_array, uint8_t* output_array, uint16_t width, uint16_t height, double max_value, uint16_t batch_size, double npmax) {
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



