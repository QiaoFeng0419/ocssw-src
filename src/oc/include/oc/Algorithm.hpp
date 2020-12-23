
#ifndef OC_ALGORITHM
#define OC_ALGORITHM

#include "DataRecord.hpp"
#include "Product.hpp"

#include <string>
#include <utility>
#include <vector>

namespace oc {
    class Algorithm {
        public:
            Algorithm(std::string name, std::string description) : name_{name}, description_{description} {}
            Algorithm(std::string name) : Algorithm(name, {}) {}

            virtual ~Algorithm();

            const std::string& name() const {return name_;}
            const std::string& description() const {return description_;}

            virtual void process(const DataRecord &data_record, const std::pair<size_t, size_t> origin, const std::pair<size_t, size_t> size) const = 0;
            virtual const std::vector<Product>& provides() const;
            virtual const std::vector<Product>& needs() const;

            bool fulfills(const Product& other) const {
                return contains(provides(), other);
            }
            bool fulfills(const std::vector<Product>& other) const {
                return contains_all(provides(), other);
            }
            bool fulfills(const Algorithm& other) const {
                return contains_all(provides(), other.needs());
            }
            bool is_fulfilled_by(const std::vector<Product>& other) const {
                return contains_all(other, needs());
            }
            bool is_fulfilled_by(const Algorithm& other) const {
                return contains_all(other.provides(), needs());
            }
            bool partially_fulfills(const std::vector<Product>& other) const {
                return contains_any(provides(), other);
            }
            bool partially_fulfills(const Algorithm& other) const {
                return contains_any(provides(), other.needs());
            }
            bool is_partially_fulfilled_by(const std::vector<Product>& other) const {
                return contains_any(other, needs());
            }
            bool is_partially_fulfilled_by(const Algorithm& other) const {
                return contains_any(other.provides(), needs());
            }

        private:
            std::string name_{};
            std::string description_{};

            static bool contains_all(const std::vector<Product>& haystack, const std::vector<Product>& needles) {
                for (auto& needle : needles){
                    if (!contains(haystack, needle)){
                        return false;
                    }
                }
                return true;
            }
            static bool contains_any(const std::vector<Product>& haystack, const std::vector<Product>& needles) {
                for (auto& needle : needles){
                    if (contains(haystack, needle)){
                        return true;
                    }
                }
                return false;
            }
            static bool contains(const std::vector<Product>& haystack, const Product& needle) {
                for (auto& hay : haystack){
                    if (hay.matches(needle)){
                        return true;
                    }
                }
                return false;
            }
    };
    class AlgorithmProvider {
        public:
            virtual ~AlgorithmProvider() = default;

            virtual std::vector<Algorithm> provides() = 0;
    };
} // namespace oc
std::ostream& operator<<(std::ostream& out, oc::Algorithm& in);
#endif // OC_ALGORITHM

