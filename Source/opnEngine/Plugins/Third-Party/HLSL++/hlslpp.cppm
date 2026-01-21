module;

// My own repackaging of the library into a module.
#include "hlsl++.h"

export module opn.Plugins.ThirdParty.hlslpp;

// The library uses this macro to prepend 'export' to classes and functions
#define HLSLPP_MODULE_DECLARATION

// Include the internal headers as listed in the library's .ixx
// Ensure your CMake include paths point to the 'hlsl++' directory
#include "hlsl++/vector_float.h"
#include "hlsl++/vector_float8.h"

#include "hlsl++/vector_int.h"
#include "hlsl++/vector_uint.h"
#include "hlsl++/vector_double.h"

#include "hlsl++/matrix_float.h"

#include "hlsl++/quaternion.h"
#include "hlsl++/dependent.h"

#include "hlsl++/data_packing.h"