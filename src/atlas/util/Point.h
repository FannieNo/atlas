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

/// @file Point.h
///
/// This file contains classes and functions working on points.
/// The Point classes are inherited from eckit::geometry::Point2
/// or eckit::geometry::Point3.
///
/// Classes:
///    - PointXY     : Point in arbitrary XY-coordinate system
///    - PointXYZ    : Point in arbitrary XYZ-coordinate system
///    - PointLonLat : Point in longitude-latitude coordinate system.
///                    This class does *not* normalise the longitude!!!
///
/// Functions:
///    - lonlat_to_geocentric : Converts a PointLonLat to a PointXYZ defined in
///                             Earth-centred coordinates.

#include <array>
#include <initializer_list>

#include "eckit/geometry/Point2.h"
#include "eckit/geometry/Point3.h"

#include "atlas/util/Earth.h"

namespace atlas {

using Point2 = eckit::geometry::Point2;
using Point3 = eckit::geometry::Point3;

class PointXY : public Point2 {
    using array_t = std::array<double, 2>;

public:
    using Point2::Point2;

    PointXY() : Point2() {}

    // Allow initialization through PointXY xy = {0,0};
    PointXY( std::initializer_list<double> list ) : PointXY( list.begin() ) {}
    PointXY( const array_t& arr ) : PointXY( arr.data() ) {}

    double x() const { return x_[0]; }
    double y() const { return x_[1]; }
    double& x() { return x_[0]; }
    double& y() { return x_[1]; }

    using Point2::assign;

    void assign( double x, double y ) {
        x_[0] = x;
        x_[1] = y;
    }
};

class PointXYZ : public Point3 {
    using array_t = std::array<double, 3>;

    PointXYZ( double, double ) { /* No automatic converion allowed, otherwise
                                inherited from Point3 */
    }

public:
    using Point3::Point3;

    PointXYZ() : Point3() {}

    // Allow initialization through PointXYZ xyz = {0,0,0};
    PointXYZ( std::initializer_list<double> list ) : PointXYZ( list.begin() ) {}

    PointXYZ( const array_t& arr ) : PointXYZ( arr.data() ) {}

    double x() const { return x_[0]; }
    double y() const { return x_[1]; }
    double z() const { return x_[2]; }
    double& x() { return x_[0]; }
    double& y() { return x_[1]; }
    double& z() { return x_[2]; }

    using Point3::assign;

    void assign( double x, double y, double z ) {
        x_[0] = x;
        x_[1] = y;
        x_[2] = z;
    }
};

class PointLonLat : public Point2 {
    using array_t = std::array<double, 2>;

public:
    using Point2::Point2;

    PointLonLat() : Point2() {}

    // Allow initialization through PointXY lonlat = {0,0};
    PointLonLat( std::initializer_list<double> list ) : PointLonLat( list.begin() ) {}

    PointLonLat( const array_t& arr ) : PointLonLat( arr.data() ) {}

    double lon() const { return x_[0]; }
    double lat() const { return x_[1]; }
    double& lon() { return x_[0]; }
    double& lat() { return x_[1]; }

    using Point2::assign;

    void assign( double lon, double lat ) {
        x_[0] = lon;
        x_[1] = lat;
    }

    PointLonLat& operator*=( double a ) {
        x_[0] *= a;
        x_[1] *= a;
        return *this;
    }

    void normalise();

    void normalise( double west );

    void normalise( double west, double east );
};

}  // namespace atlas
