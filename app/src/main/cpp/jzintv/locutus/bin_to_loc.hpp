// ======================================================================== //
//  Utilities:                                                              //
//                                                                          //
//      class t_bin_to_loc                                                  //
//                                                                          //
//  You figure it out.  ;-)                                                 //
// ======================================================================== //

#ifndef BIN_TO_LOC_HPP_
#define BIN_TO_LOC_HPP_

// ------------------------------------------------------------------------ //
//  CLASS T_BIN_TO_LOC                                                      //
// ------------------------------------------------------------------------ //
class t_bin_to_loc_impl;
class t_bin_to_loc
{
  private:
    t_bin_to_loc_impl* impl;

  public:
    t_bin_to_loc
    (
        const char* const   bin_fn_,
        const char* const   cfg_fn_,
        t_locutus&          locutus_,
        const bool          do_macros = true
    );

    ~t_bin_to_loc();

    bool                process();
    bool                is_ok()             const;
    int                 get_errors()        const;
    int                 get_warnings()      const;
    const t_string_vec& get_messages()      const;
};

#endif
