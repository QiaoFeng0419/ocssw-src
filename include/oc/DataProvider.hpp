
#ifndef OC_DATAPROVIDER
#define OC_DATAPROVIDER

#include "DataRecord.hpp"
#include "Product.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace oc {
    class DataProvider {
        public:
            virtual ~DataProvider();

            virtual DataRecord get_data_record(size_t kernel_size=1);
            virtual DataRecord get_data_record(std::pair<float, float> lat_range, std::pair<float, float> lon_range, size_t kernel_size=1);

            virtual void process(const DataRecord &data_record, const std::pair<size_t, size_t> origin, const std::pair<size_t, size_t> size);

            virtual std::vector<Product> provides();
            virtual std::vector<Product> needs();

            virtual const std::string& name() const;
            virtual const std::string& description() const;
        private:
    };
    class DataProviderProvider {
        public:
            virtual ~DataProviderProvider();

            virtual std::vector<DataProvider> provides();
    };
} // namespace oc

#endif // OC_DATAPROVIDER

