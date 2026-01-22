/*
Part of the "chopchop" project at the University of Pennsylvania, USA.
Authors: Ke Zhong, Henry Zhu, Zhilei Zheng, Nik Sultana. 2018, 2019.

   Copyright 2021 University of Pennsylvania

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef __LIBCOMPART_BASE
#define __LIBCOMPART_BASE

#include "compart_api.h"

const char * compart_name(void);
/* Log an entry to the compartmentalisation-related reporting channel. */
void compart_log(const char *, const size_t);

int libcompart_general_arg_buf_size(void);
int libcompart_msg_buf_size(void);
const char* libcompart_pitchfork_log_envar(void);
int libcompart_ext_arg_buf_size(void);

/* __LIBCOMPART_BASE */
#endif
