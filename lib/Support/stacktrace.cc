/**
	\file "util/stacktrace.cc"
	Implementation of stacktrace class.
	$Id: stacktrace.cc,v 1.1 2010/03/14 22:25:17 fang Exp $
 */

// ENABLE_STACKTRACE is forced for this module, regardless of pre-definitions!
#define	ENABLE_STACKTRACE	1

#include "llvm/Support/stacktrace.h"
#if ST_USE_RAW_OSTREAM
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_ostream_iterator.h"
#else
#include <iostream>
#include <iterator>
#endif

namespace util {
using std::list;
using std::stack;
#if ST_USE_RAW_OSTREAM
using llvm::ostream_iterator;
using llvm::endl;
static llvm::raw_ostream& cerr(llvm::errs());	// alias
#else
using std::ostream_iterator;
using std::cout;
using std::cerr;
using std::endl;
#endif

/**
	Guarantee that ios is initialized.  
 */
static const std::ios_base::Init ios_init;

//=============================================================================
/**
	Private implementation class, not visible to other modules.  
	Only written as a class for convenient static initialization.  
	To be able to trace function calls that occur during static 
	initialization, we must guarantee that the manager's static 
	objects are initialized first!  Global initialization ordering is
	generally non-trivial, so resort to the techinique of 
	interfacing through reference functions which will guarantee 
	a one-time initialization upon first invocation.  
	(This technique is also used in util::persistent_object_manager.)
 */
class stacktrace::manager {
public:
	typedef	stacktrace::stack_text_type	stack_text_type;
	typedef	stacktrace::stack_echo_type	stack_echo_type;
	typedef	stacktrace::stack_streams_type	stack_streams_type;

	static stack_text_type		stack_text;
	static stack_text_type		stack_indent;
	static stack_echo_type		stack_echo;
	static stack_streams_type	stack_streams;

private:
	manager() {
		// initialization moved to static initialization below.
	}

	~manager() { }

public:
	static
	st_ostream_type&
	print_auto_indent(st_ostream_type& o) {
		// guarantee iostream initialized before first used.  
		static const std::ios_base::Init ios_init;
		static const stack_text_type& si(manager::stack_indent);
		// INVARIANT(o.good());
		ostream_iterator<string> osi(o);
		copy(si.begin(), si.end(), osi);
		return o;
	}

};	// end class stacktrace::manager

//-----------------------------------------------------------------------------
// static construction

stacktrace::manager::stack_text_type
stacktrace::manager::stack_text;

stacktrace::manager::stack_text_type
stacktrace::manager::stack_indent;

stacktrace::manager::stack_echo_type
stacktrace::manager::stack_echo;

stacktrace::manager::stack_streams_type
stacktrace::manager::stack_streams;

static const int stack_echo_init =
(stacktrace::manager::stack_echo.push(1), 1);

static const int stack_stream_init =
(stacktrace::manager::stack_streams.push(&cerr), 1);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Stacktrace stream manipulator.  
 */
const stacktrace::indent
stacktrace_auto_indent = stacktrace::indent();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Uses the stacktrace's position to automatically indent.  
 */
st_ostream_type&
operator << (st_ostream_type& o, const stacktrace::indent&) {
	// need static initializers?
	return stacktrace::manager::print_auto_indent(o) << ":  ";
}

//=============================================================================
// class stacktrace method definitions

stacktrace::stacktrace(const string& s) {
	// cannot use string (without ref-count) because it may be destroyed
	// prematurely during static destruction, char* is robust and permanent.
	static const char* const
		default_stack_indent_string = "| ";	// permanent
	// must be static or else, new ref_counts will be locally released
	manager::stack_text.push_back(s);
	if (manager::stack_echo.top()) {
		st_ostream_type& os(*manager::stack_streams.top());
			manager::print_auto_indent(os) << "\\-{ " <<
				manager::stack_text.back() << endl;
	}
	manager::stack_indent.push_back(default_stack_indent_string);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
stacktrace::~stacktrace() {
	manager::stack_indent.pop_back();
	if (manager::stack_echo.top()) {
		st_ostream_type& os(*manager::stack_streams.top());
			manager::print_auto_indent(os) << "/-} " <<
				manager::stack_text.back() << endl;
	}
	manager::stack_text.pop_back();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Returns reference to the current stacktrace output stream.
 */
st_ostream_type&
stacktrace::stream(void) {
	return *manager::stack_streams.top();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
stacktrace::full_dump(void) {
	st_ostream_type& current_stream(stream());
	ostream_iterator<string> osi(current_stream, "\n");
	copy(manager::stack_text.begin(), manager::stack_text.end(), osi);
	current_stream << endl;
}

//=============================================================================
// struct stacktrace::echo method definitions

stacktrace::echo::echo(const int i) {
	manager::stack_echo.push(i);
}

stacktrace::echo::~echo() {
	manager::stack_echo.pop();
}

//=============================================================================
// struct redirect_stacktrace method definitions

stacktrace::redirect::redirect(st_ostream_type& o) {
	manager::stack_streams.push(&o);
}

stacktrace::redirect::~redirect() {
	manager::stack_streams.pop();
}

//=============================================================================
}	// end namespace util


