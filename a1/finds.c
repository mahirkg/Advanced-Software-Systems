#include "apue.h"
#include "pathalloc.c"
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

/* function type that is called for each filename */
typedef	int	Myfunc(const char *, const struct stat *, int);

/* prototypes for the file tree walk and search */
static Myfunc	myfunc;
static int		myftw(char *, Myfunc *);
static int		dopath(Myfunc *);
static int find(const char *, int, const char *, const char *);

// flags for the command line input
static char *string = NULL;
static int typeSpecified = 0;
static int searchS = 0;
static int searchH = 0;
static int searchC = 0;
static int processSymLinks = 0;
static int wasThereAWildCard = 0;
static int wasThereADot = 0;
static int wasThereAQuestionMark = 0;
static int wasThereAStar = 0;
static int indexOfDot = 0;
static int indexOfQuestionMark = 0;
static int indexOfStar = 0;

/*
 * Macros for my_printf();
 * PP_ARG_N, PP_RSEQ_N, PP_N_ARG_ and PP_NARG are from:
 * http://stackoverflow.com/questions/11317474/macro-to-count-number-of-arguments
 */
#define PP_ARG_N( \
		_1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
		_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
		_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
		_31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
		_41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
		_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
		_61, _62, _63, N, ...) N
#define PP_RSEQ_N()                                       \
	62, 61, 60,                                       \
	59, 58, 57, 56, 55, 54, 53, 52, 51, 50,           \
	49, 48, 47, 46, 45, 44, 43, 42, 41, 40,           \
	39, 38, 37, 36, 35, 34, 33, 32, 31, 30,           \
	29, 28, 27, 26, 25, 24, 23, 22, 21, 20,           \
	19, 18, 17, 16, 15, 14, 13, 12, 11, 10,           \
	9,  8,  7,  6,  5,  4,  3,  2,  1,  0
#define PP_NARG_(...)    PP_ARG_N(__VA_ARGS__)    
#define PP_NARG(...)     PP_NARG_(_, ##__VA_ARGS__, PP_RSEQ_N())

#define my_printf(buffer, ...)  myprint(PP_NARG(__VA_ARGS__), buffer, ##__VA_ARGS__)

// prototypes to use my_print
static int myprint(int, char *, ...);
void itoa (char *, int, int);

	int
main(int argc, char *argv[])
{

	int ret;
	char *path = NULL;
	char *types = NULL;

	// parse command line
	int opt;
	while ((opt = getopt(argc, argv, "p:s:f:l")) != -1) {
		switch (opt) {
			case 'p':
				path = optarg;
				break;
			case 's':
				string = optarg;
				break;
			case 'f':
				typeSpecified = 1;
				types = optarg;
				break;	
			case 'l':
				processSymLinks = 1;
				break;
			default:
				my_printf("Usage: ./finds -p pathname -s \"string\" [-l] [-f c|h|S]\n");
				return(0);
		}
	}

	// make sure path flag was used and path was specified
	if (path == NULL) {
		my_printf("You need to specify a path using the -p flag.\n");
		return(0);
	}
	
	// make sure string flag was used and string was specified
	if (string == NULL) {
                my_printf("You need to specify a string using the -s flag.\n");
                return(0);
        }	

	// set file search flags
	int i = 0;
	if (typeSpecified == 1) {
		for (; i < strlen(types); i++) {
			if (types[i] == 'S') {
				searchS = 1;
			} else if (types[i] == 'h') {
				searchH = 1;
			} else if (types[i] == 'c') {
				searchC = 1;
			} else { // if type wasn't specified as any of the valid types
				my_printf("Invalid type specification.\n");
				return(0);
			}
		}
	}
	
	// set wildcard flags
	i = 0;
	int numDots, numStars, numQuestionMarks;
	numDots = 0;
	numStars = 0;
	numQuestionMarks = 0;
	for (; i < strlen(string); i++) {
		if (string[i] == '.') {
			numDots++;
			wasThereAWildCard = 1;
			wasThereADot = 1;
			indexOfDot = i;
		} else if (string[i] == '*') {
			numStars++;
			wasThereAWildCard = 1;
			wasThereAStar = 1;
			indexOfStar = i;
		} else if (string[i] == '?') {
			numQuestionMarks++;
			wasThereAWildCard = 1;
			wasThereAQuestionMark = 1;
			indexOfQuestionMark = i;
		} else if (!isalnum(string[i])) {
			my_printf("String is invalid. Cannot have nonalphanumeric values.\n");
			return(0);
		}
	}

	// make sure we don't have more than one of each wildcard
	if (numDots > 1 || numStars > 1 || numQuestionMarks > 1) {
		my_printf("String is invalid. Only allowed at most one of each control character.\n");
		return(0);
	}

	// make sure ? wildcard is used appropriately
	if (indexOfQuestionMark == 0 && wasThereAWildCard == 1 && numQuestionMarks == 1) {
		my_printf("String is invalid. Question mark wildcard cannot be the first character.\n");
		return(0);
	}

	// make sure * wildcard is used appropriately
	if (indexOfStar == 0 && wasThereAWildCard == 1 && numStars == 1) {
		my_printf("String is invalid. Question mark wildcard cannot be the first character.\n");
		return(0);
	}

	// assuming we can't have question mark and star wildcards next to each other
	if ((indexOfStar - indexOfQuestionMark == 1 || indexOfStar - indexOfQuestionMark == -1)
			&& wasThereAWildCard == 1 && numStars == 1 && numQuestionMarks == 1) {
		my_printf("Assuming we can't have ?* or *? combinations.\n");
		return(0);
	}

	ret = myftw(path, myfunc);		/* does it all */
	exit(ret);
}


/*** NOTICE - code from NOTICE to END NOTICE is code I got from  ***/
/*** https://github.com/SteveVallay/apue/blob/master/lib/error.c ***/
/*** BUT IT CAN ALSO BE FOUND ON THE BOOK'S SOURCE CODE SITE ***/
static void	err_doit(int, int, const char *, va_list);

	void
err_ret(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
}

	void
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

	void
err_exit(int error, const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, error, fmt, ap);
	va_end(ap);
	exit(1);
}

	void
err_dump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	abort();		/* dump core and terminate */
	exit(1);		/* shouldn't get here */
}

	void
err_msg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
}

	void
err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}

	static void
err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
	char	buf[MAXLINE];

	vsnprintf(buf, MAXLINE, fmt, ap);
	if (errnoflag)
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf), ": %s",
				strerror(error));
	strcat(buf, "\n");
	fflush(stdout);		/* in case stdout and stderr are the same */
	fputs(buf, stderr);
	fflush(NULL);		/* flushes all stdio output streams */
}


/*** END NOTICE ***/

/*
 * myftw and dopath come from the book's source code.
 * Descend through the hierarchy, starting at "pathname".
 * The caller's func() is called for every file.
 */
#define	FTW_F	1		/* file other than directory */
#define	FTW_D	2		/* directory */
#define	FTW_DNR	3		/* directory that can't be read */
#define	FTW_NS	4		/* file that we can't stat */

static char	*fullpath;		/* contains full pathname for every file */
static size_t pathlen;

	static int					/* we return whatever func() returns */
myftw(char *pathname, Myfunc *func)
{
	fullpath = path_alloc(&pathlen);	/* malloc PATH_MAX+1 bytes */
	/* ({Prog pathalloc}) */
	if (pathlen <= strlen(pathname)) {
		pathlen = strlen(pathname) * 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	strcpy(fullpath, pathname);
	return(dopath(func));
}

/*
 * myftw and dopath come from the book's source code
 * Descend through the hierarchy, starting at "fullpath".
 * If "fullpath" is anything other than a directory, we lstat() it,
 * call func(), and return.  For a directory, we call ourself
 * recursively for each name in the directory.
 */
	static int					/* we return whatever func() returns */
dopath(Myfunc* func)
{
	struct stat		statbuf;
	struct dirent	*dirp;
	DIR				*dp;
	int				ret, n;

	if (lstat(fullpath, &statbuf) < 0)	/* stat error */
		return(func(fullpath, &statbuf, FTW_NS));
	if (S_ISDIR(statbuf.st_mode) == 0)	/* not a directory */
		return(func(fullpath, &statbuf, FTW_F));

	/*
	 * It's a directory.  First call func() for the directory,
	 * then process each filename in the directory.
	 */
	if ((ret = func(fullpath, &statbuf, FTW_D)) != 0)
		return(ret);

	n = strlen(fullpath);
	if (n + NAME_MAX + 2 > pathlen) {	/* expand path buffer */
		pathlen *= 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	fullpath[n++] = '/';
	fullpath[n] = 0;

	if ((dp = opendir(fullpath)) == NULL)	/* can't read directory */
		return(func(fullpath, &statbuf, FTW_DNR));

	while ((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, ".") == 0  ||
				strcmp(dirp->d_name, "..") == 0)
			continue;		/* ignore dot and dot-dot */
		strcpy(&fullpath[n], dirp->d_name);	/* append name after "/" */
		if ((ret = dopath(func)) != 0)		/* recursive */
			break;	/* time to leave */
	}
	fullpath[n-1] = 0;	/* erase everything from slash onward */

	if (closedir(dp) < 0)
		err_ret("can't close directory %s", fullpath);
	return(ret);
}

	static int
myfunc(const char *pathname, const struct stat *statptr, int type)
{
	char last;
	char secondLast;


	switch (type) {
		case FTW_F:
			switch (statptr->st_mode & S_IFMT) {
				case S_IFREG:
					last = pathname[strlen(pathname) - 1]; 
					secondLast = pathname[strlen(pathname) - 2];
					if (typeSpecified) {
						if (secondLast == '.') {
							if (last == 'c' && !searchC) {
								return(0);
							} else if (last == 'h' && !searchH) {
								return(0);
							} else if (last == 'S' && !searchS) {
								return(0);
							} else if (last != 'c' && last != 'h' && last != 'S') {
								return(0);
							}
						} else if (secondLast != '.') {
							return(0);
						}
					}
					find(pathname, 0, NULL, string); /* searches the file */
					break;
				case S_IFLNK:
					if (processSymLinks) {
						/*** template for reading a symbolic link obtained from: ***/
						/*** https://www.securecoding.cert.org/confluence/display/seccode/POS30-C.+Use+the+readlink()+function+properly ***/
						char buff[1024];
						int length = readlink(pathname, buff, sizeof(buff) - 1);
						if (length == -1) {
							// error reading the symbolic link
							return(0);
						}
						buff[length] = '\0';
						char last = buff[length - 1];
						char secondLast = buff[length - 2];
						if (typeSpecified) {
							if (secondLast == '.') {
								if (last == 'c' && !searchC) {
									return(0);
								} else if (last == 'h' && !searchH) {
									return(0);
								} else if (last == 'S' && !searchS) {
									return(0);
								} else if (last != 'c' && last != 'h' && last != 'S') {
									return(0);
								}
							} else if (secondLast != '.') {
								return(0);
							}
						}
						int i = 0;
						char linkPointsTo[length + 1];
						for (; i < length + 1; i++) {
							linkPointsTo[i] = buff[i];
						}
						find(linkPointsTo, 1, pathname, string);
					}
					break;
				case S_IFDIR:	/* directories should have type = FTW_D */
					err_dump("for S_IFDIR for %s", pathname);
			}
			break;
		case FTW_D:
			//my_printf("file is a directory.\n");	
			break;
		case FTW_DNR:
			err_ret("can't read directory %s", pathname);
			break;
		case FTW_NS:
			err_ret("stat error for %s", pathname);
			break;
		default:
			err_dump("unknown type %d for pathname %s", type, pathname);
	}
	return(0);
}

static int
find(const char *pathname, int isSymLink, const char *linkPath, const char *stringToSearchFor) { 
	FILE *file = fopen(pathname, "r");
	char buffer[1024];
	if (file == NULL) {
		return(0);
	}
	while (fgets(buffer, 1024, file) != NULL) {
		char *b = &buffer[0];
		const char *s = &stringToSearchFor[0];
		while (*b != '\0') {
			if (*s == '\0') {
				if (buffer[strlen(buffer) - 1] != '\n') {
					if (isSymLink) {
						my_printf("%s -> ", linkPath);
					}
					my_printf("%s:%s\n", pathname, buffer);
				} else {
					if (isSymLink) {
						my_printf("%s -> ", linkPath);
					}
					my_printf("%s:%s", pathname, buffer);
				}
				break;
			} else if (*(s + 1) == '*') {
				if (*s == '.') {
					// following code searches for one or more instances of the dot
					char current = *b;
					while (*b == current) {
						b++;
					}
					s = s + 2;
					// need to add the search for zero instances of the dot
				} else {				
					while (*b == *s) {
						b++;
					}
					s = s + 2;
				}
			} else if (*(s + 1) == '?') {
				if (*s == '.') {
					// following code searches for one instance of the dot
					char current = *b;
					if (*b == current) {
						b++;
					}
					s = s + 2;
					// need to add the search for zero instances of the dot
				} else {
					if (*b == *s) {
						b++;
					}
					s = s + 2;
				}
			} else if ((*b == *s) || (*s == '.' && isalnum(*b))) {
				b++;
				s++;
			} else if (*b != *s) {
				b++;
				s = &string[0];
			}
		}
	}
	fclose(file);
	return(0);
}



/*** my_printf code is below this - my_printf(format, ...) is defined as a macro that you can call easing ***/

static int myprint(int numArgsAfterBuffer, char *buffer, ...) {
	int numArgsExpected = 0;
	char *cp;
	for (cp = buffer; *cp; cp++) {
		if (*cp == '\\' && *(cp + 1) == '%') {
			cp++;
			continue;
		} else if (*cp == '%' && (*(cp + 1) == 's' || *(cp + 1) == 'c' || *(cp + 1) == 'd' ||
					*(cp + 1) == 'u' || *(cp + 1) == 'x')) {
			cp++;
			numArgsExpected++;
		}
	}

	if (numArgsAfterBuffer != numArgsExpected) {
		return(0);
	}

	int bufferPosition = 0;
	int argumentNumber = 1;
	int *n = &buffer; /* n[2] is the first element in the ellipsis */
	for (; bufferPosition < strlen(buffer); bufferPosition++) {
		if (buffer[bufferPosition] != '%') {
			putchar(buffer[bufferPosition]);
		} else if (buffer[bufferPosition] == '%') {
			if (bufferPosition > 0 && buffer[bufferPosition - 1] == '\\') {
				continue;
			} else if (buffer[bufferPosition + 1] == 's') {
				char *buff = n[argumentNumber];
				int i = 0;
				while (buff[i] != '\0') {
					putchar(buff[i]);
					i++;
				}
				bufferPosition++;
				argumentNumber++;
			} else if (buffer[bufferPosition + 1] == 'u') {
				/* This case doesn't work yet
				 * output the unsigned integer stored in n[argumentNumber] 
				 */
				printf("%u", n[argumentNumber]);
				bufferPosition++;
				argumentNumber++;
			} else if (buffer[bufferPosition + 1] == 'c') {
				putchar(n[argumentNumber]);
				bufferPosition++;
				argumentNumber++;
			} else if (buffer[bufferPosition + 1] == 'd') {
				char buff[32];
				itoa(buff, 'd', n[argumentNumber]);				
				int i = 0;
				while (buff[i] != '\0') {
					putchar(buff[i]);
					i++;
				}
				bufferPosition++;
				argumentNumber++;
			} else if (buffer[bufferPosition + 1] == 'x') {
				char buff[32];
				itoa(buff, 'x', n[argumentNumber]);
				int i = 0;
				while (buff[i] != '\0') {
					putchar(buff[i]);
					i++;
				}
				bufferPosition++;
				argumentNumber++;
			}
		}
	}
	return 0;
}

/* Convert the integer D to a string and save the string in BUF. If
 *  * BASE is equal to 'd', interpret that D is decimal, and if BASE is
 *   * equal to 'x', interpret that D is hexadecimal. */
void itoa (char *buf, int base, int d)
{
	char *p = buf;
	char *p1, *p2;
	unsigned long ud = d;
	int divisor = 10;

	/* If %d is specified and D is minus, put `-' in the head. */
	if (base == 'd' && d < 0)
	{
		*p++ = '-';
		buf++;
		ud = -d;
	}
	else if (base == 'x')
		divisor = 16;

	/* Divide UD by DIVISOR until UD == 0. */
	do
	{
		int remainder = ud % divisor;

		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
	}
	while (ud /= divisor);

	/* Terminate BUF. */
	*p = 0;

	/* Reverse BUF. */
	p1 = buf;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
}

