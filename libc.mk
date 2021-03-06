ifeq ($(UNAME),Darwin)

LIBC_BIN=
LIBC_OFILES=
LIBC_LIBS=
LIBC_HFILES=

else

LIBC_BIN=\
	bin/musl-gcc\

LIBC_OFILES=\
	lib/Scrt1.o\
	lib/crt1.o\
	lib/rcrt1.o\
	lib/crti.o\
	lib/crtn.o\

LIBC_LIBS=\
	lib/libc.a\
	lib/libm.a\
	lib/librt.a\
	lib/libpthread.a\
	lib/libcrypt.a\
	lib/libutil.a\
	lib/libxnet.a\
	lib/libresolv.a\
	lib/libdl.a\

LIBC_HFILES=\
	include/aio.h\
	include/alloca.h\
	include/ar.h\
	include/arpa/ftp.h\
	include/arpa/inet.h\
	include/arpa/nameser.h\
	include/arpa/nameser_compat.h\
	include/arpa/telnet.h\
	include/arpa/tftp.h\
	include/assert.h\
	include/bits/alltypes.h\
	include/bits/endian.h\
	include/bits/errno.h\
	include/bits/fcntl.h\
	include/bits/fenv.h\
	include/bits/float.h\
	include/bits/io.h\
	include/bits/ioctl.h\
	include/bits/ipc.h\
	include/bits/limits.h\
	include/bits/mman.h\
	include/bits/msg.h\
	include/bits/poll.h\
	include/bits/posix.h\
	include/bits/reg.h\
	include/bits/resource.h\
	include/bits/sem.h\
	include/bits/setjmp.h\
	include/bits/shm.h\
	include/bits/signal.h\
	include/bits/socket.h\
	include/bits/stat.h\
	include/bits/statfs.h\
	include/bits/stdarg.h\
	include/bits/stdint.h\
	include/bits/syscall.h\
	include/bits/termios.h\
	include/bits/user.h\
	include/byteswap.h\
	include/complex.h\
	include/cpio.h\
	include/crypt.h\
	include/ctype.h\
	include/dirent.h\
	include/dlfcn.h\
	include/elf.h\
	include/endian.h\
	include/err.h\
	include/errno.h\
	include/fcntl.h\
	include/features.h\
	include/fenv.h\
	include/float.h\
	include/fmtmsg.h\
	include/fnmatch.h\
	include/ftw.h\
	include/getopt.h\
	include/glob.h\
	include/grp.h\
	include/iconv.h\
	include/ifaddrs.h\
	include/inttypes.h\
	include/iso646.h\
	include/langinfo.h\
	include/lastlog.h\
	include/libgen.h\
	include/libintl.h\
	include/limits.h\
	include/link.h\
	include/locale.h\
	include/malloc.h\
	include/math.h\
	include/memory.h\
	include/mntent.h\
	include/monetary.h\
	include/mqueue.h\
	include/net/ethernet.h\
	include/net/if.h\
	include/net/if_arp.h\
	include/net/route.h\
	include/netdb.h\
	include/netinet/ether.h\
	include/netinet/icmp6.h\
	include/netinet/if_ether.h\
	include/netinet/igmp.h\
	include/netinet/in.h\
	include/netinet/in_systm.h\
	include/netinet/ip.h\
	include/netinet/ip6.h\
	include/netinet/ip_icmp.h\
	include/netinet/tcp.h\
	include/netinet/udp.h\
	include/netpacket/packet.h\
	include/nl_types.h\
	include/paths.h\
	include/poll.h\
	include/pthread.h\
	include/pty.h\
	include/pwd.h\
	include/regex.h\
	include/resolv.h\
	include/sched.h\
	include/scsi/scsi.h\
	include/scsi/scsi_ioctl.h\
	include/scsi/sg.h\
	include/search.h\
	include/semaphore.h\
	include/setjmp.h\
	include/shadow.h\
	include/signal.h\
	include/spawn.h\
	include/stdalign.h\
	include/stdarg.h\
	include/stdbool.h\
	include/stddef.h\
	include/stdint.h\
	include/stdio.h\
	include/stdio_ext.h\
	include/stdlib.h\
	include/stdnoreturn.h\
	include/string.h\
	include/strings.h\
	include/stropts.h\
	include/sys/acct.h\
	include/sys/auxv.h\
	include/sys/cachectl.h\
	include/sys/dir.h\
	include/sys/epoll.h\
	include/sys/errno.h\
	include/sys/eventfd.h\
	include/sys/fanotify.h\
	include/sys/fcntl.h\
	include/sys/file.h\
	include/sys/fsuid.h\
	include/sys/inotify.h\
	include/sys/io.h\
	include/sys/ioctl.h\
	include/sys/ipc.h\
	include/sys/kd.h\
	include/sys/klog.h\
	include/sys/mman.h\
	include/sys/mount.h\
	include/sys/msg.h\
	include/sys/mtio.h\
	include/sys/param.h\
	include/sys/personality.h\
	include/sys/poll.h\
	include/sys/prctl.h\
	include/sys/procfs.h\
	include/sys/ptrace.h\
	include/sys/quota.h\
	include/sys/reboot.h\
	include/sys/reg.h\
	include/sys/resource.h\
	include/sys/select.h\
	include/sys/sem.h\
	include/sys/sendfile.h\
	include/sys/shm.h\
	include/sys/signal.h\
	include/sys/signalfd.h\
	include/sys/socket.h\
	include/sys/soundcard.h\
	include/sys/stat.h\
	include/sys/statfs.h\
	include/sys/statvfs.h\
	include/sys/stropts.h\
	include/sys/swap.h\
	include/sys/syscall.h\
	include/sys/sysinfo.h\
	include/sys/syslog.h\
	include/sys/sysmacros.h\
	include/sys/termios.h\
	include/sys/time.h\
	include/sys/timeb.h\
	include/sys/timerfd.h\
	include/sys/times.h\
	include/sys/timex.h\
	include/sys/ttydefaults.h\
	include/sys/types.h\
	include/sys/ucontext.h\
	include/sys/uio.h\
	include/sys/un.h\
	include/sys/user.h\
	include/sys/utsname.h\
	include/sys/vfs.h\
	include/sys/vt.h\
	include/sys/wait.h\
	include/sys/xattr.h\
	include/syscall.h\
	include/sysexits.h\
	include/syslog.h\
	include/tar.h\
	include/termios.h\
	include/tgmath.h\
	include/threads.h\
	include/time.h\
	include/uchar.h\
	include/ucontext.h\
	include/ulimit.h\
	include/unistd.h\
	include/utime.h\
	include/utmp.h\
	include/utmpx.h\
	include/values.h\
	include/wait.h\
	include/wchar.h\
	include/wctype.h\
	include/wordexp.h\

LIBC_ALL=\
	$(LIBC_BIN)\
	$(LIBC_OFILES)\
	$(LIBC_HFILES)\
	$(LIBC_LIBS)\
	include/sys/queue.h\

musl/Makefile musl/config.mak: musl/configure
	(cd musl && CC=gcc ./configure --prefix=$(CURDIR) --disable-shared)
	touch -c musl/Makefile

$(LIBC_BIN) $(LIBC_OFILES) $(LIBC_HFILES) $(LIBC_LIBS): $(shell find musl -type f -name '*.[ch]') musl/Makefile musl/config.mak
	make -C musl install CC=gcc
	touch -c $(LIBC_ALL)

include/sys/queue.h: queue.h
	mkdir -p include/sys
	cp queue.h include/sys/queue.h

endif
