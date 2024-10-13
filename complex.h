#ifndef COMPLEX_H
#define COMPLEX_H

#include <iostream>
#include <cmath>
#include <limits>

struct Complex {
    double real;
    double imag;

    Complex(double r = 0.0, double i = 0.0) : real(r), imag(i) {}


    // Complex noNan() const {
    //     double realPart = (std::abs(real) > 1e-13 && !std::isnan(real) && !std::isinf(real)) ? real : (std::isinf(real) ? 3 : 0);
    //     double imagPart = (std::abs(imag) > 1e-13 && !std::isnan(imag) && !std::isinf(imag)) ? imag : 0;
    //     return Complex(realPart, imagPart);
    // }

    inline Complex noNan() const {
        
        double realPart = std::abs(real);
        double imagPart = std::abs(imag);

        return Complex((realPart > 1e-13 && realPart < 1e300) ? real : 0, (imagPart > 1e-13 && imagPart < 1e300) ? imag : 0);
    }



    // Sum
    inline Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }

    inline Complex operator+(double value) const {
        return Complex(real + value, imag);
    }

    inline Complex operator+() const {
    return Complex(+real, +imag);
    }
    
    inline Complex& operator+=(const Complex& other) {
        real += other.real;
        imag += other.imag;
        return *this;
    }

    inline Complex& operator+=(double value) {
        real += value;
        return *this;
    }
    inline friend Complex operator+(double value, const Complex& c) {
        return Complex(c.real + value, c.imag);
    }

    // Sub
    inline Complex operator-(const Complex& other) const {
        return Complex(real - other.real, imag - other.imag);
    }
    inline Complex operator-(double value) const {
        return Complex(real - value, imag);
    }

    inline Complex operator-() const {
    return Complex(-real, -imag);
    }

    inline Complex& operator-=(const Complex& other) {
        real -= other.real;
        imag -= other.imag;
        return *this;
    }

    inline Complex& operator-=(double value) {
        real -= value;
        return *this;
    }

    inline friend Complex operator-(double value, const Complex& c) {
        return Complex(value - c.real, c.imag);
    } 

    // Mul
    inline Complex operator*(const Complex& other) const {
        return Complex(real * other.real - imag * other.imag, real * other.imag + imag * other.real);
    }

    inline Complex operator*(double scalar) const {
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

    inline friend Complex operator*(double scalar, const Complex& c) {
        return c * scalar;
    }

    // Division
    Complex operator/(const Complex& other) const {
        double denom = other.real * other.real + other.imag * other.imag;
        return Complex((real * other.real + imag * other.imag) / denom,
                       (imag * other.real - real * other.imag) / denom);
    }

    inline Complex operator/(double value) const {
        return Complex(real / value, imag / value);
    }

    friend Complex operator/(double value, const Complex& c) {
        double denom = c.real * c.real + c.imag * c.imag;
        return Complex((value * c.real) / denom, (-value * c.imag) / denom);
    }

    // Conj
    inline Complex conj() const {
        return Complex(real, -imag);
    }

    // abs
    inline double abs() const {
        return std::sqrt(real * real + imag * imag);
    }

    inline Complex c_abs() const {
        return Complex (std::sqrt(real * real + imag * imag), 0);
    }

    // sqrt
    Complex sqrt() const {
        double magnitude = std::sqrt(abs());
        double angle = std::atan2(imag, real) / 2.0;
        return Complex(magnitude * std::cos(angle), magnitude * std::sin(angle));
    }
    
    // arg
    inline double arg() const {
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
        return Complex(std::log(abs()), std::atan2(imag, real));
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

    // Tan
    Complex tan() const {
        return Complex(std::sin(real) / std::cos(real), std::sin(imag) / std::cos(imag));
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

    inline Complex maximum(const Complex& arg2) const {
        return this->abs() < arg2.abs() ? arg2 : *this;
    }

    inline Complex minimum(const Complex& arg2) const {
        return this->abs() > arg2.abs() ? arg2 : *this;
    }

    Complex round() const {
        return Complex(std::round(real), std::round(imag));
    }

    Complex circle(const Complex radius) const {
        double angle = std::atan2(imag, real);
        double rad = radius.abs();
        return Complex(std::cos(angle) * rad, std::sin(angle) * rad);
    }

    Complex square(const Complex sideLength) const {
        double side = sideLength.abs()/2.0;
        double x_proj = (real > 0) ? side : -side;
        double y_proj = (imag > 0) ? side : -side;
        return Complex(x_proj, y_proj);
    }

    Complex triangle(const Complex sideLength) const {
        double side = sideLength.abs();
        double height = std::sqrt(3) / 2 * side;
        double x_proj = (real > 0) ? side / 2 : -side / 2;
        double y_proj = (imag > 0) ? height / 3 : -height / 3;
        return Complex(x_proj, y_proj);
    }

    Complex ellipsoid(Complex radiusX, Complex radiusY) const {
        double angle = std::atan2(imag, real);
        return Complex(std::cos(angle) * radiusX.abs(), std::sin(angle) * radiusY.abs());
    }

    Complex gamma() const {
        Complex z = *this;
        
        if (z.real <= 0 && z.imag == 0) {
            return Complex(0,0);
        }
        const double sqrt_2_pi = std::sqrt(2 * 3.1415926535897932384626433832795028841971693993751);

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


struct Quaternion {
    double real;
    double i;
    double j;
    double k;


    Quaternion(double r = 0.0, double i_ = 0.0, double j_ = 0.0, double k_ = 0.0)
        : real(r), i(i_), j(j_), k(k_) {}

    inline Quaternion operator+(const Quaternion& q) const {
        return Quaternion(real + q.real, i + q.i, j + q.j, k + q.k);
    }

    inline Quaternion operator-(const Quaternion& q) const {
        return Quaternion(real - q.real, i - q.i, j - q.j, k - q.k);
    }
    inline friend Quaternion operator-(double scalar, const Quaternion& q) {
        return Quaternion(scalar - q.real, -q.i, -q.j, -q.k);
    }

    Quaternion& operator+=(const Quaternion& q) {
        real += q.real;
        i += q.i;
        j += q.j;
        k += q.k;
        return *this;
    }

    inline friend Quaternion operator*(double scalar, const Quaternion& q) {
        return Quaternion(scalar * q.real, scalar * q.i, scalar * q.j, scalar * q.k);
    }

    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            real * q.real - i * q.i - j * q.j - k * q.k,
            real * q.i + i * q.real + j * q.k - k * q.j,
            real * q.j - i * q.k + j * q.real + k * q.i,
            real * q.k + i * q.j - j * q.i + k * q.real
        );
    }

    inline Quaternion conj() const {
        return Quaternion(real, -i, -j, -k);
    }

    inline double abs() const {
        return std::sqrt(real * real + i * i + j * j + k * k);
    }

    inline Quaternion q_abs() const {
        return Quaternion(std::sqrt(real * real + i * i + j * j + k * k),0);
    }

    Quaternion operator/(const Quaternion& q) const {
        Quaternion conj_q = q.conj();
        double norm_q2 = q.abs() * q.abs();

        return (*this * conj_q) / norm_q2;
    }

    inline Quaternion operator/(double scalar) const {
        return Quaternion(real / scalar, i / scalar, j / scalar, k / scalar);
    }
    
    Quaternion noNan() const {
        double realPart = (std::abs(real) > 1e-13 && (std::abs(real) < 1e300)) ? real : 0;
        double iPart = (std::abs(i) > 1e-13 && (std::abs(i) < 1e300)) ? i : 0;
        double jPart = (std::abs(j) > 1e-13 && (std::abs(j) < 1e300)) ? j : 0;
        double kPart = (std::abs(k) > 1e-13 && (std::abs(k) < 1e300)) ? k : 0;
        return Quaternion(realPart, iPart, jPart, kPart);
    }

    Quaternion sin() const {

        double imag_magnitude = std::sqrt(i * i + j * j + k * k);
    
        double scale = (imag_magnitude == 0) ? 0 : std::cos(real) * std::sinh(imag_magnitude) / imag_magnitude;
        
        return Quaternion(std::sin(real) * std::cosh(imag_magnitude), i * scale, j * scale, k * scale);
    }

    Quaternion log() const {
        double abs_q = this->abs();
        double imag_magnitude = std::sqrt(i * i + j * j + k * k);
    
        double theta = std::atan2(imag_magnitude, real);
    
        double scale = (imag_magnitude == 0) ? 0 : theta / imag_magnitude;
    
        return Quaternion(std::log(abs_q), i * scale, j * scale, k * scale);
    }
    
    Quaternion exp() const {
        double imag_magnitude = std::sqrt(i * i + j * j + k * k);
    
        double exp_real = std::exp(real);
    
        double scale = (imag_magnitude == 0) ? 0 : exp_real * std::sin(imag_magnitude) / imag_magnitude;
    
        return Quaternion(exp_real * std::cos(imag_magnitude), i * scale, j * scale, k * scale);
    }
    
    Quaternion pow(double n) const {
        return (this->log() * n).exp();
    }

    void print() const {
        std::cout << "(" << real << " + " << i << "i + " << j << "j + " << k << "k)" << std::endl;
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Quaternion& c) {
        os << "(" << c.real << " + " << c.i << "i + " << c.j << "j + " << c.k << "k)";
        return os;
    }
    
};

#endif // COMPLEX_H
