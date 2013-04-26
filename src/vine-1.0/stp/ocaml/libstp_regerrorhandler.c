/*
Vine is Copyright (C) 2006-2009, BitBlaze Team.

You can redistribute and modify it under the terms of the GNU GPL,
version 2 or later, but it is made available WITHOUT ANY WARRANTY.
See the top-level README file for more details.

For more information about Vine and other BitBlaze software, see our
web site at: http://bitblaze.cs.berkeley.edu/
*/

#include <caml/mlvalues.h>
#include <caml/fail.h>
#include "../c_interface.h"

value libstp_regerrorhandler(value _unit)
{
  // could be caml_invalid_argument or caml_failwith...
  // which is more appropriate?
  vc_registerErrorHandler(caml_invalid_argument);
  return Val_unit;
}
