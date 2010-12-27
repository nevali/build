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

#ifdef ENABLE_XCODEBUILD
extern build_handler_t xcodebuild_handler;
#endif
extern build_handler_t gnumake_handler;
extern build_handler_t autoconf_handler;


static build_handler_t *handlers[] = {
#ifdef ENABLE_XCODEBUILD
	&xcodebuild_handler,
#endif
	&autoconf_handler,
	/* The GNU Make handler will always detect true if a regular file
	 * is passed as a project, so must be last.
	 */
	&gnumake_handler,
	NULL
};

void
context_handler_list(FILE *fout)
{
	size_t c;

	for(c = 0; handlers[c]; c++)
	{
		fprintf(fout, "  %-18s %s\n", handlers[c]->name, handlers[c]->summary);
	}
}

int
context_msg(build_context_t *ctx, int verbosity, const char *fmt, ...)
{
	va_list ap;
	int r, e;

	e = errno;
	/* If verbosity < 0, only show in verbose mode.
	 * If verbosity == 0, only show if not quiet.
	 * If verbosity > 0, always show.
	 */
	r = 0;
	if(verbosity < 0 && !ctx->verbose)
	{
		return 0;
	}
	if(!verbosity && ctx->quiet)
	{
		return 0;
	}
	va_start(ap, fmt);
	if(ctx->level)
	{
		r += fprintf(stderr, "%s[%d]: ", ctx->progname, ctx->level);
	}
	else
	{
		r += fprintf(stderr, "%s: ", ctx->progname);
	}
	if(verbosity >= MSG_FATAL)
	{
		r += fprintf(stderr, "*** ");
	}
	if(verbosity >= MSG_PERROR)
	{
		r += fprintf(stderr, ": %s\n", strerror(errno));
	}
	r += vfprintf(stderr, fmt, ap);
	va_end(ap);
	return r;
}

int
context_build(build_context_t *ctx, build_phase_t phase, int isauto)
{
	static const char *phases[] = { "distclean", "clean", "prepare", "config", "build", "install" };
	int wasauto, r, s;

	wasauto = ctx->isauto;
	ctx->isauto = isauto;
	context_msg(ctx, (ctx->isauto ? MSG_INFO : MSG_ECHO), "beginning phase '%s'.\n", phases[phase]);
	r = -255;
	switch(phase)
	{
	case PH_DISTCLEAN:
		if(ctx->vt->distclean)
		{
			r = ctx->vt->distclean(ctx);
		}
		ctx->configured = ctx->built = ctx->installed = 0;
		break;
	case PH_CLEAN:
		if(ctx->vt->clean)
		{
			r = ctx->vt->clean(ctx);
		}
		ctx->built = ctx->installed = 0;
		break;
	case PH_PREPARE:
		if(!ctx->prepared && ctx->vt->prepare)
		{
			r = ctx->vt->prepare(ctx);
		}
		ctx->prepared = 1;
		break;
	case PH_CONFIG:
		if(!ctx->only && !ctx->prepared)
		{
			if((s = context_build(ctx, PH_PREPARE, 1)))
			{
				ctx->isauto = wasauto;
				return s;
			}
		}
		if(!ctx->configured && ctx->vt->config)
		{
			r = ctx->vt->config(ctx);
		}
		ctx->configured = 1;
		break;
	case PH_BUILD:
		if(!ctx->only && !ctx->configured)
		{
			if((s = context_build(ctx, PH_CONFIG, 1)))
			{
				ctx->isauto = wasauto;
				return s;
			}
		}
		if(!ctx->built && ctx->vt->build)
		{
			r = ctx->vt->build(ctx);
		}
		ctx->built = 1;
		break;
	case PH_INSTALL:
		if(!ctx->only && !ctx->built)
		{
			if((s = context_build(ctx, PH_BUILD, 1)))
			{
				ctx->isauto = wasauto;
				return s;
			}
		}
		if(!ctx->installed && ctx->vt->install)
		{
			r = ctx->vt->install(ctx);
		}
		ctx->installed = 1;
		break;
	}
	if(r == -255)
	{
		context_msg(ctx, (ctx->isauto ? MSG_INFO : MSG_ECHO), "nothing to be done for phase '%s'.\n", phases[phase]);
		r = 0;
	}
	ctx->isauto = wasauto;
	return r;
}
		

build_handler_t *
context_detect(build_context_t *ctx)
{
	size_t c;
	int r;

	for(c = 0; handlers[c]; c++)
	{
		if((r = handlers[c]->detect(ctx)))
		{
			if(r < 0)
			{
				return NULL;
			}
			ctx->vt = handlers[c];
			return handlers[c];
		}
	}
	return NULL;
}


int
context_chdir(build_context_t *ctx)
{
	int here;

	if((here = open(".", O_RDONLY)) < 0)
	{
		context_msg(ctx, MSG_PERROR, "open(\".\", O_RDONLY)");
		return -1;
	}
	if(ctx->wd)
	{
		if(chdir(ctx->wd) < 0)
		{			
			context_msg(ctx, MSG_PERROR, "%s", ctx->wd);
			close(here);
			return -1;
		}
	}
	ctx->here = open(".", O_RDONLY);
	return here;
}

int
context_returnwd(build_context_t *ctx)
{
	return fchdir(ctx->here);
}

const char *
context_pathsearch(build_context_t *ctx, const char *name)
{
	static int pwarned;
	static char *pathbuf;
	static size_t pbufsize;

	char *pp;
	const char *path, *p;
	size_t l, nl;
	
	nl = strlen(name);
	if(!(path = getenv("PATH")))
	{
		if(!pwarned)
		{
			context_msg(ctx, MSG_ERROR, "warning: the PATH environment variable is not set.\n");
		}
		pwarned = 1;
		return NULL;
	}
	l = strlen(path) + strlen(name) + 1;
	if(l > pbufsize)
	{
		if(!(pp = realloc(pathbuf, l)))
		{
			context_msg(ctx, MSG_PERROR, "realloc(..., %u)", (unsigned) l);
			return NULL;
		}
		pathbuf = pp;
	}
	p = path;
	for(; p && path; path = p + 1)
	{
		p = strchr(path, ':');
		if(p)
		{
			l = p - path;
		}
		else
		{
			l = strlen(path);
		}
		strncpy(pathbuf, path, l);
		pathbuf[l] = '/';
		strcpy(&(pathbuf[l + 1]), name);
		if(access(pathbuf, R_OK|X_OK) != -1)
		{
			return pathbuf;
		}
	}
	return NULL;
}

/*
 * context_cmd_create(ctx, "name", "othername", "differentname", NULL, "ENVVAR", "ALTENV", NULL);
 */

cmd_t *
context_cmd_create(build_context_t *ctx, const char *name, ...)
{
	const char *cmdline, *p;
	va_list ap;
	cmd_t *cmd;

	if(!(cmd = calloc(1, sizeof(cmd_t))))
	{
		context_msg(ctx, MSG_PERROR, "calloc(1, %u)", (unsigned) sizeof(cmd_t));
		return NULL;
	}
	cmd->context = ctx;
	cmdline = NULL;
	p = name;
	/* Skip the executable names */
	for(va_start(ap, name); p; p = va_arg(ap, const char *))
	{
		/* Skip argument */
	}
	for(p = va_arg(ap, const char *); p; p = va_arg(ap, const char *))
	{
		if((cmdline = getenv(p)))
		{
			break;
		}
	}	
	if(!cmdline)
	{
		va_end(ap);
		p = name;
		for(va_start(ap, name); p; p = va_arg(ap, const char *))
		{
			if(strchr(p, '/'))
			{
				cmdline = p;
				break;
			}
			else if((cmdline = context_pathsearch(ctx, p)))
			{
				break;
			}
		}
	}
	if(!cmdline)
	{
		/* No matches; pick the last name and use that */
		va_end(ap);
		p = name;
		for(va_start(ap, name); p; p = va_arg(ap, const char *))
		{
			cmdline = p;
		}
	}	
	if(cmd_arg_add(cmd, cmdline) < 0)
	{
		cmd_destroy(cmd);
		return NULL;
	}
	return cmd;
}

int
cmd_arg_add(cmd_t *cmd, const char *arg)
{
	char **p;

	if(cmd->argalloc < cmd->argc + 2)
	{
		if(!(p = realloc(cmd->argv, sizeof(char *) * (cmd->argalloc + 8))))
		{
			context_msg(cmd->context, MSG_PERROR, "realloc(..., %u)", (unsigned) (sizeof(char *) * (cmd->argalloc + 8)));
			return -1;
		}
		cmd->argalloc += 8;
		cmd->argv = p;
	}
	if(!(cmd->argv[cmd->argc] = strdup(arg)))
	{
		context_msg(cmd->context, MSG_PERROR, "strdup(...[%u])", (unsigned) strlen(arg));
		return -1;
	}
	cmd->argc++;
	cmd->argv[cmd->argc] = 0;
	return 0;
}

int
cmd_arg_vaddf(cmd_t *cmd, const char *arg, va_list ap)
{
	static char buf[2048];
	
	vsnprintf(buf, sizeof(buf), arg, ap);
	return cmd_arg_add(cmd, buf);
}

int
cmd_arg_addf(cmd_t *cmd, const char *arg, ...)
{
	va_list ap;
	
	va_start(ap, arg);
	return cmd_arg_vaddf(cmd, arg, ap);
}

int
cmd_spawn(cmd_t *cmd, int ignore)
{
	pid_t pid, r;
	int status;
	size_t c;

	if(!cmd->context->quiet)
	{
		fputc('+', stderr);
		fputc(' ', stderr);
		for(c = 0; c < cmd->argc; c++)
		{			
			fputs(cmd->argv[c], stderr);
			fputc(' ', stderr);
		}
		fputc('\n', stderr);
	}
	if(cmd->context->dryrun)
	{
		return 0;
	}
	if(posix_spawnp(&pid, cmd->argv[0], NULL, NULL, cmd->argv, environ) < 0)
	{
		return -1;
	}
	do
	{
		r = waitpid(pid, &status, 0);
	}
	while(r == -1 && errno == EINTR);   
	if(WIFEXITED(status))
	{
		r = WEXITSTATUS(status);
	}
	else
	{
		r = 127;
	}
	if(r)
	{
		if(ignore)
		{
			if(!cmd->context->quiet)
			{
				context_msg(cmd->context, MSG_FATAL, "Build phase failed with exit status %d.  (Ignored)\n", r);
			}
			return 0;
		}
		context_msg(cmd->context, MSG_FATAL, "Build phase failed with exit status %d.  Stop.\n", r);
	}
	return r;	
}

int
cmd_destroy(cmd_t *cmd)
{
	return 0;
}

build_defn_t *
context_defn_add(build_context_t *ctx, const char *name, const char *value)
{
	build_defn_t *p;
	size_t l;

	if(!(p = malloc(sizeof(build_defn_t))))
	{
		context_msg(ctx, MSG_PERROR, "malloc(%u)", (unsigned) sizeof(build_defn_t));
		return NULL;
	}
	l = strlen(name) + 1;
	if(value)
	{
		l += strlen(value) + 1;
	}
	if(!(p->name = calloc(1, l)))
	{
		context_msg(ctx, MSG_PERROR, "malloc(%u)", (unsigned) l);
		free(p);
		return NULL;
	}
	strcpy(p->name, name);
	if(value)
	{
		p->value = &(p->name[strlen(name) + 1]);
		strcpy(p->value, value);
	}
	else
	{
		p->value = NULL;
	}
	HASH_ADD_KEYPTR(hh, ctx->defs, p->name, strlen(p->name), p);
	return p;
}

build_defn_t *
context_defn_find(build_context_t *ctx, const char *name)
{
	build_defn_t *p;

	HASH_FIND_STR(ctx->defs, name, p);
	return p;
}
