
#ifndef OC_PRODUCT
#define OC_PRODUCT



#include <iostream>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <set>
#include <string>

#include <boost/variant.hpp>
#include <boost/variant/variant_fwd.hpp>

namespace oc {
    // using AttributeVectorValue = boost::variant<
    //     std::vector<std::string>, std::vector<float>, std::vector<double>, std::vector<long double>,
    //     std::vector<int8_t>, std::vector<int16_t>, std::vector<int32_t>, std::vector<int64_t>,
    //     std::vector<uint8_t>, std::vector<uint16_t>, std::vector<uint32_t>, std::vector<uint64_t>>;
    using AttributeValue = boost::variant<std::string, float, double, long double,
        int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;
        // std::vector<std::string>, std::vector<float>, std::vector<double>, std::vector<long double>,
        // std::vector<int8_t>, std::vector<int16_t>, std::vector<int32_t>, std::vector<int64_t>,
        // std::vector<uint8_t>, std::vector<uint16_t>, std::vector<uint32_t>, std::vector<uint64_t>>;

    // using AttributeValue = boost::variant< std::string, float, double, long double,
    //       bool, char, char16_t, char32_t, wchar_t, signed char, short int, int, long int, long long int,
    //       unsigned char, unsigned short int, unsigned int, unsigned long int, unsigned long long int,
    //       std::vector<std::string>, std::vector<float>, std::vector<double>, std::vector<long double>, std::vector<bool>,
    //       std::vector<char>, std::vector<char16_t>, std::vector<char32_t>, std::vector<wchar_t>,
    //       std::vector<signed char>, std::vector<short int>, std::vector<int>, std::vector<long int>,
    //       std::vector<long long int>, std::vector<unsigned char>, std::vector<unsigned short int>,
    //       std::vector<unsigned int>, std::vector<unsigned long int>, std::vector<unsigned long long int>>;

    class Attribute {
        public:
            Attribute(const std::string& name, AttributeValue value) : name_{name}, value_{value}{}

            const std::string& name() const {return name_;}
            const AttributeValue& value() const {return value_;}
            const AttributeValue& operator*() const {return value_;}

            friend std::ostream& operator<<(std::ostream& out, const Attribute& attribute){return out << attribute.name_ << " = \"" << attribute.value_ << "\"";}
            friend bool operator==(const Attribute& left, const Attribute& right){
                return left.name_ == right.name_ && left.value_ == right.value_;
            }
            friend bool operator<(const Attribute& left, const Attribute& right){
                return left.name_ < right.name_ && left.value_ < right.value_;
            }

        private:
            std::string name_;
            AttributeValue value_;
    };
    // struct AttributeNameCompare {
    //     bool operator()(const Attribute& lhs, const Attribute& rhs) const {return lhs.name() == rhs.name();}
    // };
    class Product {
        public:
            Product(std::string name, std::set<Attribute> attributes) : name_{name}, attributes_{attributes} {}
            Product(std::string name) : name_{name} {}

            const std::string& name() const {return name_;}
            auto& attributes() const {return attributes_;}
            // void add_attribute(std::unique_ptr<Attribute> attr){attributes_.insert(attributes_.end(), std::move(attr));}
            void add_attribute(Attribute attr){attributes_.insert(attributes_.end(), std::move(attr));}

            friend bool operator==(const Product& me, const Product& other){
                // return name() == other.name() && std::set_intersection(other.attributes().cbegin(), other.attributes().cend(), attributes_.cbegin(), attributes_.cend(), attributes_equal);
                // return name() == other.name() && std::set_intersection(other.attributes().cbegin(), other.attributes().cend(), attributes_.cbegin(), attributes_.cend(), attributes_equal);
                return me.name() == other.name() && std::is_permutation(other.attributes().cbegin(), other.attributes().cend(), me.attributes().cbegin(), me.attributes().cend());
            }
            bool matches(const Product& other) const {
                if (name() == other.name()){
                    for (auto it = other.attributes().cbegin(); it != other.attributes().cend(); ++it){
                        if (attributes_.find(*it) == attributes_.cend()){
                            return false;
                        }
                    }
                    return true;
                }
                return false;
            }
            friend std::ostream& operator<<(std::ostream& out, const Product& product){
                out << product.name_ << " = {\n";
                for (auto it = product.attributes().cbegin(); it != product.attributes().cend(); ++it){
                    out << '\t' << *it << '\n';
                }
                out << "};";
                return out;
            }
        private:
            std::string name_;
            // std::set<std::unique_ptr<Attribute>, AttributeNameCompare> attributes_{};
            std::set<Attribute> attributes_{};

            // static bool attributes_equal( const std::unique_ptr<Attribute>& left, const std::unique_ptr<Attribute>& right){return *left == *right;}
            // static bool attributes_equal( const Attribute& left, const Attribute& right){return left == right;}
            // static bool attributes_equal( const std::unique_ptr<Attribute>& left, const std::unique_ptr<Attribute>& right){return *left == *right;}
            // static bool attributes_equal( const std::unique_ptr<Attribute>& left, const std::unique_ptr<Attribute>& right){return *left == *right;}


    };
} // namespace oc

#endif // OC_PRODUCT

