/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Pedro Maciel
/// @author Willem Deconinck
/// @date September 2017

#pragma once

#include <iosfwd>
#include <set>
#include <utility>
#include <vector>
#include "atlas/library/config.h"

namespace atlas {
namespace mesh {
namespace detail {

//------------------------------------------------------------------------------------------------------

/// Polygon
class Polygon : public std::vector<idx_t> {
public:

    // -- Types

    struct edge_t : std::pair< idx_t, idx_t > {
        edge_t(idx_t A, idx_t B) : std::pair< idx_t, idx_t >(A, B) {
        }

        edge_t reverse() const {
            return edge_t(std::pair< idx_t, idx_t >::second, std::pair< idx_t, idx_t >::first);
        }

        struct LessThan {
            bool operator()(const edge_t& e1, const edge_t& e2) const {
                // order ascending by 'first'
                return (e1.first  < e2.first ? true :
                        e1.first  > e2.first ? false :
                        e1.second < e2.second );
            }
        };
    };

    typedef std::set< edge_t, typename edge_t::LessThan > edge_set_t;
    typedef std::vector<idx_t> container_t;

    // -- Constructors

    Polygon();
    Polygon(const edge_set_t&);

    // -- Operators

    operator bool() const;

    Polygon& operator+=(const Polygon&);

    // -- Methods

    void print(std::ostream&) const;

    // -- Friends

    friend std::ostream& operator<<(std::ostream& s, const Polygon& p) {
        p.print(s);
        return s;
    }

};

//------------------------------------------------------------------------------------------------------

}  // detail
}  // mesh
}  // atlas
