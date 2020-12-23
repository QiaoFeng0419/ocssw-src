
#include <oc.hpp>

#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <string>

using namespace oc;
using namespace std;

class Algorithm1to2 : public Algorithm {
    public:
        Algorithm1to2() : Algorithm("Algorithm1to2") {}
        ~Algorithm1to2() override {}
        const std::vector<Product>& needs() const override {
            return requires_;
        }
        const std::vector<Product>& provides() const override {
            return provides_;
        }
        void process(const DataRecord&, const std::pair<size_t, size_t>, const std::pair<size_t, size_t>) const override {}
    private:
        const static std::vector<Product> requires_;
        const static std::vector<Product> provides_;

};
const std::vector<Product> Algorithm1to2::requires_{
    {"product1a"},
    {"product1b", {{"att1", "val1"}}},
    {"product1c", {{"att1", "val1"}, {"att2", "value2"}}}
};
const std::vector<Product> Algorithm1to2::provides_{
    {"product2a"},
    {"product2b", {{"att1", "val1"}}},
    {"product2c", {{"att1", "val1"}, {"att2", "value2"}}}
};
class Algorithm2to3 : public Algorithm {
    public:
        Algorithm2to3() : Algorithm("Algorithm2to3") {}
        ~Algorithm2to3() override {}
        const std::vector<Product>& needs() const override {
            return requires_;
        }
        const std::vector<Product>& provides() const override {
            return provides_;
        }
        void process(const DataRecord&, const std::pair<size_t, size_t>, const std::pair<size_t, size_t>) const override {}
    private:
        const static std::vector<Product> requires_;
        const static std::vector<Product> provides_;

};
const std::vector<Product> Algorithm2to3::requires_{
    {"product2a"},
    {"product2b", {{"att1", "val1"}}},
    {"product2c", {{"att1", "val1"}, {"att2", "value2"}}}
};
const std::vector<Product> Algorithm2to3::provides_{
    {"product3a"},
    {"product3b", {{"att1", "val1"}}},
    {"product3c", {{"att1", "val1"}, {"att2", "value2"}}},
    {"product3d", {{"att1", 1}, {"att2", 2}}}
};
class Algorithm1to3 : public Algorithm {
    public:
        Algorithm1to3() : Algorithm("Algorithm1to3") {}
        ~Algorithm1to3() override {}
        const std::vector<Product>& needs() const override {
            return requires_;
        }
        const std::vector<Product>& provides() const override {
            return provides_;
        }
        void process(const DataRecord&, const std::pair<size_t, size_t>, const std::pair<size_t, size_t>) const override {}
    private:
        const static std::vector<Product> requires_;
        const static std::vector<Product> provides_;

};
const std::vector<Product> Algorithm1to3::requires_{
    {"product1a"},
    {"product4a"},
};
const std::vector<Product> Algorithm1to3::provides_{
    {"product3a"},
    {"product3b", {{"att1", "val1"}}},
    {"product3c", {{"att1", "val1"}, {"att2", "value2"}}}
};
class Algorithm1to4 : public Algorithm {
    public:
        Algorithm1to4() : Algorithm("Algorithm1to4") {}
        ~Algorithm1to4() override {}
        const std::vector<Product>& needs() const override {
            return requires_;
        }
        const std::vector<Product>& provides() const override {
            return provides_;
        }
        void process(const DataRecord&, const std::pair<size_t, size_t>, const std::pair<size_t, size_t>) const override {}
    private:
        const static std::vector<Product> requires_;
        const static std::vector<Product> provides_;

};
const std::vector<Product> Algorithm1to4::requires_{
    {"product1a"},
};
const std::vector<Product> Algorithm1to4::provides_{
    {"product4a"},
    {"product4b", {{"att1", "val1"}}},
    {"product4c", {{"att1", "val1"}, {"att2", "value2"}}}
};
class AlgorithmDataProvider : public Algorithm {
    public:
        AlgorithmDataProvider() : Algorithm("AlgorithmDataProvider") {}
        ~AlgorithmDataProvider() override {}
        const std::vector<Product>& needs() const override {
            return requires_;
        }
        const std::vector<Product>& provides() const override {
            return provides_;
        }
        void process(const DataRecord&, const std::pair<size_t, size_t>, const std::pair<size_t, size_t>) const override {}
    private:
        const static std::vector<Product> requires_;
        const static std::vector<Product> provides_;

};
const std::vector<Product> AlgorithmDataProvider::requires_{ };
const std::vector<Product> AlgorithmDataProvider::provides_{
    {"product4a"},
};



struct AlgorithmVertex; // an algorithm, with paths to others
struct AlgorithmEdge; // a link between algorithms, where one outputs everything another needs
struct AlgorithmGraph; // a web of connected algorithms

struct AlgorithmEdge {
    AlgorithmEdge(AlgorithmVertex* from, AlgorithmVertex* to, float cost=1) : from{from}, to{to}, cost{cost} {}

    AlgorithmVertex* from;
    AlgorithmVertex* to;
    float cost;
};
struct AlgorithmVertex {
    AlgorithmVertex(Algorithm* algorithm, std::vector<AlgorithmEdge> edges={}) : algorithm{algorithm}, edges{edges} {}

    Algorithm* algorithm;
    std::vector<AlgorithmEdge> edges{};
    size_t count_u{0};
};
struct AlgorithmPath {
    AlgorithmPath(AlgorithmVertex* vertex) : path{vertex}, cost{0} {}
    AlgorithmPath(std::vector<AlgorithmVertex*> path, float cost) : path{path}, cost{cost} {}

    AlgorithmPath(const AlgorithmPath& current, const AlgorithmEdge* new_path) : path{current.path}, cost{current.cost + new_path->cost} {path.push_back(new_path->to);}

    std::vector<AlgorithmVertex*> path;
    float cost;

    friend bool operator<(const AlgorithmPath& left, const AlgorithmPath& right){
        return left.cost < right.cost;
    }
    friend bool operator>(const AlgorithmPath& left, const AlgorithmPath& right){
        return left.cost > right.cost;
    }
};
struct AlgorithmGraph {
    static std::vector<AlgorithmVertex> create_graph(const std::vector<std::unique_ptr<Algorithm>>& algorithms){
        std::vector<AlgorithmVertex> vertexes{};
        vertexes.reserve(algorithms.size());
        for (const auto& algorithm : algorithms){
            vertexes.emplace_back(algorithm.get());
        }
        for (auto& vertex : vertexes){
            for (auto& other : vertexes){
                if (&vertex != &other){
                    // if (vertex.algorithm->is_fulfilled_by(*other.algorithm)){
                    if (vertex.algorithm->is_partially_fulfilled_by(*other.algorithm)){
                        if (other.algorithm->name() == "AlgorithmDataProvider"){
                            cout << "Found data provider\n";
                        }
                        vertex.edges.emplace_back(&vertex, &other);
                    }
                }
            }
        }
        return vertexes;
    }
    AlgorithmGraph(const std::vector<std::unique_ptr<Algorithm>>& algorithms, const std::vector<Product>& have, const Product& want) : vertexes{create_graph(algorithms)}, have{have}, want{want} { }
    void reset(){
        for (auto& vertex : vertexes){
            vertex.count_u = 0;
        }
    }
    bool is_complete(const AlgorithmPath& path){
        for (size_t i=0;i<path.path.size();i++){
            for (const auto& need : path.path[i]->algorithm->needs()){
                bool found = std::any_of(have.cbegin(), have.cend(), [&](const auto& p){return p.matches(need);});
                for (size_t j=i+1;j<path.path.size() && !found;j++){
                    found = path.path[j]->algorithm->fulfills(need);
                }
                if (!found){
                    return false;
                }
            }
        }
        return true;
    }
    std::vector<AlgorithmVertex> vertexes;
    const std::vector<Product>& have;
    const Product& want;

    std::ostream& print_verbose_graphviz(std::ostream& out) const {
        out << "digraph G {\n";
        for (const auto& vertex : vertexes){
            if (vertex.algorithm->is_partially_fulfilled_by(have)){
                out << "\t\"input\" -> \"" << vertex.algorithm->name() << "\" [label=\"" ;
                bool print_comma = false;
                for (const auto& p : vertex.algorithm->needs()){
                    if (std::find_if(have.cbegin(), have.cend(), [&](const auto& n){return n.matches(p);}) != have.cend()){
                        if (print_comma){
                            out << ", ";
                        } else {
                            print_comma = true;
                        }
                        out << p.name();
                    }
                }
                out << "\"]\n";
            }
            if (vertex.algorithm->fulfills(want)){
                out << "\t\"" << vertex.algorithm->name() << "\" -> \"output\" [label=\"" ;
                bool print_comma = false;
                // extra complexity kept for if/when Graph supports multiple output variables at once
                for (const auto& p : vertex.algorithm->provides()){
                    if (p.matches(want)){
                        if (print_comma){
                            out << ", ";
                        } else {
                            print_comma = true;
                        }
                        out << p.name();
                    }
                }
                out << "\"]\n";
            }
            if (vertex.edges.empty()){
                out << "\t\"" << vertex.algorithm->name() << "\"\n";
            } else {
                for (const auto& edge : vertex.edges){
                    out << "\t\"" << edge.to->algorithm->name() << "\" -> \"" << edge.from->algorithm->name() << "\" [label=\"";
                    bool print_comma = false;
                    for (const auto& p : edge.to->algorithm->provides()){
                        if (std::find_if(edge.from->algorithm->needs().cbegin(), edge.from->algorithm->needs().cend(), [&](const auto& n){return n.matches(p);}) != edge.from->algorithm->needs().cend()){
                            if (print_comma){
                                out << ", ";
                            } else {
                                print_comma = true;
                            }
                            out << p.name();
                        }
                    }
                    out << "\"]\n";
                }
            }
        }
        out << "}";
        return out;
    }
    std::ostream& print_graphviz(std::ostream& out) const {
        out << "digraph G {\n";
        for (const auto& vertex : vertexes){
            if (vertex.algorithm->is_partially_fulfilled_by(have)){
                out << "\t\"input\" -> \"" << vertex.algorithm->name() << "\"\n";
            }
            if (vertex.algorithm->fulfills(want)){
                out << "\t\"" << vertex.algorithm->name() << "\" -> \"output\"\n";
            }
            if (vertex.edges.empty()){
                out << "\t\"" << vertex.algorithm->name() << "\"\n";
            } else {
                for (const auto& edge : vertex.edges){
                    out << "\t\"" << edge.to->algorithm->name() << "\" -> \"" << edge.from->algorithm->name() << "\"\n";
                }
            }
        }
        out << "}";
        return out;
    }
};
std::ostream& operator<<(std::ostream& out, const AlgorithmGraph& in){
    return in.print_graphviz(out);
}
std::ostream& operator<<(std::ostream& out, const AlgorithmPath& in){
    out << "AlgorithmPath{\n";
    for (const auto& vertex : in.path){
        // if (vertex->algorithm){
            // out << *(vertex->algorithm) << '\n';
            out << vertex->algorithm->name() << '\n';
        // }
    }
    out << "};";
    return out;
}
const int MAX_ALGORITHM_PATHS = 100000; // this is nonsensically high

//// https://en.wikipedia.org/wiki/K_shortest_path_routing
// I go backwards, starting at the finish line (the product I'm looking for)
// and work towards the products I have (the input file).  And I don't use
// weights, all algorithms are cost=1.  And I allow multiple starting points.
std::vector<std::vector<Algorithm*>> algorithm_paths(const std::vector<Product>& have, const std::vector<std::unique_ptr<Algorithm>>& algorithms, const Product& want){
    // I'm just now creating the directed graph.  Since this whole function
    // will be called once per desired product, this should be done ahead of
    // time and passed in.  (This is no longer accurate, as it requires the
    // desired output product(s).  Graph should be changed to support multiple
    // output products, which probably means changing the is_complete function
    // to accept an argument for which output product it is considered
    // completed for.)
    AlgorithmGraph graph{algorithms, have, want};
    cout << graph << "\n";

    //// P = empty
    std::vector<AlgorithmPath> P;

    //// count_u = 0, for all of u in V
    graph.reset();
    auto& vertexes = graph.vertexes;

    //// insert path P_s = {s} into B with cost 0
    // This may insert multiple starting points (any algorithm that can make the product we want)
    std::priority_queue<AlgorithmPath, std::vector<AlgorithmPath>, std::greater<AlgorithmPath>> B;
    for (auto& vertex : vertexes){
        if (std::any_of(vertex.algorithm->provides().cbegin(), vertex.algorithm->provides().cend(), [&want](auto p){return p.matches(want);})){
            B.emplace(&vertex);
        }
    }
    //// while B is not empty and count_t < K:
    while (!B.empty() && P.size() < MAX_ALGORITHM_PATHS){
        //// let P_u be the shortest cost path in B with cost C
        auto P_u = B.top();
        auto& u = P_u.path.back(); // u = the current/last vertex in the current path being evaluated

        //// B = B − {P_u}
        B.pop();

        //// count_u = count_u + 1
        u->count_u += 1;

        //// if u = t then P = P U {Pu}
        // Only the graph knows of the input products, so it's the one that checks if every need is met
        if (graph.is_complete(P_u)){
            P.push_back(P_u);
        }

        //// if count_u ≤ K then
        if (u->count_u <= MAX_ALGORITHM_PATHS){
            //// for each vertex v adjacent to u:
            for (const auto& edge : u->edges){
                //// let P_v be a new path with cost C + w(u, v) formed by concatenating edge (u, v) to path P_u
                //// insert P_v into B return P
                // This constructor of AlgorithmPath (called within emplace)
                // copies a path and appends an edge, including cost
                B.emplace(P_u, &edge);
            }
        }
    }

    //// return P
    // Copy soon-to-be-dead-pointers in paths in P to real vector
    // Also, copy it in reverse, 'cause I'm backwards.
    std::vector<std::vector<Algorithm*>> ret{};
    ret.reserve(P.size());
    for (auto& path : P){
        // ret.emplace_back(path.path.begin(), path.path.end());
        std::vector<Algorithm*> p{};
        p.reserve(path.path.size());
        std::for_each(path.path.rbegin(), path.path.rend(), [&](const auto& vertex){p.push_back(vertex->algorithm);});
        ret.push_back(p);
    }
    return ret;
}
int main(){
    std::vector<Product> have_products{
        {"product1a", {{"att1", "val1"}, {"att2", "value2"}}},
        {"product1b", {{"att1", "val1"}, {"att2", "value2"}}},
        {"product1c", {{"att1", "val1"}, {"att2", "value2"}}}
    };
    std::vector<Product> want_products{
        {"product3a"},
    };

    std::cout << "\n===================================\n";
    std::cout << "I have: \n";
    for (auto it = have_products.cbegin(); it != have_products.cend(); ++it){
        std::cout << *it << '\n';
    }
    std::cout << "\n===================================\n";
    std::cout << "\nI want: \n";
    for (auto it = want_products.cbegin(); it != want_products.cend(); ++it){
        std::cout << *it << '\n';
    }
    std::cout << "\n===================================\n";

    std::vector<std::unique_ptr<Algorithm>> algorithms{};
    algorithms.push_back(std::make_unique<Algorithm1to2>());
    algorithms.push_back(std::make_unique<Algorithm2to3>());
    algorithms.push_back(std::make_unique<Algorithm1to3>());
    algorithms.push_back(std::make_unique<Algorithm1to4>());
    algorithms.push_back(std::make_unique<AlgorithmDataProvider>());

    auto possible_paths = algorithm_paths(have_products, algorithms, want_products[0]);
    cout << "Possible paths (" << possible_paths.size() << "):\n";
    for (const auto& path : possible_paths){
        std::cout << "Path:\n";
        for (const auto& algorithm : path){
            std::cout << '\t' << algorithm->name() << '\n';
        }
    }
    // For doing module directory:
        // First, check an environment variable (which will be set for tests to some build directory)
        // Next, use a static const set to a compile time constant based on CMAKE_INSTALL_PATH or something

    return 0;
}

