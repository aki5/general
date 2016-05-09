
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <evhtp/evhtp.h>

void
sendstr(evhtp_request_t *req, void *a) {
	const char *str = a;
	evbuffer_add(req->buffer_out, str, strlen(str));
	evhtp_send_reply(req, EVHTP_RES_OK);
}

int
main(int argc, char ** argv)
{
	evbase_t *evbase;
	evhtp_t  *htp;
	char *pempath;
	char *host;
	int opt, port;

	pempath = NULL;
	host = "0.0.0.0";
	port = 8443;
	while((opt = getopt(argc, argv, "c:l:")) != -1){
		char *p;
		switch(opt){
		case 'c':
			pempath = optarg;
			break;
		case 'l':
			if((p = strchr(optarg, ':')) != NULL){
				port = strtol(p+1, NULL, 10);
			} else if(optarg[0] >= '0' && optarg[0] <= '9'){
				port = strtol(optarg, NULL, 10);
			} else {
				host = optarg;
			}
			break;
		default:
		caseusage:
			fprintf(stderr, "usage: %s -c path/to/file.pem [-l host:port]\n", argv[0]);
			exit(1);
		}
	}

	if(pempath == NULL)
		goto caseusage;

	evbase = event_base_new();
	htp = evhtp_new(evbase, NULL);
	evhtp_set_cb(htp, "/", sendstr, "Hello simple World");

	evhtp_use_threads(htp, NULL, 8, NULL);

	char ciphers[] =
		"ECDH+AESGCM:"
		"DH+AESGCM:"
		"ECDH+AES256:"
		"DH+AES256:"
		"ECDH+AES128:"
		"DH+AES:"
		"ECDH+3DES:"
		"DH+3DES:"

		"RSA+AESGCM:"
		"RSA+AES:"
		"RSA+3DES:"

		"!aNULL:"
		"!MD5";

	evhtp_ssl_cfg_t sslconf;
	memset(&sslconf, 0, sizeof sslconf);
	sslconf.pemfile = pempath;
	sslconf.ciphers = ciphers;
	sslconf.ssl_opts = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
	sslconf.ssl_ctx_timeout = 60 * 60 * 48;
	sslconf.verify_peer = SSL_VERIFY_NONE;
	sslconf.verify_depth = 42;
	sslconf.x509_verify_cb = NULL;
	sslconf.x509_chk_issued_cb = NULL;
	sslconf.scache_type = evhtp_ssl_scache_type_internal;
	sslconf.scache_size = 1024;
	sslconf.scache_timeout = 1024;
	sslconf.scache_init = NULL;
	sslconf.scache_add = NULL;
	sslconf.scache_get = NULL;
	sslconf.scache_del = NULL;

	if(evhtp_ssl_init(htp,&sslconf) == -1){
		fprintf(stderr, "evhtp_ssl_init failed\n");
		return 0;
	}

	evhtp_bind_socket(htp, host, port, 2048);
	event_base_loop(evbase, 0);

	fprintf(stderr, "evhtp_base_loop exited\n");
	evhtp_unbind_socket(htp);
	evhtp_free(htp);
	event_base_free(evbase);

	return 0;
}
