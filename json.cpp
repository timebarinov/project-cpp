#include "json.h"
#include <cctype>

using namespace std;

namespace Json {

    Document::Document(Node root) : root(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root;
    }

    bool operator!=(const Node& lhs, const Node& rhs) {
        return !(lhs == rhs);
    }

    bool operator==(const Node& lhs, const Node& rhs) {
        if (lhs.Index() != rhs.Index()) {
            return false;
        }

        switch (lhs.Index()) {
            case 0: return lhs.AsArray() == rhs.AsArray();
            case 1: 
                if (lhs.AsMap().count("total_time")) {
                    return lhs.AsMap().at("total_time") == rhs.AsMap().at("total_time");
                }
                return lhs.AsMap() == rhs.AsMap();
            case 2: return lhs.AsString() == rhs.AsString();
            case 3: return lhs.AsBool() == rhs.AsBool();
            case 4: return CheckDouble(lhs.AsDouble(), rhs.AsDouble());
            default: return false;
        }

        return true;
    }

    Node LoadNode(istream& input);

    Node LoadArray(istream& input) {
        vector<Node> result;

        for (char c; input >> c && c != ']'; ) {
            if (c != ',') {
                input.putback(c);
            }
            result.push_back(LoadNode(input));
        }

        return Node(move(result));
    }

    Node LoadDigit(istream& input) {
        char c;
        string s;
        input >> c; 
        while (c == '-' || c == '.' || isdigit(c)) {
            s.push_back(c);
            input >> c;
        }
        input.putback(c);
        return Node(stold(s));
    }

    Node LoadString(istream& input) {
        string line;
        getline(input, line, '"');
        return Node(move(line));
    }

    Node LoadDict(istream& input) {
        map<string, Node> result;

        for (char c; input >> c && c != '}'; ) {
            if (c == ',') {
                input >> c;
            }

            string key = LoadString(input).AsString();
            input >> c;
            result.emplace(move(key), LoadNode(input));
        }

        return Node(move(result));
    }

    Node LoadBool(istream& input) {
        string s;
        bool result = false;
        char c;
        do {
            input >> c;
            s.push_back(c);
        } while (c != 'e');

        if (s == "true") {
            result = true;
        }
        return Node(result);
    }

    Node LoadNode(istream& input) {
        char c;
        input >> c;

        if (c == '[') {
            return LoadArray(input);
        } else if (c == '{') {
            return LoadDict(input);
        } else if (c == '"') {
            return LoadString(input);
        } else if (c == 't' || c == 'f') {
            input.putback(c);
            return LoadBool(input);
        } else {
            input.putback(c);
            return LoadDigit(input);
        }
    }

    Document Load(istream& input) {
        return Document{LoadNode(input)};
    }

    std::vector<std::string> Split(std::istream& in) {

        int count = 0;
        string s;
        vector<string> result;
        getline(in, s, '[');
        s.clear();

        while(in) {
            char c = in.get();

            if (c == '{') {
                ++count;
            }
            if (count > 0) {
                s.push_back(c);
            }

            if (c == '}') {
                --count;
                if (count == 0) {
                    result.push_back(move(s));
                }
            }
            
            if (count <= 0 && c == ']') {
                break;
            }

        }
        return result;
    }

}
