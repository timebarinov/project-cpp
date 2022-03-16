#pragma once

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <cmath>

inline bool CheckDouble(long double lhs, long double rhs, long double precision = 0.01)
{
    return std::abs(lhs - rhs) <= precision;
}

namespace Json {

    class Node : std::variant<std::vector<Node>, 
                std::map<std::string, Node>,
                long double,
                bool,
                std::string> {
    public:
        using variant::variant;

        const auto& AsArray() const {
            return std::get<std::vector<Node>>(*this);
        }
        const auto& AsMap() const {
            return std::get<std::map<std::string, Node>>(*this);
        }
        int AsInt() const {
            return static_cast<int>(std::get<long double>(*this));
        }

        const auto& AsDouble() const {
            return std::get<long double>(*this);
        }

        const auto& AsString() const {
            return std::get<std::string>(*this);
        }

        const auto& AsBool() const {
            return std::get<bool>(*this);
        }

        size_t Index() const {
            return this->index();
        }
        
    };

    bool operator==(const Node& lhs, const Node& rhs);
    bool operator!=(const Node& lhs, const Node& rhs);

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

    private:
        Node root;
    };

    Document Load(std::istream& input);
    std::vector<std::string> Split(std::istream& in);

}
