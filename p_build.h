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

#ifndef P_BUILD_H_
# define P_BUILD_H_                     1

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdarg.h>
# include <spawn.h>
# include <dirent.h>
# include <sys/types.h>
# include <sys/stat.h>

# include "nx_getopt_long.h"

# include "uthash.h"

# ifndef EXIT_SUCCESS
#  define EXIT_SUCCESS                  0
# endif

# ifndef EXIT_FAILURE
#  define EXIT_FAILURE                  1
# endif

#define AUTODEP(ctx, a, r, st) a = ctx->isauto; ctx->isauto = 1; if(!ctx->only) { st; if(r) return r; } ctx->isauto = a;

extern char **environ;

typedef struct build_context_s build_context_t;
typedef struct build_handler_s build_handler_t;
typedef struct build_defn_s build_defn_t;
typedef struct cmd_s cmd_t;

struct build_context_s
{
	build_handler_t *vt;
	/* Options */
	const char *wd;
	const char *progname;
	const char *project;
	const char *product;
	const char *build;
	const char *host;
	const char *target;
	const char *config;
	const char *sdk;
	const char *remote;
	build_defn_t *defs;
	/* State */
	struct stat sbuf;
	int here;
	int quiet;
	int verbose;
	int only;
	int dryrun;
	int isauto;
	int prepared;
	int configured;
	int built;
	int installed;
};

struct build_handler_s
{
	const char *name;
	const char *summary;
	int (*detect)(build_context_t *context);
	int (*prepare)(build_context_t *context);
	int (*config)(build_context_t *context);
	int (*build)(build_context_t *context);
	int (*install)(build_context_t *context);
	int (*clean)(build_context_t *context);
	int (*distclean)(build_context_t *context);
};

struct build_defn_s
{
	char *name;
	char *value;
	UT_hash_handle hh;
};

struct cmd_s
{
	build_context_t *context;
	size_t argc;
	size_t argalloc;
	char **argv;
};

# ifdef __cplusplus
extern "C" {
# endif

	void context_handler_list(FILE *fout);

	build_handler_t *context_detect(build_context_t *ctx);
	
	build_defn_t *context_defn_add(build_context_t *ctx, const char *name, const char *value);
	build_defn_t *context_defn_find(build_context_t *ctx, const char *name);

	int context_chdir(build_context_t *ctx);
	int context_returnwd(build_context_t *ctx);

	const char *context_pathsearch(build_context_t *ctx, const char *name);
	
	cmd_t * context_cmd_create(build_context_t *ctx, const char *cmd, ...);
	
	int cmd_arg_add(cmd_t *cmd, const char *arg);
	int cmd_arg_vaddf(cmd_t *cmd, const char *arg, va_list ap);
	int cmd_arg_addf(cmd_t *cmd, const char *arg, ...);

	int cmd_spawn(cmd_t *cmd, int ignore);

	int cmd_destroy(cmd_t *cmd);

# ifdef __cplusplus
};
# endif

#endif /*!P_BUILD_H_*/
