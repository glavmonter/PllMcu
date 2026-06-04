#pragma once

template<bool condition, class TypeIfTrue, class TypeIfFalse>
struct StaticIf
{
    typedef TypeIfTrue Result;
};

template<class TypeIfTrue, class TypeIfFalse>
struct StaticIf<false, TypeIfTrue, TypeIfFalse>
{
     typedef TypeIfFalse Result;
};
