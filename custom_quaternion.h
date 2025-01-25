#ifndef CUSTOM_QUATERNION_H
#define CUSTOM_QUATERNION_H

#include <iostream>
#include <cmath>
#include <limits>
#include <cstdint>
#include <sstream>
#include <random>


class Quaternion {
private:
    // Constrain angle to the principal branch [-π, π]
    double constrainAngle(double angle) const {
        while (angle > pi) angle -= 2 * pi;
        while (angle < -pi) angle += 2 * pi;
        return angle;
    }

    inline double noNan(double value) const {
        return ( std::abs(value) < 1e300) ? value : 0.0;
    }

public:
    double real;
    double imag;
    double j;
    double k;



    
    inline Quaternion(double r = 0.0, double i = 0.0, double j_ = 0.0, double k_ = 0.0) {
        real = noNan(r);
        imag = noNan(i);
        j = noNan(j_);
        k = noNan(k_);
    }

    static constexpr double e = 2.7182818284590452353602874713526624977572470937000;
    static constexpr double pi = 3.1415926535897932384626433832795028841971693993751;


    inline Quaternion& sanitize() {
        real = noNan(real);
        imag = noNan(imag);
        j = noNan(j);
        k = noNan(k);
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

    Quaternion& operator=(const double& value) {
        real = std::abs(value) < 1e300 ? value : 0.0;
        imag = 0;
        j = 0;
        k = 0;
        return *this;
    }

    // Conj
    inline Quaternion conj() const {
        return Quaternion(real, -imag, -j, -k);
    }
    
    Quaternion rotate_in_circle(Quaternion angle, Quaternion axis) const {
        double angle_in_radians = angle.mag(); // Magnitude as angle in radians
        double sin_angle = std::sin(angle_in_radians);
        double cos_angle = std::cos(angle_in_radians);

        // Quaternion components
        double new_r = real, new_i = imag, new_j = j, new_k = k;

        // Identify the rotation plane based on the axis
        switch (static_cast<int>(axis.mag())) {
        case 0: // Rotation in the (real, imag) plane
            new_r = real * cos_angle - imag * sin_angle;
            new_i = real * sin_angle + imag * cos_angle;
            break;
        case 1: // Rotation in the (real, j) plane
            new_r = real * cos_angle - j * sin_angle;
            new_j = real * sin_angle + j * cos_angle;
            break;
        case 2: // Rotation in the (real, k) plane
            new_r = real * cos_angle - k * sin_angle;
            new_k = real * sin_angle + k * cos_angle;
            break;
        case 3: // Rotation in the (imag, j) plane
            new_i = imag * cos_angle - j * sin_angle;
            new_j = imag * sin_angle + j * cos_angle;
            break;
        case 4: // Rotation in the (imag, k) plane
            new_i = imag * cos_angle - k * sin_angle;
            new_k = imag * sin_angle + k * cos_angle;
            break;
        case 5: // Rotation in the (j, k) plane
            new_j = j * cos_angle - k * sin_angle;
            new_k = j * sin_angle + k * cos_angle;
            break;
        default: // Invalid axis, return unchanged quaternion
            break;
        }

        return Quaternion(new_r, new_i, new_j, new_k);
    }

    
    Quaternion rotation(const Quaternion angle, const Quaternion& axis) const {
        Quaternion normalized_axis = axis / axis.mag();
    
        double angle_mag = angle.mag();
        double half_angle = std::sin(angle_mag / 2.0);
    
        Quaternion rotation_quaternion(
            std::cos(angle_mag / 2.0),
            normalized_axis.imag * half_angle,
            normalized_axis.j * half_angle,
            normalized_axis.k * half_angle
        );
    
        return rotation_quaternion * (*this) * rotation_quaternion.conj();
    }
    

    // abs / remove the sign
    inline Quaternion abs() const {
        return Quaternion(std::abs(real), std::abs(imag), std::abs(j), std::abs(k));
    }

    // magnitude
    inline double mag() const {
        return noNan(std::sqrt(real * real + imag * imag + j * j + k * k)); 
    }
    
    inline double magSquared() const {
        return noNan(real * real + imag * imag + j * j + k * k); 
    }
    
    inline Quaternion c_mag() const {
        return Quaternion(std::sqrt(real * real + imag * imag + j * j + k * k), 0.0);
    }

    inline double imag_mag() const {
        return noNan(std::sqrt(imag * imag + j * j + k * k));
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
        double imagmag = imag_mag();
        return (imagmag == 0.0) ? 0.0 : std::atan2(imagmag, real);
    }

    // log
    Quaternion log() const {
        double mag = this->mag();
        if (mag == 0) {
            return Quaternion(0); // log(0) is undefined, return 0
        }
    
        double vecMag = imag_mag();
        if (vecMag == 0) {
            return Quaternion(std::log(mag), real > 0 ? 0 : pi); // Negative real: add π (branch cut)
        }
    
        double theta = std::atan2(vecMag, real)/vecMag; // This ensures continuity
    
        // Logarithmic form
        return Quaternion(
            std::log(mag),
            theta * imag,
            theta * j,
            theta * k
        );
    }

    //exp
    Quaternion exp() const {
        double vecMag = imag_mag();
        double expReal = std::exp(real);
    
        if (vecMag == 0) {
            // Purely real quaternion
            return Quaternion(expReal);
        }
    
        // Exponential form
        double cosVecMag = std::cos(vecMag)*expReal;
        expReal *= std::sin(vecMag)/ vecMag;
    
        return Quaternion(
            cosVecMag,
            expReal * imag,
            expReal * j,
            expReal * k
        );
    }
    
    // Q power
    Quaternion pow(const Quaternion& p) const {
        if (this->isZero()) {
            return Quaternion( (p.isZero()) ? 1 : 0 ); // 0^p = 0 for non-zero p
        }
        return (this->log()*p).exp();
    }

    // Quaternion power with a scalar exponent
    Quaternion pow(double exponent) const {
        if (this->isZero()) {
            return Quaternion( (exponent == 0) ? 1 : 0 ); // 0^exponent = 0 for exponent != 0
        }
        return (this->log() * exponent).exp();
    }
    
    // root
    Quaternion root(const Quaternion& n1) const {
        return this->pow(1.0/n1);
    }

    inline bool isZero() const {
        return real == 0 && imag == 0 && j == 0 && k == 0;
    }

    inline bool isZeroQ() const {
        return j == 0 && k == 0;
    }


    double mseScore(const Quaternion& other) const {
        // Calculate Mean Squared Error (MSE) of the components
        double mse = noNan( (real - other.real) * (real - other.real)); // Difference in real parts
        mse = noNan( mse + (imag - other.imag) * (imag - other.imag)); // Difference in imag parts
        mse = noNan( mse + (j - other.j) * (j - other.j));       // Difference in j parts
        mse = noNan( mse + (k - other.k) * (k - other.k));       // Difference in k parts

        // Normalize the score to prevent extremely large values
        return mse / 4.0; // Average over the 4 components
    }
    
    
    double cosSim(const Quaternion& other) const {
        double normThis = mag();
        double normOther = other.mag();
    
        if (normThis == 0 || normOther == 0) {
            return 0.0;
        }
        return (real * other.real + imag * other.imag + j * other.j + k * other.k) / (normThis * normOther);
    }
    
    
    
    double generateRandom(const Quaternion& max, const Quaternion& seed) const {
        // Use the real part of the seed as the base for generating the random number
        unsigned int finalSeed = static_cast<unsigned int>(seed.real);

        // Validate and correct the magnitude range
        double minMag = mag();
        double maxMag = max.mag();
        if (minMag > maxMag) {
            std::swap(minMag, maxMag); // Ensure minMag <= maxMag
        }

        // Handle edge cases where the range is invalid or zero
        if (minMag == maxMag) {
            return minMag; // No range, return the only possible value
        }

        // Select random seed and initialize random number generator
        std::mt19937 generator;
        if (finalSeed < 1) {
            std::random_device device;
            generator.seed(device()); // Use a non-deterministic seed if `finalSeed` is invalid
        } else {
            generator.seed(finalSeed); // Use the provided seed
        }

        std::uniform_real_distribution<double> distribution(minMag, maxMag);
        return distribution(generator);
    }

    
    void test_quaternion_math() {
        Quaternion q(1, 1, 1, 1);
        Quaternion log_q = q.log();
        Quaternion exp_log_q = log_q.exp();
        std::cout  << ((exp_log_q - q).mag() ) << "\n"; // exp(log(q)) ≈ q
    
        Quaternion rotated = Quaternion(0, 1, 0, 0) * q * Quaternion(0, -1, 0, 0);
        Quaternion log_rotated = rotated.log();
        Quaternion transformed_log = Quaternion(0, 1, 0, 0) * log_q * Quaternion(0, -1, 0, 0);
        std::cout  << ((log_rotated - transformed_log).mag()) << "\n"; // ln(xyx^-1) ≈ x ln(y) x^-1
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
            double scale = -std::sin(real) * noNan(std::sinh(imag_magnitude) / imag_magnitude);
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
            double scale = std::cosh(real) * noNan(std::sin(imag_magnitude) / imag_magnitude);
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
            double scale = std::sinh(real) * noNan(std::sin(imag_magnitude) / imag_magnitude);
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
            x += lanczos_coeffs[i] / (z + Quaternion(static_cast<double>(i), 0.0));
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
    
    std::string to_string() const {
        std::ostringstream os;
        os << "(" << real 
           << (imag >= 0.0 ? " +" : " -") << std::abs(imag) << "i"
           << (j >= 0.0 ? " +" : " -") << std::abs(j) << "j"
           << (k >= 0.0 ? " +" : " -") << std::abs(k) << "k)";
        return os.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const Quaternion& c) {
        os << c.to_string();
        return os;
    }

    void print() const {
        std::cout << to_string() << std::endl;
    }
};


#endif // CUSTOM_COMPLEX_H

