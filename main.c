
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <evhtp/evhtp.h>
#include "servedns.h"
#include "base64.h"

#define data(x) #x

char *google_client_id;
char *roothtml;

char *
rootinit(void)
{
	int len, cap;
	char *htmldoc;

	htmldoc = NULL;
	len = -1;
	cap = 0;
loop:
	len = snprintf(htmldoc, cap, data(
		<html lang="en">
		<head>
		<meta name="google-signin-scope" content="profile email">
		<meta name="google-signin-client_id" content="%s">
		<script src="https://apis.google.com/js/platform.js" async defer></script>
		</head>
		<body>
		<div class="g-signin2" data-onsuccess="onSignIn" data-theme="dark"></div>
		<input type="button" onclick="signOut();" value="Signoff"></input>
		<script>
			function onSignIn(googleUser) {
				// Useful data for your client-side scripts:
				var profile = googleUser.getBasicProfile();
				console.log("ID: " + profile.getId()); // Don't send this directly to your server!
				console.log('Full Name: ' + profile.getName());
				console.log('Given Name: ' + profile.getGivenName());
				console.log('Family Name: ' + profile.getFamilyName());
				console.log("Image URL: " + profile.getImageUrl());
				console.log("Email: " + profile.getEmail());

				// The ID token you need to pass to your backend:
				var id_token = googleUser.getAuthResponse().id_token;
				console.log("ID Token: " + id_token);

				var xhr = new XMLHttpRequest();
				xhr.open('POST', '/tokensignin');
				xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
				xhr.onload = function() {
					console.log('Signed in as: ' + xhr.responseText);
				};
				xhr.send('idtoken=' + id_token);
			};
			function signOut() {
				var auth2 = gapi.auth2.getAuthInstance();
				auth2.signOut().then(function () {
					console.log('User signed out.');
				});
			}
		</script>
		</body>
		</html>
	), google_client_id);
	if(len > cap){
		cap = len;
		htmldoc = realloc(htmldoc, cap);
		goto loop;
	}

	return htmldoc;
}

void
serveroot(evhtp_request_t *req, void *a)
{
	if(roothtml == NULL)
		roothtml = rootinit();
	evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 1, 1));
	evbuffer_add(req->buffer_out, roothtml, strlen(roothtml));
	evhtp_send_reply(req, EVHTP_RES_OK);
}

void
servetokensignin(evhtp_request_t *req, void *a)
{
	evhtp_kv_t *kv;
	char *buf;
	int len;

	buf = evbuffer_pullup(req->buffer_in, -1);
	len = evbuffer_get_length(req->buffer_in);

	char b64out[len];
	char *p, *q;
	int i, outlen;

	p = strchr(buf, '=');
	p++;
	q = strchr(p, '.');
	outlen = base64dec(b64out, p, q - p);
	fprintf(stderr, "header: ");
	for(i = 0; i < outlen; i++)
		fprintf(stderr, "%c", isprint(b64out[i]) ? b64out[i] : '.');
	fprintf(stderr, "\n");

	q++;
	p = strchr(q, '.');
	outlen = base64dec(b64out, q, p - q);
	fprintf(stderr, "payload: ");
	for(i = 0; i < outlen; i++)
		fprintf(stderr, "%c", isprint(b64out[i]) ? b64out[i] : '.');
	fprintf(stderr, "\n");


	TAILQ_FOREACH(kv, req->headers_in, next) {
		fprintf(stderr, "%*s: %*s\n", kv->klen, kv->key, kv->vlen, kv->val);
	}
	evbuffer_add(req->buffer_out, a, strlen(a));
	evhtp_send_reply(req, EVHTP_RES_OK);
	return;

exit_failure:
	evhtp_send_reply(req, EVHTP_RES_OK);
	return;
}

int
main(int argc, char ** argv)
{
	evbase_t *evbase;
	evhtp_t  *htp;
	char *pempath;
	char *host;
	int opt, htport, dnsport;

	pempath = NULL;
	host = "0.0.0.0";
	htport = 5443;
	dnsport = 5053;
	while((opt = getopt(argc, argv, "c:p:d:g:")) != -1){
		char *p;
		switch(opt){
		case 'g':
			google_client_id = "696325580206-betr6135tc0fk6dbgh830pnmee8rjgrq.apps.googleusercontent.com";
			break;
		case 'c':
			pempath = optarg;
			break;
		case 'p':
			htport = strtol(optarg, NULL, 10);
			break;
		case 'd':
			dnsport = strtol(optarg, NULL, 10);
			break;
		default:
		caseusage:
			fprintf(stderr, "usage: %s -c path/to/file.pem [-p htport] [-d dnsport] [-g google-client-id]\n", argv[0]);
			exit(1);
		}
	}

	if(pempath == NULL)
		goto caseusage;

	evbase = event_base_new();
	htp = evhtp_new(evbase, NULL);
	evhtp_set_cb(htp, "/", serveroot, "Hello simple World");
	evhtp_set_cb(htp, "/tokensignin", servetokensignin, "Hello simple World");


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
		exit(1);
	}

	evhtp_bind_socket(htp, host, htport, 2048);

	if(servedns(evbase, dnsport) == -1){
		fprintf(stderr, "servedns failed\n");
		exit(1);
	}

	event_base_loop(evbase, 0);

	fprintf(stderr, "evhtp_base_loop exited\n");
	evhtp_unbind_socket(htp);
	evhtp_free(htp);
	event_base_free(evbase);

	return 0;
}
