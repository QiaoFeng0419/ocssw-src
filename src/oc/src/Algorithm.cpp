
#include "oc/Algorithm.hpp"

#include <iostream>

static const std::vector<oc::Product> empty_vector_of_products{};

oc::Algorithm::~Algorithm(){}
const std::vector<oc::Product>& oc::Algorithm::provides() const {return empty_vector_of_products;}
const std::vector<oc::Product>& oc::Algorithm::needs() const {return empty_vector_of_products;}

std::ostream& operator<<(std::ostream& out, oc::Algorithm& in){
    out << "Algorithm: " << in.name() << '\n';
    if (in.description().length()){
        out << "Description: " << in.description() << '\n';
    }
    if (!in.needs().empty()){
        out << "\nRequires: \n";
        for (auto it = in.needs().cbegin(); it != in.needs().cend(); ++it){
            out << *it << '\n';
        }
    }
    if (!in.provides().empty()){
        out << "Provides: \n";
        for (auto it = in.provides().cbegin(); it != in.provides().cend(); ++it){
            out << *it << '\n';
        }
    }
    return out;
}

