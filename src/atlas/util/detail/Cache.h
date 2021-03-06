/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "atlas/runtime/Log.h"
#include "atlas/util/ObjectHandle.h"

namespace atlas {
namespace util {

template <typename Key, typename Value>
class Cache {
public:
    using key_type     = Key;
    using value_type   = Value;
    using creator_type = std::function<value_type*()>;

    Cache( const std::string& name ) : name_( name ) {}

    virtual ~Cache() {}

    ObjectHandle<value_type> get_or_create( const key_type& key, const creator_type& creator ) {
        std::lock_guard<std::mutex> guard( lock_ );
        auto it = map_.find( key );
        if ( it != map_.end() ) {
            ObjectHandle<value_type> value = it->second;
            if ( value ) {
                Log::debug() << "Key \"" << key << "\" was found in cache \"" << name_ << "\"" << std::endl;
                return value;
            }
            else {
                Log::debug() << "Key \"" << key << "\" was found in cache \"" << name_
                             << "\" but is not valid. Revert to not found." << std::endl;
            }
        }
        Log::debug() << "Key \"" << key << "\" not found in cache \"" << name_ << "\" , creating new" << std::endl;
        ObjectHandle<value_type> value( creator() );
        map_[key] = value;
        return value;
    }

    void remove( const key_type& key ) {
        std::lock_guard<std::mutex> guard( lock_ );
        if ( map_.erase( key ) ) {
            Log::debug() << "Erased key \"" << key << "\" from cache \"" << name_ << "\"." << std::endl;
        }
        else {
            Log::debug() << "Tried to erase key \"" << key << "\" from cache \"" << name_ << "\" but it was not found."
                         << std::endl;
        }
    }

private:
    std::string name_;
    std::mutex lock_;
    std::map<key_type, ObjectHandle<value_type>> map_;
};

}  // namespace util
}  // namespace atlas
