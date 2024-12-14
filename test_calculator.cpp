#include <iostream>
#include <map>
#include <functional>
#include <string>
#include <algorithm>

#include "custom_quaternion.h"
#include "parser.h"

const Quaternion pi(3.141592653589793, 0.0);
const Quaternion e(2.718281828459045, 0.0);
const Quaternion phi(1.618033988749895, 0.0);

int main() {
    Quaternion z, c;
    double x = 1, y = 2;
    z = Quaternion(0.5, 1.5);//, 0.2, 0.1);
    c = Quaternion(1.0, -1.0,3.0,2.0);//, 0.8, 0.4);
    
    //sstd::cout << z.pow(2) << z.pow(Quaternion(2.0)) <<c.pow(2) << c.pow(Quaternion(2.0))<<"\n";

    const std::map<std::string, std::function<Quaternion()>> variables = {
        {"z", [&z]() { return z; }},
        {"c", [&c]() { return c; }},
        {"phi", [&]() { return phi; }},
        {"pi", [&]() { return pi; }},
        {"e", [&]() { return e; }},
        {"y", [&]() { return Quaternion(y, 0.0); }},
        {"x", [&]() { return Quaternion(x, 0.0); }}
    };

    std::string expression;
    while (true) {
        std::cout << "\nEnter expression (or type 'exit' to quit):\n";
        std::getline(std::cin, expression);
        expression.erase(std::remove(expression.begin(), expression.end(), ' '), expression.end());

        if (expression.find("exit") == 0) break;

        try {
            if (expression.find("z=") == 0) {
                expression.erase(0, 2);
                Parser parser(expression, variables);
                const auto ast = parser.parse();
                z = ast->evaluate();
                std::cout << "\nValue assigned to z: " << z << "\n";
                continue;
            }

            if (expression.find("c=") == 0) {
                expression.erase(0, 2);
                Parser parser(expression, variables);
                const auto ast = parser.parse();
                c = ast->evaluate();
                std::cout << "\nValue assigned to c: " << c << "\n";
                continue;
            }

            Parser parser(expression, variables);
            const auto ast = parser.parse();
            Quaternion result = ast->evaluate();
            std::cout << "\nResult: " << result << "\n";
        } catch (const std::exception& e) {
            std::cerr << "\nError: " << e.what() << "\n";
        }
    }

    return 0;
}
// g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -fopenmp -o test_calculator test_calculator.cpp 