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
        virtual ~Node() = default;               // required for polymorphism
        virtual std::string path() const = 0;        // every node knows its full path
    };

    struct GeoNode : Node {
        std::string name;
        GeoNode* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;

        explicit GeoNode(std::string n, GeoNode* p = nullptr)
            : name(std::move(n)), parent(p) {}
        // Path = parent path + '/' + name   (root has empty path)
        std::string path() const override {
            if (!parent) return "/" + name;
            return parent->path() + "/" + name;
        }

        // Helper: find a direct child by name (returns raw pointer for convenience)
        Node* findChild(const std::string& childName) const {
            for (const auto& ch : children) {
                if (const auto* geo = dynamic_cast<const GeoNode*>(ch.get())) {
                    if (geo->name == childName) return ch.get();
                }
            }
            return nullptr;
        }

        // Convenience: add any child (GeoNode or DataNode)
        template<class T, class... Args>
        T* emplaceChild(Args&&... args){
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = ptr.get();
            raw->setParent(this);                 // keep parent link correct
            children.push_back(std::move(ptr));
            return raw;
        }

    private:
        // internal helper used by emplaceChild
        void setParent(GeoNode* p) { parent = p; }
        friend struct DataNode;   // DataNode needs to call setParent too
    };

    GeoNode* root;

    struct DataNode : Node {
        std::string dataType;
        std::vector<float>  values;
        std::vector<std::string> labels;
        GeoNode* parent = nullptr;   // still useful for path construction

        DataNode(std::string type,
                std::vector<float>  v,
                std::vector<std::string> l,
                GeoNode* p = nullptr)
            : dataType(std::move(type))
            , values(std::move(v))
            , labels(std::move(l))
            , parent(p) {}
        // Path = parent path + '/' + "(data)"   (no real name)
        std::string path() const override {
            if (!parent) return "/(data)";
            return parent->path() + "/(data)";
        }

        // DataNode never has children, so findChild always returns nullptr
        Node* findChild(const std::string&) const { return nullptr; }

        // called from GeoNode::emplaceChild
        void setParent(GeoNode* p) { parent = p; }
        friend struct GeoNode;
    };
public:
    Tree();
    bool insert(string name, string parent, string dataType, vector<float> values, vector<string> labels);
    void destroy(GeoNode* node);
    ~Tree();
};