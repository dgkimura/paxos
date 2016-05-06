#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <callback.hpp>
#include <messages.hpp>


class Receiver
{
public:
    Receiver();

    template <typename T>
    void RegisterCallback(Callback&& callback)
    {
        registered_map[typeid(T)] = std::vector<Callback> { std::move(callback) };
    }

private:
    std::unordered_map<std::type_index, std::vector<Callback>> registered_map;
};
