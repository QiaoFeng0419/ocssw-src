
#include "oc/FileReader.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <gsl/gsl>

#include <array>
#include <memory>
#include <string>
#include <unordered_map>


#if HDF5_FOUND
#include <hdf5.h>
#endif

#if NETCDF_FOUND
#include <netcdf>
#endif

#if HDF4_FOUND
#include <hdf.h>
#include <mfhdf.h>
#endif

namespace oc {

void check_for_known_types(FileReaderHints& hints);
void read_hdf4_attributes(FileReaderHints& hints);
void read_hdf5_attributes(FileReaderHints& hints);
void read_netcdf_attributes(FileReaderHints& hints);
bool is_hdf5(const FileReaderHints& hints);

FileReaderHints::FileReaderHints(const std::string& path) : filename_{path}, size_{boost::filesystem::file_size(path)} {
    FILE *f = std::fopen(path.c_str(), "r");
    if (f != nullptr){
        for (auto i = first_bytes_.begin(); i != first_bytes_.end() && *i != EOF; i++){
            *i = getc(f);
        }
        fclose(f);
    }
    check_for_known_types(*this);
}

bool FileReaderHints::magic_number(const std::string& bytes) const noexcept {
    auto i = first_bytes_.cbegin();
    auto b = bytes.cbegin();
    for (; i != first_bytes_.cend() && b != bytes.cend() && *i != EOF; i++, b++){
        if (*i != *b){
            return false;
        }
    }
    return true;
}
template<std::size_t N>
bool FileReaderHints::magic_number(const std::array<int, N>& bytes) const noexcept {
    auto i = first_bytes_.cbegin();
    auto b = bytes.cbegin();
    for (; i != first_bytes_.cend() && b != bytes.cend() && *i != EOF; i++, b++){
        if (*i != *b){
            return false;
        }
    }
    return true;
}

// const std::array<int, 4> NETCDF1_MAGIC_NUMBER = {'C', 'D', 'F', 0x01};
// const std::array<int, 4> NETCDF2_MAGIC_NUMBER = {'C', 'D', 'F', 0x02};
const std::array<int, 4> HDF4_MAGIC_NUMBER = {0x0E, 0x03, 0x13, 0x01};
const std::array<int, 8> HDF5_MAGIC_NUMBER = {0x89, 'H', 'D', 'F', '\r', '\n', 0x1A, '\n'};

void read_hdf5_attributes(FileReaderHints& hints){
#if HDF5_FOUND
    size_t current_buffer_size = 0;
    char *value_buffer = NULL;
    const size_t max_name_size = 256;
    char name_buffer[max_name_size];
    auto file_id = H5Fopen(hints.filename().c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    auto group_id = H5Gopen(file_id, "/", H5P_DEFAULT);
    const auto numAttrs = H5Aget_num_attrs(group_id);
    for (auto i=0;i<numAttrs;i++){
        auto id = H5Aopen_idx(group_id, i);
        auto type_id = H5Aget_type(id);
        auto class_id = H5Tget_class(type_id);
        if (class_id == H5T_STRING){
            auto space_id = H5Aget_space(id);
            auto size = H5Tget_size(type_id) + 1; // +1 == null terminator
            auto ndims = H5Sget_simple_extent_dims(space_id, NULL, NULL) + 1;
            if (ndims > 0){
                auto memtype_id = H5Tcopy(H5T_C_S1);
                H5Tset_size(memtype_id, size);
                size_t buffer_size_needed = ndims * size + 1;
                if (current_buffer_size < buffer_size_needed){
                    if (current_buffer_size > 0){
                        std::free(value_buffer);
                    }
                    value_buffer = static_cast<char*>(malloc(buffer_size_needed));
                    current_buffer_size = size;
                }
                H5Aget_name(id, max_name_size, name_buffer);
                H5Aread(id, memtype_id, value_buffer);
                hints.attributes().insert({name_buffer, value_buffer});
                H5Tclose(memtype_id);
            }
            H5Sclose(space_id);
        }
        H5Tclose(type_id);
        H5Aclose(id);
    }
    H5Gclose(group_id);
    H5Fclose(file_id);
    if (current_buffer_size > 0){
        std::free(value_buffer);
    }
#else // no HDF5
    // NetCDF is a subset of HDF5, so it might be able to read it.
    read_netcdf_attributes(hints);
#endif
}
void read_netcdf_attributes(FileReaderHints& hints){
#if NETCDF_FOUND
    try {
        using namespace netCDF;
        NcFile input(hints.filename(), NcFile::read);
        auto atts = input.getAtts();
        for (auto i = atts.cbegin(); i != atts.cend(); ++i){
            try {
                if (i->second.getType() == NcType::nc_CHAR || i->second.getType() == NcType::nc_STRING){
                    std::string ret;
                    ret.reserve(i->second.getAttLength());
                    i->second.getValues(ret);

                    hints.attributes().insert({i->first, ret});
                }
            } catch (netCDF::exceptions::NcException& e) {
            }
        }
    } catch (netCDF::exceptions::NcException& e) {
    }
#else // no NetCDF
    (void)hints; // silence unused errors
#endif
}
void read_hdf4_attributes(FileReaderHints& hints){
#if HDF4_FOUND
    auto sd_id = SDstart(hints.filename().c_str(), DFACC_READ);
    auto n_datasets{0}, nattrs{0};
    SDfileinfo(sd_id, &n_datasets, &nattrs);

    char attr_name[H4_MAX_NC_NAME];
    int32 dtype, nelems;

    for (auto i{0}; i < nattrs; i++){
        if (SDattrinfo(sd_id, i, attr_name, &dtype, &nelems) != FAIL){
            if (dtype == DFNT_CHAR){
                auto attr_data = static_cast<char*>(HDmalloc(nelems * sizeof(char)));
                SDreadattr(sd_id, i, attr_data);
                hints.attributes().insert({attr_name, attr_data});
                std::free(attr_data);
            }
        }
    }

    SDend(sd_id);
#else // no HDF4
#if NETCDF_FOUND
    read_netcdf_attributes(hints);
#else // no HDF4, no NetCDF
    (void)hints; // silence unused errors
#endif
#endif
}

bool is_hdf5(const FileReaderHints& hints){
#if HDF5_FOUND
    return H5Fis_hdf5(hints.filename().c_str());
#else
    // Naive, as the magic numbers of HDF5 can be at different bytes offsets:
    // "0, 512, 1024, 2048, and multiples of two thereafter."
    // https://www.loc.gov/preservation/digital/formats/fdd/fdd000229.shtml
    return hints.magic_number(HDF5_MAGIC_NUMBER);
#endif
}
void check_for_known_types(FileReaderHints& hints){
    if (hints.magic_number(HDF4_MAGIC_NUMBER)){
        hints.format("HDF4");
        read_hdf4_attributes(hints);
    } else if (is_hdf5(hints)) {
        if (boost::algorithm::ends_with(hints.filename(), ".nc")){
            hints.format("netCDF-4");
            read_netcdf_attributes(hints);
        } else {
            hints.format("HDF5");
            read_hdf5_attributes(hints);
        }
    }
}


} // namespace oc

