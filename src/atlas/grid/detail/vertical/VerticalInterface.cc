/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <algorithm>

#include "VerticalInterface.h"
#include "atlas/grid/Vertical.h"

namespace atlas {

Vertical* atlas__Vertical__new( idx_t levels, const double z[] ) {
    std::vector<double> zvec( z, z + levels );
    return new Vertical( levels, zvec );
}

void atlas__Vertical__delete( Vertical* This ) {
    delete This;
}

}  // namespace atlas
