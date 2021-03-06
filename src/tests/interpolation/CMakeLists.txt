# (C) Copyright 2013 ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.

ecbuild_add_test( TARGET atlas_test_Quad3D
  CONDITION ECKIT_HAVE_EIGEN
  SOURCES   test_Quad3D.cc
  LIBS      atlas
)

ecbuild_add_test( TARGET atlas_test_interpolation_finite_element
  SOURCES   test_interpolation_finite_element.cc
  LIBS      atlas
)

ecbuild_add_test( TARGET atlas_test_interpolation_cubic_prototype
  SOURCES  test_interpolation_cubic_prototype.cc CubicInterpolationPrototype.h
  LIBS     atlas
)

ecbuild_add_executable( TARGET atlas_test_interpolation_structured2D
  SOURCES  test_interpolation_structured2D.cc
  LIBS     atlas
  NOINSTALL
)

ecbuild_add_test( TARGET atlas_test_interpolation_bilinear
  COMMAND atlas_test_interpolation_structured2D ARGS --scheme linear
)

ecbuild_add_test( TARGET atlas_test_interpolation_bicubic
  COMMAND atlas_test_interpolation_structured2D ARGS --scheme cubic
)

ecbuild_add_test( TARGET atlas_test_interpolation_biquasicubic
  COMMAND atlas_test_interpolation_structured2D ARGS --scheme quasicubic
)

