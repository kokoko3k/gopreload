/*
== fmlock -- map files into memory and lock them to RAM ==

Copyright (c) 2015, Wolfgang 'datenwolf' Draxinger
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of fmlock nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

static int heat_the_cache(int fd)
{
	char buf[1024];
	int rv;
	do {
	retry_read:
		rv = read(fd, buf, sizeof(buf));
		if( -1 == rv
		 && EINTR == errno ) {
			goto retry_read;
		}
	} while( 0 < rv );

	return rv != 0;
}

int main(int argc, char *argv[])
{
	unsigned long long locked_memory;
	int i, fd_null;
	fd_set phony_fdset;
	struct rlimit mlock_limit;

	if( 2 > argc ) {
		fprintf(stderr,
			"Usage:\n\n"
			"%s [filenames]\n\n"
			"Once its deed is done kill the process by sending a terminating signal.\n"
			"SIGINT (Ctrl+C) on the controlling terminal should do the trick.\n",
			argv[0] );
		return 1;
	}

	if( -1 == getrlimit(RLIMIT_MEMLOCK, &mlock_limit) ) {
		fprintf(stderr,
			"error getrlimit(RLIMIT_MEMLOCK): %s\n",
			strerror(errno) );
		return 1;
	}
	fprintf(stderr,
		"memlock rlimit: %llu\n",
		(unsigned long long)mlock_limit.rlim_cur );

	locked_memory = 0;
	for(i = 1; i < argc; ++i) {
		char const * const filename = argv[i];
		int fd;
		struct stat st;
		void *ptr;

		fprintf(stderr,
			"mlocking '%s'... ", filename);

	retry_open:
		fd = open(filename, O_RDONLY);
		if( -1 == fd ) {
			if( EINTR == errno ) {
				goto retry_open;
			} else
			{
				fprintf(stderr,
					"error open('%s'): %s\n",
					filename,
					strerror(errno) );
				continue;
			}
		}

		if( -1 == fstat(fd, &st) ) {
			fprintf(stderr,
				"error fstat(fd['%s']): %s\n",
				filename,
				strerror(errno) );
			goto finish_file;
		}

		if( locked_memory + st.st_size > mlock_limit.rlim_cur ) {
			fprintf(stderr,
				"error: exceeded the memlock resource limit for this process.\n"
				"Required limit to mlock '%s': %llu (counting already mlocked files)\n"
				"Locked memory resource limit set to: %llu\n"
				"Increase the locked memory resource limit and try again.\n",
				filename,
				(unsigned long long)locked_memory + st.st_size,
				(unsigned long long)mlock_limit.rlim_cur );
			close(fd);
			break;
		}

		ptr = mmap(
			NULL,
			st.st_size,
			PROT_READ,
			MAP_SHARED | MAP_LOCKED,
			fd,
			0 );
		if( MAP_FAILED == ptr ) {
			fprintf(stderr,
				"error mmap(fd['%s'], 0..%lld): %s\n",
				filename,
				(long long)st.st_size,
				strerror(errno) );
			goto finish_file;
		}

		if( -1 == mlock(ptr, st.st_size) ) {
			fprintf(stderr,
				"error mlock(ptr[fd['%s']]=%p): %s\n",
				filename,
				ptr,
				strerror(errno) );
			goto finish_file;
		}

		if( locked_memory + st.st_size < locked_memory ) {
			/* overflow */
			locked_memory = -1;
		} else
		{
			locked_memory += st.st_size;
		}

		heat_the_cache(fd);

	finish_file:
		close(fd);
		fputs("done\n", stderr);
	}

	if( !locked_memory ) {
		fprintf(stderr,
			"nothing locked, exiting.\n");
		return 1;
	}

	fprintf(stderr,
		"Files locked and cache heated up. pid=%lld. Going to sleep, .zZ...\n",
		(long long)getpid() );

	/* At this point the program shall sleep until a terminating
	 * signal arrives. To do so a nice side effect of the definition
	 * of /dev/null behavior is used: read on a /dev/null fd always
	 * return 0, which correspond to EOF which is a "no content
	 * available for read (yet)" situation for which select waits.
	 * So by selecting for a read a fd on /dev/null we can put the
	 * process to sleep. */
	fd_null = open("/dev/null", O_RDONLY);
	if( -1 == fd_null ) {
		fprintf(stderr,
			"error open('/dev/null'): %s\n",
			strerror(errno) );
		return 1;
	}
	FD_ZERO(&phony_fdset);
	FD_SET(fd_null, &phony_fdset);
	if( -1 == select(fd_null, &phony_fdset, NULL, NULL, NULL) ) {
		fprintf(stderr,
			"error select(...): %s\n",
			strerror(errno) );
		return 1;
	}

	return 0;
}

/* == Crazy Credits ==
 * Music listend to while writing this program:
 *     Carbon Based Lifeforms - Twentythree
 */

