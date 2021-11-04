#ifndef DELETER_H_
#define DELETER_H_ 1

#include <functional>

template<class T> struct deleter_array : public std::unary_function<T, void>
{
    void operator()(T x) { delete[] x; }
};

template<class T> struct deleter_scalar : public std::unary_function<T, void>
{
    void operator()(T x) { delete x; }
};

#endif
