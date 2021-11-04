// ======================================================================== //
//  Utilities:                                                              //
//                                                                          //
//      class t_loc_to_bin                                                  //
//                                                                          //
//  You figure it out.  ;-)                                                 //
// ======================================================================== //

#ifndef LOC_TO_BIN_HPP_
#define LOC_TO_BIN_HPP_

// ------------------------------------------------------------------------ //
//  CLASS T_LOC_TO_BIN                                                      //
// ------------------------------------------------------------------------ //
class t_loc_to_bin_impl;
class t_loc_to_bin
{
  private:
    t_loc_to_bin_impl* impl;

  public:
    t_loc_to_bin
    (
        const t_locutus& locutus_,
        FILE* const      f_bin_,
        FILE* const      f_cfg_,
        const bool       self_close_ = false
    );

    t_loc_to_bin
    (
        const t_locutus&  locutus_,
        const char* const f_bin_path,
        const char* const f_cfg_path,
        const bool        self_close_ = true
    );

    bool                is_ok() const;
    bool                process();

    int                 get_f_bin_err()     const;
    int                 get_f_cfg_err()     const;
    uint32_t            get_file_offset()   const;
    int                 get_errors()        const;
    int                 get_warnings()      const;
    const t_string_vec& get_messages()      const;

    ~t_loc_to_bin();
};

#endif
