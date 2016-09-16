/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/testing/Setup.h"
#include "eckit/mpi/mpi.h"

#include "atlas/atlas.h"

namespace atlas {
namespace test {


struct AtlasFixture : public eckit::testing::Setup {
    AtlasFixture()  { atlas_init(boost::unit_test::framework::master_test_suite().argc,
                                 boost::unit_test::framework::master_test_suite().argv); }
    ~AtlasFixture() { atlas_finalize(); }
};


struct MPIFixture : public eckit::testing::Setup {
    MPIFixture()  { eckit::mpi::init(); }
    ~MPIFixture() { eckit::mpi::finalize(); }
};

//----------------------------------------------------------------------------------------------------------------------

} // end namespace test
} // end namespace atlas
