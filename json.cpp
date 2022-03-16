#include "json.h"

using namespace std;

namespace Json {

    Document::Document(Node root) : root(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root;
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
        long double result = 0;
        input >> result;
        return Node(result);
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
        input >> s;
        bool result = false;

        if (s.substr(0,4) == "true") {
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
