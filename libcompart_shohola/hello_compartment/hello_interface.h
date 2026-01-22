/*
Part of the "chopchop" project at the University of Pennsylvania, USA.
Authors: Nik Sultana. 2019, 2020.

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
#include <compart_base.h>
#include <combin.h>

#ifndef LIBCOMPART_VERSION_SHOHOLA
#error Using incompatible version of libcompart
#endif // LIBCOMPART_VERSION_SHOHOLA

#define NO_COMPARTS 3
extern struct compart comparts[NO_COMPARTS];

extern struct extension_id *add_ten_ext;
extern struct extension_id *add_zero_ext;

#ifndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_add_int_to_arg(int num);
#else
struct extension_data ext_add_int_to_arg(int num, int fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
#ifndef LC_ALLOW_EXCHANGE_FD
int ext_add_int_from_arg(struct extension_data data);
#else
int ext_add_int_from_arg(struct extension_data data, int *fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_add_ten(struct extension_data data);
struct extension_data ext_add_uid(struct extension_data data);
