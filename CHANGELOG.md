Changelog
=========

All notable changes to this project will be documented in this file.

This project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.16.0] - 2018-02-14
### Changed
- Interpolation makes use of OpenMP
- Cleanup of header includes
- fypp Fortran preprocessor is ported to fckit 0.6

### Added
- Parallel structured interpolation methods (2D,3D): linear, cubic, quasicubic
- Interpolation for multi-level and multi-variable fields
- atlas_Trace: Fortran API and use within OpenMP parallel regions
- StructuredColumns halo-exchange for vector fields
- Field::halo_exchange() function

### Fixed
- Fortran compilation with PGI 18.10
- Access to Field view within OpenMP parallel region
- FunctionSpaces use only required halo, even if larger halo is available in mesh
- Fixed faulty name of a Field when created through Fortran API, wrapping existing memory
- Fix NodeColumns functionspace when mesh is created from projected grid.
- Parallel interpolation from regular lonlat grid.
- Spectral spherical harmonics transforms for large cases

## [0.15.2] - 2018-08-31
### Changed
- Initialisation of Fields to signalling NaN in debug builds, uninitialised in
  non-debug builds (used to be initialised to zero as part of std::vector construction)

### Added
- Implementation of cropped unstructured grids so that spectral transforms to
  unstructured grids are allowed

### Fixed
- Spectral transforms to grids including pole and equator
- Build with gridtools CUDA backend

## [0.15.1] - 2018-07-17
### Fixed
- Compilation for Intel 18 debug
- Memory bug for spherical harmonics
- Compatibility with fckit 0.5.1

## [0.15.0] - 2018-06-19
### Changed
- Native Array data storage uses now a raw C pointer instead of std::vector
- Significant performance improvements to Spherical harmonics transforms

### Fixed
- Various bugs related to parallel halos
- Bit reproducibility for parallel interpolation

## [0.14.0] - 2018-03-22
### Added
- Spherical Harmonics transforms can receive a cache memory handle

### Changed
- Earth interface (C++)
- Requires eckit 0.20.0, fckit 0.5.0

## [0.13.2] - 2018-03-20
### Fixed
- C++ compilation using PGI 17, 18 and GCC 7
- Support Python 3 to generate Fortran bindings
- Travis CI linked with github repository
- Problem with CUDA allocated memory

## [0.13.1] - 2018-03-01
### Fixed
- Fortran compilation using Intel 18
- GridTools compatibility

## 0.13.0 - 2018-02-16

[Unreleased]: https://github.com/ecmwf/atlas/compare/master...develop
[0.16.0]: https://github.com/ecmwf/atlas/compare/0.15.2...0.16.0
[0.15.2]: https://github.com/ecmwf/atlas/compare/0.15.1...0.15.2
[0.15.1]: https://github.com/ecmwf/atlas/compare/0.15.0...0.15.1
[0.15.0]: https://github.com/ecmwf/atlas/compare/0.14.0...0.15.0
[0.14.0]: https://github.com/ecmwf/atlas/compare/0.13.2...0.14.0
[0.13.2]: https://github.com/ecmwf/atlas/compare/0.13.1...0.13.2
[0.13.1]: https://github.com/ecmwf/atlas/compare/0.13.0...0.13.1
