#include <string>


class Decree
{
public:

    std::string GetAuthor() const;

    int GetNumber() const;

    std::string GetContent() const;

    void SetContent(std::string content);

    bool operator<(const Decree & rhs);

private:

    std::string author;

    int number;

    std::string content;
};
