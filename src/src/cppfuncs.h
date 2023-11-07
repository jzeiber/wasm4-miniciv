#pragma once
/*
	ver 2023-09-23
*/

#include <stddef.h>

template<size_t N, class T>
constexpr size_t countof(T(&)[N]) { return N; }

namespace wasm4
{

template<class T>
constexpr T min(T val1, T val2)
{
	return (val1<val2) ? val1 : val2;
}

template<class T>
constexpr T max(T val1, T val2)
{
	return (val2<val1) ? val1 : val2;
}

}