//===--- raw_ostream.h - Raw output stream ----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the raw_ostream_iterator class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_RAW_OSTREAM_ITERATOR_H
#define LLVM_SUPPORT_RAW_OSTREAM_ITERATOR_H

#include "llvm/Support/raw_ostream.h"
#include <iterator>

namespace llvm {

/// in the spirit of std::ostream_iterator
template<typename T>
class ostream_iterator :
	public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
  //@{
  /// Public typedef
  typedef T				value_type;
  typedef raw_ostream			ostream_type;
  //@}

private:
  ostream_type*				_M_stream;
  const char*				_M_delim;

public:
  /// Construct from an ostream.
  ostream_iterator(ostream_type& __s) : _M_stream(&__s), _M_delim(0) {}

  /**
   *  Construct from an ostream with optional delimiter.
   *  @param  s  Underlying ostream to write to.
   *  @param  c  CharT delimiter string to insert.
  */
  ostream_iterator(ostream_type& __s, const char* __c) :
    _M_stream(&__s), _M_delim(__c)  { }

  /// Copy constructor.
  ostream_iterator(const ostream_iterator& __obj) :
    _M_stream(__obj._M_stream), _M_delim(__obj._M_delim)  { }

  /// Writes @a value to underlying ostream using operator<<.  If
  /// constructed with delimiter string, writes delimiter to ostream.
  ostream_iterator&
  operator=(const T& __value)
  {
    *_M_stream << __value;
    if (_M_delim) *_M_stream << _M_delim;
    return *this;
  }

  ostream_iterator&
  operator*()
  { return *this; }

  ostream_iterator&
  operator++()
  { return *this; }

  ostream_iterator&
  operator++(int)
  { return *this; }
};	// end class raw_ostream_iterator

} // end llvm namespace

#endif
