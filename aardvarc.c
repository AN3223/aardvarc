#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERROR(msg, status) { fprintf(stderr, "aardvarc: %s\n", msg); exit(status); }

/* XXX more descriptive function comments */
/* XXX not sure if reading scripts byte-by-byte is a good idea WRT
 * scripts being modified during execution
 */

void*
ckmalloc(size_t size)
{
	void* p;

	p = malloc(size);
	if (!p)
		ERROR("out of memory", 125);
	return p;
}

void*
ckrealloc(void* p, size_t size)
{
	p = realloc(p, size);
	if (!p)
		ERROR("out of memory", 125);
	return p;
}

/* Evaluates $ from script, appends result to string */
/* XXX implement this */
static void
expand(FILE* script, char* out)
{
}

/* Reads/returns a word evaluated for quotes/escapes/comments/expansion. */
static char*
getword(FILE* script)
{
	int nest, esc, comment;
	size_t size, i;
	char* word;
	char quote;

	size = 5;
	word = ckmalloc(sizeof(char) * 1<<size);

	for (i = nest = esc = quote = comment = 0 ;; i++) {
		if (i == (size_t)(1<<size) - 1)
			word = ckrealloc(word, (sizeof(char) * 1<<(++size)));

		word[i] = getc(script);

		if (word[i] == EOF) {
			goto done;
		} else if (word[i] == '\0') {
			i--;
		} else if (comment) {
			if (word[i] == '\n')
				goto done;
			else
				i--;
		} else if (esc) {
			/* handle backslash (i.e., \) */
			esc = 0;
			if (word[i] == '\n') /* for splitting args across lines */
				i--;
		} else if (nest) {
			/* interpret all within {} as literal */
			switch (word[i]) {
			case '{':
				nest++;
				break;
			case '}':
				nest--;
				if (!nest)
					i--;
				break;
			}
		} else if (quote) {
			if (word[i] == quote) {
				quote = '\0'; i--; /* terminate quotes */
			} else if (quote == '"') {
				/* interpret appropriate chars as non-literal within "" */
				switch (word[i]) {
				case '\\': goto escape;
				case '$': goto expansion;
				}
			}
		} else {
			/* interpret current char as non-literal */
			switch (word[i]) {
			case '#':
				if (!i) {
					comment = 1;
					i--;
				}
				break;
			case '"':
			case '\'':
				quote = word[i]; i--; break;
			case '{':
				nest = 1; i--; break;
			case '\\':
escape:
				esc = 1; i--; break;
			case '$':
expansion:
				expand(script, word+i);
				break;
			case ' ':
			case '\t':
			case '\n':
				/* trim leading whitespace */
				if (!i) {
					i--; break;
				}
				/* fallthrough */
			case ';':
			case '|':
			case '&':
				goto done;
			}
		}
	}

done:
	word[i+1] = '\0';
	return word;
}

/* Reads a command line, executes it, returns exit code. */
static int
cmdloop(FILE* script)
{
	size_t size, argc, len;
	int state, status;
	char delimiter;
	char** argv;
	pid_t pid;

	size = 3;
	argv = ckmalloc(sizeof(char*) * 1<<size);

	for (argc = delimiter = 0; !delimiter; argc++) {
		if (argc == (size_t)(1<<size) - 1)
			argv = ckrealloc(argv, (sizeof(char*) * 1<<(++size)));

		argv[argc] = getword(script);

		len = strlen(argv[argc]);
		if (argv[argc][len-1] == EOF || strchr("\n;|&", argv[argc][len-1]))
			delimiter = argv[argc][len-1];

		argv[argc][len-1] = '\0'; /* truncate whitespace/delimiter */

		if (!len && !delimiter) /* prevent null args */
			argc--;
	}

	status = 0;
	argv[argc--] = NULL;

	if (!argv[0][0] && delimiter == EOF) {
		status = -1;
		goto done;
	} else if (!argv[0][0]) {
		goto done;
	}

	/* XXX implement background jobs and pipes */
	state = 0;
	pid = fork();
	if (pid < 0) {
		status = -1; /* XXX handle fork() error */
	} else if (!pid) {
		execvp(argv[0], argv); /* XXX handle error */
	} else {
		waitpid(pid, &state, 0);
		status = WEXITSTATUS(state);
	}

done:
	do
		free(argv[argc]);
	while (argc--);
	free(argv);
	return status;
}

int
main(int argc, char** argv)
{
	FILE* script;
	int status;
	char c;

	status = c = 0;
	script = NULL;

	while ((c = getopt(argc, argv, "hc:")) != -1) {
		switch(c) {
			case 'c':
				script = fmemopen(optarg, strlen(optarg), "r");
				break;
			case ':':
			case '?':
				status = 2;
				break;
			case 'h':
				fprintf(stderr, "usage: ...\n"); /* XXX add usage */
				return status;
		}
	}

	if (!script) {
		if (!argv[optind])
			script = stdin; /* XXX implement interactive shell */
		else if ((script = fopen(argv[optind], "r")))
			; /* XXX the rest of argv need to be stored as script arguments */
		else
			ERROR("could not open file", 127);
	}

	do
		status = cmdloop(script);
	while (status == 0);

	return (status < 0) ? (0) : (status);
}

