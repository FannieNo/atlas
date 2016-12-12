#ifndef atlas_grid_reduced_ClassicGaussian_h
#define atlas_grid_reduced_ClassicGaussian_h

#include "atlas/grid/reduced/ReducedGaussian.h"

namespace atlas {
namespace grid {
namespace reduced {

class ClassicGaussian: public ReducedGaussian {

  public:

    static std::string grid_type_str();

    static std::string className();
    
    ClassicGaussian(): ReducedGaussian() {};
    ClassicGaussian(const util::Config& params);

    eckit::Properties spec() const;
    
   
  protected:

    void setup(size_t N);

    //virtual void set_typeinfo() = 0;
    //static eckit::Value domain_spec(const Domain& dom);
 };


}  // namespace reduced
}  // namespace grid
}  // namespace atlas


#endif
