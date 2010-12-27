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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "p_build.h"

extern build_handler_t gnumake_handler;

static void
gnumake_args(cmd_t *cmd, build_context_t *ctx)
{
	build_defn_t *p;

	if(ctx->build)
	{
		cmd_arg_addf(cmd, "BUILD_SYSTEM=%s", ctx->build);
	}
	if(ctx->host)
	{
		cmd_arg_addf(cmd, "HOST_SYSTEM=%s", ctx->host);
	}
	if(ctx->target)
	{
		cmd_arg_addf(cmd, "TARGET_SYSTEM=%s", ctx->target);
	}
	for(p = ctx->defs; p; p = p->hh.next)
	{
		if(p->name[0] == '-')
		{
			continue;
		}
		if(p->value)
		{
			cmd_arg_addf(cmd, "%s=%s", p->name, p->value);
		}
		else
		{
			cmd_arg_addf(cmd, "%s=1", p->name);
		}
	}
}

int
gnumake_detect(build_context_t *ctx)
{
	static const char *try[] = { "GNUmakefile", "gnumakefile", "Makefile", "makefile", NULL };
	static char *pbuf;
	size_t c, l;

	if(ctx->project && !S_ISDIR(ctx->sbuf.st_mode))
	{
		/* We can't really detect whether something's a Makefile or not */
		return 1;
	}
	if(ctx->project)
	{
		l = strlen(ctx->project);
		if(pbuf)
		{
			free(pbuf);
		}
		if(!(pbuf = (char *) calloc(1, l + 32)))
		{
			return -1;
		}
		strcpy(pbuf, ctx->project);
		pbuf[l] = '/';
		l++;
	}
	else
	{
		l = 0;
		if(!pbuf)
		{
			if(!(pbuf = (char *) calloc(1, 32)))
			{
				return -1;
			}
		}
	}
	for(c = 0; try[c]; c++)
	{
		strcpy(&(pbuf[l]), try[c]);
		if(!access(pbuf, R_OK))
		{
			ctx->project = pbuf;
			return 1;
		}
	}
	return 0;
}

int
gnumake_build(build_context_t *ctx)
{
	int r;
	cmd_t *cmd;

	cmd = context_cmd_create(ctx, "gnumake", "gmake", "make", NULL, "BUILD_MAKE", "MAKE", NULL);
	if(ctx->project && ctx->vt == &gnumake_handler)
	{
		cmd_arg_addf(cmd, "-f%s", ctx->project);
	}
	if(ctx->product)
	{
		cmd_arg_add(cmd, "--");
		cmd_arg_add(cmd, ctx->product);
	}
	if(ctx->project && ctx->vt == &gnumake_handler)
	{
		gnumake_args(cmd, ctx);
	}
	r = cmd_spawn(cmd, 0);
	cmd_destroy(cmd);
	return r;
}

int
gnumake_install(build_context_t *ctx)
{
	int r;
	cmd_t *cmd;
	
	if(!ctx->quiet && !ctx->isauto && ctx->product)
	{
		fprintf(stderr, "%s: Warning: product specification '%s' is ignored by the 'install' phase of Makefile-based projects\n", ctx->progname, ctx->product);
	}
	cmd = context_cmd_create(ctx, "gnumake", "gmake", "make", NULL, "BUILD_MAKE", "MAKE", NULL);
	if(ctx->project && ctx->vt == &gnumake_handler)
	{
		cmd_arg_addf(cmd, "-f%s", ctx->project);
	}
	cmd_arg_add(cmd, "install");
	gnumake_args(cmd, ctx);
	r = cmd_spawn(cmd, 0);
	cmd_destroy(cmd);
	return r;
}

int
gnumake_clean(build_context_t *ctx)
{
	cmd_t *cmd;
	int r;

	if(!ctx->quiet && !ctx->isauto && ctx->product)
	{
		fprintf(stderr, "%s: Warning: product specification '%s' is ignored by the 'clean' phase of Makefile-based projects\n", ctx->progname, ctx->product);
	}
	cmd = context_cmd_create(ctx, "gnumake", "gmake", "make", NULL, "BUILD_MAKE", "MAKE", NULL);
	if(ctx->project && ctx->vt == &gnumake_handler)
	{
		cmd_arg_addf(cmd, "-f%s", ctx->project);
	}
	cmd_arg_add(cmd, "clean");
	gnumake_args(cmd, ctx);
	r = cmd_spawn(cmd, ctx->isauto);
	cmd_destroy(cmd);
	return r;
}

build_handler_t gnumake_handler = {
	"gnumake",
	"Builds Makefile-based projects with GNU Make",
	gnumake_detect,
	NULL,
	NULL,
	gnumake_build,
	gnumake_install,
	gnumake_clean,
	NULL,
};
