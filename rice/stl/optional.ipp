#include <optional>

namespace Rice::detail
{
  template <typename T>
  struct is_builtin<std::optional<T>> : public std::true_type {};

  template<>
  struct To_Ruby<std::nullopt_t>
  {
    static VALUE convert(std::nullopt_t& _, bool takeOwnership = false)
    {
      return Qnil;
    }
  };

  template<typename T>
  struct To_Ruby<std::optional<T>>
  {
    static VALUE convert(std::optional<T>& data, bool takeOwnership = false)
    {
      if (data.has_value())
      {
        return To_Ruby<T>::convert(data.value(), takeOwnership);
      }
      else
      {
        return Qnil;
      }
    }
  };

  template<typename T>
  struct From_Ruby<std::optional<T>>
  {
    static std::optional<T> convert(VALUE value)
    {
      if (value == Qnil)
      {
        return std::nullopt;
      }
      else
      {
        return From_Ruby<T>::convert(value);
      }
    }
  };
}