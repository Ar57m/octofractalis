#include <iostream>
#include <functional>
#include <algorithm>
#include <cstring>


#include "parser.h"

const QuaternionOrOctonion pi(3.141592653589793, 0.0);
const QuaternionOrOctonion e(2.718281828459045, 0.0);
const QuaternionOrOctonion phi(1.618033988749895, 0.0);

int main() {
    QuaternionOrOctonion z, c;

    z = QuaternionOrOctonion(0.5, 1.5);
    c = QuaternionOrOctonion(1.0, -1.0, 3.0, 2.0);
    
    VariableEntry varEntries[5] = {
        {"z", &z},
        {"c", &c},
        {"phi", const_cast<QuaternionOrOctonion*>(&phi)},
        {"pi", const_cast<QuaternionOrOctonion*>(&pi)},
        {"e", const_cast<QuaternionOrOctonion*>(&e)},
    };
    const size_t numVars = sizeof(varEntries) / sizeof(VariableEntry);

    double myArray[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    ArrayEntry arrEntries[1] = {
        {"array", myArray, 5}
    };
    const size_t numArrays = sizeof(arrEntries) / sizeof(ArrayEntry);
    

    
    while (true) {
        char expression[512] = {};

        std::cout << "\nEnter expression (or type 'exit' to quit):\n";
        std::cin.getline(expression, sizeof(expression));

        size_t len = std::strlen(expression);
        size_t j = 0;
        for (size_t i = 0; i < len; i++) {
            if (expression[i] != ' ') {
                expression[j++] = expression[i];
            }
        }
        expression[j] = '\0';

        if (std::strcmp(expression, "exit") == 0) {
            break;
        }

        char exprBody[510] = {};
        if (j > 2) {
            std::memcpy(exprBody, expression + 2, j - 2);
            exprBody[j - 2] = '\0';
        }


        try {
            size_t exprSize = std::strlen(expression);
            
            if (std::strncmp(expression, "z=", 2) == 0) {

                Parser parser(exprBody, exprSize, varEntries, numVars, arrEntries, numArrays);

                ASTNode* ast = parser.parse();
                z = ast->evaluate();
                std::cout << "\nValue assigned to z: " << z << "\n";
                continue;
            }

            if (std::strncmp(expression, "c=", 2) == 0) {

                Parser parser(exprBody, exprSize, varEntries, numVars, arrEntries, numArrays);

                ASTNode* ast = parser.parse();
                c = ast->evaluate();
                std::cout << "\nValue assigned to c: " << c << "\n";
                continue;
            }

            
            Parser parser(expression, exprSize, varEntries, numVars, arrEntries, numArrays);

            ASTNode* ast = parser.parse();
            QuaternionOrOctonion result = ast->evaluate();
            std::cout << "\nResult: " << result << "\n";
        }
        catch (const std::exception& ex) {
            std::cerr << "\nError: " << ex.what() << "\n";
        }
    }
    return 0;
}
// g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -fopenmp -DOCTO -o test_calculator test_calculator.cpp 