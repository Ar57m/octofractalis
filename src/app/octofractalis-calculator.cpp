#include <cstdio>
#include <cstring>

#include "core/parser.h"

// ===========================================================

template <int Dim>
void runCalculator();

int main() {
    printf("Choose hypercomplex dimension: 2 (complex), 4 (quaternion), 8 (octonion): ");

    int dim;
    scanf("%d", &dim);
    getchar();

    switch (dim) {
        case 2: runCalculator<2>(); break;
        case 4: runCalculator<4>(); break;
        case 8: runCalculator<8>(); break;

        default:
            printf("Invalid dimension. Exiting.\n");
            return 1;
    }

    return 0;
}

template <int Dim>
void runCalculator() {
    using H = Hypercomplex<Dim>;

    H pi(3.141592653589793, 0.0);
    H e(2.718281828459045, 0.0);
    H phi(1.618033988749895, 0.0);

    H z(0.5, 1.5);
    H c(1.0, -1.0, 3.0, 2.0);

    VariableEntry<Dim> varEntries[] = {
        {"z", &z},
        {"c", &c},
        {"phi", const_cast<H*>(&phi)},
        {"pi",  const_cast<H*>(&pi)},
        {"e",   const_cast<H*>(&e)}
    };

    const size_t numVars =
        sizeof(varEntries) / sizeof(varEntries[0]);

    double myArray[] = {
        1.0, 2.0, 3.0, 4.0, 5.0
    };

    ArrayEntry arrEntries[] = {
        {"array", myArray, 5}
    };

    const size_t numArrays =
        sizeof(arrEntries) / sizeof(arrEntries[0]);

    char expression[256];

    while (true) {
        printf(
            "\n------------------------------------------------"
            "\nEnter expression "
            "(e.g., 'z=c*z+pi', 'sqrt(z)', or 'exit'):\n> "
        );

        if (!fgets(expression, sizeof(expression), stdin)) {
            break;
        }

        expression[strcspn(expression, "\n")] = '\0';

        size_t len = strlen(expression);
        size_t j = 0;

        for (size_t i = 0; i < len; ++i) {
            if (
                expression[i] != ' '  &&
                expression[i] != '\t' &&
                expression[i] != '\r'
            ) {
                expression[j++] = expression[i];
            }
        }

        expression[j] = '\0';

        if (strcmp(expression, "exit") == 0) {
            break;
        }

        if (j == 0) {
            continue;
        }

        try {
            const char* targetExpr = expression;
            H* assignTarget = nullptr;

            if (strncmp(expression, "z=", 2) == 0) {
                targetExpr = expression + 2;
                assignTarget = &z;
            }
            else if (strncmp(expression, "c=", 2) == 0) {
                targetExpr = expression + 2;
                assignTarget = &c;
            }

            size_t exprSize = strlen(targetExpr);

            Parser<Dim> parser(
                targetExpr,
                exprSize,
                varEntries,
                numVars,
                arrEntries,
                numArrays
            );

            parser.parse();

            const Instruction<Dim>* code =
                parser.getBytecode();

            size_t codeSize =
                parser.getBytecodeSize();

            if (codeSize == 0) {
                printf(
                    "Warning: Empty bytecode generated "
                    "(possible parse error).\n"
                );

                continue;
            }

            H result;

            evaluateBytecode<Dim>(
                result,
                code,
                codeSize,
                varEntries,
                arrEntries
            );

            if (assignTarget) {
                *assignTarget = result;
                printf("Assigned result to variable.\n");
            }

            printf("Result: ");
            result.print();

            printf("\nCurrent Z: ");
            z.print();

            printf("\nCurrent C: ");
            c.print();

            printf("\n");
        }
        catch (const std::exception& ex) {
            printf("\nRuntime Error: %s\n", ex.what());
        }
    }
}

// g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -fopenmp -o test_calculator test_calculator.cpp
// ./test_calculator