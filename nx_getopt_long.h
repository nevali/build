/* Copyright 2010 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef NX_GETOPT_LONG_H_
# define NX_GETOPT_LONG_H_              1

enum {
	nx_no_argument,
	nx_required_argument,
	nx_optional_argument
};

struct nx_option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

# ifdef __cplusplus
extern "C" {
# endif

int nx_getopt_long(int argc, char *const *argv, const char *shortopts, const struct nx_option *longopts, int *indexptr);

# ifdef __cplusplus
};
# endif

# undef no_argument
# undef required_argument
# undef optional_argument
# undef option
# undef getopt_long

# define no_argument                    nx_no_argument
# define required_argument              nx_required_argument
# define optional_argument              nx_optional_argument
# define option                         nx_option
# define getopt_long                    nx_getopt_long

#endif /*!NX_GETOPT_LONG_H_*/
