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
		if(chdir(ctx->project) < 0)
		{
			fprintf(stderr, "%s: %s: %s\n", ctx->progname, ctx->project, strerror(errno));
			return -1;
		}
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
		if(chdir(ctx->project) < 0)
		{
			fprintf(stderr, "%s: %s: %s\n", ctx->progname, ctx->project, strerror(errno));
			return -1;
		}
	}
	/* TODO:
	 * 1. Find project root
	 * 2. Is it configure.in or configure.ac ?
	 * 3. Is it Makefile, makefile, GNUmakefile.{in,am} ?
	 * 4. If auto mode, only rebuild if we detect changes
	 */
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
	build_defn_t *p;

	if(ctx->configured)
	{
		return 0;
	}
	AUTODEP(ctx, a, r, r = ctx->vt->prepare(ctx));
	/* If auto mode, we should only re-run configure if it's newer
	 * than our makefile.
	 */
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
	if((p = context_defn_find(ctx, "prefix")) && p->value)
	{
		cmd_arg_addf(cmd, "--prefix=%s", p->value);
	}
	if((p = context_defn_find(ctx, "exec-prefix")) && p->value)
	{
		cmd_arg_addf(cmd, "--exec-prefix=%s", p->value);
	}
	if((p = context_defn_find(ctx, "bindir")) && p->value)
	{
		cmd_arg_addf(cmd, "--bindir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "libdir")) && p->value)
	{
		cmd_arg_addf(cmd, "--libdir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "sbindir")) && p->value)
	{
		cmd_arg_addf(cmd, "--sbindir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "sysconfdir")) && p->value)
	{
		cmd_arg_addf(cmd, "--sysconfdir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "libexecdir")) && p->value)
	{
		cmd_arg_addf(cmd, "--libexec=%s", p->value);
	}
	if((p = context_defn_find(ctx, "sharedstatedir")) && p->value)
	{
		cmd_arg_addf(cmd, "--sharedstatedir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "localstatedir")) && p->value)
	{
		cmd_arg_addf(cmd, "--localstatedir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "oldincludedir")) && p->value)
	{
		cmd_arg_addf(cmd, "--oldincludedir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "datarootdir")) && p->value)
	{
		cmd_arg_addf(cmd, "--datarootdir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "localedir")) && p->value)
	{
		cmd_arg_addf(cmd, "--localedir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "docdir")) && p->value)
	{
		cmd_arg_addf(cmd, "--docdir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "infodir")) && p->value)
	{
		cmd_arg_addf(cmd, "--infodir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "mandir")) && p->value)
	{
		cmd_arg_addf(cmd, "--mandir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "htmldir")) && p->value)
	{
		cmd_arg_addf(cmd, "--htmldir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "dvidir")) && p->value)
	{
		cmd_arg_addf(cmd, "--dvidir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "pdfdir")) && p->value)
	{
		cmd_arg_addf(cmd, "--pdfdir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "psdir")) && p->value)
	{
		cmd_arg_addf(cmd, "--psdir=%s", p->value);
	}
	if((p = context_defn_find(ctx, "program-prefix")) && p->value)
	{
		cmd_arg_addf(cmd, "--program-prefix=%s", p->value);
	}
	if((p = context_defn_find(ctx, "program-suffix")) && p->value)
	{
		cmd_arg_addf(cmd, "--program-suffix=%s", p->value);
	}
	if((p = context_defn_find(ctx, "program-transform-name")) && p->value)
	{
		cmd_arg_addf(cmd, "--program-transform-name=%s", p->value);
	}

	if((p = context_defn_find(ctx, "CPP")) && p->value)
	{
		cmd_arg_addf(cmd, "CPP=%s", p->value);
	}
	if((p = context_defn_find(ctx, "CPPFLAGS")) && p->value)
	{
		cmd_arg_addf(cmd, "CPPFLAGS=%s", p->value);
	}
	if((p = context_defn_find(ctx, "CC")) && p->value)
	{
		cmd_arg_addf(cmd, "CC=%s", p->value);
	}
	if((p = context_defn_find(ctx, "CFLAGS")) && p->value)
	{
		cmd_arg_addf(cmd, "CFLAGS=%s", p->value);
	}
	if((p = context_defn_find(ctx, "LDFLAGS")) && p->value)
	{
		cmd_arg_addf(cmd, "LDFLAGS=%s", p->value);
	}
	if((p = context_defn_find(ctx, "LIBS")) && p->value)
	{
		cmd_arg_addf(cmd, "LIBS=%s", p->value);
	}

	r = cmd_spawn(cmd, 0);
	cmd_destroy(cmd);
	return r;
}

int
autoconf_install(build_context_t *ctx)
{
	int r, a;
	cmd_t *cmd;
	build_defn_t *p;

	if(ctx->installed)
	{
		return 0;
	}
	AUTODEP(ctx, a, r, r = ctx->vt->build(ctx));
	ctx->installed = 1;
	if(!ctx->quiet && !ctx->isauto && ctx->product)
	{
		fprintf(stderr, "%s: Warning: product specification '%s' is ignored by the 'install' phase of Makefile-based projects\n", ctx->progname, ctx->product);
	}
	cmd = context_cmd_create(ctx, "gnumake", "gmake", "make", NULL, "BUILD_MAKE", "MAKE", NULL);
	cmd_arg_add(cmd, "install");
	if((p = context_defn_find(ctx, "DESTDIR")) && p->value)
	{
		cmd_arg_addf(cmd, "DESTDIR=%s", p->value);
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
	"autoconf",
	"Builds GNU autoconf projects",
	autoconf_detect,
	autoconf_prepare,
	autoconf_config,
	gnumake_build,
	autoconf_install,
	gnumake_clean,
	autoconf_distclean
};
