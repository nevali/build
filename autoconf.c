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

char *
autoconf_locate(build_context_t *ctx, int *res)
{
	static char *pbuf;
	char *cwd, *t;

	*res = 0;
	if(pbuf)
	{
		free(pbuf);
		pbuf = NULL;
	}
	if(!(cwd = getcwd(NULL, 0)))
	{
		context_msg(ctx, MSG_PERROR, "getcwd()");
		*res = -1;
		return NULL;
	}
	if(!(pbuf = calloc(1, strlen(cwd) + 32)))
	{
		context_msg(ctx, MSG_PERROR, "calloc(1, %u)", strlen(cwd) + 32);
		*res = -1;
		return NULL;
	}
	while(strlen(cwd) > 1)
	{
		sprintf(pbuf, "%s/configure.ac", cwd);
		if(access(pbuf, R_OK) != -1)
		{
			strcpy(pbuf, cwd);
			free(cwd);
			return pbuf;
		}
		sprintf(pbuf, "%s/configure.in", cwd);
		if(access(pbuf, R_OK) != -1)
		{
			strcpy(pbuf, cwd);
			free(cwd);
			return pbuf;
		}
		if(!(t = strrchr(cwd, '/')))
		{
			break;
		}
		*t = 0;
	}
	free(cwd);
	free(pbuf);
	pbuf = NULL;
	*res = -2;   
	return 0;
}
		
int
autoconf_chdir(build_context_t *ctx)
{
	int r;
	char *wd;

	if((wd = autoconf_locate(ctx, &r)))
	{
		if(chdir(wd) == -1)
		{
			context_msg(ctx, MSG_PERROR, "chdir(%s)", wd);
			return -1;
		}
		return 0;
	}
	return -1;
}

int
autoconf_detect(build_context_t *ctx)
{
	int r;
	
	if(ctx->project && !S_ISDIR(ctx->sbuf.st_mode))
	{
		/* The project path, if specified, must be a directory */
		return 0;
	}
	if(ctx->project)
	{
		if(chdir(ctx->project) < 0)
		{
			context_msg(ctx, MSG_PERROR, "chdir(%s)", ctx->project);
			return -1;
		}
	}
	if(autoconf_locate(ctx, &r))
	{
		return 1;
	}
	if(r == -1)
	{
		return -1;
	}
	return 0;
}

int
autoconf_prepare(build_context_t *ctx)
{
	cmd_t *cmd;
	int r, r1, r2;
	struct stat conf, csrc;

	if(ctx->project)
	{
		if(chdir(ctx->project) < 0)
		{
			fprintf(stderr, "%s: %s: %s\n", ctx->progname, ctx->project, strerror(errno));
			return -1;
		}
	}
	if(autoconf_chdir(ctx))
	{
		return -1;
	}
	/* TODO:
	 *  compare acinclude.m4/aclocal.m4, Makefile.am/Makefile.in, etc.
	 */
	if(ctx->isauto)
	{
		if(-1 == (r1 = stat("configure.gnu", &conf)))
		{
			r1 = stat("configure", &conf);
		}
		if(-1 == (r2 = stat("configure.ac", &csrc)))
		{
			if(-1 == (r2 = stat("configure.in", &csrc)))
			{
				context_msg(ctx, MSG_PERROR, "failed to locate configure.ac or configure.in");
				return -1;
			}
		}
		if(r1 == -1 || conf.st_mtime >= csrc.st_mtime)
		{
			/* configure script is up to date */
			context_msg(ctx, MSG_INFO, "configure script is up to date\n");
			context_returnwd(ctx);
			return -255;
		}
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
	int r;
	cmd_t *cmd;
	build_defn_t *p;

	/* If auto mode, we should only re-run configure if it's newer
	 * than our makefile.
	 */
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
	for(p = ctx->defs; p; p = p->hh.next)
	{
		if(!strncmp(p->name, "with-", 5) || !strncmp(p->name, "without-", 8) ||
		   !strncmp(p->name, "enable-", 7) || !strncmp(p->name, "disable-", 7))
		{
			if(p->value)
			{
				cmd_arg_addf(cmd, "--%s=%s", p->name, p->value);
			}
			else
			{
				cmd_arg_addf(cmd, "--%s", p->name);
			}
		}
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
	int r;
	cmd_t *cmd;
	build_defn_t *p;

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
