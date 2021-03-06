#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <regex.h>

#include "cmd.h"
#include "common.h"

int main(int argc, char *argv[]){
	const char e_open[] = "%s: can't read %s: %s\n";
	const char e_mmap[] = "%s: can't mmap %s: %s\n";
	const char usage[] = "Usage: %s [ options ] [ script ] [ input-file ]\n"
	"\nOptions:\n"
	"\t-n\tSuppress pattern buffer printing\n"
	"\t-f\tExecute script from file\n"
	"\t-E\tExtended regex\n"
	"\t-h\tDisplay this help message and exit\n"
	"\t-V\tDisplay version and exit\n";
	const char *verstr = "sed v0.1";
	const char *opts = "Enf:hV";
	const char *script = "";
	unsigned int flags = 0U;
	int hflag = 0, vflag = 0, fflag = 0;
	int c;
	while( (c = getopt(argc, argv, opts)) != -1 ){
		switch(c){
			case 'n':
				flags |= NFLAG;
			break;

			case 'E':
				flags |= EFLAG;
			break;

			case 'f':
				fflag = 1;
				struct stat s;
				int size;
				int fd = open(optarg, O_RDONLY);
				if( fd < 0 ){
					fmtprint(STDERR, e_open, argv[0], optarg, strerror(errno));
					return 1;
				}
				(void)fstat(fd, &s);
				size = s.st_size;
				script = (char *) mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
				if( script == MAP_FAILED ){
					fmtprint(STDERR, e_mmap, argv[0], optarg, strerror(errno));
					return 1;
				}
			break;

			case 'h':
				hflag = 1;
			break;

			case 'V':
				vflag = 1;
			break;
		}
	}

	if( hflag ){
		fmtprint(STDERR, usage, argv[0]);
		return 0;
	}

	if( vflag ){
		fmtprint(STDERR, "%s\n", verstr);
		return 0;
	}

	int findex = optind;
	if( !fflag && argv[findex] != NULL ){
		script = argv[findex++];
	}

	if( *script == '\0' ) return 0;

	/* Check if stdin has data */
	int n = 0;
	(void)ioctl(STDIN, I_NREAD, &n);
	if( n > 0 || findex == argc || !isatty(STDIN) ){
		int result = run_script(script, STDIN, flags);
		if( result != 0 ){
			if( result > 0 ) fmtprint(STDERR, "%s\n", err_msg(result));
			if( result < 0 ) fmtprint(STDERR, "%s\n", strerror(abs(result)));
			return 1;
		}
		return 0;
	}

	for (; findex < argc; findex++){
		int fd = open(argv[findex], O_RDONLY);
		if( fd < 0 ){
			fmtprint(STDERR, e_open, argv[0], argv[findex], strerror(errno));
			continue;
		}
		int result = run_script(script, fd, flags);
		if( result != 0 ){
			if( result > 0 ) fmtprint(STDERR, "%s\n", err_msg(result));
			if( result < 0 ) fmtprint(STDERR, "%s\n", strerror(abs(result)));
			return 1;
		}
	}
	return 0;
}
