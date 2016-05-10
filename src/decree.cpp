#include <decree.hpp>


bool
Decree::operator<(const Decree& rhs)
{
    int lhs_number = GetNumber();
    int rhs_number = rhs.GetNumber();
    return lhs_number != rhs_number ? lhs_number < rhs_number
        : GetAuthor() < rhs.GetAuthor();
}


std::string
Decree::GetAuthor() const
{
    return author;
}


int
Decree::GetNumber() const
{
    return number;
}


std::string
Decree::GetContent() const
{
    return content;
}


void
Decree::SetContent(std::string the_content)
{
    content = the_content;
}
