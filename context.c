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
		fprintf(stderr, "%s: open(\".\", O_RDONLY): %s\n", ctx->progname, strerror(errno));
		return -1;
	}
	if(ctx->wd)
	{
		if(chdir(ctx->wd) < 0)
		{
			fprintf(stderr, "%s: %s: %s\n", ctx->progname, ctx->wd, strerror(errno));
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
	static char *pathbuf;
	static size_t pbufsize;

	char *pp;
	const char *path, *p;
	size_t l, nl;
	
	nl = strlen(name);
	if(!(path = getenv("PATH")))
	{
		fprintf(stderr, "%s: Warning: PATH is unset\n", ctx->progname);
		return NULL;
	}
	l = strlen(path) + strlen(name) + 1;
	if(l > pbufsize)
	{
		if(!(pp = realloc(pathbuf, l)))
		{
			fprintf(stderr, "%s: realloc(..., %u): %s\n", ctx->progname, (unsigned) l, strerror(errno));
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
		fprintf(stderr, "%s: calloc(1, %u): %s\n", ctx->progname, (unsigned) sizeof(cmd_t), strerror(errno));
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
			fprintf(stderr, "[environment variable %s is set to %s]\n", p, cmdline);
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
			fprintf(stderr, "%s: realloc(..., %u): %s\n", cmd->context->progname, (unsigned) (sizeof(char *) * (cmd->argalloc + 8)), strerror(errno));
			return -1;
		}
		cmd->argalloc += 8;
		cmd->argv = p;
	}
	if(!(cmd->argv[cmd->argc] = strdup(arg)))
	{
		fprintf(stderr, "%s: strdup(\"%s\"): %s\n", cmd->context->progname, arg, strerror(errno));
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

	if(cmd->context->verbose)
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
				fprintf(stderr, "%s: *** Build phase failed with exit status %d.  (Ignored)\n", cmd->context->progname, r);
			}
			return 0;
		}
		fprintf(stderr, "%s: *** Build phase failed with exit status %d.  Stop.\n", cmd->context->progname, r);
	}
	return r;	
}

int
cmd_destroy(cmd_t *cmd)
{
	return 0;
}

