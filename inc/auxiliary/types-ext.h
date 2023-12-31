#ifndef TYPES_EXT_H
#define TYPES_EXT_H

#include <functional>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>

namespace my
{
  namespace optional
  {
    template <typename T> std::string to_string(std::optional<T> const& opt)
    {
      std::stringstream ss;
      ss << "{ ";
      if (opt)
        ss << *opt;
      ss << " }";
      return ss.str();
    }
  } // namespace optional
  namespace variant
  {
    // gets a reference to an alternative T if the variant holds it, or an empty
    // optional otherwise
    // reference wrapper is const, so it cannot be reassigned
    template <class T, class... Types>
    std::optional<std::reference_wrapper<T> const> get_ref(
        std::variant<Types...>& v)
    {
      T* const rv = std::get_if<T>(&v);
      if (rv == nullptr)
        return {};
      return std::ref(*rv);
    }

    // gets a const reference to an alternative T if the variant holds it, or an
    // empty optional otherwise reference is const, so it cannot be reassigned
    template <class T, class... Types>
    std::optional<std::reference_wrapper<T const> const> get_cref(
        std::variant<Types...> const& v)
    {
      T const* const rv = std::get_if<T>(&v);
      if (rv == nullptr)
        return {};
      return std::cref(*rv);
    }

    // (https://en.cppreference.com/w/cpp/utility/variant/visit)
    // helper type for std::visitor
    // allows packaging of lambdas without creating a functor struct:
    // visitor { lambda1, lambda2, ... }
    template <class... Ts> struct visitor : Ts...
    {
      using Ts::operator()...;
    };
    // explicit deduction guide (not needed as of C++20)
    template <class... Ts> visitor(Ts...) -> visitor<Ts...>;

  } // namespace variant
} // namespace my

#endif // TYPES_EXT_H
