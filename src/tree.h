#ifndef TREE_H
#define TREE_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <variant>
#include <string>
#include <memory>


using namespace std;
class Tree{
private:
    struct Node {
        virtual ~Node() = default;
        virtual std::string path() const = 0;
    };

    struct GeoNode : Node {
        std::string name;
        GeoNode* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;

        explicit GeoNode(std::string n, GeoNode* p = nullptr)
            : name(std::move(n)), parent(p) {}
        //Path = parent path + '/' + name   (root has empty path)
        std::string path() const override {
            if (!parent) return "/" + name;
            return parent->path() + "/" + name;
        }

        //Helper to find child by name
        Node* findChild(const std::string& childName) const {
            for (const auto& ch : children) {
                if (const auto* geo = dynamic_cast<const GeoNode*>(ch.get())) {
                    if (geo->name == childName) return ch.get();
                }
            }
            return nullptr;
        }

        //DataNode or GeoNode
        template<class T, class... Args>
        T* emplaceChild(Args&&... args){
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = ptr.get();
            raw->setParent(this);                 // keep parent link correct
            children.push_back(std::move(ptr));
            return raw;
        }

    private:
        //Internal helper used by emplaceChild
        void setParent(GeoNode* p) { parent = p; }
        friend struct DataNode;
    };

    GeoNode* root;

    struct DataNode : Node {
        std::string dataType;
        std::vector<float>  values;
        std::vector<std::string> labels;
        GeoNode* parent = nullptr;

        DataNode(std::string type,
                std::vector<float>  v,
                std::vector<std::string> l,
                GeoNode* p = nullptr)
            : dataType(std::move(type))
            , values(std::move(v))
            , labels(std::move(l))
            , parent(p) {}
        // Path = parent path + '/' + "(data)"
        std::string path() const override {
            if (!parent) return "/(data)";
            return parent->path() + "/(data)";
        }

        //DataNode never has children, so findChild always returns nullptr
        Node* findChild(const std::string&) const { return nullptr; }

        //called from GeoNode::emplaceChild
        void setParent(GeoNode* p) { parent = p; }
        friend struct GeoNode;
    };
public:
    Tree();
    bool insert(string name, string dataType, vector<float> values, vector<string> labels);
    void print() const;
    void printNode(const Node* n, int depth = 0) const;
    string searchValue(const string& stateAbbrev, const string& countyName, const string& dataType, string yearString) const;
    vector<float> getDisplayData() const;
    ~Tree();
};

#endif //TREE_H