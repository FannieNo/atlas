/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#pragma once

#include <array>
#include <string>

#include "atlas/util/ObjectHandle.h"

//---------------------------------------------------------------------------------------------------------------------

// Forward declarations
namespace eckit {
class Parametrisation;
class Hash;
}  // namespace eckit

//---------------------------------------------------------------------------------------------------------------------

namespace atlas {
class PointXY;

namespace util {
class Config;
}  // namespace util

namespace domain {
class Domain;
}

//---------------------------------------------------------------------------------------------------------------------

class Domain : public util::ObjectHandle<atlas::domain::Domain> {
public:
    using Spec = util::Config;

public:
    using Handle::Handle;
    Domain() = default;
    Domain( const eckit::Parametrisation& );

    /// Type of the domain
    std::string type() const;

    /// Checks if the point is contained in the domain
    bool contains( double x, double y ) const;

    /// Checks if the point is contained in the domain
    bool contains( const PointXY& p ) const;

    // Specification of Domain
    Spec spec() const;

    /// Check if domain represents the complete globe surface
    bool global() const;

    /// Check if domain does not represent any area on the globe surface
    bool empty() const;

    /// Add domain to the given hash
    void hash( eckit::Hash& ) const;

    /// Check if grid includes the North pole (can only be true when units are in
    /// degrees)
    bool containsNorthPole() const;

    /// Check if grid includes the South pole (can only be true when units are in
    /// degrees)
    bool containsSouthPole() const;

    /// String that defines units of the domain ("degrees" or "meters")
    std::string units() const;

private:
    /// Output to stream
    void print( std::ostream& ) const;

    friend std::ostream& operator<<( std::ostream& s, const Domain& d );
};

//---------------------------------------------------------------------------------------------------------------------

namespace domain {
class RectangularDomain;
}

class RectangularDomain : public Domain {
public:
    using Interval = std::array<double, 2>;

public:
    using Domain::Domain;
    RectangularDomain( const Interval& x, const Interval& y, const std::string& units = "degrees" );

    RectangularDomain( const Domain& );

    operator bool() { return domain_; }

    /// Checks if the x-value is contained in the domain
    bool contains_x( double x ) const;

    /// Checks if the y-value is contained in the domain
    bool contains_y( double y ) const;

    double xmin() const;
    double xmax() const;
    double ymin() const;
    double ymax() const;

private:
    const ::atlas::domain::RectangularDomain* domain_;
};

//---------------------------------------------------------------------------------------------------------------------

namespace domain {
class ZonalBandDomain;
}

class ZonalBandDomain : public RectangularDomain {
public:
    using Interval = std::array<double, 2>;

public:
    using RectangularDomain::RectangularDomain;
    ZonalBandDomain( const Interval& y );
    ZonalBandDomain( const Domain& );

    operator bool() { return domain_; }

private:
    const ::atlas::domain::ZonalBandDomain* domain_;
};

//---------------------------------------------------------------------------------------------------------------------

}  // namespace atlas
