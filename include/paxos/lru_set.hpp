#ifndef __LRU_SET_HPP_INCLUDED__
#define __LRU_SET_HPP_INCLUDED__

#include <deque>
#include <functional>
#include <set>


namespace paxos
{


template <class Key, class Compare=std::less<Key>>
class lru_set
{
public:

    lru_set(size_t capacity)
        : capacity(capacity)
    {
    }

    void
    insert(Key e)
    {
        if (contains(e))
        {
            return;
        }

        queue.push_back(e);
        if (queue.size() > capacity)
        {
            // remove
            auto front = queue[0];
            queue.pop_front();
            set.erase(front);
        }
        set.insert(e);
    }

    bool
    contains(Key e)
    {
        return set.find(e) != set.end();
    }

private:

    size_t capacity;

    std::set<Key, Compare> set;

    std::deque<Key> queue;
};


}


#endif
