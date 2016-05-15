
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <evhtp/evhtp.h>
#include "servedns.h"
#include "base64.h"
#include "city.h"

#define data(x) #x

char *google_client_id;
char *roothtml;
char *redirect;

void
dumpheaders(evhtp_request_t *req)
{
	evhtp_kv_t *kv;
	TAILQ_FOREACH(kv, req->headers_in, next) {
		fprintf(stderr, "%*s: %*s\n", kv->klen, kv->key, kv->vlen, kv->val);
	}
}

const char *
geohost(evhtp_request_t *req)
{
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;
	evhtp_connection_t *conn;
	const char *host;
	char buf[128];

	conn = evhtp_request_get_connection(req);
	sa4 = (struct sockaddr_in*)conn->saddr;
	sa6 = (struct sockaddr_in6*)conn->saddr;
	host = evhtp_header_find(req->headers_in, "Host");
	switch(conn->saddr->sa_family){
	case AF_INET:
		inet_ntop(AF_INET, &sa4->sin_addr, buf, sizeof buf);
		fprintf(stderr, "geohost %s\n", buf);
		if(!strncmp(buf, "127.", 4) && !strcmp(host, "localhost"))
			return redirect;
		break;
	case AF_INET6:
		inet_ntop(AF_INET6, &sa6->sin6_addr, buf, sizeof buf);
		fprintf(stderr, "geohost %s\n", buf);
		break;
	}
	return host;
}

void
redirectpath(evhtp_request_t *req, void *a)
{
	char location[1024];
	char *path;
	int n;

	dumpheaders(req);

	path = req->uri->path->full;
	n = snprintf(location, sizeof location, "https://%s%s", geohost(req), path);
	if(n >= sizeof location){
		evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/plain", 1, 1));
		evbuffer_add_printf(req->buffer_out, "moved permanently to %s\n", location);
		evhtp_send_reply(req, 500);
		return;
	}
	evhtp_headers_add_header(req->headers_out, evhtp_header_new("Location", location, 1, 1));
	evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/plain", 1, 1));
	evbuffer_add_printf(req->buffer_out, "moved to %s\n", location);
	evhtp_send_reply(req, 303); // 303:see other, 301:moved permanently
}

void
serveroot(evhtp_request_t *req, void *a)
{
	evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 1, 1));
	evbuffer_add_printf(req->buffer_out, data(
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

	evhtp_send_reply(req, EVHTP_RES_OK);
}

void
servetokensignin(evhtp_request_t *req, void *a)
{
	char *buf;
	int len;

	dumpheaders(req);

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

	evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "application/json", 1, 1));
	evbuffer_add(req->buffer_out, "{}", 2);
	evhtp_send_reply(req, EVHTP_RES_OK);
	return;

exit_failure:
	evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/plain", 1, 1));
	evbuffer_add_printf(req->buffer_out, "oopsie\n");
	evhtp_send_reply(req, 501);
	return;
}

void
servejsonptr(evhtp_request_t *req)
{

	char *buf, *path;
	int buflen, pathlen;

	buf = evbuffer_pullup(req->buffer_in, -1);
	buflen = evbuffer_get_length(req->buffer_in);

	path = req->uri->path->full;
	pathlen = strlen(path);
	fprintf(stderr, "servekeyval path %s\n", path);

	switch(req->method){
	case htp_method_PUT:
	case htp_method_GET:
	case htp_method_DELETE:

	case htp_method_HEAD:
	case htp_method_POST:
	case htp_method_MKCOL:
	case htp_method_COPY:
	case htp_method_MOVE:
	case htp_method_OPTIONS:
	case htp_method_PROPFIND:
	case htp_method_PROPPATCH:
	case htp_method_LOCK:
	case htp_method_UNLOCK:
	case htp_method_TRACE:
	case htp_method_CONNECT:
	case htp_method_PATCH:
	default:
		evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/plain", 1, 1));
		evbuffer_add_printf(req->buffer_out, "unsupported request\n");
		evhtp_send_reply(req, 501);
		return;
	}
}

int
main(int argc, char ** argv)
{
	evbase_t *evbase;
	evhtp_t  *https;
	evhtp_t *http;
	char *pempath;
	char *host;
	int opt, httpport, httpsport, dnsport;

	pempath = NULL;
	host = "0.0.0.0";
	httpsport = 5443;
	httpport = 5080;
	dnsport = 5053;
	while((opt = getopt(argc, argv, "c:h:s:d:g:r:")) != -1){
		char *p;
		switch(opt){
		case 'g':
			google_client_id = "696325580206-betr6135tc0fk6dbgh830pnmee8rjgrq.apps.googleusercontent.com";
			break;
		case 'c':
			pempath = optarg;
			break;
		case 'h':
			httpport = strtol(optarg, NULL, 10);
			break;
		case 's':
			httpsport = strtol(optarg, NULL, 10);
			break;
		case 'd':
			dnsport = strtol(optarg, NULL, 10);
			break;
		case 'r':
			redirect = optarg;
			break;
		default:
		caseusage:
			fprintf(stderr, "usage: %s -c path/to/file.pem [-h httpport] [-s httpsport] [-d dnsport] [-g google-client-id] [-r redirect]\n", argv[0]);
			exit(1);
		}
	}

	if(pempath == NULL)
		goto caseusage;

	evbase = event_base_new();
	https = evhtp_new(evbase, NULL);
	evhtp_set_cb(https, "/", serveroot, NULL);
	evhtp_set_cb(https, "/tokensignin", servetokensignin, NULL);
	evhtp_set_parser_flags(https, EVHTP_PARSE_QUERY_FLAG_LENIENT);


	evhtp_use_threads(https, NULL, 8, NULL);

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

	if(evhtp_ssl_init(https, &sslconf) == -1){
		fprintf(stderr, "evhtp_ssl_init failed\n");
		exit(1);
	}
	evhtp_bind_socket(https, host, httpsport, 2048);

	http = evhtp_new(evbase, NULL);
	evhtp_set_cb(http, "/", redirectpath, NULL);
	evhtp_set_parser_flags(http, EVHTP_PARSE_QUERY_FLAG_LENIENT);

	evhtp_use_threads(http, NULL, 8, NULL);
	evhtp_bind_socket(http, host, httpport, 2048);

	if(servedns(evbase, dnsport) == -1){
		fprintf(stderr, "servedns failed\n");
		exit(1);
	}

	event_base_loop(evbase, 0);

	fprintf(stderr, "evhtp_base_loop exited\n");
	evhtp_unbind_socket(https);
	evhtp_free(https);
	event_base_free(evbase);

	return 0;
}
