
#include "oc/StringUtils.hpp"

#include <gsl/gsl>

#include <boost/algorithm/string.hpp>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace oc {

std::string StringUtils::replace_oc_roots(const std::string& str){
    std::string ret = str;
    replace_oc_roots(ret);
    return ret;
}
std::string& StringUtils::replace_oc_roots(std::string& str){
    replace_all(str, "$OCVARROOT", std::getenv("OCVARROOT"));
    replace_all(str, "$OCDATAROOT", std::getenv("OCDATAROOT"));
    replace_all(str, "$OCSSWROOT", std::getenv("OCSSWROOT"));
    return str;
}
std::string StringUtils::insert_oc_roots(const std::string& str){
    std::string ret = str;
    insert_oc_roots(ret);
    return ret;
}
std::string& StringUtils::insert_oc_roots(std::string& str){
    replace_all(str, std::getenv("OCVARROOT"), "$OCVARROOT");
    replace_all(str, std::getenv("OCDATAROOT"), "$OCDATAROOT");
    replace_all(str, std::getenv("OCSSWROOT"), "$OCSSWROOT");
    return str;
}
std::string StringUtils::replace_all(const std::string& haystack, const char *needle, const char *replacement){
    std::string ret = haystack;
    return replace_all(ret, needle, replacement);
}
std::string& StringUtils::replace_all(std::string& haystack, const char *needle, const char *replacement){
    if (needle && replacement){
        size_t pos;
        const size_t needle_length = strlen(needle);
        while((pos = haystack.find(needle)) != std::string::npos) {
            haystack.replace(pos, needle_length, replacement);
        }
    }
    return haystack;
}

std::string StringUtils::strip_enclosure(const std::string& haystack, const char first, const char last){
    std::string ret = haystack;
    return strip_enclosure(ret, first, last);
}
std::string& StringUtils::strip_enclosure(std::string& haystack, const char first, const char last){
    if(haystack.length() > 1 && haystack.front() == first && haystack.back() == last){
        haystack.pop_back();
        haystack.erase(haystack.begin());
    }
    return haystack;
}
std::string StringUtils::strip_brackets(const std::string& haystack){
    std::string ret = haystack;
    return strip_brackets(ret);
}
std::string& StringUtils::strip_brackets(std::string& haystack){
    return strip_enclosure(haystack, '[', ']');
}
std::string StringUtils::strip_quotes(const std::string& haystack){
    std::string ret = haystack;
    return strip_quotes(ret);
}
std::string& StringUtils::strip_quotes(std::string& haystack){
    return strip_enclosure(strip_enclosure(haystack, '"', '"'), '\'', '\'');
}

template<typename T>
std::vector<T> StringUtils::stov(std::string& str, const std::string& delims){
    (void)str;
    (void)delims;
    throw "Template specialization not implemented for type, use stov(str, delims, std::function) for custom types";
}

template<typename T>
std::vector<T> StringUtils::stov(std::string& str, const std::string& delims, std::function<T(const std::string&)> parser){
    std::vector<T> r;
    if (str.length() == 0){
        return r;
    }
    std::vector<boost::iterator_range<std::string::iterator>> refs;
    ifind_all(refs, str, delims);
    r.reserve(refs.size() + 1);
    auto last_it = str.begin();
    for (auto it=refs.begin(); it != refs.end(); last_it=it->end(), it++){
        r.push_back(parser(std::string(last_it, it->begin())));
    }
    r.push_back(parser(std::string(last_it, str.end())));

    return r;
}

//! \cond
/*!
    \brief Convert delimited string to vector of std::string

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<std::string> StringUtils::stov<std::string>(std::string& str, const std::string& delims){
    return stov<std::string>(str, delims, [](const std::string& in) {return in;});
}
/*!
    \brief Convert delimited string to vector of floats

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<float> StringUtils::stov<float>(std::string& str, const std::string& delims){
    return stov<float>(str, delims, [](const std::string& in){return std::stof(in);});
}
/*!
    \brief Convert delimited string to vector of doubles

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<double> StringUtils::stov<double>(std::string& str, const std::string& delims){
    return stov<double>(str, delims, [](const std::string& in){return std::stod(in);});
}
/*!
    \brief Convert delimited string to vector of ints

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<int> StringUtils::stov<int>(std::string& str, const std::string& delims){
    return stov<int>(str, delims, [](const std::string& in){return std::stoi(in);});
}
/*!
    \brief Convert delimited string to vector of longs

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<long> StringUtils::stov<long>(std::string& str, const std::string& delims){
    return stov<long>(str, delims, [](const std::string& in){return std::stol(in);});
}
/*!
    \brief Convert delimited string to vector of long doubles

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<long double> StringUtils::stov<long double>(std::string& str, const std::string& delims){
    return stov<long double>(str, delims, [](const std::string& in){return std::stold(in);});
}
/*!
    \brief Convert delimited string to vector of long longs

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<long long> StringUtils::stov<long long>(std::string& str, const std::string& delims){
    return stov<long long>(str, delims, [](const std::string& in){return std::stoll(in);});
}
/*!
    \brief Convert delimited string to vector of unsigned ints

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<unsigned> StringUtils::stov<unsigned>(std::string& str, const std::string& delims){
    return stov<unsigned>(str, delims, [](const std::string& in){return gsl::narrow<unsigned>(std::stoul(in));});
}
/*!
    \brief Convert delimited string to vector of unsigned longs

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<unsigned long> StringUtils::stov<unsigned long>(std::string& str, const std::string& delims){
    return stov<unsigned long>(str, delims, [](const std::string& in){return std::stoul(in);});
}
/*!
    \brief Convert delimited string to vector of unsigned long longs

    \tparam T type of elements in output vector
    \param str Delimited string
    \param delims string delimiter

    \return Newly allocated vector of the template type
*/
template<>
std::vector<unsigned long long> StringUtils::stov<unsigned long long>(std::string& str, const std::string& delims){
    return stov<unsigned long long>(str, delims, [](const std::string& in){return std::stoull(in);});
}
//! \endcond

} // namespace oc
