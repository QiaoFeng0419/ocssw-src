
#include <iostream>

#include <boost/units/io.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/io.hpp>

using boost::units::quantity;

template<typename T>
class QuantityPercentage {
    public:
        constexpr QuantityPercentage(const T p_) : p{static_cast<double>(p_) / 100.0}{}
        const double p;

        constexpr inline friend std::ostream& operator<<(std::ostream &out, const QuantityPercentage& tol){return out << tol.p * 100 << "%";}
};
constexpr inline QuantityPercentage<long double> operator"" _percent(const long double tolerance){
    return QuantityPercentage<long double>{tolerance};
}
constexpr inline QuantityPercentage<unsigned long long> operator"" _percent(const unsigned long long tolerance){
    return QuantityPercentage<unsigned long long>{tolerance};
}
template<typename T>
class QuantityTolerance {
    public:
        constexpr QuantityTolerance(const T t_) : t{t_}{}
        const T t;

        constexpr inline friend std::ostream& operator<<(std::ostream &out, const QuantityTolerance& tol){return out << " +/- " << tol.t;}
};

template <typename T>
class ComparableQuantity {
    public:
        constexpr ComparableQuantity(const T q_, const T t_) : q{q_}, t{t_}{}

        template <typename R>
        constexpr ComparableQuantity(const T q_, const QuantityPercentage<R> t_) : q{q_}, t{q_ * t_.p}{}
        template <typename R>
        constexpr ComparableQuantity(const T q_, const QuantityTolerance<R> t_) : q{q_}, t{t_.t}{}

        // unsafe
        template <typename R>
        constexpr ComparableQuantity(const T q_, const R t_) : q{q_}, t{q.from_value(t_)}{}


        template <typename O>
        inline friend bool operator==(const ComparableQuantity me, const O other){
            return !(me.q < other - me.t || me.q > other + me.t);
        }
        template <typename O>
        inline friend bool operator==(const O other, const ComparableQuantity me){
            return me == other;
        }
        inline friend std::ostream& operator<<(std::ostream &out, const ComparableQuantity& quant){return out << quant.q << " +/- " << quant.t;}
    private:
        const T q;
        const T t;
};

template <class Q>
constexpr inline QuantityTolerance<quantity<Q>> operator~(const quantity<Q> tolerance){
    return QuantityTolerance<quantity<Q>>{tolerance};
}


template <class Q, typename T>
constexpr inline ComparableQuantity<quantity<Q>> operator^(const quantity<Q> q, const QuantityPercentage<T> tolerance){
    return ComparableQuantity<quantity<Q>> {q, tolerance};
}
template <class Q, typename T>
constexpr inline ComparableQuantity<quantity<Q>> operator^(const quantity<Q> q, const QuantityTolerance<T> tolerance){
    return ComparableQuantity<quantity<Q>> {q, tolerance};
}

int main(){
    using boost::units::si::force;
    using boost::units::si::newton;
    using std::cout;

    const quantity<force> q1(10.0 * newton);
    const quantity<force> q2(12.0 * newton);

    cout << "Absolute tolerance:\n";

    const auto t1 = 1. * newton;
    const auto t2 = 2. * newton;
    cout << (q2 ^ ~t1) << "\n";
    cout << (q2 ^ ~t2) << "\n";

    if (q1 == (q2 ^ ~t1)){
        cout << q1 << " == " << q2 << " +/- " << t1 << " (wrong)\n";
    } else {
        cout << q1 << " != " << q2 << " +/- " << t1 << " (correct)\n";
    }

    if (q1 == (q2 ^ ~t2)){
        cout << q1 << " == " << q2 << " +/- " << t2 << " (correct)\n";
    } else {
        cout << q1 << " != " << q2 << " +/- " << t2 << " (wrong)\n";
    }

    cout << "\nPercentage tolerance:\n";

    const auto t3 = 5_percent;
    const auto t4 = 20_percent;
    cout << (q2 ^ t3) << "\n";
    cout << (q2 ^ t4) << "\n";

    if (q1 == (q2 ^ t3)){
        cout << q1 << " == " << q2 << " +/- " << t3 << " (wrong)\n";
    } else {
        cout << q1 << " != " << q2 << " +/- " << t3 << " (correct)\n";
    }

    if (q1 == (q2 ^ t4)){
        cout << q1 << " == " << q2 << " +/- " << t4 << " (correct)\n";
    } else {
        cout << q1 << " != " << q2 << " +/- " << t4 << " (wrong)\n";
    }

    return 0;
}
