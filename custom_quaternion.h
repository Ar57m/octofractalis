#ifndef CUSTOM_QUATERNION_H
#define CUSTOM_QUATERNION_H

#include <iostream>
#include <cmath>
#include <limits>
#include <cstdint>


class Quaternion {
public:
    double real;
    double imag;
    double j;
    double k;
    // bool complex_type;


    static constexpr double MAX_THRESHOLD = 1e300;
    
    inline Quaternion(double r = 0.0, double i = 0.0, double j_ = 0.0, double k_ = 0.0) {
        real = (std::abs(r) < MAX_THRESHOLD) ? r : 0.0;
        imag = (std::abs(i) < MAX_THRESHOLD) ? i : 0.0;
        j = (std::abs(j_) < MAX_THRESHOLD) ? j_ : 0.0;
        k = (std::abs(k_) < MAX_THRESHOLD) ? k_ : 0.0;
        
    }

    static constexpr double e = 2.7182818284590452353602874713526624977572470937000;
    static constexpr double pi = 3.1415926535897932384626433832795028841971693993751;


    inline Quaternion& sanitize() {
        real = (std::abs(real) < MAX_THRESHOLD) ? real : 0.0;
        imag = (std::abs(imag) < MAX_THRESHOLD) ? imag : 0.0;
        j = (std::abs(j) < MAX_THRESHOLD) ? j : 0.0;
        k = (std::abs(k) < MAX_THRESHOLD) ? k : 0.0;
        return *this;
    }

    // Sum
    inline Quaternion operator+(const Quaternion& other) const {
        return Quaternion(real + other.real, imag + other.imag, j + other.j, k + other.k);
    }

    inline Quaternion operator+(double value) const{
        return Quaternion(real + value, imag, j, k);
    }

    inline Quaternion operator+() const {
        return *this;
    }
    
    inline Quaternion& operator+=(const Quaternion& other) {
        real += other.real;
        imag += other.imag;
        j += other.j;
        k += other.k;
        return sanitize();
    }

    inline Quaternion& operator+=(double value) {
        real += value;
        return sanitize();
    }
    inline friend Quaternion operator+(double value, const Quaternion& c) {
        return Quaternion(c.real + value, c.imag, c.j, c.k);
    }

    // Sub
    inline Quaternion operator-(const Quaternion& other) const {
        return Quaternion(real - other.real, imag - other.imag, j - other.j, k - other.k);
    }
    inline Quaternion operator-(double value) {
        real -= value; 
        return sanitize();
    }

    inline Quaternion operator-() const {
        return Quaternion(-real, -imag, -j, -k);
    }

    inline Quaternion& operator-=(const Quaternion& other) {
        real -= other.real;
        imag -= other.imag;
        j -= other.j;
        k -= other.k;
        return sanitize();
    }

    inline Quaternion& operator-=(double value) {
        real -= value;
        return sanitize();
    }

    inline friend Quaternion operator-(double value, const Quaternion& c) {
        return Quaternion(value - c.real, -c.imag, -c.j, -c.k);
    } 

    // Mul
    inline Quaternion operator*(const Quaternion& other) const {
            return Quaternion(
                real * other.real - imag * other.imag - j * other.j - k * other.k,
                real * other.imag + imag * other.real + j * other.k - k * other.j,
                real * other.j - imag * other.k + j * other.real + k * other.imag,
                real * other.k + imag * other.j - j * other.imag + k * other.real
            );
    }

    inline Quaternion operator*(double scalar) const {
        return Quaternion(real * scalar, imag * scalar, j * scalar, k * scalar);
    }

    Quaternion& operator*=(const Quaternion& other) { 
        real = real * other.real - imag * other.imag - j * other.j - k * other.k;
        imag = real * other.imag + imag * other.real + j * other.k - k * other.j;
        j = real * other.j - imag * other.k + j * other.real + k * other.imag;
        k = real * other.k + imag * other.j - j * other.imag + k * other.real;
        return sanitize();
    }

    Quaternion& operator*=(double value) {
        real *= value;
        imag *= value;
        j *= value;
        k *= value;
        return sanitize();
    }

    inline friend Quaternion operator*(double scalar, const Quaternion& c) {
        return c * scalar;
    }

    // Division
    Quaternion operator/(const Quaternion& other) const {
        double denom = other.real * other.real + other.imag * other.imag
                        + other.j * other.j + other.k * other.k;
        return ((*this * other.conj() ) / denom).sanitize(); 
    }

    inline Quaternion operator/(double value) const {
        return Quaternion(real / value, imag / value, j / value, k / value); 
    }

    friend Quaternion operator/(double value, const Quaternion& c) {
        double denom = c.real * c.real + c.imag * c.imag + c.j * c.j + c.k * c.k;
        return Quaternion(
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
    
    Quaternion operator%(const Quaternion& other) const {
        return Quaternion(
            fmod(real, other.real),
            fmod(imag, other.imag),
            fmod(j, other.j),
            fmod(k, other.k)
        );
    }
    
    // Conj
    inline Quaternion conj() const {
        return Quaternion(real, -imag, -j, -k);
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
    inline Quaternion abs() const {
        return Quaternion(std::abs(real), std::abs(imag), std::abs(j), std::abs(k));
    }

    // magnitude
    inline double mag() const {
        return std::sqrt(real * real + imag * imag + j * j + k * k); 
    }

    inline Quaternion c_mag() const {
        return Quaternion(std::sqrt(real * real + imag * imag + j * j + k * k), 0.0);
    }

    inline double imag_mag() const {
        return std::sqrt(imag * imag + j * j + k * k);
    }

    // sqrt
    Quaternion sqrt() const {
        double magnitude = mag();
        if (j == 0 && k == 0 ) {
            magnitude = std::sqrt(magnitude);
            double angle = std::atan2(imag, real) / 2.0;
            return Quaternion(magnitude * std::cos(angle), magnitude * std::sin(angle));
        } else {
            magnitude += real;
            double imag_mag =  std::sqrt(2 * magnitude);
            return Quaternion(std::sqrt( magnitude / 2.0),
                    (imag / imag_mag),
                    (j / imag_mag),
                    (k / imag_mag));
        }
    }
    // arg
    inline double arg() const {
        if (j == 0 && k == 0 ) {
            return std::atan2(imag, real);
        } else {
            double norm = mag();
            return (norm == 0.0) ? 0.0 : std::acos(real / norm);
        }
    }

    // log
    Quaternion log() const {
        double log_mag = std::log(mag());
        if (j == 0 && k == 0 ) {
            return Quaternion(log_mag, std::atan2(imag, real));
        } else {
            double imag_magnitude = imag_mag();
            double scale = std::atan2(imag_magnitude, real) / imag_magnitude;
            return Quaternion(log_mag, imag * scale, j * scale, k * scale);
        }
    }
    
    // pow
    Quaternion pow(const Quaternion& exponent) const {
        double r = this->mag();
        double theta = std::atan2(imag, real);

        double new_magnitude = std::pow(r, exponent.real) * std::exp(-exponent.imag * theta);
        double new_phase = exponent.real * theta + exponent.imag * std::log(r);

        if (j == 0 && k == 0 && exponent.j == 0 && exponent.k == 0) {
            return Quaternion(
                new_magnitude * std::cos(new_phase),
                new_magnitude * std::sin(new_phase)
            );
        }
        double imag_magnitude = this->imag_mag();
        theta = (imag_magnitude == 0) ? 0 : std::atan2(imag_magnitude, real);
        new_phase = exponent.real * theta + exponent.imag * std::log(r); 

        double scale = (imag_magnitude == 0) ? 0 : new_magnitude * std::sin(new_phase) / imag_magnitude;

        return Quaternion(
            new_magnitude * std::cos(new_phase),
            scale * imag,
            scale * j,
            scale * k
        );
    }

    // pow
    Quaternion pow(double exponent) const {
        double magnitude = std::pow(this->mag(), exponent);
        if (j == 0 && k == 0 ) {
            double angle = std::atan2(imag, real) * exponent;
            return Quaternion(magnitude * std::cos(angle), magnitude * std::sin(angle));
        } else {
            double angle = std::acos(real / magnitude) * exponent;
            double imag_magnitude = imag_mag();
            double sin_angle = magnitude * std::sin(angle);
        
            return Quaternion(
                magnitude * std::cos(angle),
                sin_angle * (imag / imag_magnitude),
                sin_angle * (j / imag_magnitude),
                sin_angle * (k / imag_magnitude)
            );
        } 
    }

    // root
    Quaternion root(const Quaternion& n1) const {
        return this->pow(1.0/n1);
    }
    
    


    // Sin
    Quaternion sin() const {
        if (j == 0 && k == 0 ) {
            return Quaternion(std::sin(real) * std::cosh(imag), std::cos(real) * std::sinh(imag));
        } else {
            double imag_magnitude = imag_mag();
            double scale = std::cos(real) * std::sinh(imag_magnitude) / imag_magnitude;
            return Quaternion(std::sin(real) * std::cosh(imag_magnitude), imag * scale, j * scale, k * scale);
        }
    }

    // Cos
    Quaternion cos() const {
        if (j == 0 && k == 0 ) {
            return Quaternion(std::cos(real) * std::cosh(imag), -std::sin(real) * std::sinh(imag));
        } else {
            double imag_magnitude = imag_mag();
            double scale = -std::sin(real) * std::sinh(imag_magnitude) / imag_magnitude;
            return Quaternion(std::cos(real) * std::cosh(imag_magnitude), imag * scale, j * scale, k * scale);
        }
    }

    // Tan
    Quaternion tan() const {
        return this->sin() / this->cos();
    }

    // Sinh
    Quaternion sinh() const {
        if (j == 0 && k == 0 ) {
            return Quaternion(std::sinh(real) * std::cos(imag), std::cosh(real) * std::sin(imag));
        } else {
            double imag_magnitude = imag_mag();
            double scale = std::cosh(real) * std::sin(imag_magnitude) / imag_magnitude;
            return Quaternion(
                std::sinh(real) * std::cos(imag_magnitude),
                imag * scale,
                j * scale,
                k * scale
            );
        }
    }
    
    // Cosh
    Quaternion cosh() const {
        if (j == 0 && k == 0 ) {
            return Quaternion(std::cosh(real) * std::cos(imag), std::sinh(real) * std::sin(imag));
        } else {
            double imag_magnitude = imag_mag();
            double scale = std::sinh(real) * std::sin(imag_magnitude) / imag_magnitude;
            return Quaternion(
                std::cosh(real) * std::cos(imag_magnitude),
                imag * scale,
                j * scale,
                k * scale
            );
        }
    }

    // Tanh
    Quaternion tanh() const {
        return this->sinh() / this->cosh(); 
    }

    // Log10
    Quaternion log10() const {
        return log() / 2.302585092994045684;
    }

    // Log nthing
    Quaternion logn(const Quaternion& base) const {
        return this->log() / base.log();
    }

    inline Quaternion maximum(const Quaternion& arg2) const {
        return this->mag() < arg2.mag() ? arg2 : *this;
    }

    inline Quaternion minimum(const Quaternion& arg2) const {
        return this->mag() > arg2.mag() ? arg2 : *this;
    }

    Quaternion round() const {
        return Quaternion(std::round(real), std::round(imag), std::round(j), std::round(k));
    }

    Quaternion circle(const Quaternion radius) const {
        double angle = std::atan2(imag, real);
        double rad = radius.mag();
        return Quaternion(std::cos(angle) * rad, std::sin(angle) * rad);
    }

    Quaternion square(const Quaternion sideLength) const {
        double side = sideLength.mag()/2.0;
        double x_proj = (real > 0) ? side : -side;
        double y_proj = (imag > 0) ? side : -side;
        return Quaternion(x_proj, y_proj);
    }

    Quaternion triangle(const Quaternion sideLength) const {
        double side = sideLength.mag();
        double height = std::sqrt(3) / 2 * side;
        double x_proj = (real > 0) ? side / 2 : -side / 2;
        double y_proj = (imag > 0) ? height / 3 : -height / 3;
        return Quaternion(x_proj, y_proj);
    }

    Quaternion ellipsoid(Quaternion radiusX, Quaternion radiusY) const {
        double angle = std::atan2(imag, real);
        return Quaternion(std::cos(angle) * radiusX.mag(), std::sin(angle) * radiusY.mag());
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


    Quaternion gamma() const {
        const Quaternion z = *this - 1.0;

        Quaternion x(lanczos_coeffs[0], 0.0);
        for (int i = 1; i < 9; ++i) {
            x = x + lanczos_coeffs[i] / (z + Quaternion(static_cast<double>(i), 0.0));
        }

        const Quaternion t = z + 7.0 + 0.5;

        return (Quaternion(2.0 * pi, 0.0).sqrt()) * t.pow(z + 0.5) * Quaternion(e, 0.0).pow(-t) * x;
    }


    Quaternion zeta() const {
        Quaternion sum(0.0, 0.0);
        const int N = 120;
        const double threshold = 1e-9;
        
        for (int n = 1; n <= N; ++n) {
            Quaternion term = Quaternion(n, 0.0).pow(-(*this));
            sum = sum + term;

            if (std::abs(term.real) < threshold && std::abs(term.imag) < threshold) break;
        }
        return sum;
    }

    // probably broken, will have to fix/rewrite
    Quaternion airy() const {
        const Quaternion z = *this;
        const int steps = 1000;
        const double upper_limit = 100.0;
        Quaternion sum(0.0, 0.0);
        const double h = upper_limit / steps;

        for (int i = 0; i < steps; ++i) {
            const double t1 = i * h;
            const double t2 = (i + 1) * h;

            const Quaternion f1 = Quaternion(std::pow(t1, 3) / 3.0 + z.real * t1, z.imag * t1).cos();
            const Quaternion f2 = Quaternion(std::pow(t2, 3) / 3.0 + z.real * t2, z.imag * t2).cos();

            sum = sum + 0.5 * (f1 + f2) * h;
        }
        return sum / pi;
    }



    friend std::ostream& operator<<(std::ostream& os, const Quaternion& c) {
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

