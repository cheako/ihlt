/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 5015  Michael Mestnik <cheako+github_com@mikemestnik.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <error.h>
#include <syslog.h>

#include <gpgme.h>

/* port we're listening on phone coded msg */
#define PORT "4458"

#include "server.h"
#include "relay_commands.h"

/* example from: http://www.4pmp.com/2009/12/a-simple-daemon-in-c/ */
#define DAEMON_NAME "ihlt"

void daemonShutdown();
void signal_handler(int sig);
void daemonize(char *pidfile);

int pidFilehandle;

void signal_handler(int sig) {
	switch (sig) {
	case SIGHUP:
		syslog(LOG_WARNING, "Received SIGHUP signal.");
		break;
	case SIGINT:
	case SIGTERM:
		syslog(LOG_INFO, "Daemon exiting");
		daemonShutdown();
		exit(EXIT_SUCCESS);
		break;
	default:
		syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
		break;
	}
}

void daemonShutdown() {
	close(pidFilehandle);
}

void no_daemonize(char *pidfile) {
}

void daemonize(char *pidfile) {
	int pid, sid, i;
	char str[10];
	struct sigaction newSigAction;
	sigset_t newSigSet;

	/* Check if parent process id is set */
	if (getppid() == 1) {
		/* PPID exists, therefore we are already a daemon */
		return;
	}

	/* Set signal mask - signals we want to block */
	sigemptyset(&newSigSet);
	sigaddset(&newSigSet, SIGCHLD); /* ignore child - i.e. we don't need to wait for it */
	sigaddset(&newSigSet, SIGTSTP); /* ignore Tty stop signals */
	sigaddset(&newSigSet, SIGTTOU); /* ignore Tty background writes */
	sigaddset(&newSigSet, SIGTTIN); /* ignore Tty background reads */
	sigprocmask(SIG_BLOCK, &newSigSet, NULL); /* Block the above specified signals */

	/* Set up a signal handler */
	newSigAction.sa_handler = signal_handler;
	sigemptyset(&newSigAction.sa_mask);
	newSigAction.sa_flags = 0;

	/* Signals to handle */
	sigaction(SIGHUP, &newSigAction, NULL); /* catch hangup signal */
	sigaction(SIGTERM, &newSigAction, NULL); /* catch term signal */
	sigaction(SIGINT, &newSigAction, NULL); /* catch interrupt signal */

	/* Fork*/
	pid = fork();

	if (pid < 0) {
		/* Could not fork */
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		/* Child created ok, so exit parent process */
		printf("Child process created: %d\n", pid);
		exit(EXIT_SUCCESS);
	}

	/* Child continues */

	umask(027); /* Set file permissions 750 */

	/* Get a new process group */
	sid = setsid();

	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	/* close all descriptors */
	for (i = getdtablesize(); i >= 0; --i) {
		close(i);
	}

	/* Route I/O connections */

	/* Open STDIN */
	i = open("/dev/null", O_RDWR);

	/* STDOUT */
	dup(i);

	/* STDERR */
	dup(i);

	chdir("/"); /* change running directory */

	/* Ensure only one copy */
	pidFilehandle = open(pidfile, O_RDWR | O_CREAT, 0600);

	if (pidFilehandle == -1) {
		/* Couldn't open lock file */
		syslog(LOG_INFO, "Could not open PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	/* Try to lock file */
	if (lockf(pidFilehandle, F_TLOCK, 0) == -1) {
		/* Couldn't get lock on lock file */
		syslog(LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	/* Get and format PID */
	sprintf(str, "%d\n", getpid());

	/* write pid to lockfile */
	write(pidFilehandle, str, strlen(str));
}

void main(int argc, char *argv[]) {
	struct ListenerOptions lopts;
	int c;
	void (*bg)(char *) = &daemonize;
	char *pidfile = "/tmp/ihlt.pid";
	int log_level = LOG_DEBUG, log_opts = LOG_PID;

	lopts.nodename = NULL;
	lopts.servname = PORT;
	lopts.path_home = getenv("HOME");
	lopts.path_name = basename(argv[0]);
	lopts.path_config = NULL;
	lopts.certfile = NULL;
	lopts.keyfile = NULL;
	memset(&lopts.hints, 0, sizeof(struct addrinfo));
	lopts.hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	lopts.hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	lopts.hints.ai_flags = AI_PASSIVE;

	/* Example from: http://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html */
	while (1) {
		static struct option long_options[] = { /**/
		{ "verbose", no_argument, NULL, 'v' }, /**/
		{ "quiet", no_argument, NULL, 'q' }, /**/
		{ "foreground", no_argument, NULL, 'f' }, /**/
		{ "stderr", no_argument, NULL, 's' }, /**/
		{ "err", no_argument, NULL, 's' }, /**/
		{ "pidfile", required_argument, NULL, 'P' }, /**/
		{ "bind", required_argument, NULL, 'b' }, /**/
		{ "port", required_argument, NULL, 'p' }, /**/
		{ 0, 0, 0, 0 } };
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "vqsSp:P:p:f", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			printf("option %s", long_options[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			break;

		case 'v':
			log_level--;
			break;

		case 'q':
			log_level++;
			break;

		case 's':
			log_opts |= LOG_PERROR;
			break;

		case 'f':
			bg = &no_daemonize;
			break;

		case 'P':
			pidfile = optarg;
			break;

		case 'p':
			lopts.servname = optarg;
			break;

		case 'b':
			lopts.nodename = optarg;
			break;

		case '?':
			/* getopt_long already printed an error message. */
			break;

		default:
			abort();
		}
	}

	gpgme_check_version(NULL);

	gpgme_ctx_t gm_ctx;
	gpgme_error_t err = gpgme_new(&gm_ctx);
	if (err) {
		error_at_line(EXIT_FAILURE, 0, __FILE__, (__LINE__ - 2),
				"creating GpgME context failed: %s: %s\n", gpgme_strsource(err),
				gpgme_strerror(err));
	}
	// GPGME_PK_ECDH
	// GPGME_MD_SHA256
	gpgme_set_armor(gm_ctx, 1);
	gpgme_set_textmode(gm_ctx, 1);
	// GPGME_DATA_ENCODING_ARMOR
	// GPGME_DATA_TYPE_PGP_KEY

	size_t path_len = strlen(lopts.path_home) + strlen(lopts.path_name) + 3;
	while (lopts.path_config == NULL)
		lopts.path_config = malloc(path_len);
	sprintf(lopts.path_config, "%s/.%s", lopts.path_home, lopts.path_name);
	char *path_key = NULL;
	path_len += 4;
	while (path_key == NULL)
		path_key = malloc(path_len);
	sprintf(path_key, "%s/key", lopts.path_config);

	path_len += 9;
	while (lopts.certfile == NULL)
		lopts.certfile = malloc(path_len);
	sprintf(lopts.certfile, "%s/certfile.txt", lopts.path_config);
	path_len -= 1;
	while (lopts.keyfile == NULL)
		lopts.keyfile = malloc(path_len);
	sprintf(lopts.keyfile, "%s/keyfile.txt", lopts.path_config);

	FILE *fh = fopen(path_key, "rb");
	char *key = NULL;
	if (fh != NULL) {
		fseek(fh, 0L, SEEK_END);
		long s = ftell(fh) + 1;
		rewind(fh);
		while (key == NULL)
			key = malloc(s);
		key[s] = '\0';
		fread(key, --s, 1, fh);
		// we can now close the file
		fclose(fh);

		key = strtok(key, "\r\n");

		gpgme_key_t akey;

		err = gpgme_get_key(gm_ctx, key, &akey, 1);
		if (err)
			error_at_line(EXIT_FAILURE, 0, __FILE__, (__LINE__ - 2),
					"failed to set key %s: %s: %s\n", key, gpgme_strsource(err),
					gpgme_strerror(err));

		free(key);

		err = gpgme_signers_add(gm_ctx, akey);
		if (gpg_err_code(err) != GPG_ERR_NO_ERROR)
			error_at_line(EXIT_FAILURE, 0, __FILE__, (__LINE__ - 2),
					"failed to call gpgme_signers_add(): %s: %s\n",
					gpgme_strsource(err), gpgme_strerror(err));

		gpgme_key_unref(akey);
	} else {
		mkdir(lopts.path_config, 0777);

		/*
		 * The next step is to create OpenPGP credentials for the server.

		 gpg --gen-key
		 ...enter whatever details you want, use 'test.gnutls.org' as name...

		 * Make a note of the OpenPGP key identifier of the newly generated key, here it was 5D1D14D8. You will need to export the key for GnuTLS to be able to use it.

		 gpg -a --export 5D1D14D8 > openpgp-server.txt
		 gpg --export 5D1D14D8 > openpgp-server.bin
		 gpg --export-secret-keys 5D1D14D8 > openpgp-server-key.bin
		 gpg -a --export-secret-keys 5D1D14D8 > openpgp-server-key.txt

		 *
		 */

		/*
		 fh = fopen(path_key, "wb");
		 if (fh == NULL )
		 perror("Error creating file");

		 key = "Hello";
		 fwrite(key, strlen(key), 1, fh);
		 fwrite("\n", 1, 1, fh);
		 close(fh);
		 */

	}

	unsigned int bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH,
			GNUTLS_SEC_PARAM_LEGACY);

	/* Generate Diffie-Hellman parameters - for use with DHE
	 * kx algorithms. These should be discarded and regenerated
	 * once a day, once a week or once a month. Depending on the
	 * security requirements.
	 */
	gnutls_dh_params_init(&lopts.dh_params);
	gnutls_dh_params_generate2(lopts.dh_params, bits);

	/* Logging */
	setlogmask(LOG_UPTO(log_level));
	openlog(DAEMON_NAME, log_opts, LOG_USER);

	syslog(LOG_INFO, "Daemon starting up");

	/* Deamonize */
	bg(pidfile);

	syslog(LOG_INFO, "Daemon running");

	init_relay();
	EnterListener(&lopts);
	exit(EXIT_SUCCESS);
}
