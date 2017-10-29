#pragma once

/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */
#include "atlas/array/ArrayView.h"
#include "atlas/array/SVector.h"

namespace atlas {
namespace parallel {

#ifdef __CUDACC__
template<typename DATA_TYPE, int RANK>
__global__
void pack_kernel(int sendcnt, array::SVector<int> sendmap,
         const array::ArrayView<DATA_TYPE, RANK> field, array::SVector<DATA_TYPE> send_buffer);
#endif

template<typename DATA_TYPE, int RANK>
void pack_send_cuda( const int sendcnt, array::SVector<int> const & sendmap,
                     const array::ArrayView<DATA_TYPE, RANK>& field, array::SVector<DATA_TYPE>& send_buffer )
{
#ifdef __CUDACC__
    const unsigned int block_size_x = 32;
    const unsigned int block_size_y = 4;
    dim3 threads(block_size_x, block_size_y);
    dim3 blocks((sendcnt+block_size_x-1)/block_size_x, (field.data_view().template length<1>()+block_size_y-1)/block_size_y);

    pack_kernel<<<blocks,threads>>>(sendcnt, sendmap, field, send_buffer);
#endif
}


} // namespace paralllel
} // namespace atlas

