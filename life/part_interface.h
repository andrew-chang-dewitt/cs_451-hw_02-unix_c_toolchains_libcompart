#include <combin.h>
#include <compart_base.h>

#define NO_COMPARTS 2

/* * * * * * * * * * * * * * * * * * * * * * * *
 * Globals
 * -------
 *
 * includes comparts map, extension id,
 * & world history state
 *
 * declared as extern to be available to all
 * compartments
 * * * * * * * * * * * * * * * * * * * * * * * */
extern struct compart comparts[NO_COMPARTS];
extern struct extension_id *step_ext;
extern char *world_history;

/* * * * * * * * * * * * * * * * * * * * * * * *
 * Compartment extenion fn
 * * * * * * * * * * * * * * * * * * * * * * * */
struct extension_data ext_step(struct extension_data data);

/* * * * * * * * * * * * * * * * * * * * * * * *
 * (De)Serializers
 * -------
 *
 * for inter-compart comms
 * * * * * * * * * * * * * * * * * * * * * * * */

// pack (serialize to bytes) a given step number (integer)
// & world history (size int & string of states)
#ifndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_step_to_arg(unsigned long step_num,
                                      unsigned long size, char *history);
#else
struct extension_data ext_step_to_arg(unsigned long step_num,
                                      unsigned long size, char *history,
                                      int fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD

// unpack a step number (integer), size (integer), & world history (string of
// states) from a given argument & store each in given pointers
#ifndef LC_ALLOW_EXCHANGE_FD
void ext_step_from_arg(struct extension_data data, unsigned long *step_num_ptr,
                       unsigned long *size_ptr, char *history_ptr);
#else
void ext_step_from_arg(struct extension_data data, unsigned long *step_num_ptr,
                       unsigned long *size_ptr, char *history_ptr, int *fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD

// unpack a step number (integer), size (integer), & world history (string of
// states) from a given argument & store each in given pointers
#ifndef LC_ALLOW_EXCHANGE_FD
void ext_history_from_arg(struct extension_data data, unsigned long offset,
                          char *history_ptr);
#else
void ext_history_from_arg(struct extension_data data, unsigned long offset,
                          char *history_ptr, int *fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
