/*
 * (C) Copyright 1996-2014 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/log/Log.h"
#include "atlas/meshgen/Delaunay.h"
#include "atlas/Tesselation.h"
#include "atlas/GridDistribution.h"

namespace atlas {
namespace meshgen {

Delaunay::Delaunay()
{
}

Delaunay::~Delaunay() {
}

void Delaunay::generate(const Grid &g, const GridDistribution &d , Mesh &m) const
{
  if( d.nb_partitions() > 1 )
  {
    eckit::Log::warning() << "Delaunay triangulation does not support a GridDistribution"
                             "with more than 1 partition"
                          << std::endl;
    NOTIMP;
    /// TODO: Read mesh on 1 MPI task, and distribute according to GridDistribution
    /// HINT: use atlas/actions/DistributeMesh
  }
  else
  {
    Tesselation::delaunay_triangulation(m);
  }
}

void Delaunay::generate(const Grid &g, Mesh &m) const
{
  Tesselation::delaunay_triangulation(m);
}

namespace {
static MeshGeneratorBuilder< Delaunay > __delaunay("Delaunay");
}


} // namespace meshgen
} // namespace atlas
