#ifndef Rice__stl__hpp_
#define Rice__stl__hpp_


// =========   string.hpp   =========


// ---------   string.ipp   ---------
#include <string>

namespace Rice::detail
{
  template<>
  struct Type<std::string>
  {
    static bool verify()
    {
      return true;
    }
  };

  template<>
  class To_Ruby<std::string>
  {
  public:
    VALUE convert(std::string const& x)
    {
      return detail::protect(rb_external_str_new, x.data(), (long)x.size());
    }
  };

  template<>
  class To_Ruby<std::string&>
  {
  public:
    VALUE convert(std::string const& x)
    {
      return detail::protect(rb_external_str_new, x.data(), (long)x.size());
    }
  };

  template<>
  class From_Ruby<std::string>
  {
  public:
    From_Ruby() = default;

    explicit From_Ruby(Arg* arg) : arg_(arg)
    {
    }

    std::string convert(VALUE value)
    {
      if (value == Qnil && this->arg_ && this->arg_->hasDefaultValue())
      {
        return this->arg_->defaultValue<std::string>();
      }
      else
      {
        detail::protect(rb_check_type, value, (int)T_STRING);
        return std::string(RSTRING_PTR(value), RSTRING_LEN(value));
      }
    }

  private:
    Arg* arg_ = nullptr;
  };

  template<>
  class From_Ruby<std::string*>
  {
  public:
    std::string* convert(VALUE value)
    {
      detail::protect(rb_check_type, value, (int)T_STRING);
      this->converted_ = std::string(RSTRING_PTR(value), RSTRING_LEN(value));
      return &this->converted_;
    }

  private:
    std::string converted_;
  };

  template<>
  class From_Ruby<std::string&>
  {
  public:
    From_Ruby() = default;

    explicit From_Ruby(Arg* arg) : arg_(arg)
    {
    }

    std::string& convert(VALUE value)
    {
      if (value == Qnil && this->arg_ && this->arg_->hasDefaultValue())
      {
        return this->arg_->defaultValue<std::string>();
      }
      else
      {
        detail::protect(rb_check_type, value, (int)T_STRING);
        this->converted_ = std::string(RSTRING_PTR(value), RSTRING_LEN(value));
        return this->converted_;
      }
    }

  private:
    Arg* arg_ = nullptr;
    std::string converted_;
  };
}

// =========   complex.hpp   =========


// ---------   complex.ipp   ---------
#include <complex>


namespace Rice::detail
{
  template<typename T>
  struct Type<std::complex<T>>
  {
    static bool verify()
    {
      return true;
    }
  };

  template<typename T>
  class To_Ruby<std::complex<T>>
  {
  public:
    VALUE convert(const std::complex<T>& data)
    {
      std::vector<VALUE> args(2);
      args[0] = To_Ruby<T>().convert(data.real());
      args[1] = To_Ruby<T>().convert(data.imag());
      return protect(rb_funcall2, rb_mKernel, rb_intern("Complex"), (int)args.size(), (const VALUE*)args.data());
    }
  };

  template<typename T>
  class From_Ruby<std::complex<T>>
  {
  public:
    std::complex<T> convert(VALUE value)
    {
      VALUE real = protect(rb_funcall2, value, rb_intern("real"), 0, (const VALUE*)nullptr);
      VALUE imaginary = protect(rb_funcall2, value, rb_intern("imaginary"), 0, (const VALUE*)nullptr);

      return std::complex<T>(From_Ruby<T>().convert(real), From_Ruby<T>().convert(imaginary));
    }
  };

  template<typename T>
  class From_Ruby<std::complex<T>&>
  {
  public:
    std::complex<T>& convert(VALUE value)
    {
      VALUE real = protect(rb_funcall2, value, rb_intern("real"), 0, (const VALUE*)nullptr);
      VALUE imaginary = protect(rb_funcall2, value, rb_intern("imaginary"), 0, (const VALUE*)nullptr);
      this->converted_ = std::complex<T>(From_Ruby<T>().convert(real), From_Ruby<T>().convert(imaginary));

      return this->converted_;
    }

  private:
    std::complex<T> converted_;
  };
}

// =========   map.hpp   =========


namespace Rice
{
  template<typename U>
  Data_Type<U> define_map(std::string name);

  template<typename U>
  Data_Type<U> define_map_under(Object module, std::string name);
}


// ---------   map.ipp   ---------

#include <sstream>
#include <stdexcept>
#include <map>

namespace Rice
{
  namespace stl
  {
    template<typename T>
    class MapHelper
    {
      using Key_T = typename T::key_type;
      using Mapped_T = typename T::mapped_type;
      using Value_T = typename T::value_type;
      using Size_T = typename T::size_type;
      using Difference_T = typename T::difference_type;

    public:
      MapHelper(Data_Type<T> klass) : klass_(klass)
      {
        this->define_constructor();
        this->define_copyable_methods();
        this->define_capacity_methods();
        this->define_access_methods();
        this->define_comparable_methods();
        this->define_modify_methods();
        this->define_to_s();
        this->define_enumerable();
      }

    private:

      void define_constructor()
      {
        klass_.define_constructor(Constructor<T>());
      }

      void define_copyable_methods()
      {
        if constexpr (std::is_copy_constructible_v<Value_T>)
        {
          klass_.define_method("copy", [](T& self) -> T
            {
              return self;
            });
        }
        else
        {
          klass_.define_method("copy", [](T& self) -> T
            {
              throw std::runtime_error("Cannot copy maps with non-copy constructible types");
              return self;
            });
        }
      }

      void define_capacity_methods()
      {
        klass_.define_method("empty?", &T::empty)
          .define_method("max_size", &T::max_size)
          .define_method("size", &T::size);
        
        rb_define_alias(klass_, "count", "size");
        rb_define_alias(klass_, "length", "size");
      }

      void define_access_methods()
      {
        // Access methods
        klass_.define_method("[]", [](const T& self, const Key_T& key) -> std::optional<Mapped_T>
          {
            auto iter = self.find(key);

            if (iter != self.end())
            {
              return iter->second;
            }
            else
            {
              return std::nullopt;
            }
          })
          .define_method("include?", [](T& self, Key_T& key) -> bool
          {
              return self.find(key) != self.end();
          })
          .define_method("keys", [](T& self) -> std::vector<Key_T>
            {
              std::vector<Key_T> result;
              std::transform(self.begin(), self.end(), std::back_inserter(result),
                [](const auto& pair)
                {
                  return pair.first;
                });

              return result;
            })
          .define_method("values", [](T& self) -> std::vector<Mapped_T>
            {
              std::vector<Mapped_T> result;
              std::transform(self.begin(), self.end(), std::back_inserter(result),
                [](const auto& pair)
                {
                  return pair.second;
                });

              return result;
            });

            rb_define_alias(klass_, "has_key", "include?");
      }

      // Methods that require Value_T to support operator==
      void define_comparable_methods()
      {
        if constexpr (detail::is_comparable_v<Mapped_T>)
        {
          klass_.define_method("value?", [](T& self, Mapped_T& value) -> bool
            {
              auto it = std::find_if(self.begin(), self.end(),
              [&value](auto& pair)
                {
                  return pair.second == value;
                });

              return it != self.end();
          });
        }
        else
        {
          klass_.define_method("value?", [](T& self, Mapped_T& value) -> bool
          {
              return false;
          });
        }

        rb_define_alias(klass_, "has_value", "value?");
      }

      void define_modify_methods()
      {
        klass_.define_method("clear", &T::clear)
          .define_method("delete", [](T& self, Key_T& key) -> std::optional<Mapped_T>
            {
              auto iter = self.find(key);

              if (iter != self.end())
              {
                Mapped_T result = iter->second;
                self.erase(iter);
                return result;
              }
              else
              {
                return std::nullopt;
              }
            })
          .define_method("[]=", [](T& self, Key_T key, Mapped_T value) -> Mapped_T
            {
              self[key] = value;
              return value;
            });

          rb_define_alias(klass_, "store", "[]=");
      }

      void define_enumerable()
      {
        // Add enumerable support
        klass_.include_module(rb_mEnumerable)
          .define_method("each", [](T& self) -> const T&
            {
              for (Value_T& pair : self)
              {
                VALUE key = detail::To_Ruby<Key_T>().convert(pair.first);
                VALUE value = detail::To_Ruby<Mapped_T>().convert(pair.second);
                VALUE result = rb_ary_new3(2, key, value);
                rb_yield(result);
              }
              return self;
            });
      }

      void define_to_hash()
      {
        // Add enumerable support
        klass_.define_method("to_h", [](T& self)
        {
          VALUE result = rb_hash_new();
          std::for_each(self.begin(), self.end(), [&result](const auto& pair)
          {
            VALUE key = detail::To_Ruby<Key_T>().convert(pair->first);
            VALUE value = detail::To_Ruby<Mapped_T>().convert(pair->second);
            rb_hash_aset(result, key, value);
          });

          return result;
        });
      }

      void define_to_s()
      {
        if constexpr (detail::is_ostreamable_v<Key_T> && detail::is_ostreamable_v<Mapped_T>)
        {
          klass_.define_method("to_s", [](const T& self)
            {
              auto iter = self.begin();

              std::stringstream stream;
              stream << "{";

              for (; iter != self.end(); iter++)
              {
                if (iter != self.begin())
                {
                  stream << ", ";
                }
                stream << iter->first << " => " << iter->second;
              }

              stream << "}";
              return stream.str();
            });
        }
        else
        {
          klass_.define_method("to_s", [](const T& self)
            {
              return "[Not printable]";
            });
        }
      }

      private:
        Data_Type<T> klass_;
    };
  } // namespace

  template<typename T>
  Data_Type<T> define_map_under(Object module, std::string name)
  {
    if (detail::TypeRegistry::isDefined<T>())
    {
      // If the map has been previously seen it will be registered but may
      // not be associated with the constant Module::<name>
      module.const_set_maybe(name, Data_Type<T>().klass());
      return Data_Type<T>();
    }

    Data_Type<T> result = define_class_under<detail::intrinsic_type<T>>(module, name.c_str());
    stl::MapHelper helper(result);
    return result;
  }

  template<typename T>
  Data_Type<T> define_map(std::string name)
  {
    if (detail::TypeRegistry::isDefined<T>())
    {
      // If the map has been previously seen it will be registered but may
      // not be associated with the constant Object::<name>
      Object(rb_cObject).const_set_maybe(name, Data_Type<T>().klass());
      return Data_Type<T>();
    }

    Data_Type<T> result = define_class<detail::intrinsic_type<T>>(name.c_str());
    stl::MapHelper<T> helper(result);
    return result;
  }

  template<typename T>
  Data_Type<T> define_map_auto()
  {
    std::string klassName = detail::makeClassName(typeid(T));
    Module rb_mRice = define_module("Rice");
    Module rb_mmap = define_module_under(rb_mRice, "Std");
    return define_map_under<T>(rb_mmap, klassName);
  }
   
  namespace detail
  {
    template<typename T, typename U>
    struct Type<std::map<T, U>>
    {
      static bool verify()
      {
        Type<T>::verify();
        Type<U>::verify();

        if (!detail::TypeRegistry::isDefined<std::map<T, U>>())
        {
          define_map_auto<std::map<T, U>>();
        }

        return true;
      }
    };

    template<typename T, typename U>
    int mapFromHashPair(VALUE key, VALUE value, VALUE user_data)
    {
      std::map<T, U> *map = (std::map<T, U>*)(user_data);

      // This method is being called from Ruby so we cannot let any C++ 
      // exceptions propogate back to Ruby
      return cpp_protect([&]
        {
          map->operator[](From_Ruby<T>().convert(key)) = From_Ruby<U>().convert(value);
          return ST_CONTINUE;
        });
    }

    template<typename T, typename U>
    std::map<T, U> mapFromHash(VALUE value)
    {
      std::map<T, U> result;
      VALUE user_data = (VALUE)(&result);
      detail::protect<void>(rb_hash_foreach, value, mapFromHashPair<T, U>, user_data);
      rb_hash_foreach(value, mapFromHashPair<T, U>, user_data);

      return result;
    }

    template<typename T, typename U>
    class From_Ruby<std::map<T, U>>
    {
    public:
      From_Ruby() = default;

      explicit From_Ruby(Arg * arg) : arg_(arg)
      {
      }

      std::map<T, U> convert(VALUE value)
      {
        switch (rb_type(value))
        {
          case T_DATA:
          {
            // This is a wrapped map (hopefully!)
            return *Data_Object<std::map<T, U>>::from_ruby(value);
          }
          case T_HASH:
          {
            // If this an Ruby hash and the mapped type is copyable
            if constexpr (std::is_default_constructible_v<U>)
            {
              return mapFromHash<T, U>(value);
            }
          }
          case T_NIL:
          {
            if (this->arg_ && this->arg_->hasDefaultValue())
            {
              return this->arg_->template defaultValue<std::map<T, U>>();
            }
          }
          default:
          {
            throw Exception(rb_eTypeError, "wrong argument type %s (expected % s)",
              detail::protect(rb_obj_classname, value), "std::map");
          }
        }
      }

    private:
      Arg* arg_ = nullptr;
    };

    template<typename T, typename U>
    class From_Ruby<std::map<T, U>&>
    {
    public:
      From_Ruby() = default;

      explicit From_Ruby(Arg * arg) : arg_(arg)
      {
      }

      std::map<T, U>& convert(VALUE value)
      {
        switch (rb_type(value))
        {
          case T_DATA:
          {
            // This is a wrapped map (hopefully!)
            return *Data_Object<std::map<T, U>>::from_ruby(value);
          }
          case T_HASH:
          {
            // If this an Ruby array and the map type is copyable
            if constexpr (std::is_default_constructible_v<std::map<T, U>>)
            {
              this->converted_ = mapFromHash<T, U>(value);
              return this->converted_;
            }
          }
          case T_NIL:
          {
            if (this->arg_ && this->arg_->hasDefaultValue())
            {
              return this->arg_->template defaultValue<std::map<T, U>>();
            }
          }
          default:
          {
            throw Exception(rb_eTypeError, "wrong argument type %s (expected % s)",
              detail::protect(rb_obj_classname, value), "std::map");
          }
        }
      }

    private:
      Arg* arg_ = nullptr;
      std::map<T, U> converted_;
    };

    template<typename T, typename U>
    class From_Ruby<std::map<T, U>*>
    {
    public:
      std::map<T, U>* convert(VALUE value)
      {
        switch (rb_type(value))
        {
          case T_DATA:
          {
            // This is a wrapped map (hopefully!)
            return Data_Object<std::map<T, U>>::from_ruby(value);
          }
          case T_HASH:
          {
            // If this an Ruby array and the map type is copyable
            if constexpr (std::is_default_constructible_v<U>)
            {
              this->converted_ = mapFromHash<T, U>(value);
              return &this->converted_;
            }
          }
          default:
          {
            throw Exception(rb_eTypeError, "wrong argument type %s (expected % s)",
              detail::protect(rb_obj_classname, value), "std::map");
          }
        }
      }

    private:
      std::map<T, U> converted_;
    };
  }
}

// =========   optional.hpp   =========


// ---------   optional.ipp   ---------
#include <optional>

namespace Rice::detail
{
  template<typename T>
  struct Type<std::optional<T>>
  {
    constexpr static bool verify()
    {
      return Type<T>::verify();
    }
  };

  template<>
  class To_Ruby<std::nullopt_t>
  {
  public:
    VALUE convert(std::nullopt_t& _)
    {
      return Qnil;
    }
  };

  template<typename T>
  class To_Ruby<std::optional<T>>
  {
  public:
    static VALUE convert(std::optional<T>& data, bool takeOwnership = false)
    {
      if (data.has_value())
      {
        return To_Ruby<T>().convert(data.value());
      }
      else
      {
        return Qnil;
      }
    }
  };

  template<typename T>
  class To_Ruby<std::optional<T>&>
  {
  public:
    static VALUE convert(std::optional<T>& data, bool takeOwnership = false)
    {
      if (data.has_value())
      {
        return To_Ruby<T>().convert(data.value());
      }
      else
      {
        return Qnil;
      }
    }
  };

  template<typename T>
  class From_Ruby<std::optional<T>>
  {
  public:
    std::optional<T> convert(VALUE value)
    {
      if (value == Qnil)
      {
        return std::nullopt;
      }
      else
      {
        return From_Ruby<T>().convert(value);
      }
    }
  };

  template<typename T>
  class From_Ruby<std::optional<T>&>
  {
  public:
    std::optional<T>& convert(VALUE value)
    {
      if (value == Qnil)
      {
        this->converted_ = std::nullopt;
      }
      else
      {
        this->converted_ = From_Ruby<T>().convert(value);
      }
      return this->converted_;
    }
    
  private:
    std::optional<T> converted_;
  };
}

// =========   pair.hpp   =========


namespace Rice
{
  template<typename T>
  Data_Type<T> define_pair(std::string name);

  template<typename T>
  Data_Type<T> define_pair_under(Object module, std::string name);
}


// ---------   pair.ipp   ---------

#include <sstream>
#include <stdexcept>
#include <utility>

namespace Rice
{
  namespace stl
  {
    template<typename T>
    class PairHelper
    {
    public:
      PairHelper(Data_Type<T> klass) : klass_(klass)
      {
        this->define_constructor();
        this->define_copyable_methods();
        this->define_access_methods();
        this->define_modify_methods();
        this->define_to_s();
      }

    private:
      void define_constructor()
      {
        klass_.define_constructor(Constructor<T, typename T::first_type&, typename T::second_type&>());
      }

      void define_copyable_methods()
      {
        if constexpr (std::is_copy_constructible_v<typename T::first_type> && std::is_copy_constructible_v<typename T::second_type>)
        {
          klass_.define_method("copy", [](T& self) -> T
            {
              return self;
            });
        }
        else
        {
          klass_.define_method("copy", [](T& self) -> T
            {
              throw std::runtime_error("Cannot copy pair with non-copy constructible types");
              return self;
            });
        }
      }

      void define_access_methods()
      {
        // Access methods
        klass_.define_method("first", [](T& self) -> typename T::first_type&
          {
            return self.first;
          })
        .define_method("second", [](T& self) -> typename T::second_type&
          {
            return self.second;
          });
      }

      void define_modify_methods()
      {
        // Access methods
        klass_.define_method("first=", [](T& self, typename T::first_type& value) -> typename T::first_type&
          {
            self.first = value;
            return self.first;
          })
        .define_method("second=", [](T& self, typename T::second_type& value) -> typename T::second_type&
          {
            self.second = value;
            return self.second;
          });
      }

      void define_to_s()
      {
        if constexpr (detail::is_ostreamable_v<typename T::first_type> && detail::is_ostreamable_v<typename T::second_type>)
        {
          klass_.define_method("to_s", [](const T& self)
            {
              std::stringstream stream;
              stream << "[" << self.first << ", " << self.second << "]";
              return stream.str();
            });
        }
        else
        {
          klass_.define_method("to_s", [](const T& self)
            {
              return "[Not printable]";
            });
        }
      }

      private:
        Data_Type<T> klass_;
    };
  } // namespace

  template<typename T>
  Data_Type<T> define_pair_under(Object module, std::string name)
  {
    if (detail::TypeRegistry::isDefined<T>())
    {
      // If the pair has been previously seen it will be registered but may
      // not be associated with the constant Module::<name>
      module.const_set_maybe(name, Data_Type<T>().klass());
      return Data_Type<T>();
    }

    Data_Type<T> result = define_class_under<detail::intrinsic_type<T>>(module, name.c_str());
    stl::PairHelper helper(result);
    return result;
  }

  template<typename T>
  Data_Type<T> define_pair(std::string name)
  {
    if (detail::TypeRegistry::isDefined<T>())
    {
      // If the pair has been previously seen it will be registered but may
      // not be associated with the constant Object::<name>
      Object(rb_cObject).const_set_maybe(name, Data_Type<T>().klass());
      return Data_Type<T>();
    }

    Data_Type<T> result = define_class<detail::intrinsic_type<T>>(name.c_str());
    stl::PairHelper<T> helper(result);
    return result;
  }

  template<typename T>
  Data_Type<T> define_pair_auto()
  {
    std::string klassName = detail::makeClassName(typeid(T));
    Module rb_mRice = define_module("Rice");
    Module rb_mpair = define_module_under(rb_mRice, "Std");
    return define_pair_under<T>(rb_mpair, klassName);
  }
   
  namespace detail
  {
    template<typename T1, typename T2>
    struct Type<std::pair<T1, T2>>
    {
      static bool verify()
      {
        Type<T1>::verify();
        Type<T2>::verify();

        if (!detail::TypeRegistry::isDefined<std::pair<T1, T2>>())
        {
          define_pair_auto<std::pair<T1, T2>>();
        }

        return true;
      }
    };
  }
}



// =========   smart_ptr.hpp   =========


namespace Rice::detail
{
  template <template <typename, typename...> typename SmartPointer_T, typename...Arg_Ts>
  class WrapperSmartPointer : public Wrapper
  {
  public:
    WrapperSmartPointer(SmartPointer_T<Arg_Ts...>& data);
    void* get() override;
    SmartPointer_T<Arg_Ts...>& data();

  private:
    SmartPointer_T<Arg_Ts...> data_;
  };
}


// ---------   smart_ptr.ipp   ---------

#include <assert.h>
#include <memory>

namespace Rice::detail
{
  // ---- WrapperSmartPointer ------
  template <template <typename, typename...> typename SmartPointer_T, typename...Arg_Ts>
  inline WrapperSmartPointer<SmartPointer_T, Arg_Ts...>::WrapperSmartPointer(SmartPointer_T<Arg_Ts...>& data) 
    : data_(std::move(data))
  {
  }

  template <template <typename, typename...> typename SmartPointer_T, typename...Arg_Ts>
  inline void* WrapperSmartPointer<SmartPointer_T, Arg_Ts...>::get()
  {
    return (void*)this->data_.get();
  }

  template <template <typename, typename...> typename SmartPointer_T, typename...Arg_Ts>
  inline SmartPointer_T<Arg_Ts...>& WrapperSmartPointer<SmartPointer_T, Arg_Ts...>::data()
  {
    return data_;
  }

  // ---- unique_ptr ------
  template <typename T>
  class To_Ruby<std::unique_ptr<T>>
  {
  public:
    VALUE convert(std::unique_ptr<T>& data)
    {
      std::pair<VALUE, rb_data_type_t*> rubyTypeInfo = detail::TypeRegistry::figureType<T>(*data);

      // Use custom wrapper type 
      using Wrapper_T = WrapperSmartPointer<std::unique_ptr, T>;
      return detail::wrap<std::unique_ptr<T>, Wrapper_T>(rubyTypeInfo.first, rubyTypeInfo.second, data, true);
    }
  };

  template <typename T>
  class From_Ruby<std::unique_ptr<T>&>
  {
  public:
    std::unique_ptr<T>& convert(VALUE value)
    {
      Wrapper* wrapper = detail::getWrapper(value, Data_Type<T>::ruby_data_type());

      using Wrapper_T = WrapperSmartPointer<std::unique_ptr, T>;
      Wrapper_T* smartWrapper = dynamic_cast<Wrapper_T*>(wrapper);
      if (!smartWrapper)
      {
        std::string message = "Invalid smart pointer wrapper";
        throw std::runtime_error(message.c_str());
      }
      return smartWrapper->data();
    }
  };

  template<typename T>
  struct Type<std::unique_ptr<T>>
  {
    static bool verify()
    {
      return Type<T>::verify();
    }
  };

  // ----- shared_ptr -------------
  template <typename T>
  class To_Ruby<std::shared_ptr<T>>
  {
  public:
    VALUE convert(std::shared_ptr<T>& data)
    {
      std::pair<VALUE, rb_data_type_t*> rubyTypeInfo = detail::TypeRegistry::figureType<T>(*data);

      // Use custom wrapper type 
      using Wrapper_T = WrapperSmartPointer<std::shared_ptr, T>;
      return detail::wrap<std::shared_ptr<T>, Wrapper_T>(rubyTypeInfo.first, rubyTypeInfo.second, data, true);
    }
  };

  template <typename T>
  class From_Ruby<std::shared_ptr<T>>
  {
  public:
    std::shared_ptr<T> convert(VALUE value)
    {
      Wrapper* wrapper = detail::getWrapper(value, Data_Type<T>::ruby_data_type());

      using Wrapper_T = WrapperSmartPointer<std::shared_ptr, T>;
      Wrapper_T* smartWrapper = dynamic_cast<Wrapper_T*>(wrapper);
      if (!smartWrapper)
      {
        std::string message = "Invalid smart pointer wrapper";
          throw std::runtime_error(message.c_str());
      }
      return smartWrapper->data();
    }
  };

  template <typename T>
  class From_Ruby<std::shared_ptr<T>&>
  {
  public:
    std::shared_ptr<T>& convert(VALUE value)
    {
      Wrapper* wrapper = detail::getWrapper(value, Data_Type<T>::ruby_data_type());

      using Wrapper_T = WrapperSmartPointer<std::shared_ptr, T>;
      Wrapper_T* smartWrapper = dynamic_cast<Wrapper_T*>(wrapper);
      if (!smartWrapper)
      {
        std::string message = "Invalid smart pointer wrapper";
          throw std::runtime_error(message.c_str());
      }
      return smartWrapper->data();
    }
  };

  template<typename T>
  struct Type<std::shared_ptr<T>>
  {
    static bool verify()
    {
      return Type<T>::verify();
    }
  };
}

// =========   vector.hpp   =========


namespace Rice
{
  template<typename T>
  Data_Type<T> define_vector(std::string name);

  template<typename T>
  Data_Type<T> define_vector_under(Object module, std::string name);
}


// ---------   vector.ipp   ---------

#include <sstream>
#include <stdexcept>
#include <vector>

namespace Rice
{
  namespace stl
  {
    template<typename T>
    class VectorHelper
    {
      using Value_T = typename T::value_type;
      using Size_T = typename T::size_type;
      using Difference_T = typename T::difference_type;

    public:
      VectorHelper(Data_Type<T> klass) : klass_(klass)
      {
        this->define_constructor();
        this->define_copyable_methods();
        this->define_constructable_methods();
        this->define_capacity_methods();
        this->define_access_methods();
        this->define_comparable_methods();
        this->define_modify_methods();
        this->define_to_s();
        this->define_enumerable();
      }

    private:

      // Helper method to translate Ruby indices to vector indices
      Difference_T normalizeIndex(Size_T size, Difference_T index, bool enforceBounds = false)
      {
        // Negative indices mean count from the right. Note that negative indices
        // wrap around!
        if (index < 0)
        {
          index = ((-index) % size);
          index = index > 0 ? size - index : index;
        }

        if (enforceBounds && (index < 0 || index >= (Difference_T)size))
        {
          throw std::out_of_range("Invalid index: " + std::to_string(index));
        }

        return index;
      };

      void define_constructor()
      {
        klass_.define_constructor(Constructor<T>());
      }

      void define_copyable_methods()
      {
        if constexpr (std::is_copy_constructible_v<Value_T>)
        {
          klass_.define_method("copy", [](T& self) -> T
            {
              return self;
            });
        }
        else
        {
          klass_.define_method("copy", [](T& self) -> T
            {
              throw std::runtime_error("Cannot copy vectors with non-copy constructible types");
              return self;
            });
        }
      }

      void define_constructable_methods()
      {
        if constexpr (std::is_default_constructible_v<Value_T>)
        {
          klass_.define_method("resize", static_cast<void (T::*)(const size_t)>(&T::resize));
        }
        else
        {
          klass_.define_method("resize", [](const T& self, Size_T newSize)
              {
                // Do nothing
              });
        }
      }

      void define_capacity_methods()
      {
        klass_.define_method("empty?", &T::empty)
          .define_method("capacity", &T::capacity)
          .define_method("max_size", &T::max_size)
          .define_method("reserve", &T::reserve)
          .define_method("size", &T::size);
        
        rb_define_alias(klass_, "count", "size");
        rb_define_alias(klass_, "length", "size");
        //detail::protect(rb_define_alias, klass_, "count", "size");
        //detail::protect(rb_define_alias, klass_, "length", "size");
      }

      void define_access_methods()
      {
        // Access methods
        klass_.define_method("first", [](const T& self) -> std::optional<Value_T>
          {
            if (self.size() > 0)
            {
              return self.front();
            }
            else
            {
              return std::nullopt;
            }
          })
          .define_method("last", [](const T& self) -> std::optional<Value_T>
            {
              if (self.size() > 0)
              {
                return self.back();
              }
              else
              {
                return std::nullopt;
              }
            })
            .define_method("[]", [this](const T& self, Difference_T index) -> std::optional<Value_T>
              {
                index = normalizeIndex(self.size(), index);
                if (index < 0 || index >= (Difference_T)self.size())
                {
                  return std::nullopt;
                }
                else
                {
                  return self[index];
                }
              });

            rb_define_alias(klass_, "at", "[]");
      }

      // Methods that require Value_T to support operator==
      void define_comparable_methods()
      {
        if constexpr (detail::is_comparable_v<Value_T>)
        {
          klass_.define_method("delete", [](T& self, Value_T& element) -> std::optional<Value_T>
            {
              auto iter = std::find(self.begin(), self.end(), element);
              if (iter == self.end())
              {
                return std::nullopt;
              }
              else
              {
                Value_T result = *iter;
                self.erase(iter);
                return result;
              }
            })
          .define_method("include?", [](T& self, Value_T& element)
            {
              return std::find(self.begin(), self.end(), element) != self.end();
            })
          .define_method("index", [](T& self, Value_T& element) -> std::optional<Difference_T>
            {
              auto iter = std::find(self.begin(), self.end(), element);
              if (iter == self.end())
              {
                return std::nullopt;
              }
              else
              {
                return iter - self.begin();
              }
            });
        }
        else
        {
          klass_.define_method("delete", [](T& self, Value_T& element) -> std::optional<Value_T>
            {
              return std::nullopt;
            })
          .define_method("include?", [](const T& self, Value_T& element)
            {
              return false;
            })
          .define_method("index", [](const T& self, Value_T& element) -> std::optional<Difference_T>
            {
              return std::nullopt;
            });
        }
      }

      void define_modify_methods()
      {
        klass_.define_method("clear", &T::clear)
          .define_method("delete_at", [](T& self, const size_t& pos)
            {
              auto iter = self.begin() + pos;
              Value_T result = *iter;
              self.erase(iter);
              return result;
            })
          .define_method("insert", [this](T& self, Difference_T index, Value_T& element) -> T&
            {
              index = normalizeIndex(self.size(), index, true);
              auto iter = self.begin() + index;
              self.insert(iter, element);
              return self;
            })
          .define_method("pop", [](T& self) -> std::optional<Value_T>
            {
              if (self.size() > 0)
              {
                Value_T result = self.back();
                self.pop_back();
                return result;
              }
              else
              {
                return std::nullopt;
              }
            })
          .define_method("push", [](T& self, Value_T& element) -> T&
            {
              self.push_back(element);
              return self;
            })
          .define_method("shrink_to_fit", &T::shrink_to_fit)
          .define_method("[]=", [this](T& self, Difference_T index, Value_T& element) -> Value_T&
            {
              index = normalizeIndex(self.size(), index, true);
              self[index] = element;
              return element;
            });

            rb_define_alias(klass_, "<<", "push");
            rb_define_alias(klass_, "append", "push");
      }

      void define_enumerable()
      {
        // Add enumerable support
        klass_.include_module(rb_mEnumerable)
          .define_method("each", [](T& self) -> const T&
            {
              for (Value_T& item : self)
              {
                VALUE element = detail::To_Ruby<Value_T>().convert(item);
                rb_yield(element);
              }
              return self;
            });
      }

      void define_to_s()
      {
        if constexpr (detail::is_ostreamable_v<Value_T>)
        {
          klass_.define_method("to_s", [](const T& self)
            {
              auto iter = self.begin();
              auto finish = self.size() > 1000 ? self.begin() + 1000 : self.end();

              std::stringstream stream;
              stream << "[";

              for (; iter != finish; iter++)
              {
                if (iter == self.begin())
                {
                  stream << *iter;
                }
                else
                {
                  stream << ", " << *iter;
                }
              }

              stream << "]";
              return stream.str();
            });
        }
        else
        {
          klass_.define_method("to_s", [](const T& self)
            {
              return "[Not printable]";
            });
        }
      }

      private:
        Data_Type<T> klass_;
    };
  } // namespace

  template<typename T>
  Data_Type<T> define_vector_under(Object module, std::string name)
  {
    if (detail::TypeRegistry::isDefined<T>())
    {
      // If the vector has been previously seen it will be registered but may
      // not be associated with the constant Module::<name>
      module.const_set_maybe(name, Data_Type<T>().klass());

      return Data_Type<T>();
    }

    Data_Type<T> result = define_class_under<detail::intrinsic_type<T>>(module, name.c_str());
    stl::VectorHelper helper(result);
    return result;
  }

  template<typename T>
  Data_Type<T> define_vector(std::string name)
  {
    if (detail::TypeRegistry::isDefined<T>())
    {
      // If the vector has been previously seen it will be registered but may
      // not be associated with the constant Module::<name>
      Object(rb_cObject).const_set_maybe(name, Data_Type<T>().klass());

      return Data_Type<T>();
    }

    Data_Type<T> result = define_class<detail::intrinsic_type<T>>(name.c_str());
    stl::VectorHelper<T> helper(result);
    return result;
  }

  template<typename T>
  Data_Type<T> define_vector_auto()
  {
    std::string klassName = detail::makeClassName(typeid(T));
    Module rb_mRice = define_module("Rice");
    Module rb_mVector = define_module_under(rb_mRice, "Std");
    return define_vector_under<T>(rb_mVector, klassName);
  }
   
  namespace detail
  {
    template<typename T>
    struct Type<std::vector<T>>
    {
      static bool verify()
      {
        Type<T>::verify();

        if (!detail::TypeRegistry::isDefined<std::vector<T>>())
        {
          define_vector_auto<std::vector<T>>();
        }

        return true;
      }
    };

    template<typename T>
    std::vector<T> vectorFromArray(VALUE value)
    {
      size_t length = protect(rb_array_len, value);
      std::vector<T> result(length);

      for (long i = 0; i < length; i++)
      {
        VALUE element = protect(rb_ary_entry, value, i);
        result[i] = From_Ruby<T>().convert(element);
      }

      return result;
    }

    template<typename T>
    class From_Ruby<std::vector<T>>
    {
    public:
      From_Ruby() = default;

      explicit From_Ruby(Arg * arg) : arg_(arg)
      {
      }

      std::vector<T> convert(VALUE value)
      {
        switch (rb_type(value))
        {
          case T_DATA:
          {
            // This is a wrapped vector (hopefully!)
            return *Data_Object<std::vector<T>>::from_ruby(value);
          }
          case T_ARRAY:
          {
            // If this an Ruby array and the vector type is copyable
            if constexpr (std::is_default_constructible_v<T>)
            {
              return vectorFromArray<T>(value);
            }
          }
          case T_NIL:
          {
            if (this->arg_ && this->arg_->hasDefaultValue())
            {
              return this->arg_->template defaultValue<std::vector<T>>();
            }
          }
          default:
          {
            throw Exception(rb_eTypeError, "wrong argument type %s (expected % s)",
              detail::protect(rb_obj_classname, value), "std::vector");
          }
        }
      }

    private:
      Arg* arg_ = nullptr;
    };

    template<typename T>
    class From_Ruby<std::vector<T>&>
    {
    public:
      From_Ruby() = default;

      explicit From_Ruby(Arg * arg) : arg_(arg)
      {
      }

      std::vector<T>& convert(VALUE value)
      {
        switch (rb_type(value))
        {
          case T_DATA:
          {
            // This is a wrapped vector (hopefully!)
            return *Data_Object<std::vector<T>>::from_ruby(value);
          }
          case T_ARRAY:
          {
            // If this an Ruby array and the vector type is copyable
            if constexpr (std::is_default_constructible_v<T>)
            {
              this->converted_ = vectorFromArray<T>(value);
              return this->converted_;
            }
          }
          case T_NIL:
          {
            if (this->arg_ && this->arg_->hasDefaultValue())
            {
              return this->arg_->template defaultValue<std::vector<T>>();
            }
          }
          default:
          {
            throw Exception(rb_eTypeError, "wrong argument type %s (expected % s)",
              detail::protect(rb_obj_classname, value), "std::vector");
          }
        }
      }

    private:
      Arg* arg_ = nullptr;
      std::vector<T> converted_;
    };

    template<typename T>
    class From_Ruby<std::vector<T>*>
    {
    public:
      std::vector<T>* convert(VALUE value)
      {
        switch (rb_type(value))
        {
          case T_DATA:
          {
            // This is a wrapped vector (hopefully!)
            return Data_Object<std::vector<T>>::from_ruby(value);
          }
          case T_ARRAY:
          {
            // If this an Ruby array and the vector type is copyable
            if constexpr (std::is_default_constructible_v<T>)
            {
              this->converted_ = vectorFromArray<T>(value);
              return &this->converted_;
            }
          }
          default:
          {
            throw Exception(rb_eTypeError, "wrong argument type %s (expected % s)",
              detail::protect(rb_obj_classname, value), "std::vector");
          }
        }
      }

    private:
      std::vector<T> converted_;
    };
  }
}

#endif // Rice__stl__hpp_
