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
            return UNINITIALIZED;
        }
    }

    std::streampos get_last_position()
    {
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
            return UNINITIALIZED;
        }
    }

    std::streampos get_eof_position()
    {
        stream.seekg(0, std::ios::end);
        return stream.tellg();
    }

    std::fstream file;

    std::iostream& stream;

    std::string filename;

    std::streampos rollover_size;

    constexpr static const int64_t INDEX_SIZE = 10;

    constexpr static const int64_t UNINITIALIZED = 0;

    constexpr static const int64_t HEADER_SIZE = INDEX_SIZE * 2;

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
            std::streampos position,
            std::streampos rollover
        )
            : stream(stream),
              position(position),
              rollover(rollover)
        {
        }

        T operator*()
        {
            stream.seekg(position, std::ios::beg);

            return Deserialize<T>(stream);
        }

        Iterator& operator++()
        {
            if (position <= rollover)
            {
                stream.seekg(position, std::ios::beg);
                position += Serialize<T>(Deserialize<T>(stream)).length();
            }
            else
            {
                position = HEADER_SIZE;
            }

            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return position != iterator.position;
        }

    private:

        std::iostream& stream;

        std::streampos position;

        std::streampos rollover;
    };

public:

    RolloverQueue(std::string filename)
        : RolloverQueue(".", filename)
    {
    }

    RolloverQueue(
        const RolloverQueue<T>&& rhs
    ) : file(rhs.filename,
             std::ios::out | std::ios::in | std::ios::binary),
        stream(rhs.stream),
        filename(rhs.filename),
        rollover_size(rhs.rollover_size)
    {
    }

    RolloverQueue<T>& operator=(RolloverQueue<T> rhs)
    {
        rollover_size = rhs.rollover_size;
        file.swap(rhs.file);
        filename = rhs.filename;
        return *this;
    }

    RolloverQueue(
        std::string dirname,
        std::string filename,
        std::streampos rollover_size=DEFAULT_ROLLOVER_SIZE
    ) : file((boost::filesystem::path(dirname) /
              boost::filesystem::path(filename)).string(),
               std::ios::out | std::ios::in | std::ios::binary),
        stream(file),
        filename((boost::filesystem::path(dirname) /
                  boost::filesystem::path(filename)).string()),
        rollover_size(rollover_size)
    {
        auto path = (boost::filesystem::path(dirname) /
                     boost::filesystem::path(filename)).string();
        if (!boost::filesystem::exists(path))
        {
            file.open(path, std::ios::out | std::ios::app | std::ios::binary);
            file << std::setw(INDEX_SIZE) << HEADER_SIZE;
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
        rollover_size(rollover_size)
    {
        stream.seekg(0, std::ios::end);
        if (stream.tellg() < HEADER_SIZE)
        {
            stream << std::setw(INDEX_SIZE) << HEADER_SIZE;
            stream << std::setw(INDEX_SIZE) << UNINITIALIZED;
        }
    }

    void Enqueue(T e)
    {
        std::string element_as_string = Serialize<T>(e);
        auto size = element_as_string.length();

        std::streampos position = get_last_position();
        if (position == UNINITIALIZED)
        {
            position = HEADER_SIZE;
        }
        else
        {
            stream.seekg(position, std::ios::beg);
            position += Serialize<T>(Deserialize<T>(stream)).length();
        }

        if (rollover_size < position + static_cast<std::streampos>(size))
        {
            // rollover the queue
            file.close();
            boost::filesystem::rename(filename, filename + ".0");
            file.open(filename, std::ios::out | std::ios::app | std::ios::binary);

            position = HEADER_SIZE;
            stream.seekp(0 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << position;
            stream.flush();

            stream.seekp(1 * INDEX_SIZE, std::ios::beg);
            stream << std::setw(INDEX_SIZE) << position;
            stream.flush();

            // flush element
            stream.seekp(position, std::ios::beg);
            stream << element_as_string;
            stream.flush();

            // hack to re-initialize std::iostream
            *this = RolloverQueue<T>(filename);
            return;
        }

        // flush element
        stream.seekp(position, std::ios::beg);
        stream << element_as_string;
        stream.flush();

        // update end position
        stream.seekp(1 * INDEX_SIZE, std::ios::beg);
        stream << std::setw(INDEX_SIZE) << position;
        stream.flush();
    }

    void Dequeue()
    {
        auto position = get_first_position();
        stream.seekg(position, std::ios::beg);

        auto next = position + static_cast<std::streampos>(
            Serialize<T>(Deserialize<T>(stream)).length());

        stream.seekp(0 * INDEX_SIZE, std::ios::beg);
        stream << std::setw(INDEX_SIZE) << next;
    }

    T Last()
    {
        auto position = get_last_position();

        stream.seekg(position, std::ios::beg);
        return Deserialize<T>(stream);
    }

    Iterator begin()
    {
        return Iterator(stream, get_first_position(), rollover_size);
    }

    Iterator end()
    {
        auto first = get_first_position();
        auto last = get_last_position();
        auto eof = get_eof_position();
        if (last == UNINITIALIZED || last == eof)
        {
            last = first;
        }
        else
        {
            stream.seekg(last, std::ios::beg);
            last += Serialize<T>(Deserialize<T>(stream)).length();
        }
        return Iterator(stream, last, rollover_size);
    }
};


}


#endif
