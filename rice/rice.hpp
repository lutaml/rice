#ifndef Rice__hpp_
#define Rice__hpp_

#include "detail/ruby.hpp"
#include "detail/rice_traits.hpp"

#include "detail/Arguments.hpp"
#include "detail/default_allocation_func.hpp"
#include "detail/demangle.hpp"
#include "detail/from_ruby.hpp"
#include "detail/Exception_Handler.hpp"
#include "detail/Iterator.hpp"
#include "detail/Jump_Tag.hpp"
#include "detail/method_data.hpp"
#include "detail/NativeArg.hpp"
#include "detail/protect.hpp"
#include "detail/to_ruby.hpp"
#include "detail/Ruby_Function.hpp"
#include "detail/Wrapped_Function.hpp"
#include "detail/wrap_function.hpp"

#include "Arg.hpp"

// Arguments implementation is dependent on Arg
#include "detail/Arguments.ipp"

// Deprecated - this code is deprecated and should be removed in the future
#include "Arg_operators.hpp"

#include "Exception.hpp"
#include "protect.hpp"

#include "ruby_mark.hpp"
#include "ruby_try_catch.hpp"

#include "Identifier.hpp"

#include "Object.hpp"

#include "Builtin_Object.hpp"
#include "String.hpp"
#include "Array.hpp"
#include "Hash.hpp"
#include "Symbol.hpp"

#include "Address_Registration_Guard.hpp"
#include "Module.hpp"
#include "global_function.hpp"
#include "detail/Caster.hpp"

#include "Class.hpp"

#include "Constructor.hpp"
#include "Director.hpp"
#include "Data_Type.hpp"
#include "Data_Object.hpp"
#include "detail/Iterator.ipp"

// Dependent on Data_Object due to the way method metadata is stored in the Ruby class
#include "detail/default_allocation_func.ipp"
#include "detail/Wrapped_Function.ipp"

#include "Enum.hpp"
#include "Struct.hpp"

// Dependent on Module, Class, Array and String
#include "forward_declares.ipp"

#endif // Rice__hpp_
