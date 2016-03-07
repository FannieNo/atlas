/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Willem Deconinck
/// @date Jan 2014

#include <limits>

#include "eckit/memory/ScopedPtr.h"
#include "atlas/internals/atlas_config.h"
#include "atlas/grid/GaussianLatitudes.h"
#include "atlas/grid/predefined/gausslat/gausslat.h"
#include "atlas/internals/Parameters.h"
#include "atlas/util/Constants.h"
#include "atlas/runtime/Log.h"
#include "atlas/array/Array.h"

using eckit::ConcreteBuilderT0;
using eckit::Factory;
using eckit::ScopedPtr;

using atlas::grid::predefined::gausslat::GaussianLatitudes;

namespace atlas {
namespace grid {

//----------------------------------------------------------------------------------------------------------------------

void predict_gaussian_colatitudes_hemisphere(const size_t N, double colat[]);
void predict_gaussian_latitudes_hemisphere(const size_t N, double lat[]);
void compute_gaussian_colatitudes_hemisphere(const size_t N, double colat[]);
void compute_gaussian_latitudes_hemisphere(const size_t N, double lat[]);

namespace {

void colat_to_lat_hemisphere(const size_t N, const double colat[], double lats[], const internals::AngleUnit unit)
{
  std::copy( colat, colat+N, lats );
  double pole = (unit == internals::DEG ? 90. : M_PI_2);

  for(size_t i=0; i<N; ++i) {
    lats[i]=pole-lats[i];
  }
}

} //  anonymous namespace

//----------------------------------------------------------------------------------------------------------------------

void gaussian_latitudes_npole_equator(const size_t N, double lats[])
{
  std::stringstream Nstream; Nstream << N;
  std::string Nstr = Nstream.str();
  if( Factory<GaussianLatitudes>::instance().exists(Nstr) )
  {
    ScopedPtr<GaussianLatitudes> gl ( Factory<GaussianLatitudes>::instance().get(Nstr).create() );
    gl->assign(lats,N);
  }
  else
  {
    compute_gaussian_latitudes_hemisphere(N,lats);
  }
}




void gaussian_latitudes_npole_spole(const size_t N, double lats[])
{
    gaussian_latitudes_npole_equator(N,lats);
    size_t end = 2*N-1;
    for(size_t jlat=0; jlat<N; ++jlat) {
        lats[end--] = -lats[jlat];
    }
}

void predict_gaussian_colatitudes_hemisphere(const size_t N, double colat[])
{
  double z;
  for(size_t i=0; i<N; ++i )
  {
    z = (4.*(i+1.)-1.)*M_PI/(4.*2.*N+2.);
    colat[i] = ( z+1./(tan(z)*(8.*(2.*N)*(2.*N))) ) * util::Constants::radiansToDegrees();
  }
}

void predict_gaussian_latitudes_hemisphere(const size_t N, double lats[])
{
  std::vector<double> colat(N);
  predict_gaussian_colatitudes_hemisphere(N,colat.data());
  colat_to_lat_hemisphere(N,colat.data(),lats,internals::DEG);
}

//----------------------------------------------------------------------------------------------------------------------


namespace {

void cpledn(int kn, int kodd, const double pfn[], double px, double &pxn, double &pxmod )
{
  //Routine to perform a single Newton iteration step to find
  //                the zero of the ordinary Legendre polynomial of degree N

  //        Explicit arguments :
  //        --------------------
  //          KN       :  Degree of the Legendre polynomial              (in)
  //          KODD     :  odd or even number of latitudes                (in)
  //          PFN      :  Fourier coefficients of series expansion       (in)
  //                      for the ordinary Legendre polynomials
  //          PX       :  abcissa where the computations are performed   (in)
  //          KFLAG    :  When KFLAG.EQ.1 computes the weights           (in)
  //          PW       :  Weight of the quadrature at PXN                (out)
  //          PXN      :  new abscissa (Newton iteration)                (out)
  //          PXMOD    :  PXN-PX                                         (out)

  double zdlx, zdlk,zdlkm1, zdlkm2, zdlldn, zdlxn, zdlmod;
  int ik;

  zdlx = px;
  zdlk = 0.;
  if( kodd==0 ) zdlk=0.5*pfn[0];
  zdlxn = 0.;
  zdlldn = 0.;
  ik=1;

  for( int jn=2-kodd; jn<=kn; jn+=2 )
  {
    // normalised ordinary Legendre polynomial == \overbar{P_n}^0
    zdlk+= pfn[ik]*std::cos( static_cast<double>(jn)*zdlx );
    // normalised derivative == d/d\theta(\overbar{P_n}^0)
    zdlldn -= pfn[ik]*static_cast<double>(jn)*std::sin( static_cast<double>(jn)*zdlx );
    ++ik;
  }
  // Newton method
  zdlmod = -zdlk/zdlldn;
  zdlxn = zdlx+zdlmod;
  pxn = zdlxn;
  pxmod = zdlmod;
}

void gawl(double pfn[], double &pl, double peps, int kn, int& kiter, double &pmod)
{
  //**** *GAWL * - Routine to perform the Newton loop

  //     Purpose.
  //     --------
  //           Find 0 of Legendre polynomial with Newton loop
  //**   Interface.
  //     ----------
  //        *CALL* *GAWL(PFN,PL,PW,PEPS,KN,KITER,PMOD)

  //        Explicit arguments :
  //        --------------------
  // PFN    Fourier coefficients of series expansion
  //        for the ordinary Legendre polynomials     (in)
  // PL     Gaussian latitude                         (inout)
  // PW     Gaussian weight                           (out)
  // PEPS   0 of the machine                          (in)
  // KN     Truncation                                (in)
  // KITER  Number of iterations                      (out)
  // PMOD   Last modification                         (inout)

  int iflag, itemax, jter, iodd;
  double zw, zx, zxn;

  //*       1. Initialization.
  //           ---------------

  itemax = 20;
  zx = pl;
  iflag = 0;
  iodd = kn % 2;  // mod(kn,2)

  //*       2. Newton iteration.
  //           -----------------

  for( int jter=1;jter<=itemax+1; ++jter)
  {
    kiter = jter;
    cpledn(kn,iodd,pfn,zx,zxn,pmod);
    zx = zxn;
    if( iflag == 1 ) break;
    if( std::abs(pmod) <= peps*1000. )
      iflag = 1;
  }
  pl = zxn;
}

}

void compute_gaussian_colatitudes_hemisphere(const size_t N, double colat[])
{
  Log::info() << "Atlas computing Gaussian latitudes for N " << N << "\n";
  double z;
  for(size_t i=0; i<N; ++i )
  {
    z = (4.*(i+1.)-1.)*M_PI/(4.*2.*N+2.);
    colat[i] = ( z+1./(tan(z)*(8.*(2.*N)*(2.*N))) );
  }

  int kdgl = 2*N;
  array::ArrayT<double> zfn(kdgl+1,kdgl+1);
  std::vector<double> zzfn(N+1);
  std::vector<double> zmod(kdgl);
  std::vector<int> iter(kdgl);
  double zfnn;
  int iodd;
  int ik;
  double zeps;

  for( size_t n=0; n<zfn.size(); ++n )
    zfn.data()[n] = 0.;


  // Belousov, Swarztrauber use zfn(0,0)=std::sqrt(2.)
  // IFS normalisation chosen to be 0.5*Integral(Pnm**2) = 1
  zfn(0,0) = 2.;
  for( int jn=1; jn<=kdgl; ++jn )
  {
    zfnn = zfn(0,0);
    for( int jgl=1;jgl<=jn; ++jgl)
    {
      zfnn *= std::sqrt(1.-0.25/(static_cast<double>(jgl*jgl)));
    }
    iodd = jn % 2;
    zfn(jn,jn)=zfnn;
    for( int jgl=2; jgl<=jn-iodd; jgl+=2 )
    {
      zfn(jn,jn-jgl) = zfn(jn,jn-jgl+2) *
          static_cast<double>  ((jgl-1)*(2*jn-jgl+2)) /
          static_cast<double>  (jgl*(2*jn-jgl+1));
    }
  }

  iodd = kdgl % 2;
  ik = iodd;

  for( int jgl=iodd; jgl<=kdgl; jgl+=2 )
  {
    zzfn[ik] = zfn(kdgl,jgl);
    ++ik;
  }

  zeps = std::numeric_limits<double>::epsilon();
  for( int jgl=0; jgl<N; ++jgl )
  {
    // refine colat first gues here via Newton's method
    gawl(zzfn.data(),colat[jgl],zeps,kdgl,iter[jgl],zmod[jgl]);
    colat[jgl] *= util::Constants::radiansToDegrees();
  }
}

void compute_gaussian_latitudes_hemisphere(const size_t N, double lats[])
{
  std::vector<double> colat(N);
  compute_gaussian_colatitudes_hemisphere(N,colat.data());
  colat_to_lat_hemisphere(N,colat.data(),lats,internals::DEG);
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace grid
} // namespace atlas

