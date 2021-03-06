/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Structured.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <numeric>

#include "eckit/types/FloatCompare.h"
#include "eckit/utils/Hash.h"

#include "atlas/domain/Domain.h"
#include "atlas/grid/StructuredGrid.h"
#include "atlas/grid/detail/grid/GridBuilder.h"
#include "atlas/grid/detail/grid/GridFactory.h"
#include "atlas/grid/detail/spacing/CustomSpacing.h"
#include "atlas/grid/detail/spacing/LinearSpacing.h"
#include "atlas/runtime/Exception.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/NormaliseLongitude.h"
#include "atlas/util/Point.h"
#include "atlas/util/UnitSphere.h"

namespace atlas {
namespace grid {
namespace detail {
namespace grid {

std::string Structured::static_type() {
    return "structured";
}

std::string Structured::name() const {
    return name_;
}

Structured::Structured( XSpace xspace, YSpace yspace, Projection p, Domain domain ) :
    Structured( Structured::static_type(), xspace, yspace, p, domain ) {}

Structured::Structured( const std::string& name, XSpace xspace, YSpace yspace, Projection projection, Domain domain ) :
    Grid(),
    name_( name ),
    xspace_( xspace ),
    yspace_( yspace ) {
    // Copry members
    if ( projection )
        projection_ = projection;
    else
        projection_ = Projection();

    y_.assign( yspace_.begin(), yspace_.end() );
    idx_t ny{static_cast<idx_t>( y_.size() )};

    if ( xspace_.ny() == 1 && yspace_.size() > 1 ) {
        nx_.resize( ny, xspace_.nx()[0] );
        dx_.resize( ny, xspace_.dx()[0] );
        xmin_.resize( ny, xspace_.xmin()[0] );
        xmax_.resize( ny, xspace_.xmax()[0] );
    }
    else {
        nx_   = xspace_.nx();
        dx_   = xspace_.dx();
        xmin_ = xspace_.xmin();
        xmax_ = xspace_.xmax();
    }

    ATLAS_ASSERT( static_cast<idx_t>( nx_.size() ) == ny );

    // Further setup
    nxmin_ = nxmax_ = nx_.front();
    for ( idx_t j = 1; j < ny; ++j ) {
        nxmin_ = std::min( nx_[j], nxmin_ );
        nxmax_ = std::max( nx_[j], nxmax_ );
    }
    npts_ = std::accumulate( nx_.begin(), nx_.end(), idx_t{0} );

    if ( domain ) { crop( domain ); }

    computeTruePeriodicity();

    if ( domain && domain.global() )
        domain_ = Domain( Grid::Config( "type", "global" ) );
    else
        computeDomain();
}

void Structured::computeDomain() {
    if ( periodic() ) {
        if ( yspace().max() - yspace().min() == 180. ) { domain_ = Domain( Grid::Config( "type", "global" ) ); }
        else {
            Grid::Config config;
            config.set( "type", "zonal_band" );
            config.set( "ymin", yspace().min() );
            config.set( "ymax", yspace().max() );
            domain_ = Domain( config );
        }
    }
    else if ( not domain_ ) {
        Grid::Config config;
        config.set( "type", "rectangular" );
        config.set( "xmin", xmin_[0] );
        config.set( "xmax", xmax_[0] );
        config.set( "ymin", yspace().min() );
        config.set( "ymax", yspace().max() );
        config.set( "units", projection_.units() );
        domain_ = Domain( config );
    }
}

Structured::~Structured() {}

Structured::XSpace::XSpace() : impl_( nullptr ) {}

Structured::XSpace::XSpace( const XSpace& xspace ) : impl_( xspace.impl_ ) {}

template <typename NVector>
Structured::XSpace::XSpace( const std::array<double, 2>& interval, const NVector& N, bool endpoint ) :
    impl_( new Implementation( interval, N, endpoint ) ) {}
template Structured::XSpace::XSpace( const std::array<double, 2>& interval, const std::vector<int>& N, bool endpoint );
template Structured::XSpace::XSpace( const std::array<double, 2>& interval, const std::vector<long>& N, bool endpoint );

Structured::XSpace::XSpace( const std::array<double, 2>& interval, std::initializer_list<int>&& N, bool endpoint ) :
    XSpace( interval, std::vector<int>{N}, endpoint ) {}

Structured::XSpace::XSpace( const Spacing& spacing ) : impl_( new Implementation( spacing ) ) {}

Structured::XSpace::XSpace( const Config& config ) : impl_( new Implementation( config ) ) {}

Structured::XSpace::XSpace( const std::vector<Config>& config ) : impl_( new Implementation( config ) ) {}

Grid::Spec Structured::XSpace::spec() const {
    return impl_->spec();
}

Structured::XSpace::Implementation::Implementation( const Config& config ) {
    Config config_xspace( config );

    std::string xspace_type;
    config_xspace.get( "type", xspace_type );
    ATLAS_ASSERT( xspace_type == "linear" );

    std::vector<idx_t> v_N;
    std::vector<double> v_start;
    std::vector<double> v_end;
    std::vector<double> v_length;
    config_xspace.get( "N[]", v_N );
    config_xspace.get( "start[]", v_start );
    config_xspace.get( "end[]", v_end );
    config_xspace.get( "length[]", v_length );

    idx_t ny =
        std::max( v_N.size(), std::max( v_start.size(), std::max( v_end.size(), std::max( v_length.size(), 1ul ) ) ) );
    reserve( ny );

    if ( not v_N.empty() ) ATLAS_ASSERT( static_cast<idx_t>( v_N.size() ) == ny );
    if ( not v_start.empty() ) ATLAS_ASSERT( static_cast<idx_t>( v_start.size() ) == ny );
    if ( not v_end.empty() ) ATLAS_ASSERT( static_cast<idx_t>( v_end.size() ) == ny );
    if ( not v_length.empty() ) ATLAS_ASSERT( static_cast<idx_t>( v_length.size() ) == ny );

    nxmin_ = std::numeric_limits<idx_t>::max();
    nxmax_ = 0;

    for ( idx_t j = 0; j < ny; ++j ) {
        if ( not v_N.empty() ) config_xspace.set( "N", v_N[j] );
        if ( not v_start.empty() ) config_xspace.set( "start", v_start[j] );
        if ( not v_end.empty() ) config_xspace.set( "end", v_end[j] );
        if ( not v_length.empty() ) config_xspace.set( "length", v_length[j] );
        spacing::LinearSpacing::Params xspace( config_xspace );
        xmin_.push_back( xspace.start );
        xmax_.push_back( xspace.end );
        nx_.push_back( xspace.N );
        dx_.push_back( xspace.step );
        nxmin_ = std::min( nxmin_, nx_[j] );
        nxmax_ = std::max( nxmax_, nx_[j] );
    }
}

Structured::XSpace::Implementation::Implementation( const std::vector<Config>& config_list ) {
    reserve( config_list.size() );

    nxmin_ = std::numeric_limits<idx_t>::max();
    nxmax_ = 0;

    std::string xspace_type;
    for ( idx_t j = 0; j < ny(); ++j ) {
        config_list[j].get( "type", xspace_type );
        ATLAS_ASSERT( xspace_type == "linear" );
        spacing::LinearSpacing::Params xspace( config_list[j] );
        xmin_.push_back( xspace.start );
        xmax_.push_back( xspace.end );
        nx_.push_back( xspace.N );
        dx_.push_back( xspace.step );
        nxmin_ = std::min( nxmin_, nx_[j] );
        nxmax_ = std::max( nxmax_, nx_[j] );
    }
}

void Structured::XSpace::Implementation::Implementation::reserve( idx_t ny ) {
    ny_ = ny;
    nx_.reserve( ny );
    xmin_.reserve( ny );
    xmax_.reserve( ny );
    dx_.reserve( ny );
}

template <typename NVector>
Structured::XSpace::Implementation::Implementation( const std::array<double, 2>& interval, const NVector& N,
                                                    bool endpoint ) :
    ny_( N.size() ),
    nx_( N.begin(), N.end() ),
    xmin_( ny_, interval[0] ),
    xmax_( ny_, interval[1] ),
    dx_( ny_ ) {
    nxmin_        = std::numeric_limits<idx_t>::max();
    nxmax_        = 0;
    double length = interval[1] - interval[0];
    for ( idx_t j = 0; j < ny_; ++j ) {
        nxmin_ = std::min( nxmin_, nx_[j] );
        nxmax_ = std::max( nxmax_, nx_[j] );
        dx_[j] = endpoint ? length / double( nx_[j] - 1 ) : length / double( nx_[j] );
    }
}
template Structured::XSpace::Implementation::Implementation( const std::array<double, 2>& interval,
                                                             const std::vector<int>& N, bool endpoint );
template Structured::XSpace::Implementation::Implementation( const std::array<double, 2>& interval,
                                                             const std::vector<long>& N, bool endpoint );

Structured::XSpace::Implementation::Implementation( const std::array<double, 2>& interval,
                                                    std::initializer_list<int>&& N, bool endpoint ) :
    Implementation( interval, std::vector<int>{N}, endpoint ) {}


Structured::XSpace::Implementation::Implementation( const Spacing& spacing ) :
    ny_( 1 ),
    nx_( ny_, spacing.size() ),
    xmin_( ny_, spacing.min() ),
    xmax_( ny_, spacing.max() ),
    dx_( ny_ ) {
    const spacing::LinearSpacing& linspace = dynamic_cast<const spacing::LinearSpacing&>( *spacing.get() );
    dx_[0]                                 = linspace.step();
    nxmax_                                 = nx_[0];
    nxmin_                                 = nx_[0];
}

std::string Structured::XSpace::Implementation::type() const {
    return "linear";
}

Grid::Spec Structured::XSpace::Implementation::spec() const {
    Grid::Spec spec;

    bool same_xmin = true;
    bool same_xmax = true;
    bool same_nx   = true;

    double xmin = xmin_[0];
    double xmax = xmax_[0];
    idx_t nx    = nx_[0];
    double dx   = dx_[0];

    ATLAS_ASSERT( static_cast<idx_t>( xmin_.size() ) == ny_ );
    ATLAS_ASSERT( static_cast<idx_t>( xmax_.size() ) == ny_ );
    ATLAS_ASSERT( static_cast<idx_t>( nx_.size() ) == ny_ );

    for ( idx_t j = 1; j < ny_; ++j ) {
        same_xmin = same_xmin && ( eckit::types::is_approximately_equal( xmin_[j], xmin ) );
        same_xmax = same_xmax && ( eckit::types::is_approximately_equal( xmax_[j], xmax ) );
        same_nx   = same_nx && ( nx_[j] == nx );
    }

    bool endpoint = std::abs( ( xmax - xmin ) - ( nx - 1 ) * dx ) < 1.e-10;

    spec.set( "type", "linear" );
    if ( same_xmin ) { spec.set( "start", xmin ); }
    else {
        spec.set( "start[]", xmin_ );
    }
    if ( same_xmax ) { spec.set( "end", xmax ); }
    else {
        spec.set( "end[]", xmax_ );
    }
    if ( same_nx ) { spec.set( "N", nx ); }
    else {
        spec.set( "N[]", nx_ );
    }
    spec.set( "endpoint", endpoint );

    return spec;
}

namespace {
class Normalise {
public:
    Normalise( const RectangularDomain& domain ) :
        degrees_( domain.units() == "degrees" ),
        normalise_( domain.xmin(), domain.xmax() ) {}

    double operator()( double x ) const {
        if ( degrees_ ) { x = normalise_( x ); }
        return x;
    }

private:
    const bool degrees_;
    NormaliseLongitude normalise_;
};
}  // namespace

void Structured::crop( const Domain& dom ) {
    if ( dom.global() ) return;

    ATLAS_ASSERT( dom.units() == projection().units() );

    auto zonal_domain = ZonalBandDomain( dom );
    auto rect_domain  = RectangularDomain( dom );

    if ( zonal_domain ) {
        const double cropped_ymin = zonal_domain.ymin();
        const double cropped_ymax = zonal_domain.ymax();

        idx_t jmin = ny();
        idx_t jmax = 0;
        for ( idx_t j = 0; j < ny(); ++j ) {
            if ( zonal_domain.contains_y( y( j ) ) ) {
                jmin = std::min( j, jmin );
                jmax = std::max( j, jmax );
            }
        }
        idx_t cropped_ny = jmax - jmin + 1;
        std::vector<double> cropped_y( y_.begin() + jmin, y_.begin() + jmin + cropped_ny );
        std::vector<double> cropped_xmin( xmin_.begin() + jmin, xmin_.begin() + jmin + cropped_ny );
        std::vector<double> cropped_xmax( xmax_.begin() + jmin, xmax_.begin() + jmin + cropped_ny );
        std::vector<double> cropped_dx( dx_.begin() + jmin, dx_.begin() + jmin + cropped_ny );
        std::vector<idx_t> cropped_nx( nx_.begin() + jmin, nx_.begin() + jmin + cropped_ny );
        ATLAS_ASSERT( idx_t( cropped_nx.size() ) == cropped_ny );

        idx_t cropped_nxmin, cropped_nxmax;
        cropped_nxmin = cropped_nxmax = cropped_nx.front();
        for ( idx_t j = 1; j < cropped_ny; ++j ) {
            cropped_nxmin = std::min( cropped_nx[j], cropped_nxmin );
            cropped_nxmax = std::max( cropped_nx[j], cropped_nxmax );
        }
        idx_t cropped_npts = std::accumulate( cropped_nx.begin(), cropped_nx.end(), idx_t{0} );

        Spacing cropped_yspace(
            new spacing::CustomSpacing( cropped_ny, cropped_y.data(), {cropped_ymin, cropped_ymax} ) );

        // Modify grid
        {
            domain_ = dom;
            yspace_ = cropped_yspace;
            xmin_   = cropped_xmin;
            xmax_   = cropped_xmax;
            dx_     = cropped_dx;
            nx_     = cropped_nx;
            nxmin_  = cropped_nxmin;
            nxmax_  = cropped_nxmax;
            npts_   = cropped_npts;
            y_      = cropped_y;
        }
    }
    else if ( rect_domain ) {
        const double cropped_ymin = rect_domain.ymin();
        const double cropped_ymax = rect_domain.ymax();

        // Cropping in Y
        idx_t jmin = ny();
        idx_t jmax = 0;
        for ( idx_t j = 0; j < ny(); ++j ) {
            if ( rect_domain.contains_y( y( j ) ) ) {
                jmin = std::min( j, jmin );
                jmax = std::max( j, jmax );
            }
        }
        ATLAS_ASSERT( jmax >= jmin );

        idx_t cropped_ny = jmax - jmin + 1;
        std::vector<double> cropped_y( y_.begin() + jmin, y_.begin() + jmin + cropped_ny );
        std::vector<double> cropped_dx( dx_.begin() + jmin, dx_.begin() + jmin + cropped_ny );

        std::vector<double> cropped_xmin( cropped_ny, std::numeric_limits<double>::max() );
        std::vector<double> cropped_xmax( cropped_ny, -std::numeric_limits<double>::max() );
        std::vector<idx_t> cropped_nx( cropped_ny );

        // Cropping in X
        Normalise normalise( rect_domain );
        for ( idx_t j = jmin, jcropped = 0; j <= jmax; ++j, ++jcropped ) {
            idx_t n = 0;
            for ( idx_t i = 0; i < nx( j ); ++i ) {
                const double _x = normalise( x( i, j ) );
                if ( rect_domain.contains_x( _x ) ) {
                    cropped_xmin[jcropped] = std::min( cropped_xmin[jcropped], _x );
                    cropped_xmax[jcropped] = std::max( cropped_xmax[jcropped], _x );
                    ++n;
                }
            }
            cropped_nx[jcropped] = n;
        }

        // Complete structures

        idx_t cropped_nxmin, cropped_nxmax;
        cropped_nxmin = cropped_nxmax = cropped_nx.front();

        for ( idx_t j = 1; j < cropped_ny; ++j ) {
            cropped_nxmin = std::min( cropped_nx[j], cropped_nxmin );
            cropped_nxmax = std::max( cropped_nx[j], cropped_nxmax );
        }
        idx_t cropped_npts = std::accumulate( cropped_nx.begin(), cropped_nx.end(), idx_t{0} );

        Spacing cropped_yspace(
            new spacing::CustomSpacing( cropped_ny, cropped_y.data(), {cropped_ymin, cropped_ymax} ) );

        // Modify grid
        {
            domain_ = dom;
            yspace_ = cropped_yspace;
            xmin_   = cropped_xmin;
            xmax_   = cropped_xmax;
            dx_     = cropped_dx;
            nx_     = cropped_nx;
            nxmin_  = cropped_nxmin;
            nxmax_  = cropped_nxmax;
            npts_   = cropped_npts;
            y_      = cropped_y;
        }
    }
    else {
        std::stringstream errmsg;
        errmsg << "Cannot crop the grid with domain " << dom;
        throw_Exception( errmsg.str(), Here() );
    }
}

void Structured::computeTruePeriodicity() {
    if ( projection_.strictlyRegional() ) { periodic_x_ = false; }
    else if ( domain_ && domain_.global() ) {
        periodic_x_ = true;
    }
    else {
        // domain could be zonal band

        idx_t j = ny() / 2;
        if ( std::abs( xmin_[j] + ( nx_[j] - 1 ) * dx_[j] - xmax_[j] ) < 1.e-11 ) {
            periodic_x_ = false;  // This would lead to duplicated points
        }
        else {
            // High chance to be periodic. Check anyway.
            const PointLonLat Pllmin = projection().lonlat( PointXY( xmin_[j], y_[j] ) );
            const PointLonLat Pllmax = projection().lonlat( PointXY( xmax_[j], y_[j] ) );

            Point3 Pxmin;
            util::UnitSphere::convertSphericalToCartesian( Pllmin, Pxmin );

            Point3 Pxmax;
            util::UnitSphere::convertSphericalToCartesian( Pllmax, Pxmax );

            periodic_x_ = points_equal( Pxmin, Pxmax );
        }
    }
}

void Structured::print( std::ostream& os ) const {
    os << "Structured(Name:" << name() << ")";
}

std::string Structured::type() const {
    return static_type();
}

void Structured::hash( eckit::Hash& h ) const {
    h.add( y().data(), sizeof( double ) * y().size() );

    // We can use nx() directly, but it could change the hash
    std::vector<long> hashed_nx( nx().begin(), nx().end() );
    h.add( hashed_nx.data(), sizeof( long ) * ny() );

    // also add lonmin and lonmax
    h.add( xmin_.data(), sizeof( double ) * xmin_.size() );
    h.add( dx_.data(), sizeof( double ) * dx_.size() );

    // also add projection information
    projection().hash( h );

    // also add domain information, even though already encoded in grid.
    domain().hash( h );
}

Grid::Spec Structured::spec() const {
    Grid::Spec grid_spec;

    if ( name() == "structured" ) {
        grid_spec.set( "type", type() );
        grid_spec.set( "xspace", xspace().spec() );
        grid_spec.set( "yspace", yspace().spec() );
    }
    else {
        grid_spec.set( "name", name() );
    }
    grid_spec.set( "domain", domain().spec() );
    grid_spec.set( "projection", projection().spec() );
    return grid_spec;
}

    // --------------------------------------------------------------------

#if 1
namespace {  // anonymous

static class structured : public GridBuilder {
    using Implementation = atlas::Grid::Implementation;
    using Config         = Grid::Config;
    using XSpace         = StructuredGrid::XSpace;

public:
    structured() : GridBuilder( "structured" ) {}

    virtual void print( std::ostream& os ) const {
        os << std::left << std::setw( 20 ) << " "
           << "Structured grid";
    }

    virtual const Implementation* create( const std::string& /* name */, const Config& ) const {
        throw_NotImplemented( "Cannot create structured grid from name", Here() );
    }

    virtual const Implementation* create( const Config& config ) const {
        Projection projection;
        Spacing yspace;
        Domain domain;

        Config config_proj;
        if ( config.get( "projection", config_proj ) ) projection = Projection( config_proj );

        Config config_domain;
        if ( config.get( "domain", config_domain ) ) { domain = Domain( config_domain ); }

        Config config_yspace;
        if ( not config.get( "yspace", config_yspace ) ) throw_Exception( "yspace missing in configuration", Here() );
        yspace = Spacing( config_yspace );

        XSpace xspace;

        Config config_xspace;
        std::vector<Config> config_xspace_list;

        if ( config.get( "xspace[]", config_xspace_list ) ) { xspace = XSpace( config_xspace_list ); }
        else if ( config.get( "xspace", config_xspace ) ) {
            xspace = XSpace( config_xspace );
        }
        else {
            throw_Exception( "xspace missing in configuration", Here() );
        }

        return new StructuredGrid::grid_t( xspace, yspace, projection, domain );
    }

} structured_;

}  // anonymous namespace
#endif

// --------------------------------------------------------------------

extern "C" {

idx_t atlas__grid__Structured__ny( Structured* This ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->ny();
}

idx_t atlas__grid__Structured__nx( Structured* This, idx_t jlat ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->nx( jlat );
}

void atlas__grid__Structured__nx_array( Structured* This, const idx_t*& nx_array, idx_t& size ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    nx_array = This->nx().data();
    size     = idx_t( This->nx().size() );
}

idx_t atlas__grid__Structured__nxmax( Structured* This ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->nxmax();
}

idx_t atlas__grid__Structured__nxmin( Structured* This ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->nxmin();
}

idx_t atlas__grid__Structured__size( Structured* This ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->size();
}

double atlas__grid__Structured__y( Structured* This, idx_t j ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->y( j );
}

double atlas__grid__Structured__x( Structured* This, idx_t i, idx_t j ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->x( i, j );
}

void atlas__grid__Structured__xy( Structured* This, idx_t i, idx_t j, double crd[] ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    This->xy( i, j, crd );
}

void atlas__grid__Structured__lonlat( Structured* This, idx_t i, idx_t j, double crd[] ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    This->lonlat( i, j, crd );
}

void atlas__grid__Structured__y_array( Structured* This, const double*& y_array, idx_t& size ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    y_array = This->y().data();
    size    = idx_t( This->y().size() );
}

int atlas__grid__Structured__reduced( Structured* This ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    return This->reduced();
}

const Structured* atlas__grid__Structured( char* identifier ) {
    const Structured* grid = dynamic_cast<const Structured*>( Grid::create( std::string( identifier ) ) );
    ATLAS_ASSERT( grid != nullptr );
    return grid;
}

const Structured* atlas__grid__Structured__config( util::Config* conf ) {
    ATLAS_ASSERT( conf != nullptr );
    const Structured* grid = dynamic_cast<const Structured*>( Grid::create( *conf ) );
    ATLAS_ASSERT( grid != nullptr );
    return grid;
}

void atlas__grid__Structured__delete( Structured* This ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    delete This;
}

Structured* atlas__grid__regular__RegularGaussian( long N ) {
    ATLAS_NOTIMPLEMENTED;
}
Structured* atlas__grid__regular__RegularLonLat( long nlon, long nlat ) {
    ATLAS_NOTIMPLEMENTED;
}
Structured* atlas__grid__regular__ShiftedLonLat( long nlon, long nlat ) {
    ATLAS_NOTIMPLEMENTED;
}
Structured* atlas__grid__regular__ShiftedLon( long nlon, long nlat ) {
    ATLAS_NOTIMPLEMENTED;
}
Structured* atlas__grid__regular__ShiftedLat( long nlon, long nlat ) {
    ATLAS_NOTIMPLEMENTED;
}

idx_t atlas__grid__Gaussian__N( Structured* This ) {
    ATLAS_ASSERT( This != nullptr, "Cannot access uninitialised atlas_StructuredGrid" );
    GaussianGrid gaussian( This );
    ATLAS_ASSERT( gaussian, "This grid is not a Gaussian grid" );
    return gaussian.N();
}
}

namespace {
GridFactoryBuilder<Structured> __register_Structured( Structured::static_type() );
}

bool Structured::IteratorXYPredicated::next( PointXY& /*xy*/ ) {
    ATLAS_NOTIMPLEMENTED;
#if 0
    if ( j_ < grid_.ny() && i_ < grid_.nx( j_ ) ) {
        xy = grid_.xy( i_++, j_ );

        if ( i_ == grid_.nx( j_ ) ) {
            j_++;
            i_ = 0;
        }
        return true;
    }
    return false;
#endif
}

}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
