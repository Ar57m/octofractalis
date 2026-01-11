#ifndef CUSTOM_QUATERNION_H
#define CUSTOM_QUATERNION_H



#include "fun_cuda.h"


#include <iostream>
#include <cmath>
#include <limits>
#include <cstdint>
#include <sstream>

#ifndef USE_CUDA
    #include <random>
#endif


class QuaternionOrOctonion {
private:
    // Constrain angle to the principal branch [-π, π]
    // HOST_DEVICE DefaultType constrainAngle(DefaultType angle) const {
    //     while (angle > pi) angle -= 2 * pi;
    //     while (angle < -pi) angle += 2 * pi;
    //     return angle;
    // }
    
    HOST_DEVICE inline DefaultType noNan(DefaultType value) const {
        return ( my_abs(value) < Max_flt) ? value : Max_flt;
    }

    HOST_DEVICE inline DefaultType signDefaultType(DefaultType value) const {
        return (value > 0) - (value < 0);
    }

    HOST_DEVICE inline DefaultType combineDistances(DefaultType baseDist, const QuaternionOrOctonion& q) const {
        DefaultType totalMag = q.mag();
        DefaultType base2dMag = my_sqrt(q.real * q.real + q.imag * q.imag);
        DefaultType extraSq = (totalMag * totalMag) - (base2dMag * base2dMag);
        DefaultType extraDist = (extraSq > 0) ? my_sqrt(extraSq) : 0;

        return my_sqrt(baseDist * baseDist + extraDist * extraDist);
    }

public:
    DefaultType real;
    DefaultType imag;
    DefaultType j;
    DefaultType k;



    
    HOST_DEVICE inline QuaternionOrOctonion(DefaultType r = 0.0, DefaultType i = 0.0, DefaultType j_ = 0.0, DefaultType k_ = 0.0) {
        real = noNan(r);
        imag = noNan(i);
        j = noNan(j_);
        k = noNan(k_);
    }

    static constexpr DefaultType  e = 2.7182818284590452353602874713526624977572470937000;
    static constexpr DefaultType pi = 3.1415926535897932384626433832795028841971693993751;


    HOST_DEVICE inline QuaternionOrOctonion& sanitize() {
        real = noNan(real);
        imag = noNan(imag);
        j = noNan(j);
        k = noNan(k);
        return *this;
    }

    // Sum
    HOST_DEVICE inline QuaternionOrOctonion operator+(const QuaternionOrOctonion& other) const {
        return QuaternionOrOctonion(real + other.real, imag + other.imag, j + other.j, k + other.k);
    }

    HOST_DEVICE inline QuaternionOrOctonion operator+(DefaultType value) const{
        return QuaternionOrOctonion(real + value, imag, j, k);
    }

    HOST_DEVICE inline QuaternionOrOctonion operator+() const {
        return *this;
    }
    
    HOST_DEVICE inline QuaternionOrOctonion& operator+=(const QuaternionOrOctonion& other) {
        real += other.real;
        imag += other.imag;
        j += other.j;
        k += other.k;
        return sanitize();
    }

    HOST_DEVICE inline QuaternionOrOctonion& operator+=(DefaultType value) {
        real += value;
        return sanitize();
    }
    HOST_DEVICE inline friend QuaternionOrOctonion operator+(DefaultType value, const QuaternionOrOctonion& c) {
        return QuaternionOrOctonion(c.real + value, c.imag, c.j, c.k);
    }

    // Sub
    HOST_DEVICE inline QuaternionOrOctonion operator-(const QuaternionOrOctonion& other) const {
        return QuaternionOrOctonion(real - other.real, imag - other.imag, j - other.j, k - other.k);
    }
    HOST_DEVICE inline QuaternionOrOctonion operator-(DefaultType value) {
        real -= value; 
        return sanitize();
    }

    HOST_DEVICE inline QuaternionOrOctonion operator-() const {
        return QuaternionOrOctonion(-real, -imag, -j, -k);
    }

    HOST_DEVICE inline QuaternionOrOctonion& operator-=(const QuaternionOrOctonion& other) {
        real -= other.real;
        imag -= other.imag;
        j -= other.j;
        k -= other.k;
        return sanitize();
    }

    HOST_DEVICE inline QuaternionOrOctonion& operator-=(DefaultType value) {
        real -= value;
        return sanitize();
    }

    HOST_DEVICE inline friend QuaternionOrOctonion operator-(DefaultType value, const QuaternionOrOctonion& c) {
        return QuaternionOrOctonion(value - c.real, -c.imag, -c.j, -c.k);
    } 

    // Mul
    HOST_DEVICE inline QuaternionOrOctonion operator*(const QuaternionOrOctonion& other) const {
            return QuaternionOrOctonion(
                real * other.real - imag * other.imag - j * other.j - k * other.k,
                real * other.imag + imag * other.real + j * other.k - k * other.j,
                real * other.j - imag * other.k + j * other.real + k * other.imag,
                real * other.k + imag * other.j - j * other.imag + k * other.real
            );
    }

    HOST_DEVICE inline QuaternionOrOctonion operator*(DefaultType scalar) const {
        return QuaternionOrOctonion(real * scalar, imag * scalar, j * scalar, k * scalar);
    }

    HOST_DEVICE QuaternionOrOctonion& operator*=(const QuaternionOrOctonion& other) { 
        DefaultType real0 = real * other.real - imag * other.imag - j * other.j - k * other.k;
        DefaultType imag0 = real * other.imag + imag * other.real + j * other.k - k * other.j;
        DefaultType j0 = real * other.j - imag * other.k + j * other.real + k * other.imag;
        DefaultType k0 = real * other.k + imag * other.j - j * other.imag + k * other.real;
        real = real0;
        imag = imag0;
        j = j0;
        k = k0;
        return sanitize();
    }

    HOST_DEVICE QuaternionOrOctonion& operator*=(DefaultType value) {
        real *= value;
        imag *= value;
        j *= value;
        k *= value;
        return sanitize();
    }

    HOST_DEVICE inline friend QuaternionOrOctonion operator*(DefaultType scalar, const QuaternionOrOctonion& c) {
        return c * scalar;
    }

    // Division
    HOST_DEVICE QuaternionOrOctonion operator/(const QuaternionOrOctonion& other) const {
        DefaultType denom = other.real * other.real + other.imag * other.imag
                        + other.j * other.j + other.k * other.k;
        return ((*this * other.conj() ) / denom).sanitize(); 
    }

    HOST_DEVICE inline QuaternionOrOctonion operator/(DefaultType value) const {
        return QuaternionOrOctonion(real / value, imag / value, j / value, k / value); 
    }
    HOST_DEVICE friend QuaternionOrOctonion operator/(DefaultType value, const QuaternionOrOctonion& c) {
        DefaultType denom = c.real * c.real + c.imag * c.imag + c.j * c.j + c.k * c.k;
        return QuaternionOrOctonion(
            (value * c.real) / denom,
            (-value * c.imag) / denom,
            (-value * c.j) / denom,
            (-value * c.k) / denom
        );
    }


    HOST_DEVICE inline bool operator==(const QuaternionOrOctonion& other) const {
        return this->mseScore(other) < 1e-9;
    }

    HOST_DEVICE inline bool operator<(const QuaternionOrOctonion& other) const {
        return this->mag() < other.mag();
    }

    HOST_DEVICE inline bool operator>(const QuaternionOrOctonion& other) const {
        return this->mag() > other.mag();
    }


    HOST_DEVICE inline DefaultType fmod(const DefaultType a, const DefaultType b) const {
        DefaultType abs_b = my_abs(b);
        DefaultType abs_a = my_abs(a);
        if (!(abs_b < 1e300) || !(abs_b > 1e-300) || !(abs_a < 1e300)) return 0.0;
    
        DefaultType result = a - my_floor(a / b) * b;
        return (b > 0) ? (result < 0 ? result + abs_b : result) : (result > 0 ? result - abs_b : result);
    }
    
    HOST_DEVICE QuaternionOrOctonion operator%(const QuaternionOrOctonion& other) const {
        return QuaternionOrOctonion(
            fmod(real, other.real),
            fmod(imag, other.imag),
            fmod(j, other.j),
            fmod(k, other.k)
        );
    }

    HOST_DEVICE QuaternionOrOctonion& operator=(const DefaultType& value) {
        real = noNan(value);
        imag = 0;
        j = 0;
        k = 0;
        return *this;
    }

    // Conj
    HOST_DEVICE inline QuaternionOrOctonion conj() const {
        return QuaternionOrOctonion(real, -imag, -j, -k);
    }
    


    HOST_DEVICE QuaternionOrOctonion rotate_in_circle(QuaternionOrOctonion angle, QuaternionOrOctonion axis) const {
        DefaultType angle_in_radians = angle.real;
        DefaultType sin_angle = my_sin(angle_in_radians);
        DefaultType cos_angle = my_cos(angle_in_radians);

        // QuaternionOrOctonion components
        DefaultType new_r = real, new_i = imag, new_j = j, new_k = k;

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
        default: // Invalid axis, return unchanged QuaternionOrOctonion
            break;
        }

        return QuaternionOrOctonion(new_r, new_i, new_j, new_k);
    }
    
    HOST_DEVICE QuaternionOrOctonion rotation(const QuaternionOrOctonion angle, const QuaternionOrOctonion& axis) const {
        QuaternionOrOctonion normalized_axis = axis / axis.mag();
    
        DefaultType angle_d = angle.real;
        DefaultType half_angle = my_sin(angle_d / 2.0);
    
        QuaternionOrOctonion rotation_QuaternionOrOctonion(
            my_cos(angle_d / 2.0),
            normalized_axis.imag * half_angle,
            normalized_axis.j * half_angle,
            normalized_axis.k * half_angle
        );
    
        return rotation_QuaternionOrOctonion * (*this) * rotation_QuaternionOrOctonion.conj();
    }
    

    HOST_DEVICE inline QuaternionOrOctonion sign() const {
        return QuaternionOrOctonion(signDefaultType(real),signDefaultType(imag),signDefaultType(j),signDefaultType(k));
    }

    // abs / remove the sign
    HOST_DEVICE inline QuaternionOrOctonion abs() const {
        return QuaternionOrOctonion(my_abs(real), my_abs(imag), my_abs(j), my_abs(k));
    }

    // magnitude
    HOST_DEVICE inline DefaultType mag() const {
        return noNan(my_sqrt(real * real + imag * imag + j * j + k * k)); 
    }
    
    HOST_DEVICE inline DefaultType magSquared() const {
        return noNan(real * real + imag * imag + j * j + k * k); 
    }
    
    HOST_DEVICE inline QuaternionOrOctonion c_mag() const {
        return QuaternionOrOctonion(my_sqrt(real * real + imag * imag + j * j + k * k), 0.0);
    }

    HOST_DEVICE inline DefaultType imag_mag() const {
        return noNan(my_sqrt(imag * imag + j * j + k * k));
    }

    // sqrt
    HOST_DEVICE QuaternionOrOctonion sqrt() const {
        DefaultType magnitude = mag();
        if (j == 0 && k == 0 ) {
            magnitude = my_sqrt(magnitude);
            DefaultType angle = my_atan2(imag, real) / 2.0;
            return QuaternionOrOctonion(magnitude * my_cos(angle), magnitude * my_sin(angle));
        } else {
            magnitude += real;
            DefaultType imag_mag =  my_sqrt(2 * magnitude);
            return QuaternionOrOctonion(my_sqrt( magnitude / 2.0),
                    (imag / imag_mag),
                    (j / imag_mag),
                    (k / imag_mag));
        }
    }

    // arg
    HOST_DEVICE inline DefaultType arg() const {
        DefaultType imagmag = imag_mag();
        return (imagmag == 0.0) ? 0.0 : my_atan2(imagmag, real);
    }

    // log
    HOST_DEVICE QuaternionOrOctonion log() const {
        DefaultType mag = this->mag();
        if (mag == 0) {
            return QuaternionOrOctonion(0); // log(0) is undefined, return 0
        }
    
        DefaultType vecMag = imag_mag();
        if (vecMag == 0) {
            return QuaternionOrOctonion(my_log(mag), real > 0 ? 0 : pi); // Negative real: add π (branch cut)
        }
    
        DefaultType theta = my_atan2(vecMag, real)/vecMag; // This ensures continuity
    
        // Logarithmic form
        return QuaternionOrOctonion(
            my_log(mag),
            theta * imag,
            theta * j,
            theta * k
        );
    }

    //exp
    HOST_DEVICE QuaternionOrOctonion exp() const {
        DefaultType vecMag = imag_mag();
        DefaultType expReal = my_exp(real);
    
        if (vecMag == 0) {
            // Purely real QuaternionOrOctonion
            return QuaternionOrOctonion(expReal);
        }
    
        // Exponential form
        DefaultType cosVecMag = my_cos(vecMag)*expReal;
        expReal *= my_sin(vecMag)/ vecMag;
    
        return QuaternionOrOctonion(
            cosVecMag,
            expReal * imag,
            expReal * j,
            expReal * k
        );
    }
    
    // Q power
    HOST_DEVICE QuaternionOrOctonion pow(const QuaternionOrOctonion& p) const {
        if (this->isZero()) {
            return QuaternionOrOctonion( (p.isZero()) ? 1 : 0 ); // 0^p = 0 for non-zero p
        }
        return (this->log()*p).exp();
    }

    // QuaternionOrOctonion power with a scalar exponent
    HOST_DEVICE QuaternionOrOctonion pow(DefaultType exponent) const {
        if (this->isZero()) {
            return QuaternionOrOctonion( (exponent == 0) ? 1 : 0 ); // 0^exponent = 0 for exponent != 0
        }
        return (this->log() * exponent).exp();
    }
    
    // root
    HOST_DEVICE QuaternionOrOctonion root(const QuaternionOrOctonion& n1) const {
        return this->pow(1.0/n1);
    }



    HOST_DEVICE inline bool isZero() const {
        return real == 0 && imag == 0 && j == 0 && k == 0;
    }

    HOST_DEVICE inline bool isReal() const {
        return (imag == 0 && j == 0 && k == 0);
    }


    HOST_DEVICE DefaultType mseScore(const QuaternionOrOctonion& other) const {
        // Calculate Mean Squared Error (MSE) of the components
        DefaultType mse = noNan( (real - other.real) * (real - other.real)); // Difference in real parts
        mse = noNan( mse + (imag - other.imag) * (imag - other.imag)); // Difference in imag parts
        mse = noNan( mse + (j - other.j) * (j - other.j));       // Difference in j parts
        mse = noNan( mse + (k - other.k) * (k - other.k));       // Difference in k parts

        // Normalize the score to prevent extremely large values
        return mse / 4.0; // Average over the 4 components
    }
    
    HOST_DEVICE DefaultType cosSim(const QuaternionOrOctonion& other) const {
        DefaultType normThis = mag();
        DefaultType normOther = other.mag();
    
        if (normThis == 0 || normOther == 0) {
            return 0.0;
        }
        return (real * other.real + imag * other.imag + j * other.j + k * other.k) / (normThis * normOther);
    }
    




    // Sin
    HOST_DEVICE QuaternionOrOctonion sin() const {
        if (j == 0 && k == 0 ) {
            return QuaternionOrOctonion(my_sin(real) * my_cosh(imag), my_cos(real) * my_sinh(imag));
        } else {
            DefaultType imag_magnitude = imag_mag();
            DefaultType scale = my_cos(real) * my_sinh(imag_magnitude) / imag_magnitude;
            return QuaternionOrOctonion(my_sin(real) * my_cosh(imag_magnitude), imag * scale, j * scale, k * scale);
        }
    }

    // Cos
    HOST_DEVICE QuaternionOrOctonion cos() const {
        if (j == 0 && k == 0 ) {
            return QuaternionOrOctonion(my_cos(real) * my_cosh(imag), -my_sin(real) * my_sinh(imag));
        } else {
            DefaultType imag_magnitude = imag_mag();
            DefaultType scale = -my_sin(real) * noNan(my_sinh(imag_magnitude) / imag_magnitude);
            return QuaternionOrOctonion(my_cos(real) * my_cosh(imag_magnitude), imag * scale, j * scale, k * scale);
        }
    }

    // Tan
    HOST_DEVICE QuaternionOrOctonion tan() const {
        return this->sin() / this->cos();
    }

    // Sinh
    HOST_DEVICE QuaternionOrOctonion sinh() const {
        if (j == 0 && k == 0 ) {
            return QuaternionOrOctonion(my_sinh(real) * my_cos(imag), my_cosh(real) * my_sin(imag));
        } else {
            DefaultType imag_magnitude = imag_mag();
            DefaultType scale = my_cosh(real) * noNan(my_sin(imag_magnitude) / imag_magnitude);
            return QuaternionOrOctonion(
                my_sinh(real) * my_cos(imag_magnitude),
                imag * scale,
                j * scale,
                k * scale
            );
        }
    }
    
    // Cosh
    HOST_DEVICE QuaternionOrOctonion cosh() const {
        if (j == 0 && k == 0 ) {
            return QuaternionOrOctonion(my_cosh(real) * my_cos(imag), my_sinh(real) * my_sin(imag));
        } else {
            DefaultType imag_magnitude = imag_mag();
            DefaultType scale = my_sinh(real) * noNan(my_sin(imag_magnitude) / imag_magnitude);
            return QuaternionOrOctonion(
                my_cosh(real) * my_cos(imag_magnitude),
                imag * scale,
                j * scale,
                k * scale
            );
        }
    }

    // Tanh
    HOST_DEVICE QuaternionOrOctonion tanh() const {
        return this->sinh() / this->cosh(); 
    }

    // Log10
    HOST_DEVICE QuaternionOrOctonion log10() const {
        return log() / 2.302585092994045684;
    }

    // Log nthing
    HOST_DEVICE QuaternionOrOctonion logn(const QuaternionOrOctonion& base) const {
        return this->log() / base.log();
    }



    HOST_DEVICE inline QuaternionOrOctonion maximum(const QuaternionOrOctonion& arg2) const {
        return this->mag() < arg2.mag() ? arg2 : *this;
    }

    HOST_DEVICE inline QuaternionOrOctonion minimum(const QuaternionOrOctonion& arg2) const {
        return this->mag() > arg2.mag() ? arg2 : *this;
    }

    HOST_DEVICE QuaternionOrOctonion round() const {
        return QuaternionOrOctonion(my_round(real), my_round(imag), my_round(j), my_round(k));
    }




    HOST_DEVICE DefaultType dot(const QuaternionOrOctonion& other) const {
        return real * other.real + imag * other.imag + 
               j * other.j + k * other.k;
    }

    HOST_DEVICE QuaternionOrOctonion c_dot(const QuaternionOrOctonion& other) const {
        return QuaternionOrOctonion(
            real * other.real,
            imag * other.imag,
            j * other.j,
            k * other.k
        );
    }



    HOST_DEVICE QuaternionOrOctonion gamma() const {
        static constexpr DefaultType lanczos_coeffs[9] = {
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
        const QuaternionOrOctonion z = *this - 1.0;

        QuaternionOrOctonion x(lanczos_coeffs[0], 0.0);
        for (int i = 1; i < 9; ++i) {
            x += lanczos_coeffs[i] / (z + QuaternionOrOctonion(static_cast<DefaultType>(i), 0.0));
        }

        const QuaternionOrOctonion t = z + 7.0 + 0.5;

        return (QuaternionOrOctonion(2.0 * pi, 0.0).sqrt()) * t.pow(z + 0.5) * QuaternionOrOctonion(e, 0.0).pow(-t) * x;
    }


    HOST_DEVICE QuaternionOrOctonion zeta() const {
    // Handle special cases for real numbers
        const QuaternionOrOctonion this_num = *this;
        const QuaternionOrOctonion one_this = 1.0 - this_num;
        if (isReal()) {
            const DefaultType s = real;
            
            // Zero at negative even integers and at s = 1
            if ((s < 0 && my_abs(my_fmod(s, 2.0)) < 1e-12) || my_abs(s - 1.0) < 1e-12) {
                return QuaternionOrOctonion(0.0);
            }
            
            // Special value at s = 0
            if (my_abs(s) < 1e-12) {
                return QuaternionOrOctonion(-0.5);
            }
        }
    
        // Functional equation for Re(s) < 0
        if (real < 0.0) {
            const QuaternionOrOctonion q_pi(pi);
            const QuaternionOrOctonion two(2.0);
            
            
            // Compute Γ(1 - s)
            const QuaternionOrOctonion gamma_reflected = (one_this).gamma();
            
            // Compute sin(πs/2)
            const QuaternionOrOctonion sin_factor = two.pow(this_num) * q_pi.pow(this_num - 1.0) * (pi * (this_num / 2.0)).sin();
            
            // Compute 2^s and π^(s - 1)
            
            // Compute reflected ζ(1 - s)
            const QuaternionOrOctonion reflected = (one_this).zeta();
            
            // Apply the functional equation
            return sin_factor * gamma_reflected * reflected;
        }
    
        // Main series computation for Re(s) ≥ 0
        const int max_iter = 100;
        const DefaultType threshold = 1e-12;
        QuaternionOrOctonion eta(0.0);
        bool alt_sign = true;  // Alternating series for η
    
        for (int n = 1; n <= max_iter; ++n) {
            QuaternionOrOctonion term = QuaternionOrOctonion(n).pow(-this_num);
            if (!alt_sign) term = term * -1.0;
            eta = eta + term;
            
            if (term.mag() < threshold) break;
            alt_sign = !alt_sign;
        }
    
        // Conversion factor for η → ζ
        const QuaternionOrOctonion factor = (1.0 - QuaternionOrOctonion(2.0).pow(one_this));
    
        // Handle near-singular points
        if (factor.mag() < 1e-12) {
            return QuaternionOrOctonion(0);
        }
    
        return eta / factor;
    } 

    // Seems to be working *thinking* deepseek fixed it
    HOST_DEVICE QuaternionOrOctonion airy() const {
        const int maxTerms = 100;
        QuaternionOrOctonion sum(0.0);
        
        
        
        // Constants
        const DefaultType A0 = 1.0 / my_pow(3.0, 1.0/3.0);  // ~0.693361
        const DefaultType r  = 1.0 / 9.0;                       // ~0.111111
        //const DefaultType B0 = A0;                              // Same as A0
        
        // Gamma constants
        const DefaultType gamma13 = my_tgamma(1.0/3.0);
        const DefaultType gamma23 = my_tgamma(2.0/3.0);
        const QuaternionOrOctonion Q = *this * gamma23;
        QuaternionOrOctonion Q3 = this->pow(3);
        const DefaultType mul_gammas = gamma13 * gamma23;
        
        // Initialize iterative variables:
        DefaultType fact = 1.0;      // k! for k = 0
        DefaultType poch23 = 1.0;    // (2/3)_0 = gamma(2/3)/gamma(2/3)
        DefaultType poch43 = 1.0;    // (4/3)_0 = gamma(4/3)/gamma(4/3)
        DefaultType r_k = 1.0;       // r^0
        QuaternionOrOctonion Q3_k(1.0);  // Q3^0 (the multiplicative identity)
        
        for (int k = 0; k < maxTerms; ++k) {
            // Compute the bracket part of the term //B0
            QuaternionOrOctonion bracket = (Q * poch23) - (A0 * gamma13 * poch43);
            
            // Compute term: -(A0 * r^k) * (Q3^k) * bracket / (k! * gamma13 * gamma23 * poch23 * poch43)
            QuaternionOrOctonion term = -(A0 * r_k) * Q3_k * bracket;
            DefaultType denom = fact * mul_gammas * poch23 * poch43;
            term = term / denom;
            
            sum = sum + term;
            
            // Early termination if the term is sufficiently small
            if (term.mag() < 1e-12)
                break;
            
            // Update iterative variables for next iteration:
            fact *= (k + 1);                   // factorial: (k+1)! = k! * (k+1)
            poch23 *= (2.0/3.0 + k);             // (2/3)_{k+1} = (2/3)_k * (2/3 + k)
            poch43 *= (4.0/3.0 + k);             // (4/3)_{k+1} = (4/3)_k * (4/3 + k)
            r_k *= r;                          // r^(k+1)
            Q3_k = Q3_k * Q3;                  // Q3^(k+1)
        }
        
        return sum;
    }




    

    HOST_DEVICE inline QuaternionOrOctonion lerp(const QuaternionOrOctonion& other, QuaternionOrOctonion t_q) const {
        const DefaultType t = t_q.mag();
        
        DefaultType dot = real * other.real + imag * other.imag +
                          j * other.j + k * other.k;
        QuaternionOrOctonion target = other;
        if(dot < 0.0) {
            target = -other;
        }
        
        QuaternionOrOctonion result;
        result.real = real + t * (target.real - real);
        result.imag = imag + t * (target.imag - imag);
        result.j    = j    + t * (target.j    - j);
        result.k    = k    + t * (target.k    - k);

        
        return result;
    }



    HOST_DEVICE QuaternionOrOctonion circle(const QuaternionOrOctonion& q) const {

        DefaultType r = this->mag();
        DefaultType base2d = my_sqrt(q.real * q.real + q.imag * q.imag);
        DefaultType baseDist = my_abs(base2d - r);

        DefaultType combinedDist = combineDistances(baseDist, q);
        return QuaternionOrOctonion(combinedDist);
    }

    HOST_DEVICE QuaternionOrOctonion rect(const QuaternionOrOctonion& q) const {

        DefaultType dx = my_abs(q.real) - this->real;
        DefaultType dy = my_abs(q.imag) - this->imag;

        DefaultType outsideX = (dx > 0) ? dx : 0;
        DefaultType outsideY = (dy > 0) ? dy : 0;
        DefaultType outsideDist = my_sqrt(outsideX * outsideX + outsideY * outsideY);
        
        DefaultType baseDist = (dx < 0 && dy < 0) ? 0 : outsideDist;

        DefaultType combinedDist = combineDistances(baseDist, q);
        
        return QuaternionOrOctonion(combinedDist);
    }

    HOST_DEVICE QuaternionOrOctonion line( const QuaternionOrOctonion& p1,
                                                const QuaternionOrOctonion& q) const{
        const QuaternionOrOctonion p0 = *this;
        QuaternionOrOctonion v = p1 - p0;
        DefaultType len2 = v.dot( v);
        if (len2 == 0)
            return (q - p0).mag();
        DefaultType t = (q - p0).dot( v) / len2;
        t = (t < 0) ? 0 : ((t > 1) ? 1 : t);
        QuaternionOrOctonion projection = p0 + v * t;
        return QuaternionOrOctonion((q - projection).mag() );
    }

    HOST_DEVICE QuaternionOrOctonion triangle(const QuaternionOrOctonion& q) const {
        const DefaultType s = this->mag();
        const DefaultType ko = my_sqrt(3.0);
        
        DefaultType baseReal = q.real;
        DefaultType baseImag = q.imag;
        
        baseImag += s / (2 * ko);
        
        baseReal = my_abs(baseReal);
        
        baseReal -= s / 2.0;
        baseImag -= s / (2 * ko);
        

        if (baseReal + ko * baseImag > 0) {
            DefaultType tempReal = baseReal - ko * baseImag;
            DefaultType tempImag = -ko * baseReal - baseImag;
            baseReal = tempReal / 2.0;
            baseImag = tempImag / 2.0;
        }
        
        DefaultType baseDist = my_sqrt(baseReal * baseReal + baseImag * baseImag);
        
        DefaultType combinedDist = combineDistances(baseDist, q);
        
        return QuaternionOrOctonion(combinedDist);
    }


    HOST_DEVICE QuaternionOrOctonion ellipsoid(const QuaternionOrOctonion& radiusY,
                                                const QuaternionOrOctonion& q) const {
        DefaultType a = this->mag();
        DefaultType b = radiusY.mag();
        
        DefaultType baseReal = q.real / a;
        DefaultType baseImag = q.imag / b;
        DefaultType k = my_sqrt(baseReal * baseReal + baseImag * baseImag);
        
        DefaultType baseDist = my_abs(k - 1.0) * ((a < b) ? a : b);
        
        DefaultType combinedDist = combineDistances(baseDist, q);
        
        return QuaternionOrOctonion(combinedDist);
    }



    #ifndef USE_CUDA

    QuaternionOrOctonion generateRandom(const QuaternionOrOctonion& max, const QuaternionOrOctonion& seed) const {
        // Use the real part of the seed as the base for generating the random number
        unsigned int finalSeed = static_cast<unsigned int>(my_abs(seed.real));

        // Validate and correct the magnitude range
        DefaultType minMag = mag();
        DefaultType maxMag = max.mag();
        if (minMag > maxMag) {
            my_swap(minMag, maxMag); // Ensure minMag <= maxMag
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

        std::uniform_real_distribution<DefaultType> distribution(minMag, maxMag);
        return QuaternionOrOctonion(distribution(generator));
    }




    
    std::string to_string() const {
        std::ostringstream os;
        os << "(" << real 
           << (imag >= 0.0 ? " +" : " -") << my_abs(imag) << "i"
           << (j >= 0.0 ? " +" : " -") << my_abs(j) << "j"
           << (k >= 0.0 ? " +" : " -") << my_abs(k) << "k)";
        return os.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const QuaternionOrOctonion& c) {
        os << c.to_string();
        return os;
    }

    void print() const {
        std::cout << to_string() << std::endl;
    }
    #endif
};


#endif


