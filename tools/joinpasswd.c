/*
 * $Id: joinpasswd.c,v 1.14 2011/12/08 19:25:04 arekm Exp $
 *
 * Copyright (c) 2001 Michal Moskal <malekith@pld-linux.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Michal Moskal.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAL MOSKAL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 *
 * USAGE: joinpasswd
 *
 * Add entries from freshly created {passwd,shadow,group}.rpmnew to existing
 * {passwd,shadow,group}. It is usable as part of setup package, during upgrade
 * where new system user/group is to be added. If entry is already found in
 * system database, it is left, otherwise it is added. UIDs/GIDs are *not*
 * checked anyhow.
 *
 * For typical sizes of files in setup package, it takes about 1 second per
 * 20000 users in system database on Pentium class machine. After static link
 * against uClibc it is under 2k on x86. Stdio hasn't been used intentionally.
 *
 * Written for PLD Linux (http://www.pld-linux.org/) setup package.
 *
 * Compilation against uClibc:
 * UCROOT=/usr/lib/bootdisk/usr
 * gcc -I$UCROOT/include -nostdlib -O2 joinpasswd.c $UCROOT/lib/crt0.o \
 * 		$UCROOT/lib/libc.a -lgcc -o joinpasswd
 * strip -R .comment -R .note joinpasswd
 *
 * The idea of this program comes from Lukasz Dobrek <dobrek@pld-linux.org>.
 *
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define FILE1 "/etc/passwd"
#define FILE2 "/etc/shadow"
#define FILE3 "/etc/group"
#define FILE4 "/etc/gshadow"

/* #define OLD_LOCK */

#define LOCK_FILE "/etc/.pwd.lock"

#define SUFFIX ".rpmnew"
/* maybe "-" or sth? */
#define BACKUP ".old"

/* #define SILENT */

void eputs(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void fatal(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	eputs(fmt, args);
	va_end(args);

	eputs("\n");
	exit(1);
}

char *map_file(const char *name, int *sz)
{
	int fd;
	void *ptr;
	struct stat st;

	fd = open(name, O_RDONLY);
	if (fd == -1)
		return NULL;
	if (fstat(fd, &st) < 0)
		return NULL;
	*sz = st.st_size;
	ptr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED)
		return NULL;

	return ptr;
}

int exist(char *id, int id_len, char *ptr, int sz)
{
	int i;

	for (i = 0; i < sz; ) {
		if (sz - i > id_len && memcmp(id, ptr + i, id_len + 1) == 0)
			return 1;
		while (i < sz && ptr[i] != '\n')
			i++;
		i++;
	}

	return 0;
}

void itoa(char *buf, long i)
{
	char tmp[32];
	char *p;

	if (i < 0) {
		strcpy(buf, "-");
		buf++;
		i = -i;
	}
	if (i == 0) {
		strcpy(buf, "0");
		return;
	}
	for (p = tmp; i; i /= 10)
		*p++ = (i % 10) + '0';
	while (p > tmp)
		*buf++ = *--p;
	*buf = 0;
}

#ifndef OLD_LOCK
int lock_fd = -1;
void noop(int x)
{
	(void)x;
}
#endif

int try_lock(const char *name)
{
#ifdef OLD_LOCK
	char file[strlen(name) + 32], lock[strlen(name) + 32];
	char buf[32];
	int fd;
	long pid;

	strcpy(lock, name);
	strcpy(file, name);
	strcat(lock, ".lock");
	itoa(buf, (long)getpid());
	strcat(file, ".");
	strcat(file, buf);

	fd = open(file, O_WRONLY|O_CREAT|O_EXCL|O_TRUNC, 0600);
	if (fd < 0)
		return -1;
	if (write(fd, buf, strlen(buf)) < 0) {
		close(fd);
		return -1;
	}
	if (close(fd) < 0)
		return -1;

	if (link(file, lock) == 0) {
		unlink(file);
		return 0;
	}

	fd = open(lock, O_RDONLY);
	if (fd < 0)
		goto oops;
	memset(buf, 0, sizeof(buf));
	read(fd, buf, sizeof(buf));
	pid = atol(buf);
	if (pid == 0 || kill(pid, 0) != 0) {
		/* stale lock */
		unlink(file);
		unlink(lock);
		/* try again */
		return try_lock(name);
	}

oops:
	unlink(file);
	return -1;
#else
	struct flock fl;

	if (lock_fd != -1)
		return -1;
	lock_fd = open(LOCK_FILE, O_RDWR|O_CREAT, 0600);
	if (lock_fd == -1)
		return -1;
	signal(SIGALRM, noop);
	alarm(15);
	memset(&fl, 0, sizeof(fl));
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	if (fcntl(lock_fd, F_SETLKW, &fl) != 0) {
		alarm(0);
		close(lock_fd);
		lock_fd = -1;
		return -1;
	}
	alarm(0);

	return 0;
#endif
}

void unlock(const char *name)
{
#ifdef OLD_LOCK
	char lock[strlen(name) + 32];

	strcpy(lock, name);
	strcat(lock, ".lock");
	unlink(lock);
#else
	if (lock_fd != -1)
		close(lock_fd);
	lock_fd = -1;
#endif
}

void lock(const char *name)
{
	int n;

	n = 5;
	while (n--) {
		if (try_lock(name) == 0)
			return;
		eputs("waiting for lock...\n");
		sleep(1);
	}
	fatal("cannot get lock");
}

int join(const char *old_name, const char *new_name, const char *backup_name)
{
	char *old, *new, *id;
	int i, fd;
	int old_sz, new_sz;

	new = map_file(new_name, &new_sz);
	if (new == NULL)
		return -1;

	lock(old_name);
	old = map_file(old_name, &old_sz);
	if (old == NULL)
		fatal("cannot mmap file `%s': %m", old_name);

	fd = open(backup_name, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if (fd < 0)
		fatal("cannot create backup file `%s': %m", backup_name);
	if (write(fd, old, old_sz) < 0)
		fatal("writting to backup file `%s' failed: %m", backup_name);
	if (fsync(fd) < 0)
		fatal("syncing backup file `%s' failed: %m", backup_name);
	if (close(fd) < 0)
		fatal("closing backup file `%s' failed: %m", backup_name);

#ifndef SILENT
	eputs("merging content of `");
	eputs(old_name);
	eputs("' with `");
	eputs(new_name);
	eputs("'\n");
#endif /* SILENT */

	fd = open(old_name, O_WRONLY|O_APPEND);
	if (fd < 0)
		fatal("cannot open old file `%s': %m", old_name);

	for (i = 0; i < new_sz; ) {
		id = new + i;
		while (i < new_sz && new[i] != ':' && new[i] != '\n')
			i++;
		if (i < new_sz && new[i] == ':') {
			int id_len, line_len;

			id_len = i - (id - new);
			while (i < new_sz && new[i] != '\n')
				i++;
			if (i < new_sz)
				i++;
			line_len = i - (id - new);

			if (!exist(id, id_len, old, old_sz)) {
#ifndef SILENT
				eputs(old_name);
				eputs(": adding `");
				write(2, id, id_len);
				eputs("'\n");
#endif /* SILENT */
				if (write(fd, id, line_len) < 0)
					fatal("writting line to `%s' failed, check backup file: %m", old_name);
			}
		} else if (i < new_sz)
			i++;
	}
	fsync(fd);
	close(fd);

#if 0
	/* user may want to exime this file... */
	unlink(new_name);
#endif
	unlock(old_name);

	return 0;
}

int main()
{
#if 1
	join(FILE1, FILE1 SUFFIX, FILE1 BACKUP);
	join(FILE2, FILE2 SUFFIX, FILE2 BACKUP);
	join(FILE3, FILE3 SUFFIX, FILE3 BACKUP);
	join(FILE4, FILE4 SUFFIX, FILE4 BACKUP);
#else
	join("test", "test.new", "test.old");
#endif
	return 0;
}
