#ifndef HYPERCOMPLEX_H
#define HYPERCOMPLEX_H




#include <cstdio>
#include "math_wrapper.h"

#ifndef USE_CUDA
    #include <random>
    namespace ZetaTables {
        static const DefaultType Weights[33] = {
            9.999999998835846782e-01, -9.999999960418790579e-01, 9.999999345745891333e-01,
            -9.999992994125932455e-01, 9.999945356976240873e-01, -9.999669061508029699e-01,
            9.998379682656377554e-01, -9.993406364228576422e-01, 9.977243079338222742e-01,
            -9.932345065753906965e-01, 9.824589833151549101e-01, -9.599283437710255384e-01,
            9.186221712734550238e-01, -8.518968157004565001e-01, 7.565748791676014662e-01,
            -6.358337595593184233e-01, 5.000000000000000000e-01, -3.641662404406815767e-01,
            2.434251208323985338e-01, -1.481031842995434999e-01, 8.137782872654497623e-02,
            -4.007165622897446156e-02, 1.754101668484508991e-02, -6.765493424609303474e-03,
            2.275692066177725792e-03, -6.593635771423578262e-04, 1.620317343622446060e-04,
            -3.309384919703006744e-05, 5.464302375912666321e-06, -7.005874067544937134e-07,
            6.542541086673736572e-08, -3.958120942115783691e-09, 1.164153218269348145e-10
        };

        static const DefaultType LogK[33] = {
            0.0, 0.693147180559945286, 1.098612288668109782, 1.386294361119890572, 
            1.609437912434100282, 1.791759469228054957, 1.945910149055313232, 2.079441541679835748, 
            2.197224577336219564, 2.302585092994045901, 2.397895272798370669, 2.484906649788000355, 
            2.564949357461536739, 2.639057329615258407, 2.708050201102210064, 2.772588722239781145, 
            2.833213344056216165, 2.890371757896164517, 2.944438979166440262, 2.995732273553990854, 
            3.044522437723423014, 3.091042453358316067, 3.135494215929149675, 3.178053830347945752, 
            3.218875824868200564, 3.258096538021482136, 3.295836866004329124, 3.332204510175203804, 
            3.367295829986474143, 3.401197381662155461, 3.433987204485146272, 3.465735902799726542, 
            3.496507561466480229
        };

        static const DefaultType LN2 = 0.6931471805599452862;
        static const DefaultType LNPI = 1.1447298858494001741;
    }
#endif


static constexpr DefaultType EPS = sizeof(DefaultType) == sizeof(float)
    ? 1e-6f
    : 1e-12;

template <int Dim>
class Hypercomplex {
private:
    HOST_DEVICE static inline DefaultType noNan(const DefaultType v) {
        return (my_abs(v) < Max_flt) ? v : copysign(Max_flt, v);
    }

    HOST_DEVICE static inline DefaultType signDefaultType(const DefaultType value) {
        return (value > 0) - (value < 0);
    }
    HOST_DEVICE static inline DefaultType my_fmod(const DefaultType a, const DefaultType b) {
        DefaultType abs_b = my_abs(b);
        DefaultType abs_a = my_abs(a);

        if (!(abs_b < Max_flt) || !(abs_a < Max_flt) || abs_b < EPS)
            return 0.0;

        DefaultType result = a - my_floor(a / b) * b;

        // Optional cleanup for near-zero residue
        if (my_abs(result) < EPS) return 0.0;

        return (b > 0)
            ? (result < 0 ? result + abs_b : result)
            : (result > 0 ? result - abs_b : result);
    }

    HOST_DEVICE static inline DefaultType combineDistances(const DefaultType baseDist, const Hypercomplex& q) {
        DefaultType totalMag = 0.0;
        mag(totalMag, q);
        DefaultType base2dMag = my_sqrt(q.v[0] * q.v[0] + q.v[1] * q.v[0]);
        DefaultType extraSq = (totalMag * totalMag) - (base2dMag * base2dMag);
        DefaultType extraDist = (extraSq > 0) ? my_sqrt(extraSq) : 0;

        return my_sqrt(baseDist * baseDist + extraDist * extraDist);
    }

public:
    DefaultType v[Dim];

    HOST_DEVICE inline Hypercomplex() {
        for(int i = 0; i < Dim; ++i) v[i] = 0.0;
    }

    HOST_DEVICE inline Hypercomplex(const DefaultType* values) {
        for(int i = 0; i < Dim; ++i) v[i] = noNan(values[i]);
    }
    HOST_DEVICE Hypercomplex(const Hypercomplex&) = default;
    HOST_DEVICE Hypercomplex(DefaultType r, DefaultType i=0.0, DefaultType j=0.0, DefaultType k=0.0, DefaultType l=0.0, DefaultType m=0.0, DefaultType n=0.0, DefaultType o=0.0) {
        v[0] = noNan(r);
        if (Dim >= 2) v[1] = noNan(i);
        if (Dim >= 4) v[2] = noNan(j), v[3] = noNan(k);
        if (Dim >= 8) v[4] = noNan(l), v[5] = noNan(m), v[6] = noNan(n), v[7] = noNan(o);
    }

    HOST_DEVICE inline void set_raw(const DefaultType* values) {
        for(int i = 0; i < Dim; ++i) v[i] = values[i];
    }
    HOST_DEVICE inline void set_raw_r(const DefaultType value) {
        for(int i = 1; i < Dim; ++i) v[i] = 0.0;
        v[0]=value;
    }

    HOST_DEVICE inline Hypercomplex& operator=(const Hypercomplex& other) {
        for(int i = 0; i < Dim; ++i) v[i] = other.v[i];
        return *this;
    }

    HOST_DEVICE inline Hypercomplex& operator=(const DefaultType* values) {
        for(int i = 0; i < Dim; ++i) v[i] = noNan(values[i]);
        return *this;
    }

    HOST_DEVICE inline Hypercomplex& operator=(const DefaultType& value) {
        v[0] = noNan(value);
        for(int i = 1; i < Dim; ++i) v[i] = 0.0;
        return *this;
    }
        // Instance methods for convenience
    HOST_DEVICE inline Hypercomplex operator+(const Hypercomplex& other) const {
        Hypercomplex res;
        for(int i = 0; i < Dim; ++i) res.v[i] = noNan(v[i] + other.v[i]);
        return res;
    }

    HOST_DEVICE inline Hypercomplex operator-(const Hypercomplex& other) const {
        Hypercomplex res;
        for(int i = 0; i < Dim; ++i) res.v[i] = noNan(v[i] - other.v[i]);
        return res;
    }

    HOST_DEVICE inline Hypercomplex operator-(DefaultType value) const {
        Hypercomplex res = *this;
        res.v[0] = noNan(res.v[0] - value);
        return res;
    }

    HOST_DEVICE inline Hypercomplex operator-() const {
        Hypercomplex res;
        for(int i = 0; i < Dim; ++i) res.v[i] = -v[i];
        return res;
    }

    HOST_DEVICE inline void operator*=(const Hypercomplex& other) {
        mul(*this, *this, other);
    }

    HOST_DEVICE inline Hypercomplex operator*(const Hypercomplex& other) const {
        Hypercomplex res;
        mul(res, *this, other);
        return res;
    }
    
    HOST_DEVICE inline bool operator==(const Hypercomplex& other) const {
        const DefaultType eps = 5e-5;
        for(int i = 0; i < Dim; ++i) if (my_abs(v[i] - other.v[i]) > eps) return false;
        return true; 
    }

    HOST_DEVICE Hypercomplex operator%(const Hypercomplex& other) const {
        Hypercomplex res;
        for(int i = 0; i < Dim; ++i) res.v[i] = my_fmod(v[i], other.v[i]);
        return res; 
    }

    HOST_DEVICE inline bool operator<(const Hypercomplex& other) const {
        DefaultType ma, mb;
        mag(ma, *this);
        mag(mb, other);
        return ma < mb;
    }

    HOST_DEVICE inline bool operator>(const Hypercomplex& other) const {
        DefaultType ma, mb;
        mag(ma, *this);
        mag(mb, other);
        return ma > mb;
    }








    HOST_DEVICE static inline void sum(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        for(int i = 0; i < Dim; ++i) out.v[i] = noNan(a.v[i] + b.v[i]);
    }
    HOST_DEVICE static inline void sub(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        for(int i = 0; i < Dim; ++i) out.v[i] = noNan(a.v[i] - b.v[i]);
    }

    HOST_DEVICE static inline void inv(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType m2 = 0;
        mag(m2, a);

        // Handle near-zero (avoid explosion)
        if (m2 < EPS) {
            for (int i = 0; i < Dim; ++i) out.v[i] = 0;
            return;
        }

        DefaultType inv_m2 = DefaultType(1) / m2;

        // Conjugate divided by |a|^2
        out.v[0] =  a.v[0] * inv_m2;
        for (int i = 1; i < Dim; ++i) {
            out.v[i] = -a.v[i] * inv_m2;
        }
    }
    HOST_DEVICE static inline void div(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        DefaultType denom;
        Hypercomplex res;
        mag_squared(denom, b);
        denom = 1.0 / denom;
        mul(res, a, b.conj());
        for(int i = 0; i < Dim; ++i)
            out.v[i] = noNan(res.v[i] * denom);
    }

    HOST_DEVICE static inline void mul(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        DefaultType res[Dim];

        if constexpr (Dim == 2) {
            res[0] = a.v[0] * b.v[0] - a.v[1] * b.v[1];
            res[1] = a.v[0] * b.v[1] + a.v[1] * b.v[0];
        }
        else if constexpr (Dim == 4) {
            const DefaultType r = a.v[0], i = a.v[1], j = a.v[2], k = a.v[3];
            const DefaultType or_ = b.v[0], oi = b.v[1], oj = b.v[2], ok = b.v[3];
            res[0] = r * or_ - i * oi - j * oj - k * ok;
            res[1] = r * oi + i * or_ + j * ok - k * oj;
            res[2] = r * oj - i * ok + j * or_ + k * oi;
            res[3] = r * ok + i * oj - j * oi + k * or_;
        }
        else if constexpr (Dim == 8) {
            // Cayley-Dickson: (A,B)(C,D) = (AC - conj(D)B, DA + B conj(C))
            const DefaultType ar = a.v[0], ai = a.v[1], aj = a.v[2], ak = a.v[3];
            const DefaultType br = a.v[4], bi = a.v[5], bj = a.v[6], bk = a.v[7];
            const DefaultType cr = b.v[0], ci = b.v[1], cj = b.v[2], ck = b.v[3];
            const DefaultType dr = b.v[4], di = b.v[5], dj = b.v[6], dk = b.v[7];

            // AC (quaternion multiplication)
            const DefaultType acr = ar * cr - ai * ci - aj * cj - ak * ck;
            const DefaultType aci = ar * ci + ai * cr + aj * ck - ak * cj;
            const DefaultType acj = ar * cj - ai * ck + aj * cr + ak * ci;
            const DefaultType ack = ar * ck + ai * cj - aj * ci + ak * cr;

            // conj(D) B
            const DefaultType dbr = dr * br + di * bi + dj * bj + dk * bk;
            const DefaultType dbi = dr * bi - di * br - dj * bk + dk * bj;
            const DefaultType dbj = dr * bj + di * bk - dj * br - dk * bi;
            const DefaultType dbk = dr * bk - di * bj + dj * bi - dk * br;

            // first half = AC - conj(D)B
            res[0] = acr - dbr;
            res[1] = aci - dbi;
            res[2] = acj - dbj;
            res[3] = ack - dbk;

            // D A
            const DefaultType dar = dr * ar - di * ai - dj * aj - dk * ak;
            const DefaultType dai = dr * ai + di * ar + dj * ak - dk * aj;
            const DefaultType daj = dr * aj - di * ak + dj * ar + dk * ai;
            const DefaultType dak = dr * ak + di * aj - dj * ai + dk * ar;

            // B conj(C)
            const DefaultType bcr = br * cr + bi * ci + bj * cj + bk * ck;
            const DefaultType bci = -br * ci + bi * cr - bj * ck + bk * cj;
            const DefaultType bcj = -br * cj + bi * ck + bj * cr - bk * ci;
            const DefaultType bck = -br * ck - bi * cj + bj * ci + bk * cr;

            // second half = D A + B conj(C)
            res[4] = dar + bcr;
            res[5] = dai + bci;
            res[6] = daj + bcj;
            res[7] = dak + bck;
        }

        for(int i = 0; i < Dim; ++i)
            out.v[i] = noNan(res[i]);
    }

    HOST_DEVICE static inline void mul_sum(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b, const Hypercomplex& c) {
        Hypercomplex tmp;
        mul(tmp, a, b);
        for(int i = 0; i < Dim; ++i)
            out.v[i] = noNan(tmp.v[i] + c.v[i]);
    }









    HOST_DEVICE static inline void rotate_in_circle(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& angle, const Hypercomplex& axis) {
        DefaultType angle_in_radians = angle.v[0];
        const DefaultType cos_angle = my_cos(angle_in_radians);
        const DefaultType sin_angle = my_sin(angle_in_radians);

        // Copy input to output first (preserves components not being rotated)
        out = a;

        int idx_i = -1;
        int idx_j = -1;

        DefaultType axis_m;
        mag(axis_m, axis);
        int selector = static_cast<int>(axis_m);

        switch (selector) {
            case 0:  idx_i = 0; idx_j = 1; break; // (real, imag)
            case 1:  idx_i = 0; idx_j = 2; break; // (real, j)
            case 2:  idx_i = 0; idx_j = 3; break; // (real, k)
            case 3:  idx_i = 0; idx_j = 4; break; // (real, l)
            case 4:  idx_i = 0; idx_j = 5; break; // (real, m)
            case 5:  idx_i = 0; idx_j = 6; break; // (real, n)
            case 6:  idx_i = 0; idx_j = 7; break; // (real, o)
            case 7:  idx_i = 1; idx_j = 2; break; // (imag, j)
            case 8:  idx_i = 1; idx_j = 3; break; // (imag, k)
            case 9:  idx_i = 1; idx_j = 4; break; // (imag, l)
            case 10: idx_i = 1; idx_j = 5; break; // (imag, m)
            case 11: idx_i = 1; idx_j = 6; break; // (imag, n)
            case 12: idx_i = 1; idx_j = 7; break; // (imag, o)
            case 13: idx_i = 2; idx_j = 3; break; // (j, k)
            case 14: idx_i = 2; idx_j = 4; break; // (j, l)
            case 15: idx_i = 2; idx_j = 5; break; // (j, m)
            case 16: idx_i = 2; idx_j = 6; break; // (j, n)
            case 17: idx_i = 2; idx_j = 7; break; // (j, o)
            case 18: idx_i = 3; idx_j = 4; break; // (k, l)
            case 19: idx_i = 3; idx_j = 5; break; // (k, m)
            case 20: idx_i = 3; idx_j = 6; break; // (k, n)
            case 21: idx_i = 3; idx_j = 7; break; // (k, o)
            case 22: idx_i = 4; idx_j = 5; break; // (l, m)
            case 23: idx_i = 4; idx_j = 6; break; // (l, n)
            case 24: idx_i = 4; idx_j = 7; break; // (l, o)
            case 25: idx_i = 5; idx_j = 6; break; // (m, n)
            case 26: idx_i = 5; idx_j = 7; break; // (m, o)
            case 27: idx_i = 6; idx_j = 7; break; // (n, o)
            default: break;
        }

        // Apply rotation logic using the input 'a' and writing to 'out'
        if (idx_i >= 0 && idx_j < Dim) {
            out.v[idx_i] = noNan(a.v[idx_i] * cos_angle - a.v[idx_j] * sin_angle);
            out.v[idx_j] = noNan(a.v[idx_i] * sin_angle + a.v[idx_j] * cos_angle);
        }
    }

    HOST_DEVICE static inline void rotation(Hypercomplex& out, const Hypercomplex& input, const Hypercomplex& angle, const Hypercomplex& axis) {
        DefaultType angle_d = angle.v[0];
        
        // Handle Dim=2 (Complex) separately as the sandwich product is identity in commutative fields
        if constexpr (Dim == 2) {
            // Standard 2D rotation: out = input * exp(i * angle)
            DefaultType c = my_cos(angle_d);
            DefaultType s = my_sin(angle_d);
            Hypercomplex rotor;
            rotor.v[0] = c;
            rotor.v[1] = s;
            mul(out, input, rotor);
            return;
        }

        // For Dim 4 and 8: Use the Sandwich Product (R * input * conj(R))
        DefaultType half_angle_sin = my_sin(angle_d / 2.0);
        DefaultType half_angle_cos = my_cos(angle_d / 2.0);

        // 1. Calculate magnitude of the vector (imaginary) part of the axis
        DefaultType vec_mag = 0.0;
        Hypercomplex axis_ = axis;
        imag_mag(vec_mag, axis_);
        // 2. Construct the Rotor (Quaternion/Octonion style)
        Hypercomplex rotor;
        rotor.v[0] = half_angle_cos;
        
        if (vec_mag > EPS) {
            DefaultType factor = half_angle_sin / vec_mag;
            for (int i = 1; i < Dim; ++i) {
                rotor.v[i] = axis_.v[i] * factor;
            }
        } else {
            // If axis is null, return input unchanged
            out = input;
            return;
        }

        // 3. Apply the sandwich: out = (rotor * input) * conj(rotor)
        Hypercomplex intermediate;
        mul(intermediate, rotor, input);
        mul(out, intermediate, rotor.conj());
    }





    


    HOST_DEVICE static inline void sign(Hypercomplex& out, const Hypercomplex& other) {
        for (int i = 0; i < Dim; ++i) {
            out.v[i] = signDefaultType(other.v[i]);
        }
    }
    
    HOST_DEVICE static inline void abs(Hypercomplex& out, const Hypercomplex& other) {
        for (int i = 0; i < Dim; ++i) {
            out.v[i] = my_abs(other.v[i]);
        }
    }

    HOST_DEVICE static inline void round(Hypercomplex& out, const Hypercomplex& other) {
        for (int i = 0; i < Dim; ++i) {
            out.v[i] = my_round(other.v[i]);
        }
    }
    
    HOST_DEVICE static inline void dot(DefaultType& out, const Hypercomplex& a, const Hypercomplex& b) {
        DefaultType sum_ = 0.0;
    
        for (int i = 0; i < Dim; ++i) {
            sum_ += a.v[i] * b.v[i];
        }
    
        out = noNan(sum_);
    }
    
    HOST_DEVICE static inline void hc_dot(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        for (int i = 0; i < Dim; ++i) {
            out.v[i] = noNan(a.v[i] * b.v[i]);
        }
    }

    HOST_DEVICE static inline void imag_mag(DefaultType& out, const Hypercomplex& other) {
        DefaultType sum = 0.0;
    
        for (int i = 1; i < Dim; ++i) {
            sum += other.v[i] * other.v[i];
        }
    
        out = noNan(my_sqrt(sum));
    }
    
    HOST_DEVICE static inline void mag_squared(DefaultType& out, const Hypercomplex& other) {
        DefaultType sum = 0.0;
        for(int i = 0; i < Dim; ++i) sum += other.v[i] * other.v[i];
        out = noNan(sum);
    }

    HOST_DEVICE static inline void mag(DefaultType& out, const Hypercomplex& other) {
        DefaultType sum = 0.0;
        mag_squared(sum, other);
        out = my_sqrt(sum);
    }

    HOST_DEVICE static inline void hc_mag(Hypercomplex& out, const Hypercomplex& other) {
        out = Hypercomplex();
        mag(out.v[0], other);
    }

    HOST_DEVICE static inline void get_imag(Hypercomplex& out, const Hypercomplex& other) {
        for(int i = 1; i < Dim; ++i) out.v[i] = other.v[i];
        out.v[0] = 0.0;
    }

    HOST_DEVICE inline Hypercomplex conj() const {
        Hypercomplex res;
        res.v[0] = v[0];
        for(int i = 1; i < Dim; ++i) res.v[i] = -v[i];
        return res;
    }

    HOST_DEVICE static inline void arg(DefaultType& out, const Hypercomplex& a) {
        DefaultType img_mag = 0.0;
        imag_mag(img_mag, a);

        out = (img_mag == 0.0)
            ? 0.0
            : my_atan2(img_mag, a.v[0]);
    }

    HOST_DEVICE static inline void hc_arg(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType ar = 0.0;
        arg(ar,a);
        out = Hypercomplex(ar);
    }

    HOST_DEVICE inline bool isZero() const {
        for (int i = 0; i < Dim; ++i) {
            if (my_abs(v[i]) > EPS)
                return false;
        }
        return true;
    }
    HOST_DEVICE inline bool isReal() const {
        for (int i = 1; i < Dim; ++i) {
            if (my_abs(v[i]) > EPS)
                return false;
        }
        return true;
    }

    HOST_DEVICE static inline void maximum(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        DefaultType ma, mb;
    
        mag(ma, a);
        mag(mb, b);
    
        out = (ma < mb) ? b : a;
    }
    
    HOST_DEVICE static inline void minimum(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        DefaultType ma, mb;
    
        mag(ma, a);
        mag(mb, b);
    
        out = (ma > mb) ? b : a;
    }

    HOST_DEVICE static inline void cosSim(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        DefaultType ma, mb, d = 0.0;
        mag(ma, a);
        mag(mb, b);

        if (ma < EPS || mb < EPS) {
            out = 0.0;
            return;
        }

        dot(d,a,b);
        out.set_raw_r(noNan(d / (ma * mb)));
    }

    HOST_DEVICE static inline void lerp(
        Hypercomplex& out,
        const Hypercomplex& a,
        const Hypercomplex& b,
        const Hypercomplex& t_q
    ) {
        DefaultType t;
        mag(t, t_q);
    
        // shortest path
        DefaultType d;
        dot(d, a, b);
    
        DefaultType sign = (d < 0.0) ? -1.0 : 1.0;
    
        for (int i = 0; i < Dim; ++i) {
            out.v[i] =
                a.v[i] +
                t * ((b.v[i] * sign) - a.v[i]);
        }
    }










    

    HOST_DEVICE static inline void sqrt(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType m, mi;
        mag(m, a);       
        imag_mag(mi, a); 

        // Handle zero case
        if (m < EPS) {
            for (int i = 0; i < Dim; ++i) out.v[i] = 0.0;
            return;
        }

        // Half-angle components
        DefaultType s_real = my_sqrt((m + a.v[0]) * 0.5);
        DefaultType s_imag = my_sqrt((m - a.v[0]) * 0.5);

        // 1. Assign real part
        out.v[0] = noNan(s_real);

        // 2. Assign imaginary parts
        if (mi > EPS) {
            // General case: distribute s_imag across existing vector direction
            DefaultType factor = s_imag / mi;
            for (int i = 1; i < Dim; ++i) 
                out.v[i] = noNan(a.v[i] * factor);
        } else {
            // Pure real case
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
            
            if (a.v[0] < 0.0) {
                out.v[1] = s_imag; 
            }
        }
    }

    HOST_DEVICE static inline void exp(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType mi;
        imag_mag(mi, a);
        DefaultType e_real = my_exp(a.v[0]);

        if (mi < EPS) {
            out.v[0] = noNan(e_real);
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
        } else {
            out.v[0] = noNan(e_real * my_cos(mi));
            DefaultType factor = (e_real * my_sin(mi)) / mi;
            for (int i = 1; i < Dim; ++i) 
                out.v[i] = noNan(a.v[i] * factor);
        }
    }

    HOST_DEVICE static inline void log(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType m, mi;
        mag(m, a);
        imag_mag(mi, a);

        if (m < EPS) {
            out.v[0] = -Max_flt; // Approx 
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
            return;
        }

        out.v[0] = noNan(my_log(m));

        if (mi < EPS) {
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
            // Branch cut: log of negative real has an imaginary part of pi*i
            if (a.v[0] < 0.0) out.v[1] = 3.14159265358979323846; 
        } else {
            DefaultType factor = my_atan2(mi, a.v[0]) / mi;
            for (int i = 1; i < Dim; ++i)
                out.v[i] = noNan(a.v[i] * factor);
        }
    }

    HOST_DEVICE static inline void logn(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        Hypercomplex temp, temp1;
        log(temp, a);
        log(temp1, b);
        div(out, temp, temp1);
    }

    HOST_DEVICE static inline void pow(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        // Check if base is zero
        DefaultType m;
        mag(m, a);
        if (m < EPS) {
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
            out.v[0]= b.isZero() ? 1.0 : 0.0;
            return;
        }

        Hypercomplex temp_log, temp_mul;
        log(temp_log, a);
        mul(temp_mul, temp_log, b);
        exp(out, temp_mul);
    }

    HOST_DEVICE static inline void root(Hypercomplex& out, const Hypercomplex& a, const Hypercomplex& b) {
        DefaultType m;
        mag(m, b);
        if (m < EPS) {
            out.v[0] = a.isZero() ? 0.0 : Max_flt;
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
            return;
        }

        Hypercomplex inv_b;
        inv(inv_b, b); 
        pow(out, a, inv_b);
    }

    HOST_DEVICE static inline void sin(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType mi;
        imag_mag(mi, a);
        out.v[0] = noNan(my_sin(a.v[0]) * my_cosh(mi));

        if (mi < EPS) {
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
        } else {
            DefaultType scale = my_cos(a.v[0]) * (my_sinh(mi) / mi);
            for (int i = 1; i < Dim; ++i) out.v[i] = noNan(a.v[i] * scale);
        }
    }

    HOST_DEVICE static inline void cos(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType mi;
        imag_mag(mi, a);
        out.v[0] = noNan(my_cos(a.v[0]) * my_cosh(mi));

        if (mi < EPS) {
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
        } else {
            DefaultType scale = -my_sin(a.v[0]) * (my_sinh(mi) / mi);
            for (int i = 1; i < Dim; ++i) out.v[i] = noNan(a.v[i] * scale);
        }
    }

    HOST_DEVICE static inline void tan(Hypercomplex& out, const Hypercomplex& a) {
        Hypercomplex s, c;
        sin(s, a); 
        cos(c, a);
        div(out, s, c);
    }

    HOST_DEVICE static inline void sinh(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType mi;
        imag_mag(mi, a);
        out.v[0] = noNan(my_sinh(a.v[0]) * my_cos(mi));

        if (mi < EPS) {
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
        } else {
            DefaultType scale = my_cosh(a.v[0]) * (my_sin(mi) / mi);
            for (int i = 1; i < Dim; ++i) out.v[i] = noNan(a.v[i] * scale);
        }
    }

    HOST_DEVICE static inline void cosh(Hypercomplex& out, const Hypercomplex& a) {
        DefaultType mi;
        imag_mag(mi, a);
        out.v[0] = noNan(my_cosh(a.v[0]) * my_cos(mi));

        if (mi < EPS) {
            for (int i = 1; i < Dim; ++i) out.v[i] = 0.0;
        } else {
            DefaultType scale = my_sinh(a.v[0]) * (my_sin(mi) / mi);
            for (int i = 1; i < Dim; ++i) out.v[i] = noNan(a.v[i] * scale);
        }
    }

    HOST_DEVICE static inline void tanh(Hypercomplex& out, const Hypercomplex& a) {
        Hypercomplex s, c;
        sinh(s, a); 
        cosh(c, a);
        div(out, s, c);
    }












    HOST_DEVICE static inline void gamma(Hypercomplex& out, const Hypercomplex& a) {
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
    
        Hypercomplex z, x, c, tmp;
    
        // z = a - 1
        sub(z, a, Hypercomplex(1.0));
    
        // x = coeff[0]
        x = Hypercomplex(lanczos_coeffs[0]);
    
        for (int i = 1; i < 9; ++i) {
    
            // c = z + i
            sum(c, z, Hypercomplex(static_cast<DefaultType>(i)));
    
            // tmp = coeff[i] / c
            div(tmp, Hypercomplex(lanczos_coeffs[i]), c);
    
            // x += tmp
            sum(x, x, tmp);
        }
    
        // c = z + 7.5
        sum(c, z, Hypercomplex(7.5));
    
        // z = c^(z + 0.5)
        sum(tmp, z, Hypercomplex(0.5));
        pow(z, c, tmp);
    
        // z *= sqrt(2*pi)
        mul(z, Hypercomplex(2.50662827463100050241), z);
    
        // tmp = e^(-c)
        exp(tmp, -c);
    
        // z *= e^(-c)
        mul(z, z, tmp);
    
        // out = z * x
        mul(out, z, x);
    }




    #ifndef USE_CUDA
    // High-precision precomputed constants for Riemann Zeta (N=32)
    HOST_DEVICE static inline void zeta_series_cpu(Hypercomplex& out, const Hypercomplex& s) {
        Hypercomplex eta_sum(0.0);
        Hypercomplex term, denom, tmp;
    
        // k=0 is always Weights[0] because (1)^-s = 1
        eta_sum = Hypercomplex(ZetaTables::Weights[0]);
    
        for (int k = 1; k <= 32; ++k) {
    
            // term = exp(-s * ln(k+1))
            mul(tmp, s, Hypercomplex(ZetaTables::LogK[k]));
            exp(term, -tmp);
    
            // eta_sum += term * weight[k]
            mul(tmp, term, Hypercomplex(ZetaTables::Weights[k]));
            sum(eta_sum, eta_sum, tmp);
        }
    
        // denom = 1 - 2^(1-s)
        sub(tmp, Hypercomplex(1.0), s);
        mul(tmp, tmp, Hypercomplex(ZetaTables::LN2));
    
        exp(denom, tmp);
        sub(denom, Hypercomplex(1.0), denom);
    
        DefaultType m;
        mag(m, denom);
    
        if (m < EPS) {
            out.set_raw_r(Max_flt);
        } else {
            div(out, eta_sum, denom);
        }
    }
    #else
    HOST_DEVICE static inline void zeta_series(Hypercomplex& out, const Hypercomplex& s) {
        const int N = 32;
    
        Hypercomplex eta_sum(0.0);
        Hypercomplex term, tmp;
    
        for (int k = 0; k <= N; ++k) {
    
            // scalar coefficient
            DefaultType wk = 0.0;
            DefaultType binom = 1.0;
    
            DefaultType cur_weight =
                1.0 / (double)(1ULL << (k + 1));
    
            for (int j = k; j <= N; ++j) {
                wk += cur_weight * binom;
    
                binom =
                    binom *
                    (DefaultType)(j + 1) /
                    (DefaultType)(j + 1 - k);
    
                cur_weight *= 0.5;
            }
    
            DefaultType sign =
                (k & 1) ? -1.0 : 1.0;
    
            DefaultType coeff = sign * wk;
    
            // term = exp(-s * ln(k+1))
            mul(tmp, s, Hypercomplex(my_log((DefaultType)(k + 1))));
            exp(term, -tmp);
    
            // accumulate
            mul(tmp, term, Hypercomplex(coeff));
            sum(eta_sum, eta_sum, tmp);
        }
    
        // denom = 1 - 2^(1-s)
        Hypercomplex denom;
    
        sub(tmp, Hypercomplex(1.0), s);
        mul(tmp, tmp, Hypercomplex(0.6931471805599452862));
    
        exp(denom, tmp);
    
        sub(denom, Hypercomplex(1.0), denom);
    
        DefaultType m;
        mag(m, denom);
    
        if (m < EPS) {
            out.set_raw_r(Max_flt);
        } else {
            div(out, eta_sum, denom);
        }
    }
    #endif
    
    
    // Main zeta function – no recursion, minimal stack.
    HOST_DEVICE static inline void zeta(Hypercomplex& out, const Hypercomplex& s) {
    
        const Hypercomplex ONE(1.0);
        const Hypercomplex TWO(2.0);
        const Hypercomplex PI(3.14159265358979323846);
    
        // ----- 1. exact special cases -----
    
        if (s.isReal()) {
    
            DefaultType r = s.v[0];
    
            if (my_abs(r - 1.0) < EPS) {
                out.set_raw_r(Max_flt);
                return;
            }
    
            if (my_abs(r) < EPS) {
                out.set_raw_r(-0.5);
                return;
            }
    
            // trivial zeros at negative even integers
            if (r < 0.0) {
    
                DefaultType half = r * 0.5;
    
                if (my_abs(half - my_round(half)) < EPS) {
                    out.set_raw_r(0.0);
                    return;
                }
            }
        }
    
        // ----- 2. reflection formula for Re(s) < 0.5 -----
    
    #ifndef USE_CUDA
    
        if (s.v[0] < 0.5) {
    
            Hypercomplex one_minus_s;
            sub(one_minus_s, ONE, s);
    
            Hypercomplex zeta_1ms;
            zeta_series_cpu(zeta_1ms, one_minus_s);
    
            Hypercomplex gamma_1ms;
            gamma(gamma_1ms, one_minus_s);
    
            // sin(pi*s/2)
            Hypercomplex sin_term, tmp;
    
            mul(tmp, s, Hypercomplex(0.5));
            mul(tmp, tmp, PI);
    
            sin(sin_term, tmp);
    
            // 2^s
            Hypercomplex two_pow_s;
            mul(tmp, s, Hypercomplex(ZetaTables::LN2));
            exp(two_pow_s, tmp);
    
            // pi^(s-1)
            Hypercomplex pi_pow_sm1;
            sub(tmp, s, ONE);
            mul(tmp, tmp, Hypercomplex(ZetaTables::LNPI));
    
            exp(pi_pow_sm1, tmp);
    
            // out = 2^s * pi^(s-1) * sin(...) * gamma(...) * zeta(...)
            mul(out, two_pow_s, pi_pow_sm1);
            mul(out, out, sin_term);
            mul(out, out, gamma_1ms);
            mul(out, out, zeta_1ms);
    
            return;
        }
    
        // ----- 3. direct Euler series -----
    
        zeta_series_cpu(out, s);
    
    #else
    
        if (s.v[0] < 0.5) {
    
            Hypercomplex one_minus_s;
            sub(one_minus_s, ONE, s);
    
            Hypercomplex zeta_1ms;
            zeta_series(zeta_1ms, one_minus_s);
    
            Hypercomplex gamma_1ms;
            gamma(gamma_1ms, one_minus_s);
    
            // sin(pi*s/2)
            Hypercomplex sin_term, tmp;
    
            mul(tmp, s, Hypercomplex(0.5));
            mul(tmp, tmp, PI);
    
            sin(sin_term, tmp);
    
            // 2^s
            Hypercomplex two_pow_s;
    
            mul(tmp, s,
                Hypercomplex(0.6931471805599452862));
    
            exp(two_pow_s, tmp);
    
            // pi^(s-1)
            Hypercomplex pi_pow_sm1;
    
            sub(tmp, s, ONE);
    
            mul(tmp, tmp,
                Hypercomplex(1.1447298858494001741));
    
            exp(pi_pow_sm1, tmp);
    
            // final multiplication chain
            mul(out, two_pow_s, pi_pow_sm1);
            mul(out, out, sin_term);
            mul(out, out, gamma_1ms);
            mul(out, out, zeta_1ms);
    
            return;
        }
        // ----- 3. direct Euler series -----
        zeta_series(out, s);
    
    #endif
    }
    
    HOST_DEVICE static inline void airy(Hypercomplex& out, const Hypercomplex& input) {
        const int maxTerms = 100;
        Hypercomplex summ(0.0);
    
        // Constants
        const DefaultType A0 = 1.0 / my_pow(3.0, 1.0 / 3.0);
        const DefaultType r  = 1.0 / 9.0;
        const DefaultType gamma13 = my_tgamma(1.0 / 3.0);
        const DefaultType gamma23 = my_tgamma(2.0 / 3.0);
    
        // Q = input * gamma23
        Hypercomplex Q;
        mul(Q, input, Hypercomplex(gamma23));
        // Q3 = input^3
        Hypercomplex Q3;
        pow(Q3, input, Hypercomplex(3.0));
    
        const DefaultType mul_gammas =
            gamma13 * gamma23;
    
        // Iterative variables
        DefaultType fact     = 1.0;
        DefaultType poch23   = 1.0;
        DefaultType poch43   = 1.0;
        DefaultType r_k      = 1.0;
    
        Hypercomplex Q3_k(1.0);
        Hypercomplex term;
        Hypercomplex bracket;
        Hypercomplex temp1;
        Hypercomplex temp2;
    
        for (int k = 0; k < maxTerms; ++k) {
    
            // temp1 = Q * poch23
            mul(temp1, Q, Hypercomplex(poch23));
    
            // temp2 = A0 * gamma13 * poch43
            temp2 = Hypercomplex(
                A0 * gamma13 * poch43
            );
    
            // bracket = temp1 - temp2
            sub(bracket, temp1, temp2);
    
            // temp1 = Q3_k * bracket
            mul(temp1, Q3_k, bracket);
    
            // scale by -(A0 * r_k)
            mul(
                bracket,
                temp1,
                Hypercomplex(-(A0 * r_k))
            );
    
            // divide by denominator
            DefaultType denom =
                fact *
                mul_gammas *
                poch23 *
                poch43;
    
            mul(
                term,
                bracket,
                Hypercomplex(1.0 / denom)
            );
    
            // summ += term
            sum(summ, summ, term);
    
            // Early termination
            DefaultType m;
            mag(m, term);
    
            if (m < EPS)
                break;
    
            // Update iteration variables
            fact *= (k + 1);
            // 2/3 and 4/3
            poch23 *= (0.6666666666666667 + k);
            poch43 *= (1.333333333333333 + k);
    
            r_k *= r;
    
            // Q3_k *= Q3
            mul(Q3_k, Q3_k, Q3);
        }
    
        out = summ;
    }
    

    #ifndef USE_CUDA

    HOST_DEVICE static inline void random(
        Hypercomplex& out,
        const Hypercomplex& minVal,
        const Hypercomplex& maxVal,
        const Hypercomplex& seed)
    {
        unsigned int finalSeed = static_cast<unsigned int>(my_abs(seed.v[0]));

        static thread_local std::mt19937 generator;
        if (finalSeed < 1) {
            static thread_local std::random_device device;
            generator.seed(device());
        } else {
            generator.seed(finalSeed);
        }
        std::uniform_real_distribution<DefaultType> dist;
        for (int i = 0; i < Dim; ++i) {
            DefaultType a = minVal.v[i];
            DefaultType b = maxVal.v[i];

            // Ensure valid range per component
            if (a > b) {
                DefaultType tmp = a;
                a = b;
                b = tmp;
            }

            // Handle degenerate range
            if (my_abs(b - a) < EPS) {
                out.v[i] = a;
                continue;
            }
            dist.param(typename std::uniform_real_distribution<DefaultType>::param_type(a, b));
            out.v[i] = dist(generator);
        }
    }
    #endif
    





    HOST_DEVICE void print() const {
        printf("(");
        // real
        printf("%.6g", v[0]);
        // imaginary parts
        for (int i = 1; i < Dim; ++i) {
            if (v[i] >= 0.0)
                printf(" +%.6g%c", v[i], 'i' + (i - 1));
            else
                printf(" -%.6g%c", -v[i], 'i' + (i - 1));
        }
        printf(")\n");
    }



};



#endif