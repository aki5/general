
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "city.h"

#define hash64 CityHash64

typedef struct Keyval Keyval;
struct Keyval {
	void *key;
	void *val;
	int keylen;
	int vallen;
};

static Keyval *kvtable;
static uint64_t kvcap;
static uint64_t kvlen;
static pthread_rwlock_t kvlock = PTHREAD_RWLOCK_INITIALIZER;

static int
keyval_put_locked(void *key, int keylen, void *val, int vallen)
{
	Keyval *kvp;
	uint64_t hash;
	uint64_t i;

	hash = hash64(key, keylen);
	for(i = 0; i < kvcap; i++){
		kvp = kvtable + ((hash + i) & (kvcap - 1));
		if(kvp->key == NULL){
			kvp->keylen = keylen;
			kvp->key = key;
			kvp->vallen = vallen;
			kvp->val = val;
			kvlen++;
			return 0;
		}
		// bail out if key is already in this chain..
		if(keylen == kvp->keylen && !memcmp(key, kvp->key, keylen))
			break;

	}
	return -1;
}

int
keyval_put(void *key, int keylen, void *val, int vallen)
{
	Keyval *kvp;
	uint64_t i;
	int err;

	pthread_rwlock_wrlock(&kvlock);
	if(2*kvlen >= kvcap){
		Keyval *oldkv;
		uint64_t oldkvcap;

		oldkv = kvtable;
		oldkvcap = kvcap;

		kvcap = kvcap < 16 ? 16 : 2*kvcap;
		kvtable = malloc(kvcap * sizeof kvtable[0]);
		memset(kvtable, 0, kvcap * sizeof kvtable[0]);
		kvlen = 0;
		if(oldkv != NULL){
			for(i = 0; i < oldkvcap; i++){
				kvp = oldkv + i;
				if(kvp->key != NULL)
					if(keyval_put_locked(kvp->key, kvp->keylen, kvp->val, kvp->vallen) == -1)
						abort();
			}
			free(oldkv);
		}
	}

	err = keyval_put_locked(key, keylen, val, vallen);
	pthread_rwlock_unlock(&kvlock);
	return err;
}

int
keyval_get(void *key, int keylen, void **keyp, int *keylenp, void **valp, int *vallenp)
{
	Keyval *kvp;
	uint64_t hash;
	uint64_t i;

	hash = hash64(key, keylen);
	pthread_rwlock_rdlock(&kvlock);
	for(i = 0; i < kvcap; i++){
		kvp = kvtable + ((hash + i) & (kvcap - 1));
		if(kvp->key == NULL)
			break;
		if(keylen == kvp->keylen && !memcmp(key, kvp->key, keylen)){
			if(keyp != NULL)
				*keyp = kvp->key;
			if(keylenp != NULL)
				*keylenp = kvp->keylen;
			if(valp != NULL)
				*valp = kvp->val;
			if(vallenp != NULL)
				*vallenp = kvp->vallen;
			pthread_rwlock_unlock(&kvlock);
			return 0;
		}
	}
	pthread_rwlock_unlock(&kvlock);
	return -1;
}

int
keyval_delete(void *key, int keylen, void **keyp, int *keylenp, void **valp, int *vallenp)
{
	Keyval *kvp;
	uint64_t hash;
	uint64_t i, j, k, chlen;

	hash = hash64(key, keylen);
	pthread_rwlock_wrlock(&kvlock);
	for(i = 0; i < kvcap; i++){
		kvp = kvtable + ((hash + i) & (kvcap - 1));
		if(kvp->key == NULL)
			break;
		if(keylen == kvp->keylen && !memcmp(key, kvp->key, keylen)){
			if(keyp != NULL)
				*keyp = kvp->key;
			if(keylenp != NULL)
				*keylenp = kvp->keylen;
			if(valp != NULL)
				*valp = kvp->val;
			if(vallenp != NULL)
				*vallenp = kvp->vallen;

			memset(kvp, 0, sizeof kvp[0]);
			kvlen--;
			i++;
			chlen = 0;
			for(j = i; j < kvcap; j++){
				kvp = kvtable + ((hash + j) & (kvcap - 1));
				if(kvp->key == NULL)
					break;
				chlen++;
			}
			if(chlen > 0){
				Keyval kvtmp[chlen];
				for(k = 0; k < chlen; k++){
					j = i + k;
					kvp = kvtable + ((hash + j) & (kvcap - 1));
					if(kvp->key == NULL)
						abort();
					memcpy(kvtmp + k, kvp, sizeof kvp[0]);
					memset(kvp, 0, sizeof kvp[0]);
					kvlen--;
				}
				for(k = 0; k < chlen; k++){
					kvp = kvtmp + k;
					if(keyval_put_locked(kvp->key, kvp->keylen, kvp->val, kvp->vallen) == -1)
						abort();
				}
			}
			pthread_rwlock_unlock(&kvlock);
			return 0;
		}
	}
	pthread_rwlock_unlock(&kvlock);
	return -1;
}

#ifdef KEYVAL_TEST
#include "nsec.h"

enum {
	NumKeys = 100000,
};

int
main(int argc, char *argv[])
{
	char **keys;
	int *keylens;
	int64_t start, end;
	int i, j, numkeys;

	numkeys = NumKeys;
	if(argc > 1)
		numkeys = strtol(argv[1], NULL, 10);

	srandom(time(NULL));
	keys = malloc(numkeys * sizeof keys[0]);
	keylens = malloc(numkeys * sizeof keylens[0]);
	for(i = 0; i < numkeys; i++){
		char *key;
		key = malloc(32);
		snprintf(key, 32, "%d.%ld", i, random());
		keys[i] = key;
		keylens[i] = strlen(key);
	}

	for(j = 0; j < 10; j++){

		// insert keys, time the result
		start = nsec();
		for(i = 0; i < numkeys; i++){
			if(keyval_put(keys[i], keylens[i], NULL, 0) == -1){
				fprintf(stderr, "keyval_put %s kvlen %ld kvcap %ld\n", keys[i], kvlen, kvcap);
				abort();
			}
		}
		end = nsec();
		fprintf(stderr, "inserted %d keys in %ld nsec, %g keys/s\n", numkeys, end-start, numkeys / (1e-9*(end-start)));
		if(kvlen != numkeys)
			abort();

		// fetch keys, time the result
		start = nsec();
		for(i = 0; i < numkeys; i++){
			if(keyval_get(keys[i], keylens[i], NULL, NULL, NULL, NULL) == -1){
				fprintf(stderr, "keyval_put %s kvlen %ld kvcap %ld\n", keys[i], kvlen, kvcap);
				abort();
			}
		}
		end = nsec();
		fprintf(stderr, "fetched %d keys in %ld nsec, %g keys/s\n", numkeys, end-start, numkeys / (1e-9*(end-start)));
		if(kvlen != numkeys)
			abort();


		// delete keys, time the result
		start = nsec();
		for(i = 0; i < numkeys; i++){
			if(keyval_delete(keys[i], keylens[i], NULL, NULL, NULL, NULL) == -1){
				fprintf(stderr, "fail keyval_delete %s kvlen %ld kvcap %ld\n", keys[i], kvlen, kvcap);
				abort();
			}
		}
		end = nsec();
		fprintf(stderr, "deleted %d keys in %ld nsec, %g keys/s\n", numkeys, end-start, numkeys / (1e-9*(end-start)));

		if(kvlen != 0)
			abort();
	}
}
#endif
