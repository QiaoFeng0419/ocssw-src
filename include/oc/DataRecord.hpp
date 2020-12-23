
#ifndef OC_DATARECORD
#define OC_DATARECORD

#include "Product.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace oc {

class DataRecord {
    public:
        template<typename T = double>
        std::vector<T>& get_product(const std::string& product) const {
            if (std::is_same<T, double>::value){
                return double_products_.at(product);
            } else {
                return int_products_.at(product);
            }
        }

        std::unordered_map<std::string, const Product>::const_iterator products_begin() const;
        std::unordered_map<std::string, const Product>::const_iterator products_end() const;

        std::pair<size_t, size_t> get_size() const noexcept { return size_; }
    private:
        std::unordered_map<std::string, std::vector<double>> double_products_{};
        std::unordered_map<std::string, std::vector<long>> int_products_{};

        std::unordered_map<std::string, const Product> products_;

        std::pair<size_t, size_t> size_;
};

} // namespace oc

#endif // OC_DATARECORD
