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

#include "atlas/array/DataType.h"
#include "atlas/field/Field.h"
#include "atlas/functionspace/FunctionSpace.h"
#include "atlas/functionspace/detail/FunctionSpaceImpl.h"
#include "atlas/grid/Vertical.h"
#include "atlas/library/config.h"
#include "atlas/option.h"
#include "atlas/util/Config.h"
#include "atlas/util/ObjectHandle.h"
#include "atlas/util/Point.h"

namespace eckit {
class Configuration;
}

namespace atlas {
namespace parallel {
class GatherScatter;
class HaloExchange;
class Checksum;
}  // namespace parallel
}  // namespace atlas

namespace atlas {
class Field;
class FieldSet;
class Grid;
class StructuredGrid;
}  // namespace atlas

namespace atlas {
namespace grid {
class Distribution;
class Partitioner;
}  // namespace grid
}  // namespace atlas

namespace atlas {
namespace functionspace {

// -------------------------------------------------------------------

namespace detail {
class StructuredColumns : public FunctionSpaceImpl {
public:
    StructuredColumns( const Grid&, const eckit::Configuration& = util::NoConfig() );

    StructuredColumns( const Grid&, const grid::Partitioner&, const eckit::Configuration& = util::NoConfig() );

    StructuredColumns( const Grid&, const grid::Distribution&, const eckit::Configuration& = util::NoConfig() );

    StructuredColumns( const Grid&, const grid::Distribution&, const Vertical&,
                       const eckit::Configuration& = util::NoConfig() );

    StructuredColumns( const Grid&, const Vertical&, const eckit::Configuration& = util::NoConfig() );

    StructuredColumns( const Grid&, const Vertical&, const grid::Partitioner&,
                       const eckit::Configuration& = util::NoConfig() );

    virtual ~StructuredColumns() override;

    static std::string static_type() { return "StructuredColumns"; }
    virtual std::string type() const override { return static_type(); }
    virtual std::string distribution() const override;

    /// @brief Create a Structured field
    virtual Field createField( const eckit::Configuration& ) const override;

    virtual Field createField( const Field&, const eckit::Configuration& ) const override;

    void gather( const FieldSet&, FieldSet& ) const;
    void gather( const Field&, Field& ) const;

    void scatter( const FieldSet&, FieldSet& ) const;
    void scatter( const Field&, Field& ) const;

    virtual void haloExchange( const FieldSet&, bool on_device = false ) const override;
    virtual void haloExchange( const Field&, bool on_device = false ) const override;

    idx_t sizeOwned() const { return size_owned_; }
    idx_t sizeHalo() const { return size_halo_; }
    virtual idx_t size() const override { return size_halo_; }

    idx_t levels() const { return nb_levels_; }

    idx_t halo() const { return halo_; }

    std::string checksum( const FieldSet& ) const;
    std::string checksum( const Field& ) const;


    const Vertical& vertical() const { return vertical_; }

    const StructuredGrid& grid() const;

    idx_t i_begin( idx_t j ) const { return i_begin_[j]; }
    idx_t i_end( idx_t j ) const { return i_end_[j]; }

    idx_t i_begin_halo( idx_t j ) const { return i_begin_halo_[j]; }
    idx_t i_end_halo( idx_t j ) const { return i_end_halo_[j]; }

    idx_t j_begin() const { return j_begin_; }
    idx_t j_end() const { return j_end_; }

    idx_t j_begin_halo() const { return j_begin_halo_; }
    idx_t j_end_halo() const { return j_end_halo_; }

    idx_t k_begin() const { return vertical_.k_begin(); }
    idx_t k_end() const { return vertical_.k_end(); }

    idx_t index( idx_t i, idx_t j ) const {
        check_bounds( i, j );
        return ij2gp_( i, j );
    }

    Field xy() const { return field_xy_; }
    Field partition() const { return field_partition_; }
    Field global_index() const { return field_global_index_; }
    Field remote_index() const {
        if ( not field_remote_index_ ) { create_remote_index(); }
        return field_remote_index_;
    }
    Field index_i() const { return field_index_i_; }
    Field index_j() const { return field_index_j_; }

    void compute_xy( idx_t i, idx_t j, PointXY& xy ) const;
    PointXY compute_xy( idx_t i, idx_t j ) const {
        PointXY xy;
        compute_xy( i, j, xy );
        return xy;
    }

    virtual size_t footprint() const override;


private:  // methods
    idx_t config_size( const eckit::Configuration& config ) const;
    array::DataType config_datatype( const eckit::Configuration& ) const;
    std::string config_name( const eckit::Configuration& ) const;
    idx_t config_levels( const eckit::Configuration& ) const;
    array::ArrayShape config_shape( const eckit::Configuration& ) const;
    void set_field_metadata( const eckit::Configuration&, Field& ) const;

    void check_bounds( idx_t i, idx_t j ) const {
#if ATLAS_ARRAYVIEW_BOUNDS_CHECKING
        if ( j < j_begin_halo() || j >= j_end_halo() ) { throw_outofbounds( i, j ); }
        if ( i < i_begin_halo( j ) || i >= i_end_halo( j ) ) { throw_outofbounds( i, j ); }
#endif
    }
    [[noreturn]] void throw_outofbounds( idx_t i, idx_t j ) const;

    const parallel::GatherScatter& gather() const;
    const parallel::GatherScatter& scatter() const;
    const parallel::Checksum& checksum() const;
    const parallel::HaloExchange& halo_exchange() const;

    void create_remote_index() const;

private:  // data
    std::string distribution_;

    const Vertical vertical_;
    idx_t nb_levels_;

    idx_t size_owned_;
    idx_t size_halo_;
    idx_t halo_;

    const StructuredGrid* grid_;
    mutable util::ObjectHandle<parallel::GatherScatter> gather_scatter_;
    mutable util::ObjectHandle<parallel::Checksum> checksum_;
    mutable util::ObjectHandle<parallel::HaloExchange> halo_exchange_;

    Field field_xy_;
    Field field_partition_;
    Field field_global_index_;
    mutable Field field_remote_index_;
    Field field_index_i_;
    Field field_index_j_;

    class Map2to1 {
    public:
        Map2to1() { resize( {1, 0}, {1, 0} ); }

        Map2to1( std::array<idx_t, 2> i_range, std::array<idx_t, 2> j_range ) { resize( i_range, j_range ); }

        void resize( std::array<idx_t, 2> i_range, std::array<idx_t, 2> j_range ) {
            i_min_    = i_range[0];
            i_max_    = i_range[1];
            j_min_    = j_range[0];
            j_max_    = j_range[1];
            j_stride_ = ( i_max_ - i_min_ + 1 );
            data_.resize( ( i_max_ - i_min_ + 1 ) * ( j_max_ - j_min_ + 1 ), missing() + 1 );
        }

        std::vector<idx_t> data_;
        idx_t i_min_;
        idx_t i_max_;
        idx_t j_min_;
        idx_t j_max_;
        idx_t j_stride_;

        idx_t operator()( idx_t i, idx_t j ) const { return data_[( i - i_min_ ) + ( j - j_min_ ) * j_stride_] - 1; }

        void set( idx_t i, idx_t j, idx_t n ) { data_[( i - i_min_ ) + ( j - j_min_ ) * j_stride_] = n + 1; }

        static idx_t missing() { return std::numeric_limits<idx_t>::max() - 1; }

        size_t footprint() const;

    private:
        void print( std::ostream& ) const;

        friend std::ostream& operator<<( std::ostream& s, const Map2to1& p ) {
            p.print( s );
            return s;
        }
    };

    class IndexRange {
    public:
        IndexRange() { resize( 1, 0 ); }

        IndexRange( idx_t min, idx_t max ) { resize( min, max ); }

        std::vector<idx_t> data_;
        idx_t min_;
        idx_t max_;

        idx_t operator()( idx_t i ) const { return data_[i - min_]; }

        idx_t& operator()( idx_t i ) { return data_[i - min_]; }

        idx_t operator[]( idx_t i ) const { return data_[i - min_]; }

        idx_t& operator[]( idx_t i ) { return data_[i - min_]; }

        idx_t missing() const { return std::numeric_limits<idx_t>::max() - 1; }

        idx_t size() const { return idx_t( data_.size() ); }

        void resize( idx_t min, idx_t max ) {
            min_ = min;
            max_ = max;
            data_.resize( max_ - min_ + 1, missing() + 1 );
        }

    private:
        void print( std::ostream& ) const;

        friend std::ostream& operator<<( std::ostream& s, const IndexRange& p ) {
            p.print( s );
            return s;
        }
    };

    idx_t j_begin_;
    idx_t j_end_;
    std::vector<idx_t> i_begin_;
    std::vector<idx_t> i_end_;
    idx_t j_begin_halo_;
    idx_t j_end_halo_;
    IndexRange i_begin_halo_;
    IndexRange i_end_halo_;

    idx_t north_pole_included_;
    idx_t south_pole_included_;
    idx_t ny_;

    friend struct StructuredColumnsFortranAccess;
    Map2to1 ij2gp_;

    void setup( const grid::Distribution& distribution, const eckit::Configuration& config );
};

// -------------------------------------------------------------------

}  // namespace detail

// -------------------------------------------------------------------

class StructuredColumns : public FunctionSpace {
public:
    StructuredColumns();
    StructuredColumns( const FunctionSpace& );
    StructuredColumns( const Grid&, const eckit::Configuration& = util::NoConfig() );
    StructuredColumns( const Grid&, const grid::Partitioner&, const eckit::Configuration& = util::NoConfig() );
    StructuredColumns( const Grid&, const Vertical&, const eckit::Configuration& = util::NoConfig() );
    StructuredColumns( const Grid&, const Vertical&, const grid::Partitioner&,
                       const eckit::Configuration& = util::NoConfig() );

    static std::string type() { return detail::StructuredColumns::static_type(); }

    operator bool() const { return valid(); }
    bool valid() const { return functionspace_; }

    idx_t size() const { return functionspace_->size(); }
    idx_t sizeOwned() const { return functionspace_->sizeOwned(); }
    idx_t sizeHalo() const { return functionspace_->sizeHalo(); }

    idx_t levels() const { return functionspace_->levels(); }

    idx_t halo() const { return functionspace_->halo(); }

    const Vertical& vertical() const { return functionspace_->vertical(); }

    const StructuredGrid& grid() const { return functionspace_->grid(); }

    void gather( const FieldSet&, FieldSet& ) const;
    void gather( const Field&, Field& ) const;

    void scatter( const FieldSet&, FieldSet& ) const;
    void scatter( const Field&, Field& ) const;

    std::string checksum( const FieldSet& ) const;
    std::string checksum( const Field& ) const;

    idx_t index( idx_t i, idx_t j ) const { return functionspace_->index( i, j ); }

    idx_t i_begin( idx_t j ) const { return functionspace_->i_begin( j ); }
    idx_t i_end( idx_t j ) const { return functionspace_->i_end( j ); }

    idx_t i_begin_halo( idx_t j ) const { return functionspace_->i_begin_halo( j ); }
    idx_t i_end_halo( idx_t j ) const { return functionspace_->i_end_halo( j ); }

    idx_t j_begin() const { return functionspace_->j_begin(); }
    idx_t j_end() const { return functionspace_->j_end(); }

    idx_t j_begin_halo() const { return functionspace_->j_begin_halo(); }
    idx_t j_end_halo() const { return functionspace_->j_end_halo(); }

    idx_t k_begin() const { return functionspace_->k_begin(); }
    idx_t k_end() const { return functionspace_->k_end(); }

    Field xy() const { return functionspace_->xy(); }
    Field partition() const { return functionspace_->partition(); }
    Field global_index() const { return functionspace_->global_index(); }
    Field remote_index() const { return functionspace_->remote_index(); }
    Field index_i() const { return functionspace_->index_i(); }
    Field index_j() const { return functionspace_->index_j(); }

    void compute_xy( idx_t i, idx_t j, PointXY& xy ) const { return functionspace_->compute_xy( i, j, xy ); }
    PointXY compute_xy( idx_t i, idx_t j ) const { return functionspace_->compute_xy( i, j ); }

    size_t footprint() const { return functionspace_->footprint(); }

    class For {
    public:
        For( const StructuredColumns& fs ) : fs_( fs ) {}

    protected:
        const StructuredColumns& fs_;
    };

    class For_ij : public For {
    public:
        using For::For;

        template <typename Functor>
        void operator()( const Functor& f ) const {
            for ( auto j = fs_.j_begin(); j < fs_.j_end(); ++j ) {
                for ( auto i = fs_.i_begin( j ); i < fs_.i_end( j ); ++i ) {
                    f( i, j );
                }
            }
        }
    };

    class For_n : public For {
    public:
        using For::For;

        template <typename Functor>
        void operator()( const Functor& f ) const {
            const auto size = fs_.sizeOwned();
            for ( auto n = 0 * size; n < size; ++n ) {
                f( n );
            }
        }
    };

    For_ij for_ij() const { return For_ij( *this ); }

    For_n for_n() const { return For_n( *this ); }

private:
    const detail::StructuredColumns* functionspace_;
    void setup( const Grid& grid, const Vertical& vertical, const grid::Distribution& distribution,
                const eckit::Configuration& config );
};

// -------------------------------------------------------------------


}  // namespace functionspace
}  // namespace atlas
