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

#include <string>

#include "atlas/util/Object.h"
#include "atlas/util/ObjectHandle.h"

namespace eckit {
class Parametrisation;
}
namespace atlas {
namespace numerics {
class Method;
}
}  // namespace atlas
namespace atlas {
namespace field {
class FieldImpl;
}
}  // namespace atlas
namespace atlas {
class Field;
}

namespace atlas {
namespace numerics {

class NablaImpl : public util::Object {
public:
    NablaImpl( const Method&, const eckit::Parametrisation& );
    virtual ~NablaImpl();

    virtual void gradient( const Field& scalar, Field& grad ) const       = 0;
    virtual void divergence( const Field& vector, Field& div ) const      = 0;
    virtual void curl( const Field& vector, Field& curl ) const           = 0;
    virtual void laplacian( const Field& scalar, Field& laplacian ) const = 0;
};

// ------------------------------------------------------------------

class Nabla : public util::ObjectHandle<NablaImpl> {
public:
    using Handle::Handle;
    Nabla() = default;
    Nabla( const Method& );
    Nabla( const Method&, const eckit::Parametrisation& );

    void gradient( const Field& scalar, Field& grad ) const;
    void divergence( const Field& vector, Field& div ) const;
    void curl( const Field& vector, Field& curl ) const;
    void laplacian( const Field& scalar, Field& laplacian ) const;
};

// ------------------------------------------------------------------

class NablaFactory {
public:
    /*!
   * \brief build Nabla with factory key, constructor arguments
   * \return Nabla
   */
    static const Nabla::Implementation* build( const Method&, const eckit::Parametrisation& );

    /*!
   * \brief list all registered field creators
   */
    static void list( std::ostream& );
    static bool has( const std::string& name );

private:
    virtual const Nabla::Implementation* make( const Method&, const eckit::Parametrisation& ) = 0;

protected:
    NablaFactory( const std::string& );
    virtual ~NablaFactory();

private:
    std::string name_;
};

// ------------------------------------------------------------------

template <class T>
class NablaBuilder : public NablaFactory {
public:
    NablaBuilder( const std::string& name ) : NablaFactory( name ) {}

private:
    virtual const Nabla::Implementation* make( const Method& method, const eckit::Parametrisation& p ) {
        return new T( method, p );
    }
};

// ------------------------------------------------------------------

extern "C" {

void atlas__Nabla__delete( Nabla::Implementation* This );
const Nabla::Implementation* atlas__Nabla__create( const Method* method, const eckit::Parametrisation* params );
void atlas__Nabla__gradient( const Nabla::Implementation* This, const field::FieldImpl* scalar,
                             field::FieldImpl* grad );
void atlas__Nabla__divergence( const Nabla::Implementation* This, const field::FieldImpl* vector,
                               field::FieldImpl* div );
void atlas__Nabla__curl( const Nabla::Implementation* This, const field::FieldImpl* vector, field::FieldImpl* curl );
void atlas__Nabla__laplacian( const Nabla::Implementation* This, const field::FieldImpl* scalar,
                              field::FieldImpl* laplacian );
}

}  // namespace numerics
}  // namespace atlas
