#pragma once

template<typename It>
struct range
{
    It begin_;
    It end_;

    It begin() const { return begin_; };
    It end() const { return end_; };
};

template<typename It>
range<It> make_range(It begin, It end)
{
    return range<It>{begin, end};
}
