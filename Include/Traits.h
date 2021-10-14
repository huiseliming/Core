#pragma once


template <typename T>
struct TIsFloatingPoint
{
	enum { Value = false };
};

template <> struct TIsFloatingPoint<float>       { enum { Value = true }; };
template <> struct TIsFloatingPoint<double>      { enum { Value = true }; };
template <> struct TIsFloatingPoint<long double> { enum { Value = true }; };

template <typename T> struct TIsFloatingPoint<const          T> { enum { Value = TIsFloatingPoint<T>::Value }; };
template <typename T> struct TIsFloatingPoint<      volatile T> { enum { Value = TIsFloatingPoint<T>::Value }; };
template <typename T> struct TIsFloatingPoint<const volatile T> { enum { Value = TIsFloatingPoint<T>::Value }; };

template <typename T>
struct TIsIntegral
{
	enum { Value = false };
};

template <> struct TIsIntegral<         bool>      { enum { Value = true }; };
template <> struct TIsIntegral<         char>      { enum { Value = true }; };
template <> struct TIsIntegral<signed   char>      { enum { Value = true }; };
template <> struct TIsIntegral<unsigned char>      { enum { Value = true }; };
template <> struct TIsIntegral<         char16_t>  { enum { Value = true }; };
template <> struct TIsIntegral<         char32_t>  { enum { Value = true }; };
template <> struct TIsIntegral<         wchar_t>   { enum { Value = true }; };
template <> struct TIsIntegral<         short>     { enum { Value = true }; };
template <> struct TIsIntegral<unsigned short>     { enum { Value = true }; };
template <> struct TIsIntegral<         int>       { enum { Value = true }; };
template <> struct TIsIntegral<unsigned int>       { enum { Value = true }; };
template <> struct TIsIntegral<         long>      { enum { Value = true }; };
template <> struct TIsIntegral<unsigned long>      { enum { Value = true }; };
template <> struct TIsIntegral<         long long> { enum { Value = true }; };
template <> struct TIsIntegral<unsigned long long> { enum { Value = true }; };

template <typename T> struct TIsIntegral<const          T> { enum { Value = TIsIntegral<T>::Value }; };
template <typename T> struct TIsIntegral<      volatile T> { enum { Value = TIsIntegral<T>::Value }; };
template <typename T> struct TIsIntegral<const volatile T> { enum { Value = TIsIntegral<T>::Value }; };