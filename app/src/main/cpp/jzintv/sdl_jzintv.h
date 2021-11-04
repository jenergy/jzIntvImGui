/* Include the appropriate SDL headers based on which version we're building. */
#ifndef SDL_JZINTV_H_
#  define SDL_JZINTV_H_ 1
#  if !defined(USE_SDL2)
#    include <SDL/SDL.h>
#    include <SDL/SDL_audio.h>
#    include <SDL/SDL_events.h>
#    include <SDL/SDL_error.h>
#    if defined(__EMSCRIPTEN__)
#      include <SDL/SDL_mixer.h>
#    endif
#    include <SDL/SDL_thread.h>
#  else /* USE_SDL2 */
#    include <SDL.h>
#    include <SDL_audio.h>
#    include <SDL_events.h>
#    include <SDL_error.h>
#    include <SDL_thread.h>
#  endif /* USE_SDL2 */
#endif /* SDL_JZINTV_H_ */
