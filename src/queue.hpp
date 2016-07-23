#ifndef __QUEUE_HPP_INCLUDED__
#define __QUEUE_HPP_INCLUDED__

#include <fstream>

#include "serialization.hpp"


template <typename T>
class BaseQueue
{
private:

    virtual T getElementAt(int index) = 0;

    virtual int getElementSizeAt(int index) = 0;

    virtual int getLastElementIndex() = 0;

    class Iterator
    {
    public:

        Iterator(BaseQueue<T> *queue_, int index_)
            : queue(queue_), index(index_)
        {
        }

        T operator*()
        {
            return queue->getElementAt(index);
        }

        Iterator& operator++()
        {
            int size = queue->getElementSizeAt(index);
            index += size;
            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return index != iterator.index;
        }

    private:

        BaseQueue<T> *queue;

        int index;
    };

public:

    virtual void Enqueue(T e) = 0;

    virtual void Dequeue() = 0;

    virtual int Size() = 0;

    Iterator begin()
    {
        return Iterator(this, 0);
    }

    Iterator end()
    {
        return Iterator(this, getLastElementIndex());
    }
};


template <typename T>
class VolatileQueue : public BaseQueue<T>
{
public:

    void Enqueue(T e)
    {
        data.push_back(e);
    }

    void Dequeue()
    {
        if (!data.empty())
        {
            data.erase(data.begin());
        }
    }

    int Size()
    {
        return data.size();
    }

private:

    T getElementAt(int index)
    {
        return data[index];
    }

    int getElementSizeAt(int index)
    {
        return 1;
    }

    int getLastElementIndex()
    {
        return Size();
    }

    std::vector<T> data;
};


template <typename T>
class PersistentQueue : public BaseQueue<T>
{
public:

    PersistentQueue(std::string name)
        : file(name, std::ios::out | std::ios::in | std::ios::app | std::ios::binary)
    {
    }

    void Enqueue(T e)
    {
        file.seekp(0, std::ios::end);
        std::string element_as_string = Serialize<T>(e);
        file << element_as_string;
        file.flush();
    }

    void Dequeue()
    {
        file.seekg(0, std::ios::beg);

        T first_element = Deserialize<T>(file);
        std::string first_element_serialized = Serialize<T>(first_element);
        int offset = first_element_serialized.length();

        std::fstream tmpfile("persistent-queue.tmp");
        for (T element : *this)
        {
            tmpfile << Serialize<T>(element);
        }
        file << tmpfile.rdbuf();
    }

    int Size()
    {
        int size = 0;
        for (auto e : *this)
        {
            size += 1;
        }
        return size;
    }

private:

    T getElementAt(int index)
    {
        file.seekg(index, std::ios::beg);
        return Deserialize<T>(file);
    }

    int getElementSizeAt(int index)
    {
        file.seekg(index, std::ios::beg);
        return Serialize<T>(Deserialize<T>(file)).length();
    }

    int getLastElementIndex()
    {
        file.seekg(0, std::ios::end);
        return file.tellg();
    }

    std::fstream file;
};


#endif
