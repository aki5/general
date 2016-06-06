
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>
#include <sys/wait.h>
#include "nsec.h"

char *dsthost;
int verifypeer = 1;
int verifyhost = 1;
int numkeys = 1000;
int numloops = 10;
int numforks = 1;

int
nopwrite(void *ptr, size_t size, size_t nmemb, void *aux)
{
	return nmemb;
}

int
main(int argc, char *argv[])
{
	CURL *curl;
	uint64_t startns, endns;
	int res;
	int i, loop, forki, pid, opt, err;

	while((opt = getopt(argc, argv, "phn:l:f:")) != -1){
		switch(opt){
		case 'f':
			numforks = strtol(optarg, NULL, 10);
			break;
		case 'l':
			numloops = strtol(optarg, NULL, 10);
			break;
		case 'n':
			numkeys = strtol(optarg, NULL, 10);
			break;
		case 'p':
			verifypeer = 0;
			break;
		case 'h':
			verifyhost = 0;
			break;
		}
	}

	dsthost = argv[optind]++;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl == NULL){
		fprintf(stderr, "curl_easy_init failed\n");
		exit(1);
	}

	startns = nsec();
	for(forki = 1; forki < numforks; forki++)
		if(fork() == 0)
			break;

	pid = getpid();
	for(loop = 0; loop < numloops; loop++){
		char url[256];
		char val[256];

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifypeer);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyhost);

		for(i = 0; i < numkeys; i++){
			int urllen, vallen;
			urllen = snprintf(url, sizeof url, "%s/keyval/%d.%d", dsthost, pid, i);
			vallen = snprintf(val, sizeof val, "{\"url\": \"%s\", \"foo\": false }", url);
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, val);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, vallen);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nopwrite);

			res = curl_easy_perform(curl);
			if(res != CURLE_OK){
				fprintf(stderr, "curl_easy_perform put %s failed: %s\n", url, curl_easy_strerror(res));
				goto exit_failure;
			}
		}

		curl_easy_reset(curl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifypeer);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyhost);

		for(i = 0; i < numkeys; i++){
			snprintf(url, sizeof url, "%s/keyval/%d.%d", dsthost, pid, i);
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nopwrite);

			res = curl_easy_perform(curl);
			if(res != CURLE_OK){
				fprintf(stderr, "curl_easy_perform get %s failed: %s\n", url, curl_easy_strerror(res));
				goto exit_failure;
			}
		}

		curl_easy_reset(curl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifypeer);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyhost);

		for(i = 0; i < numkeys; i++){
			int urllen;
			urllen = snprintf(url, sizeof url, "%s/keyval/%d.%d", dsthost, pid, i);
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nopwrite);

			res = curl_easy_perform(curl);
			if(res != CURLE_OK){
				fprintf(stderr, "curl_easy_perform delete %s failed: %s\n", url, curl_easy_strerror(res));
				goto exit_failure;
			}
		}
	}

	err = 0;
	if(forki == numforks){
		int status;
		for(forki = 1; forki < numforks; forki++){
			wait(&status);
			if(!WIFEXITED(status) || WEXITSTATUS(status) != 0){
				fprintf(stderr, "bad exit status %d!\n", status);
				err++;
			}
		}
		endns = nsec();
		fprintf(stderr, "%d puts/gets/deletes in %lld nsec, %f qps\n",
			numforks*numloops*numkeys,
			endns-startns,
			(1e9*numforks*numloops*numkeys)/(endns-startns)
		);
	}



	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return err;

exit_failure:
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 1;
}
