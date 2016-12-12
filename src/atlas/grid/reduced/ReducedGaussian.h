#ifndef atlas_grid_reduced_ReducedGaussian_h
#define atlas_grid_reduced_ReducedGaussian_h

#include "atlas/grid/Structured.h"

namespace atlas {
namespace grid {
namespace reduced {

class ReducedGaussian: public Structured {

  public:

    static std::string grid_type_str();

    static std::string className();
    
    ReducedGaussian(): Structured() {};
    ReducedGaussian(const util::Config& params);

    eckit::Properties spec() const;
    
  protected:

    void setup(size_t N, long pl[]);

    //virtual void set_typeinfo() = 0;
    //static eckit::Value domain_spec(const Domain& dom);
    
};


}  // namespace reduced
}  // namespace grid
}  // namespace atlas


#endif
