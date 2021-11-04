// ======================================================================== //
//  LUIGI -- Locutus Universal Intellivision Game Image                     //
//                                                                          //
//  The classes defined here will either populate a Locutus object from a   //
//  LUIGI file, or generate a LUIGI file from a Locutus object.             //
//                                                                          //
//  Both classes are themselves data-less.  They just encapsulate           //
//  serialization / deserialization functionality.                          //
// ======================================================================== //
#ifndef LUIGI_HPP_
#define LUIGI_HPP_

#include "locutus.hpp"
#include <stdint.h>
#include <vector>

class t_luigi
{
  public:
    static void       deserialize( t_locutus& locutus,
                                   const t_byte_vec& data );
    static t_byte_vec serialize  ( const t_locutus& locutus );
};

#endif
