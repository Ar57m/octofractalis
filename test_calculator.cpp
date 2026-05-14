#include <iostream>
#include <cstring>



#include "parser.h"
// ===========================================================

// Forward declaration of the dimension‑specific runner
template <int Dim>
void runCalculator();

int main() {
    std::cout << "Choose hypercomplex dimension: 2 (complex), 4 (quaternion), 8 (octonion): ";
    int dim;
    std::cin >> dim;
    std::cin.ignore();   // ignore newline after number

    switch (dim) {
        case 2: runCalculator<2>(); break;
        case 4: runCalculator<4>(); break;
        case 8: runCalculator<8>(); break;
        default:
            std::cerr << "Invalid dimension. Exiting.\n";
            return 1;
    }
    return 0;
}

template <int Dim>
void runCalculator() {
    // Dimension‑specific types
    using H = Hypercomplex<Dim>;

    // Constants for this dimension
    H pi(3.141592653589793, 0.0);
    H e(2.718281828459045, 0.0);
    H phi(1.618033988749895, 0.0);

    // Variables
    H z(0.5, 1.5);
    H c(1.0, -1.0, 3.0, 2.0);

    // Variable entries – note: template argument <Dim>
    VariableEntry<Dim> varEntries[] = {
        {"z", &z},
        {"c", &c},
        {"phi", const_cast<H*>(&phi)},
        {"pi",  const_cast<H*>(&pi)},
        {"e",   const_cast<H*>(&e)}
    };
    const size_t numVars = sizeof(varEntries) / sizeof(varEntries[0]);

    // Arrays (dimension‑independent)
    double myArray[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    ArrayEntry arrEntries[] = {
        {"array", myArray, 5}
    };
    const size_t numArrays = sizeof(arrEntries) / sizeof(arrEntries[0]);

    char expression[256];

    while (true) {
        std::cout << "\n------------------------------------------------";
        std::cout << "\nEnter expression (e.g., 'z=c*z+pi', 'sqrt(z)', or 'exit'):\n> ";
        std::cin.getline(expression, sizeof(expression));

        // Remove whitespace
        size_t len = strlen(expression);
        size_t j = 0;
        for (size_t i = 0; i < len; ++i) {
            if (expression[i] != ' ' && expression[i] != '\t' && expression[i] != '\r')
                expression[j++] = expression[i];
        }
        expression[j] = '\0';

        if (strcmp(expression, "exit") == 0)
            break;

        if (j == 0)
            continue;

        try {
            const char* targetExpr = expression;
            H* assignTarget = nullptr;

            // Handle "z=" or "c=" assignments
            if (strncmp(expression, "z=", 2) == 0) {
                targetExpr = expression + 2;
                assignTarget = &z;
            } else if (strncmp(expression, "c=", 2) == 0) {
                targetExpr = expression + 2;
                assignTarget = &c;
            }

            size_t exprSize = strlen(targetExpr);

            // 1. Compile to bytecode
            Parser<Dim> parser(targetExpr, exprSize, varEntries, numVars, arrEntries, numArrays);
            parser.parse();

            const Instruction<Dim>* code = parser.getBytecode();
            size_t codeSize = parser.getBytecodeSize();

            if (codeSize == 0) {
                std::cout << "Warning: Empty bytecode generated (possible parse error).\n";
                continue;
            }

            // 2. Evaluate
            H result;
            evaluateBytecode<Dim>(result, code, codeSize, varEntries, arrEntries);

            if (assignTarget) {
                *assignTarget = result;
                std::cout << "Assigned result to variable.\n";
            }

            // Print results (assumes print() method exists)
            std::cout << "Result: ";
            result.print();   // you need to implement this for Hypercomplex<Dim>
            std::cout << "\nCurrent Z: ";
            z.print();
            std::cout << "\nCurrent C: ";
            c.print();
            std::cout << std::endl;
        }
        catch (const std::exception& ex) {
            std::cerr << "\nRuntime Error: " << ex.what() << "\n";
        }
    }
}

// g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -fopenmp -o test_calculator test_calculator.cpp
// ./test_calculator