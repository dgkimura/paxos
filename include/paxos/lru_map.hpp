#ifndef __LRU_MAP_HPP_INCLUDED__
#define __LRU_MAP_HPP_INCLUDED__

#include <deque>
#include <functional>
#include <map>


namespace paxos
{


template <class Key, class T, class Compare=std::less<Key>>
class lru_map
{
public:

    using map_type = std::map<Key, T>;
    using iterator = typename map_type::iterator;

    lru_map(size_t capacity)
        : capacity(capacity)
    {
    }

    T&
    operator[](const Key e)
    {
        if (map.find(e) != map.end())
        {
            return map[e];
        }

        queue.push_back(e);
        if (queue.size() > capacity)
        {
            // remove
            auto front = queue[0];
            queue.pop_front();
            map.erase(front);
        }
        return map[e];
    }

    iterator
    find(const Key e)
    {
        return map.find(e);
    }

    void
    erase(const Key e)
    {
        for (int i=0; i<queue.size(); i++)
        {
            if (Compare{}(queue.at(i), e))
            {
                queue.erase(queue.begin() + i);
                break;
            }
        }
        map.erase(e);
    }

    iterator
    begin()
    {
        return map.begin();
    }

    iterator
    end()
    {
        return map.end();
    }

    size_t
    size() const
    {
        return map.size();
    }

private:

    size_t capacity;

    std::map<Key, T, Compare> map;

    std::deque<Key> queue;
};


}


#endif

