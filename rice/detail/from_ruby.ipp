#ifndef Rice__detail__from_ruby__ipp_
#define Rice__detail__from_ruby__ipp_

#include <stdexcept>
#include "../Exception_defn.hpp"
#include "Ruby_Function.hpp"

/* This file implements conversions from Ruby to native values fo fundamental types 
   such as bool, int, float, etc. It also includes conversions for chars and strings */
namespace Rice
{
  namespace detail
  {
    template<>
    struct From_Ruby<short>
    {
      static short convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return NUM2SHORT(value);
      }
    };

    template<>
    struct From_Ruby<int>
    {
      static int convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return NUM2INT(value);
      }
    };

    template<>
    struct From_Ruby<long>
    {
      static long convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return (long)NUM2LONG(value);
      }
    };

    template<>
    struct From_Ruby<long long>
    {
      static long long convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return RB_NUM2LL(value);
      }
    };

    template<>
    struct From_Ruby<unsigned short>
    {
      static unsigned short convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return NUM2USHORT(value);
      }
    };

    template<>
    struct From_Ruby<unsigned int>
    {
      static unsigned int convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return NUM2UINT(value);
      }
    };

    template<>
    struct From_Ruby<unsigned long>
    {
      static unsigned long convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return (unsigned long)RB_NUM2ULONG(value);
      }
    };

    template<>
    struct From_Ruby<unsigned long long>
    {
      static unsigned long long convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_FIXNUM);
        return RB_NUM2ULL(value);
      }
    };

    template<>
    struct From_Ruby<bool>
    {
      static bool convert(VALUE value)
      {
        return RTEST(value);
      }
    };

    template<>
    struct From_Ruby<char>
    {
      static char convert(VALUE value)
      {
        switch (rb_type(value))
        {
          case T_STRING:
          {
            if (RSTRING_LEN(value) == 1)
            {
              return RSTRING_PTR(value)[0];
            }
            else
            {
              throw std::invalid_argument("from_ruby<char>: string must have length 1");
            }
            break;
          }
          case T_FIXNUM:
          {
            return From_Ruby<long>::convert(value) & 0xff;
            break;
          }
          default:
          {
            throw Exception(rb_eTypeError, "wrong argument type %s (expected % s)",
                            rb_obj_classname(value), "char");
          }
        }
      }
    };

    template<>
    struct From_Ruby<char*>
    {
      static char* convert(VALUE value)
      {
        if (value == Qnil)
        {
          return nullptr;
        }
        else
        {
          return RSTRING_PTR(value);
        }
      }
    };

    template<>
    struct From_Ruby<char const*>
    {
      static char const* convert(VALUE value)
      {
        if (value == Qnil)
        {
          return nullptr;
        }
        else
        {
          return RSTRING_PTR(value);
        }
      }
    };

    template<>
    struct From_Ruby<unsigned char>
    {
      static unsigned char convert(VALUE value)
      {
        switch (rb_type(value))
        {
        case T_FIXNUM:
        {
          return From_Ruby<long>::convert(value) & 0xff;
          break;
        }
        default:
        {
          throw std::invalid_argument("Cannot convert Ruby value to unsigned char");
        }
        }
      }
    };

    template<>
    struct From_Ruby<signed char>
    {
      static signed char convert(VALUE value)
      {
        switch (rb_type(value))
        {
        case T_FIXNUM:
        {
          return From_Ruby<long>::convert(value) & 0xff;
          break;
        }
        default:
        {
          throw std::invalid_argument("Cannot convert Ruby value to unsigned char");
        }
        }
      }
    };

    template<>
    struct From_Ruby<float>
    {
      static float convert(VALUE value)
      {
        if (rb_type(value) != T_FLOAT && rb_type(value) != T_FIXNUM)
        {
          detail::protect(rb_check_type, value, T_FLOAT);
        }
        return (float)rb_num2dbl(value);
      }
    };

    template<>
    struct From_Ruby<double>
    {
      static double convert(VALUE value)
      {
        if (rb_type(value) != T_FLOAT && rb_type(value) != T_FIXNUM)
        {
          detail::protect(rb_check_type, value, T_FLOAT);
        }
        return rb_num2dbl(value);
      }
    };

    template<>
    struct From_Ruby<std::string>
    {
      static std::string convert(VALUE value)
      {
        detail::protect(rb_check_type, value, T_STRING);
        return std::string(RSTRING_PTR(value), RSTRING_LEN(value));
      }
    };
  }
}
#endif // Rice__detail__from_ruby__ipp_