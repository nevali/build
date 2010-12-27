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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_build.h"

extern int gnumake_install(build_context_t *ctx);
extern int gnumake_clean(build_context_t *ctx);
extern int gnumake_build(build_context_t *ctx);

int
autoconf_detect(build_context_t *ctx)
{
	static const char *try[] = {
		"Makefile.am", "makefile.am", "Makefile.in", "makefile.in", 
		"GNUmakefile.in", "gnumakefile.in", "configure.ac",
		"configure.in"
	};
	size_t c;
	
	if(ctx->project && !S_ISDIR(ctx->sbuf.st_mode))
	{
		/* The project path, if specified, must be a directory */
		return 0;
	}
	if(ctx->project)
	{
		chdir(ctx->project);
	}
	for(c = 0; try[c]; c++)
	{
		if(!access(try[c], R_OK))
		{
			context_returnwd(ctx);
			return 1;
		}
	}
	return 0;
}

int
autoconf_prepare(build_context_t *ctx)
{
	cmd_t *cmd;
	int r;

	if(ctx->prepared)
	{
		return 0;
	}
	ctx->prepared = 1;
	if(ctx->project)
	{
		chdir(ctx->project);
	}
	cmd = context_cmd_create(ctx, "autoreconf", NULL, "BUILD_AUTORECONF", "AUTORECONF", NULL);
	cmd_arg_add(cmd, "--install");
	r = cmd_spawn(cmd, 0);
	cmd_destroy(cmd);
	context_returnwd(ctx);
	return r;
}

int
autoconf_config(build_context_t *ctx)
{
	int r, a;
	cmd_t *cmd;

	if(ctx->configured)
	{
		return 0;
	}
	AUTODEP(ctx, a, r, r = ctx->vt->prepare(ctx));
	ctx->configured = 1;
	cmd = context_cmd_create(ctx, "/bin/sh", NULL, NULL);
	if(ctx->project)
	{
		cmd_arg_addf(cmd, "%s/configure", ctx->project);
	}
	else
	{
		cmd_arg_add(cmd, "./configure");
	}
	if(ctx->build)
	{
		cmd_arg_addf(cmd, "--build=%s", ctx->build);
	}
	if(ctx->host)
	{
		cmd_arg_addf(cmd, "--host=%s", ctx->host);
	}
	if(ctx->target)
	{
		cmd_arg_addf(cmd, "--target=%s", ctx->target);
	}
	r = cmd_spawn(cmd, 0);
	cmd_destroy(cmd);
	return r;
}
	
int
autoconf_distclean(build_context_t *ctx)
{
	cmd_t *cmd;
	int r;

	ctx->built = 0;
	ctx->installed = 0;
	ctx->configured = 0;
	if(!ctx->quiet && !ctx->isauto && ctx->product)
	{
		fprintf(stderr, "%s: Warning: product specification '%s' is ignored by the 'distclean' phase of Makefile-based projects\n", ctx->progname, ctx->product);
	}
	cmd = context_cmd_create(ctx, "gnumake", "gmake", "make", NULL, "BUILD_MAKE", "MAKE", NULL);
	cmd_arg_add(cmd, "distclean");
	r = cmd_spawn(cmd, ctx->isauto);
	cmd_destroy(cmd);
	return r;
}

build_handler_t autoconf_handler = {
	autoconf_detect,
	autoconf_prepare,
	autoconf_config,
	gnumake_build,
	gnumake_install,
	gnumake_clean,
	autoconf_distclean
};
