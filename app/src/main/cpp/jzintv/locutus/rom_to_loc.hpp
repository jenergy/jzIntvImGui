// ======================================================================== //
//  Utilities:                                                              //
//                                                                          //
//      class t_rom_to_loc                                                  //
//                                                                          //
//  You figure it out.  ;-)                                                 //
// ======================================================================== //

#ifndef ROM_TO_LOC_HPP_
#define ROM_TO_LOC_HPP_

// ------------------------------------------------------------------------ //
//  CLASS T_ROM_TO_LOC                                                      //
// ------------------------------------------------------------------------ //
class t_rom_to_loc_impl;
class t_rom_to_loc
{
  private:
    t_rom_to_loc_impl*  impl;

  public:
    t_rom_to_loc
    (
        icartrom_t*     icartrom_,
        t_locutus&      locutus_,
        uint32_t        crc_
    );

    ~t_rom_to_loc();

    bool                process();
    bool                is_ok()             const;
    int                 get_errors()        const;
    int                 get_warnings()      const;
    const t_string_vec& get_messages()      const;
};

#endif

