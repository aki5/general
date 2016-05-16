
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>

char *dsthost;
int verifypeer = 1;
int verifyhost = 1;
int numkeys = 10000;

int
main(int argc, char *argv[])
{
	CURL *curl;
	int res;
	int i, pid, opt;

	while((opt = getopt(argc, argv, "ph")) != -1){
		switch(opt){
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

	pid = getpid();
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifypeer);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyhost);

	for(i = 0; i < numkeys; i++){
		char url[256];
		int urllen;
		urllen = snprintf(url, sizeof url, "%s/keyval/%d.%d", dsthost, pid, i);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, urllen);

		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
			goto exit_failure;
		}
	}

	curl_easy_reset(curl);
	for(i = 0; i < numkeys; i++){
		char url[256];
		snprintf(url, sizeof url, "%s/keyval/%d.%d", dsthost, pid, i);
		curl_easy_setopt(curl, CURLOPT_URL, url);

		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
			goto exit_failure;
		}
	}

	curl_easy_reset(curl);
	for(i = 0; i < numkeys; i++){
		char url[256];
		int urllen;
		urllen = snprintf(url, sizeof url, "%s/keyval/%d.%d", dsthost, pid, i);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
			goto exit_failure;
		}
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;

exit_failure:
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 1;
}
