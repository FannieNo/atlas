/*
 * (C) Copyright 1996-2015 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#ifndef atlas_numerics_Nabla_h
#define atlas_numerics_Nabla_h

#include "eckit/memory/Owned.h"

namespace eckit { class Parametrisation; }
namespace atlas { namespace next { class FunctionSpace; } }
namespace atlas { class Field; }

namespace atlas {
namespace numerics {

class Nabla : public eckit::Owned {

public:
  Nabla(const next::FunctionSpace &, const eckit::Parametrisation &);
  virtual ~Nabla();

  virtual void gradient(const Field &field, Field &grad) = 0;

private:
  const next::FunctionSpace& fs_;
  const eckit::Parametrisation& config_;

};


// ------------------------------------------------------------------

class NablaFactory {
public:
    /*!
     * \brief build Nabla with factory key, constructor arguments
     * \return Nabla
     */
    static Nabla* build(const next::FunctionSpace &, const eckit::Parametrisation &);

    /*!
     * \brief list all registered field creators
     */
    static void list(std::ostream &);
    static bool has(const std::string& name);

private:
    virtual Nabla* make(const next::FunctionSpace &, const eckit::Parametrisation &) = 0 ;

protected:
    NablaFactory(const std::string&);
    virtual ~NablaFactory();

private:
  std::string name_;
};

// ------------------------------------------------------------------

template<class T>
class NablaBuilder : public NablaFactory {

public:
    NablaBuilder(const std::string& name) : NablaFactory(name) {}

private:
    virtual Nabla* make(const next::FunctionSpace & fs, const eckit::Parametrisation & p) {
        return new T(fs,p);
    }
};

// ------------------------------------------------------------------

} // namespace numerics
} // namespace atlas

#endif // atlas_numerics_Nabla_h