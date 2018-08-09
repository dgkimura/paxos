#ifndef __QUEUE_HPP_INCLUDED__
#define __QUEUE_HPP_INCLUDED__

#include <cstdint>
#include <fstream>
#include <mutex>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "serialization.hpp"


namespace paxos
{


template <typename T>
class BaseQueue
{
private:

    virtual T getElementAt(int64_t index) = 0;

    virtual int64_t getElementSizeAt(int64_t index) = 0;

    virtual int64_t getFirstElementIndex() = 0;

    virtual int64_t getLastElementIndex() = 0;

    class Iterator
    {
    public:

        Iterator(BaseQueue<T> *queue_, int64_t index_)
            : queue(queue_), index(index_)
        {
        }

        T operator*()
        {
            return queue->getElementAt(index);
        }

        Iterator& operator++()
        {
            int64_t size = queue->getElementSizeAt(index);
            index += size;
            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return index != iterator.index;
        }

    private:

        BaseQueue<T> *queue;

        int64_t index;
    };

public:

    virtual void Enqueue(T e) = 0;

    virtual void Dequeue() = 0;

    virtual T Last() = 0;

    Iterator begin()
    {
        return Iterator(this, getFirstElementIndex());
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

    T Last()
    {
        T empty;
        return data.size() > 0 ? data[data.size() - 1 ] : empty;
    }

private:

    T getElementAt(int64_t index)
    {
        return data[index];
    }

    int64_t getElementSizeAt(int64_t index)
    {
        return 1;
    }

    int64_t getFirstElementIndex()
    {
        return 0;
    }

    int64_t getLastElementIndex()
    {
        return data.size();
    }

    std::vector<T> data;
};


template <typename T>
class PersistentQueue : public BaseQueue<T>
{
public:

    PersistentQueue(std::string filename)
        : PersistentQueue(".", filename)
    {
    }

    PersistentQueue(std::string dirname, std::string filename)
        : file((boost::filesystem::path(dirname) /
                boost::filesystem::path(filename)).string(),
               std::ios::out | std::ios::in | std::ios::app | std::ios::binary),
          stream(file),
          insert_file((boost::filesystem::path(dirname) /
                       boost::filesystem::path(filename)).string(),
                       std::ios::in | std::ios::out | std::ios::binary),
          insert_stream(insert_file),
          start_position(0),
          end_position(0),
          mutex()
    {
        SaveOffsets();
        LoadOffsets();
    }

    PersistentQueue(std::iostream& stream)
        : stream(stream),
          insert_stream(stream),
          start_position(0),
          end_position(0),
          mutex()
    {
        SaveOffsets();
        LoadOffsets();
    }

    ~PersistentQueue()
    {
        stream.flush();
    }

    void SaveOffsets()
    {
        stream.seekg(0, std::ios::end);
        if (stream.tellg() < HEADER_SIZE)
        {
            start_position = HEADER_SIZE;
            end_position = UNINITIALIZED;
        }

        //insert_stream.seekp(0, std::ios::beg);
        insert_stream << std::setw(INDEX_SIZE) << start_position;
        insert_stream << std::setw(INDEX_SIZE) << end_position;
        insert_stream.flush();
    }

    void LoadOffsets()
    {
        stream.seekg(0, std::ios::beg);

        std::string buffer;
        stream.read(&buffer[0], INDEX_SIZE);
        boost::trim(buffer);
        start_position = std::stoi(buffer);

        stream.read(&buffer[0], INDEX_SIZE);
        boost::trim(buffer);
        end_position = std::stoi(buffer);
    }

    void Enqueue(T e)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        std::string element_as_string = Serialize<T>(e);

        LoadOffsets();

        stream.seekg(0, std::ios::end);
        end_position = stream.tellg();

        insert_stream.seekp(INDEX_SIZE, std::ios::beg);
        insert_stream << std::setw(INDEX_SIZE) << end_position;
        insert_stream.flush();

        stream.seekp(0, std::ios::end);
        stream << element_as_string;
        stream.flush();
    }

    void Dequeue()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        LoadOffsets();

        for (T element : *this)
        {
            start_position += Serialize<T>(element).length();
            stream.seekp(0, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << start_position;
            break;
        }
    }

    T Last()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        T empty;
        stream.seekg(0, std::ios::end);
        if (stream.tellg() <= HEADER_SIZE)
        {
            return empty;
        }

        LoadOffsets();

        stream.seekg(end_position, std::ios::beg);

        return Deserialize<T>(stream);
    }

private:

    T getElementAt(int64_t index)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        stream.seekg(index, std::ios::beg);
        return Deserialize<T>(stream);
    }

    int64_t getElementSizeAt(int64_t index)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        stream.seekg(index, std::ios::beg);
        return Serialize<T>(Deserialize<T>(stream)).length();
    }

    int64_t getFirstElementIndex()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        LoadOffsets();

        return start_position;
    }

    int64_t getLastElementIndex()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        stream.seekg(0, std::ios::end);
        return stream.tellg();
    }

    std::fstream file;

    std::iostream& stream;

    std::fstream insert_file;

    std::iostream& insert_stream;

    std::streampos start_position;

    std::streampos end_position;

    std::recursive_mutex mutex;

    constexpr static const int64_t INDEX_SIZE = 10;

    constexpr static const int64_t HEADER_SIZE = INDEX_SIZE * 2;

    constexpr static const int64_t UNINITIALIZED = -1;
};


template <typename T>
class RolloverQueue
{
private:

    std::streampos get_first_position()
    {
        stream.seekg(0 * INDEX_SIZE, std::ios::beg);
        std::string buffer;
        stream.read(&buffer[0], INDEX_SIZE);
        boost::trim(buffer);
        try
        {
            return static_cast<std::streampos>(std::stoi(buffer));
        }
        catch (const std::exception& e)
        {
            return -1;
        }
    }

    std::streampos get_last_position()
    {
        if (file.is_open())
        {
            stream.rdbuf(file.rdbuf());
        }
        stream.seekg(1 * INDEX_SIZE, std::ios::beg);
        std::string buffer(INDEX_SIZE, '\0');
        stream.read(&buffer[0], INDEX_SIZE);
        boost::trim(buffer);
        try
        {
            return static_cast<std::streampos>(std::stoi(buffer));
        }
        catch (const std::exception& e)
        {
            return -1;
        }
    }

    std::streampos get_eof_position()
    {
        if (file.is_open())
        {
            stream.rdbuf(file.rdbuf());
        }
        stream.seekg(2 * INDEX_SIZE, std::ios::beg);
        std::string buffer(INDEX_SIZE, '\0');
        stream.read(&buffer[0], INDEX_SIZE);
        boost::trim(buffer);
        try
        {
            return static_cast<std::streampos>(std::stoi(buffer));
        }
        catch (const std::exception& e)
        {
            return -1;
        }
    }

    std::fstream file;

    std::iostream& stream;

    std::streampos rollover_size;

    constexpr static const int64_t INDEX_SIZE = 10;

    constexpr static const int64_t UNINITIALIZED = -1;

    constexpr static const int64_t HEADER_SIZE = INDEX_SIZE * 3;

    //
    // If queue rollover size is too large then it will take an unreasonable
    // amount of time to iterate over items in the queue.
    //
    constexpr static const int64_t DEFAULT_ROLLOVER_SIZE = 0x100000;

    class Iterator
    {
    public:

        Iterator(
            std::iostream& stream,
            std::streampos first,
            std::streampos position,
            std::streampos last,
            std::streampos eof
        )
            : stream(stream),
              first(first),
              position(position),
              last(last),
              eof(eof)
        {
        }

        T operator*()
        {
            stream.seekg(position, std::ios::beg);

            return Deserialize<T>(stream);
        }

        Iterator& operator++()
        {
            if (position == last)
            {
                position = UNINITIALIZED;
            }
            else
            {
                stream.seekg(position, std::ios::beg);
                position += Serialize<T>(Deserialize<T>(stream)).length();

                if (position == eof)
                {
                    position = HEADER_SIZE;
                }
            }

            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return position != iterator.position;
        }

    private:

        std::iostream& stream;

        std::streampos first;

        std::streampos position;

        std::streampos last;

        std::streampos eof;
    };

public:

    RolloverQueue(std::string filename)
        : RolloverQueue(".", filename)
    {
    }

    RolloverQueue(
        std::string dirname,
        std::string filename,
        std::streampos rollover_size=DEFAULT_ROLLOVER_SIZE
    ) : file((boost::filesystem::path(dirname) /
              boost::filesystem::path(filename)).string(),
               std::ios::out | std::ios::in | std::ios::binary),
        stream(file),
        rollover_size(rollover_size + HEADER_SIZE)
    {
        auto path = (boost::filesystem::path(dirname) /
                     boost::filesystem::path(filename)).string();
        if (!boost::filesystem::exists(path))
        {
            file.open(path, std::ios::out | std::ios::app | std::ios::binary);
            file << std::setw(INDEX_SIZE) << UNINITIALIZED;
            file << std::setw(INDEX_SIZE) << UNINITIALIZED;
            file << std::setw(INDEX_SIZE) << UNINITIALIZED;
            file.flush();
            file.close();
            file.open(path, std::ios::out | std::ios::in | std::ios::binary);
        }
    }

    RolloverQueue(
        std::iostream& stream,
        std::streampos rollover_size=DEFAULT_ROLLOVER_SIZE
    ) : stream(stream),
        rollover_size(rollover_size + HEADER_SIZE)
    {
        stream.seekg(0, std::ios::end);
        if (stream.tellg() < HEADER_SIZE)
        {
            stream << std::setw(INDEX_SIZE) << UNINITIALIZED;
            stream << std::setw(INDEX_SIZE) << UNINITIALIZED;
            stream << std::setw(INDEX_SIZE) << UNINITIALIZED;
        }
    }

    void Enqueue(T e)
    {
        std::string element_as_string = Serialize<T>(e);
        auto element_length = static_cast<std::streampos>(element_as_string.length());

        std::streampos first_pointer = get_first_position();
        std::streampos last_pointer = get_last_position();
        std::streampos eof_pointer = get_eof_position();

        //
        // case 0: if first insert ever then insert into beginning of file
        //
        if (first_pointer == UNINITIALIZED && last_pointer == UNINITIALIZED &&
            eof_pointer == UNINITIALIZED)
        {
            stream.seekp(HEADER_SIZE, std::ios::beg);
            stream << element_as_string;

            stream.seekp(0 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << HEADER_SIZE;

            stream.seekp(1 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << HEADER_SIZE;

            stream.seekp(2 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << (HEADER_SIZE + element_length);
            stream.flush();
            return;
        }

        stream.seekg(last_pointer, std::ios::beg);
        auto last_element = Serialize<T>(Deserialize<T>(stream));
        auto last_element_length = static_cast<std::streampos>(last_element.length());

        //
        // case 1: if last < first end then reclaim space from first for next
        //         insert
        //
        if (last_pointer < first_pointer)
        {
            std::streampos insert_pointer = last_pointer + last_element_length;

            stream.seekg(first_pointer, std::ios::beg);
            while ((first_pointer - insert_pointer) < element_length)
            {
                stream.seekg(first_pointer, std::ios::beg);
                first_pointer += static_cast<std::streampos>(
                    Serialize<T>(Deserialize<T>(stream)).length());
            }

            if (first_pointer == eof_pointer)
            {
                // special case where we wrap around
                first_pointer = HEADER_SIZE;
            }

            // update first pointer
            stream.seekp(0 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << first_pointer;

            // update last pointer
            stream.seekp(1 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << insert_pointer;

            // insert the object
            stream.seekp(insert_pointer, std::ios::beg);
            stream << element_as_string;
        }


        //
        // case 2: if insert causes rollover then reclaim space at begining of
        //         file for next insert
        //
        else if (last_pointer + last_element_length + element_length >
                 rollover_size)
        {
            std::streampos reclaim_pointer = HEADER_SIZE;
            stream.seekg(reclaim_pointer, std::ios::beg);

            // calculate how much space at start to free
            auto reclaim_object = Serialize<T>(Deserialize<T>(stream));
            int reclaim_size = reclaim_object.length();
            while (reclaim_size < element_length)
            {
                reclaim_pointer += reclaim_object.length();

                stream.seekg(reclaim_pointer, std::ios::beg);
                reclaim_object = Serialize<T>(Deserialize<T>(stream));
            }

            // insert element at start since we are rolled over
            stream.seekp(HEADER_SIZE, std::ios::beg);
            stream << element_as_string;

            // update start pointer.
            if (first_pointer == last_pointer)
            {
                // special case, single element
                stream.seekp(0 * INDEX_SIZE, std::ios::beg);
                stream << std::setw(INDEX_SIZE) << HEADER_SIZE;

                // update eof pointer
                stream.seekp(2 * INDEX_SIZE, std::ios::beg);
                stream << std::setw(INDEX_SIZE) << first_pointer +
                    element_length;
            }
            else
            {
                // typical case
                stream.seekp(0 * INDEX_SIZE, std::ios::beg);
                stream << std::setw(INDEX_SIZE) << reclaim_pointer +
                    static_cast<std::streampos>(reclaim_object.length());

                // eof pointer stays the same.
            }

            // update last pointer.
            stream.seekp(1 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << HEADER_SIZE;
            stream.flush();
        }

        //
        // case 3: else insert into end of file and increment pointers
        //
        else
        {
            stream.seekg(last_pointer, std::ios::beg);
            auto insert_pointer = last_pointer + last_element_length;

            stream.seekp(insert_pointer, std::ios::beg);
            stream << element_as_string;

            // update last pointer
            stream.seekp(1 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << insert_pointer;

            // update eof pointer
            stream.seekp(2 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << insert_pointer + element_length;
            stream.flush();
        }
    }

    void Dequeue()
    {
        auto first = get_first_position();
        auto last = get_last_position();
        auto eof = get_eof_position();

        if (first == last)
        {
            stream.seekp(0, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << UNINITIALIZED;
            stream << std::setw(INDEX_SIZE) << UNINITIALIZED;
            stream << std::setw(INDEX_SIZE) << UNINITIALIZED;
            return;
        }
        else
        {
            stream.seekg(first, std::ios::beg);

            auto next = first + static_cast<std::streampos>(
                Serialize<T>(Deserialize<T>(stream)).length());

            stream.seekp(0 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << next;
        }
    }

    T Last()
    {
        auto position = get_last_position();
        if (position == UNINITIALIZED)
        {
            return T{};
        }

        stream.seekg(position, std::ios::beg);
        return Deserialize<T>(stream);
    }

    Iterator begin()
    {
        auto first = get_first_position();
        auto last = get_last_position();
        auto eof = get_eof_position();

        return Iterator(stream, first, first, last, eof);
    }

    Iterator end()
    {
        auto first = get_first_position();
        auto last = get_last_position();
        auto eof = get_eof_position();

        return Iterator(stream, first, UNINITIALIZED, last, eof);
    }
};


}


#endif
