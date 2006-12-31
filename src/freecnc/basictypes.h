#ifndef _BASICTYPES_H
#define _BASICTYPES_H
//
// Includes common STL and boost types we use all over the place, puts them in
// the global namespace.
//

#include <ostream>
#include <map>
#include <string>
#include <vector>

#include <boost/array.hpp>
#include <boost/smart_ptr.hpp>

using std::endl;
using std::map;
using std::max;
using std::min;
using std::string;
using std::vector;

using boost::array;
using boost::scoped_ptr;
using boost::shared_ptr;

#endif
