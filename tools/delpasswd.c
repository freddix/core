/*
 * $Id: delpasswd.c,v 1.3 2009/03/02 00:19:02 baggins Exp $
 *
 * Copyright (c) 2001 Michal Moskal <malekith@pld-linux.org>.
 * Copyright (c) 2009 Jan Rękorajski <baggins@pld-linux.org>.
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
 * USAGE: delpasswd [-u|-g] name1 name2 ...
 *
 * Remove specified groups from /etc/{passwd,shadow,group}.
 * It is usable as part of setup package, during upgrade where some system
 * users/groups should be removed. UIDs/GIDs are *not* checked anyhow.
 *
 * Written for PLD Linux (http://www.pld-linux.org/) setup package.
 *
 * Compilation against uClibc:
 * UCROOT=/usr/lib/bootdisk/usr
 * gcc -I$UCROOT/include -nostdlib -O2 delpasswd.c $UCROOT/lib/crt0.o \
 * 		$UCROOT/lib/libc.a -lgcc -o delpasswd
 * strip -R .comment -R .note delpasswd
 *
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if 0
#define FILE1 "passwd"
#define FILE2 "shadow"
#define FILE3 "group"
#define FILE4 "gshadow"
#else
#define FILE1 "/etc/passwd"
#define FILE2 "/etc/shadow"
#define FILE3 "/etc/group"
#define FILE4 "/etc/gshadow"
#endif

/* #define OLD_LOCK */

#define LOCK_FILE "/etc/.pwd.lock"

/* maybe "-" or sth? */
#define BACKUP ".old"

/* #define SILENT */

void eputs(const char *msg)
{
	write(2, msg, strlen(msg));
}

void fatal(const char *msg)
{
	eputs(msg);
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

int exist(char *id, int id_len, int namesc, const char **names)
{
	int i;

	for (i = 0; i < namesc; i++) {
		if (strlen(names[i]) == id_len && memcmp(id, names[i], id_len) == 0)
			return 1;
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
	write(fd, buf, strlen(buf));
	close(fd);

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

int verifyp(const char *old_name, int namesc, const char **names)
{
	char *old, *id;
	int i;
	int old_sz;

	// Fail silently if file does not exist
	if (access(old_name, F_OK) == -1)
		return -1;

	old = map_file(old_name, &old_sz);
	if (old == NULL)
		fatal("cannot mmap old");

	for (i = 0; i < old_sz; ) {
		id = old + i;
		while (i < old_sz && old[i] != ':' && old[i] != '\n')
			i++;
		if (i < old_sz && old[i] == ':') {
			int id_len, line_len;

			id_len = i - (id - old);
			while (i < old_sz && old[i] != '\n')
				i++;
			if (i < old_sz)
				i++;
			line_len = i - (id - old);

			if (exist(id, id_len, namesc, names))
				return 1;
		} else if (i < old_sz)
			i++;
	}
	return 0;
}

int delp(const char *old_name, const char *backup_name,
		int namesc, const char **names)
{
	char *old, *tmp, *id;
	int i, fd;
	int old_sz;

	// Fail silently if file does not exist
	if (access(old_name, F_OK) == -1)
		return -1;

	lock(old_name);
	tmp = map_file(old_name, &old_sz);
	if (tmp == NULL)
		fatal("cannot mmap old for backup");

	fd = open(backup_name, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if (fd < 0)
		fatal("cannot make backup");
	write(fd, tmp, old_sz);
	close(fd);

	old = map_file(backup_name, &old_sz);
	if (old == NULL)
		fatal("cannot mmap old");

#ifndef SILENT
	eputs("removing from `");
	eputs(old_name);
	eputs("'\n");
#endif /* SILENT */

	fd = open(old_name, O_WRONLY|O_TRUNC);
	if (fd < 0)
		fatal("cannot open old file");

	for (i = 0; i < old_sz; ) {
		id = old + i;
		while (i < old_sz && old[i] != ':' && old[i] != '\n')
			i++;
		if (i < old_sz && old[i] == ':') {
			int id_len, line_len;

			id_len = i - (id - old);
			while (i < old_sz && old[i] != '\n')
				i++;
			if (i < old_sz)
				i++;
			line_len = i - (id - old);

			if (!exist(id, id_len, namesc, names)) {
				write(fd, id, line_len);
			}
#ifndef SILENT
			else {
				eputs(old_name);
				eputs(": removing `");
				write(2, id, id_len);
				eputs("'\n");
			}
#endif /* SILENT */
		} else if (i < old_sz)
			i++;
	}

	close(fd);
	unlock(old_name);

	return 0;
}

int main(int argc, const char **argv)
{
	int what = 0;

	if (argc < 3)
		fatal("Usage: delpasswd [-u|-g] name1 name2 ... nameN");

	if (strncmp(argv[1], "-u", 2) == 0)
		what = 1;
	else if (strncmp(argv[1], "-g", 2) == 0)
		what = 2;

	if (what == 0)
		fatal("Usage: delpasswd [-u|-g] name1 name2 ... nameN");
#if 1
	if (what == 1) {
		if (verifyp(FILE1, argc-2, argv+2))
			delp(FILE1, FILE1 BACKUP, argc-2, argv+2);
		if (verifyp(FILE2, argc-2, argv+2))
			delp(FILE2, FILE2 BACKUP, argc-2, argv+2);
	}
	if (what == 2) {
		if (verifyp(FILE3, argc-2, argv+2))
			delp(FILE3, FILE3 BACKUP, argc-2, argv+2);
		if (verifyp(FILE4, argc-2, argv+2))
			delp(FILE4, FILE4 BACKUP, argc-2, argv+2);
	}
#else
	delp("test", "test.old", argc-2, argv+2);
#endif
	return 0;
}
