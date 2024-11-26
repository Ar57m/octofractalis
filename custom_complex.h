#ifndef CUSTOM_COMPLEX_H
#define CUSTOM_COMPLEX_H

#include <iostream>
#include <cmath>
#include <limits>
#include <cstdint>


class Complex {
public:
    double real;
    double imag;
    double j;
    double k;
    bool complex_type;


    static constexpr double MAX_THRESHOLD = 1e300;
    
    inline Complex(double r = 0.0, double i = 0.0, double j_ = 0.0, double k_ = 0.0) {
        real = (std::abs(r) < MAX_THRESHOLD) ? r : 0.0;
        imag = (std::abs(i) < MAX_THRESHOLD) ? i : 0.0;
        j = (std::abs(j_) < MAX_THRESHOLD) ? j_ : 0.0;
        k = (std::abs(k_) < MAX_THRESHOLD) ? k_ : 0.0;
        
        complex_type = (j == 0.0 && k == 0.0);
    }

    static constexpr double e = 2.7182818284590452353602874713526624977572470937000;
    static constexpr double pi = 3.1415926535897932384626433832795028841971693993751;


    inline Complex& sanitize() {
        real = (std::abs(real) < MAX_THRESHOLD) ? real : 0.0;
        imag = (std::abs(imag) < MAX_THRESHOLD) ? imag : 0.0;
        j = (std::abs(j) < MAX_THRESHOLD) ? j : 0.0;
        k = (std::abs(k) < MAX_THRESHOLD) ? k : 0.0;
        return *this;
    }

    // Sum
    inline Complex operator+(const Complex& other) const {
        return (complex_type && other.complex_type) ? Complex(real + other.real, imag + other.imag) : Complex(real + other.real, imag + other.imag, j + other.j, k + other.k);
    }

    inline Complex operator+(double value) const{
        return Complex(real + value, imag, j, k);
    }

    inline Complex operator+() const {
        return *this;
    }
    
    inline Complex& operator+=(const Complex& other) {
        real += other.real;
        imag += other.imag;
        j += other.j;
        k += other.k;
        return sanitize();
    }

    inline Complex& operator+=(double value) {
        real += value;
        return sanitize();
    }
    inline friend Complex operator+(double value, const Complex& c) {
        return c.complex_type ? Complex(c.real + value, c.imag) : Complex(c.real + value, c.imag, c.j, c.k);
    }

    // Sub
    inline Complex operator-(const Complex& other) const {
        return Complex(real - other.real, imag - other.imag, j - other.j, k - other.k);
    }
    inline Complex operator-(double value) {
        real -= value; 
        return sanitize();
    }

    inline Complex operator-() const {
        return Complex(-real, -imag, -j, -k);
    }

    inline Complex& operator-=(const Complex& other) {
        real -= other.real;
        imag -= other.imag;
        j -= other.j;
        k -= other.k;
        return sanitize();
    }

    inline Complex& operator-=(double value) {
        real -= value;
        return sanitize();
    }

    inline friend Complex operator-(double value, const Complex& c) {
        return Complex(value - c.real, -c.imag, -c.j, -c.k);
    } 

    // Mul
    inline Complex operator*(const Complex& other) const {
            return (complex_type && other.complex_type) ? Complex(real * other.real - imag * other.imag, real * other.imag + imag * other.real) : Complex(
                real * other.real - imag * other.imag - j * other.j - k * other.k,
                real * other.imag + imag * other.real + j * other.k - k * other.j,
                real * other.j - imag * other.k + j * other.real + k * other.imag,
                real * other.k + imag * other.j - j * other.imag + k * other.real
            );
    }

    inline Complex operator*(double scalar) const {
        return complex_type ? Complex(real * scalar, imag * scalar) : Complex(real * scalar, imag * scalar, j * scalar, k * scalar);
    }

    Complex& operator*=(const Complex& other) {
        if (!complex_type || !other.complex_type) { 
            real = real * other.real - imag * other.imag - j * other.j - k * other.k;
            imag = real * other.imag + imag * other.real + j * other.k - k * other.j;
            j = real * other.j - imag * other.k + j * other.real + k * other.imag;
            k = real * other.k + imag * other.j - j * other.imag + k * other.real;
            return sanitize();
        }
        real = real * other.real - imag * other.imag;
        imag = real * other.imag + imag * other.real;

        return sanitize();
    }

    Complex& operator*=(double value) {
        real *= value;
        imag *= value;
        if (!complex_type) {
            j *= value;
            k *= value;
        }
        return sanitize();
    }

    inline friend Complex operator*(double scalar, const Complex& c) {
        return c * scalar;
    }

    // Division
    Complex operator/(const Complex& other) const {
        double denom = other.real * other.real + other.imag * other.imag
                        + other.j * other.j + other.k * other.k;
        return ((*this * other.conj() ) / denom).sanitize(); 
    }

    inline Complex operator/(double value) const {
        return Complex(real / value, imag / value, j / value, k / value); 
    }

    friend Complex operator/(double value, const Complex& c) {
        double denom = c.real * c.real + c.imag * c.imag + c.j * c.j + c.k * c.k;
        return Complex(
            (value * c.real) / denom,
            (-value * c.imag) / denom,
            (-value * c.j) / denom,
            (-value * c.k) / denom
        );
    }
    
    inline double fmod(const double a, const double b) const {
        double abs_b = std::abs(b);
        double abs_a = std::abs(a);
        if (!(abs_b < 1e300) || !(abs_b > 1e-300) || !(abs_a < 1e300)) return 0.0;
    
        double result = a - std::floor(a / b) * b;
        return (b > 0) ? (result < 0 ? result + abs_b : result) : (result > 0 ? result - abs_b : result);
    }
    
    Complex operator%(const Complex& other) const {
        if (complex_type && other.complex_type) {
            return Complex(
                fmod(real, other.real),
                fmod(imag, other.imag)
            );
        } else {
            return Complex(
                fmod(real, other.real),
                fmod(imag, other.imag),
                fmod(j, other.j),
                fmod(k, other.k)
            );
        }
    }
    
    // Conj
    inline Complex conj() const {
        return Complex(real, -imag, -j, -k);
    }


    void rotate(double angle, int axis) {
        double rad, cosTheta, sinTheta;
        double x = real;
        double y = imag;
        double z = j;

        switch (axis) {
            case 0: // X
                rad = angle * (pi / 180.0);
                cosTheta = std::cos(rad);
                sinTheta = std::sin(rad);

                imag = y * cosTheta - z * sinTheta;
                j = y * sinTheta + z * cosTheta;
                break;

            case 1: // Y
                rad = angle * (pi / 180.0);
                cosTheta = std::cos(rad);
                sinTheta = std::sin(rad);

                real = x * cosTheta + z * sinTheta;
                j = -x * sinTheta + z * cosTheta;
                break;

            case 2: // Z
                rad = angle * (pi / 180.0);
                cosTheta = std::cos(rad);
                sinTheta = std::sin(rad);

                real = x * cosTheta - y * sinTheta;
                imag = x * sinTheta + y * cosTheta;
                break;

            default:
                break;
        }
    }

    // abs / remove the sign
    inline Complex abs() const {
        return Complex(std::abs(real), std::abs(imag), std::abs(j), std::abs(k));
    }

    // magnitude
    inline double mag() const {
        return std::sqrt(real * real + imag * imag + j * j + k * k); 
    }

    inline Complex c_mag() const {
        return Complex(std::sqrt(real * real + imag * imag + j * j + k * k), 0.0);
    }

    // sqrt
    Complex sqrt() const {
        if (complex_type) {
            double magnitude = std::sqrt(mag());
            double angle = std::atan2(imag, real) / 2.0;
            return Complex(magnitude * std::cos(angle), magnitude * std::sin(angle));
        } else {
            double magnitude = std::sqrt(real * real + imag * imag + j * j + k * k);
            return Complex(std::sqrt((magnitude + real) / 2.0),
                    (imag / std::sqrt(2 * (magnitude + real))),
                    (j / std::sqrt(2 * (magnitude + real))),
                    (k / std::sqrt(2 * (magnitude + real))));
        }
    }
    // arg
    inline double arg() const {
        if (complex_type) {
            return std::atan2(imag, real);
        } else {
            double norm = std::sqrt(real * real + imag * imag + j * j + k * k);
            return (norm == 0.0) ? 0.0 : 2.0 * std::acos(real / norm);
        }
    }

    // log
    Complex log() const {
        if (complex_type) {
            return Complex(std::log(mag()), std::atan2(imag, real));
        } else {
            double imag_magnitude = std::sqrt(imag * imag + j * j + k * k);
            double scale = std::atan2(imag_magnitude, real) / imag_magnitude;
            return Complex(std::log(this->mag()), imag * scale, j * scale, k * scale);
        }
    }
    
    // pow
    Complex pow(const Complex& exponent) const {
        double r = this->mag();
        double theta = std::atan2(imag, real);

        double new_magnitude = std::pow(r, exponent.real) * std::exp(-exponent.imag * theta);
        double new_phase = exponent.real * theta + exponent.imag * std::log(r);

        if (j == 0 && k == 0 && exponent.j == 0 && exponent.k == 0) {
            return Complex(
                new_magnitude * std::cos(new_phase),
                new_magnitude * std::sin(new_phase)
            );
        }
        double imag_magnitude = std::sqrt(imag * imag + j * j + k * k);
        theta = (imag_magnitude == 0) ? 0 : std::atan2(imag_magnitude, real);
        new_phase = exponent.real * theta + exponent.imag * std::log(r); 

        double scale = (imag_magnitude == 0) ? 0 : new_magnitude * std::sin(new_phase) / imag_magnitude;

        return Complex(
            new_magnitude * std::cos(new_phase),
            scale * imag,
            scale * j,
            scale * k
        );
    }

    // pow
    Complex pow(double exponent) const {
        if (complex_type) {
            double magnitude = std::pow(mag(), exponent);
            double angle = std::atan2(imag, real) * exponent;
            return Complex(magnitude * std::cos(angle), magnitude * std::sin(angle));
        } else {
            double magnitude = std::pow(this->mag(), exponent);
            double angle = std::acos(real / this->mag()) * exponent;
            double imag_magnitude = std::sqrt(imag * imag + j * j + k * k);
        
            return Complex(
                magnitude * std::cos(angle),
                magnitude * std::sin(angle) * (imag / imag_magnitude),
                magnitude * std::sin(angle) * (j / imag_magnitude) ,
                magnitude * std::sin(angle) * (k / imag_magnitude)
            );
        } 
    }

    // root
    Complex root(const Complex& n1) const {
        return this->pow(1.0/n1);
    }
    
    


    // Sin
    Complex sin() const {
        if (complex_type) {
            return Complex(std::sin(real) * std::cosh(imag), std::cos(real) * std::sinh(imag));
        } else {
            double imag_magnitude = std::sqrt(imag * imag + j * j + k * k);
            double scale = std::cos(real) * std::sinh(imag_magnitude) / imag_magnitude;
            return Complex(std::sin(real) * std::cosh(imag_magnitude), imag * scale, j * scale, k * scale);
        }
    }

    // Cos
    Complex cos() const {
        if (complex_type) {
            return Complex(std::cos(real) * std::cosh(imag), -std::sin(real) * std::sinh(imag));
        } else {
            double imag_magnitude = std::sqrt(imag * imag + j * j + k * k);
            double scale = -std::sin(real) * std::sinh(imag_magnitude) / imag_magnitude;
            return Complex(std::cos(real) * std::cosh(imag_magnitude), imag * scale, j * scale, k * scale);
        }
    }

    // Tan
    Complex tan() const {
        return this->sin() / this->cos();
    }

    // Sinh
    Complex sinh() const {
        if (complex_type) {
            return Complex(std::sinh(real) * std::cos(imag), std::cosh(real) * std::sin(imag));
        } else {
            double imag_magnitude = std::sqrt(imag * imag + j * j + k * k);
            double scale = std::cosh(real) * std::sin(imag_magnitude) / imag_magnitude;
            return Complex(
                std::sinh(real) * std::cos(imag_magnitude),
                imag * scale,
                j * scale,
                k * scale
            );
        }
    }
    
    // Cosh
    Complex cosh() const {
        if (complex_type) {
            return Complex(std::cosh(real) * std::cos(imag), std::sinh(real) * std::sin(imag));
        } else {
            double imag_magnitude = std::sqrt(imag * imag + j * j + k * k);
            double scale = std::sinh(real) * std::sin(imag_magnitude) / imag_magnitude;
            return Complex(
                std::cosh(real) * std::cos(imag_magnitude),
                imag * scale,
                j * scale,
                k * scale
            );
        }
    }

    // Tanh
    Complex tanh() const {
        return this->sinh() / this->cosh(); 
    }

    // Log10
    Complex log10() const {
        return log() / std::log(10.0);
    }

    // Log nthing
    Complex logn(const Complex& base) const {
        return this->log() / base.log();
    }

    inline Complex maximum(const Complex& arg2) const {
        return this->mag() < arg2.mag() ? arg2 : *this;
    }

    inline Complex minimum(const Complex& arg2) const {
        return this->mag() > arg2.mag() ? arg2 : *this;
    }

    Complex round() const {
        return Complex(std::round(real), std::round(imag), std::round(j), std::round(k));
    }

    Complex circle(const Complex radius) const {
        double angle = std::atan2(imag, real);
        double rad = radius.mag();
        return Complex(std::cos(angle) * rad, std::sin(angle) * rad);
    }

    Complex square(const Complex sideLength) const {
        double side = sideLength.mag()/2.0;
        double x_proj = (real > 0) ? side : -side;
        double y_proj = (imag > 0) ? side : -side;
        return Complex(x_proj, y_proj);
    }

    Complex triangle(const Complex sideLength) const {
        double side = sideLength.mag();
        double height = std::sqrt(3) / 2 * side;
        double x_proj = (real > 0) ? side / 2 : -side / 2;
        double y_proj = (imag > 0) ? height / 3 : -height / 3;
        return Complex(x_proj, y_proj);
    }

    Complex ellipsoid(Complex radiusX, Complex radiusY) const {
        double angle = std::atan2(imag, real);
        return Complex(std::cos(angle) * radiusX.mag(), std::sin(angle) * radiusY.mag());
    }





    static constexpr double lanczos_coeffs[9] = {
        0.99999999999980993, 
        676.5203681218851, 
        -1259.1392167224028, 
        771.32342877765313, 
        -176.61502916214059, 
        12.507343278686905, 
        -0.13857109526572012, 
        9.9843695780195716e-6, 
        1.5056327351493116e-7
    };


    Complex gamma() const {
        const Complex z = *this - 1.0;

        Complex x(lanczos_coeffs[0], 0.0);
        for (int i = 1; i < 9; ++i) {
            x = x + lanczos_coeffs[i] / (z + Complex(static_cast<double>(i), 0.0));
        }

        const Complex t = z + 7.0 + 0.5;

        return (Complex(2.0 * pi, 0.0).sqrt()) * t.pow(z + 0.5) * Complex(e, 0.0).pow(-t) * x;
    }


    Complex zeta() const {
        Complex sum(0.0, 0.0);
        const int N = 120;
        const double threshold = 1e-9;
        
        for (int n = 1; n <= N; ++n) {
            Complex term = Complex(n, 0.0).pow(-(*this));
            sum = sum + term;

            if (std::abs(term.real) < threshold && std::abs(term.imag) < threshold) break;
        }
        return sum;
    }

    // probably broken, will have to fix/rewrite
    Complex airy() const {
        const Complex z = *this;
        const int steps = 1000;
        const double upper_limit = 100.0;
        Complex sum(0.0, 0.0);
        const double h = upper_limit / steps;

        for (int i = 0; i < steps; ++i) {
            const double t1 = i * h;
            const double t2 = (i + 1) * h;

            const Complex f1 = Complex(std::pow(t1, 3) / 3.0 + z.real * t1, z.imag * t1).cos();
            const Complex f2 = Complex(std::pow(t2, 3) / 3.0 + z.real * t2, z.imag * t2).cos();

            sum = sum + 0.5 * (f1 + f2) * h;
        }
        return sum / pi;
    }



    friend std::ostream& operator<<(std::ostream& os, const Complex& c) {
        os << "(" << c.real 
            << (c.imag >= 0.0 ? " +" : " -") << std::abs(c.imag) << "i"
            << (c.j >= 0.0 ? " +" : " -") << std::abs(c.j) << "j"
            << (c.k >= 0.0 ? " +" : " -") << std::abs(c.k) << "k)";
        return os;
    }
    
    void print() const {
        std::cout << "(" << real 
                    << (imag >= 0.0 ? " +" : " -") << std::abs(imag) << "i"
                    << (j >= 0.0 ? " +" : " -") << std::abs(j) << "j"
                    << (k >= 0.0 ? " +" : " -") << std::abs(k) << "k)\n";
    }
};


#endif // CUSTOM_COMPLEX_H

