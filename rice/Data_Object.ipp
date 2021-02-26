#ifndef Rice__Data_Object__ipp_
#define Rice__Data_Object__ipp_

#include "protect.hpp"
#include "Data_Type_defn.hpp"
#include "detail/Caster.hpp"

#include <algorithm>

template <typename T>
Rice::Exception create_type_exception(VALUE value)
{
  return Rice::Exception(rb_eTypeError, "Wrong argument type. Expected: %s. Received: %s.",
    rb_class2name(Rice::Data_Type<T>::klass()), rb_obj_classname(value));
}

template<typename T>
inline Rice::Data_Object<T>::
Data_Object(T* data, Module klass)
{
  VALUE value = detail::wrap(klass, Data_Type<T>::rb_type(), data);
  this->set_value(value);
}

template<typename T>
inline Rice::Data_Object<T>::
Data_Object(Object value)
  : Object(value)
{  
  Data_Type<T> klass;
  check_ruby_type(value);
}

template<typename T>
template<typename U>
inline Rice::Data_Object<T>::
Data_Object(Object value)
  : Object(value)
{  
  check_ruby_type(value);
}

template<typename T>
inline void Rice::Data_Object<T>::
check_ruby_type(VALUE value)
{
  if (rb_obj_is_kind_of(value, Data_Type<T>::klass()) == Qfalse)
  {
    throw create_type_exception<T>(value);
  }
}

template<typename T>
inline T& Rice::Data_Object<T>::
operator*() const
{
  return *this->get();
}

template<typename T>
inline T* Rice::Data_Object<T>::
operator->() const
{
  return this->get();
}

template<typename T>
inline T* Rice::Data_Object<T>::
get() const
{
  if (this->value() == Qnil)
  {
    return nullptr;
  }
  else
  {
    return detail::unwrap<T>(this->value(), Data_Type<T>::rb_type());
  }
}

template<typename T>
inline T* Rice::Data_Object<T>::
from_ruby(VALUE value)
{
  if (Data_Type<T>::is_descendant(value))
  {
    return detail::unwrap<T>(value, Data_Type<T>::rb_type());;
  }
  else
  {
    return nullptr;
  }
}

template<typename T>
inline std::optional<T> Rice::Data_Object<T>::
implicit_from_ruby(VALUE value)
{
  VALUE from_klass = rb_class_of(value);
  VALUE to_klass = Data_Type<T>::klass();

  detail::CasterAbstract<T>* caster = detail::CasterRegistry::find<T>(from_klass, to_klass);
  if (!caster)
  {
    return std::nullopt;
  }
  else
  {
    // Directly get value skipping any type checking since we know that value.instance_of?(T) is false
    void* data = detail::unwrap(value);
    // TODO - this will cause crashes
    return caster->cast(data);
  }
}

template<typename T, typename>
struct Rice::detail::To_Ruby
{
  static VALUE convert(T&& data, bool takeOwnership)
  {
    using Base_T = base_type<T>;
    Data_Type<Base_T>::check_is_bound();
    return detail::wrap(Data_Type<Base_T>::klass(), Data_Type<Base_T>::rb_type(), std::forward<T>(data), takeOwnership);
  }
};

template <typename T>
struct Rice::detail::To_Ruby<T*, std::enable_if_t<!Rice::detail::is_primitive_v<T> &&
  !Rice::detail::is_kind_of_object<T>>>
{
  static VALUE convert(T* data, bool takeOwnership)
  {
    using Base_T = base_type<T>;
    return detail::wrap(Data_Type<Base_T>::klass(), Data_Type<Base_T>::rb_type(), data, takeOwnership);
  }
};

template <typename T>
struct Rice::detail::From_Ruby
{
  static T convert(VALUE value)
  {
    using Base_T = base_type<T>;
    Base_T* result = Data_Object<Base_T>::from_ruby(value);
    if (result)
    {
      return *result;
    }

    if constexpr (std::is_copy_constructible_v<Base_T>)
    {
      std::optional<Base_T> implicit_result = Data_Object<Base_T>::implicit_from_ruby(value);
      if (implicit_result)
      {
        return implicit_result.value();
      }
    }

    throw create_type_exception<Base_T>(value);
  }
};
 
template<typename T>
struct Rice::detail::From_Ruby<T&>
{
  static T& convert(VALUE value)
  {
    using Base_T = base_type<T>;
    Base_T* result = Data_Object<Base_T>::from_ruby(value);
    if (result)
    {
      return *result;
    }

    if constexpr (std::is_copy_constructible_v<Base_T>)
    {
      std::optional<Base_T> implicit_result = Data_Object<Base_T>::implicit_from_ruby(value);
      if (implicit_result)
      {
        return implicit_result.value();
      }
    }

    throw create_type_exception<Base_T>(value);
  }
};

template<typename T>
struct Rice::detail::From_Ruby<T*>
{
  static T* convert(VALUE value)
  {
    using Base_T = base_type<T>;
    Base_T* result = Data_Object<Base_T>::from_ruby(value);
    if (result)
    {
      return result;
    }

    if constexpr (std::is_copy_constructible_v<Base_T>)
    {
      std::optional<Base_T> implicit_result = Data_Object<Base_T>::implicit_from_ruby(value);
      if (implicit_result)
      {
        // TODO - Memory Leak
        return new Base_T(implicit_result.value());
      }
    }

    throw create_type_exception<Base_T>(value);
  }
};
#endif // Rice__Data_Object__ipp_

