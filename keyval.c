
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "city.h"
#include "keyval.h"

#define hash64 CityHash64

static int
keyval_put_locked(Kvstore *kv, void *key, int keylen, void *val, int vallen)
{
	Keyval *kvp;
	uint64_t hash;
	uint64_t i;

	hash = hash64(key, keylen);
	for(i = 0; i < kv->cap; i++){
		kvp = kv->table + ((hash + i) & (kv->cap - 1));
		if(kvp->key == NULL){
			kvp->keylen = keylen;
			kvp->key = key;
			kvp->vallen = vallen;
			kvp->val = val;
			kv->len++;
			return 0;
		}
		// bail out if key is already in this chain..
		if(keylen == kvp->keylen && !memcmp(key, kvp->key, keylen))
			break;

	}
	return -1;
}

void
keyval_init_bucket(Kvstore *kv)
{
	memset(kv, 0, sizeof kv[0]);
	kv->lock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
}

int
keyval_put(Kvstore *kv, void *key, int keylen, void *val, int vallen)
{
	Keyval *kvp;
	uint64_t i;
	int err;

	pthread_rwlock_wrlock(&kv->lock);
	if(2*kv->len >= kv->cap){
		Keyval *oldkv;
		uint64_t oldkvcap;

		oldkv = kv->table;
		oldkvcap = kv->cap;

		kv->cap = kv->cap < 16 ? 16 : 2*kv->cap;
		kv->table = malloc(kv->cap * sizeof kv->table[0]);
		memset(kv->table, 0, kv->cap * sizeof kv->table[0]);
		kv->len = 0;
		if(oldkv != NULL){
			for(i = 0; i < oldkvcap; i++){
				kvp = oldkv + i;
				if(kvp->key != NULL)
					if(keyval_put_locked(kv, kvp->key, kvp->keylen, kvp->val, kvp->vallen) == -1)
						abort();
			}
			free(oldkv);
		}
	}

	err = keyval_put_locked(kv, key, keylen, val, vallen);
	pthread_rwlock_unlock(&kv->lock);
	return err;
}

int
keyval_get(Kvstore *kv, void *key, int keylen, void **keyp, int *keylenp, void **valp, int *vallenp)
{
	Keyval *kvp;
	uint64_t hash;
	uint64_t i;

	hash = hash64(key, keylen);
	pthread_rwlock_rdlock(&kv->lock);
	for(i = 0; i < kv->cap; i++){
		kvp = kv->table + ((hash + i) & (kv->cap - 1));
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
			pthread_rwlock_unlock(&kv->lock);
			return 0;
		}
	}
	pthread_rwlock_unlock(&kv->lock);
	return -1;
}

int
keyval_delete(Kvstore *kv, void *key, int keylen, void **keyp, int *keylenp, void **valp, int *vallenp)
{
	Keyval *kvp;
	uint64_t hash;
	uint64_t i, j, k, chlen;

	hash = hash64(key, keylen);
	pthread_rwlock_wrlock(&kv->lock);
	for(i = 0; i < kv->cap; i++){
		kvp = kv->table + ((hash + i) & (kv->cap - 1));
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

			// zap the found kv pair
			memset(kvp, 0, sizeof kvp[0]);
			kv->len--;
			i++;

			// calculate chain length
			chlen = 0;
			for(j = i; j < kv->cap; j++){
				kvp = kv->table + ((hash + j) & (kv->cap - 1));
				if(kvp->key == NULL)
					break;
				chlen++;
			}

			if(chlen > 0){
				// remove everything in chain (put in kvtmp)
				Keyval kvtmp[chlen];
				for(k = 0; k < chlen; k++){
					j = i + k;
					kvp = kv->table + ((hash + j) & (kv->cap - 1));
					if(kvp->key == NULL)
						abort();
					memcpy(kvtmp + k, kvp, sizeof kvp[0]);
					memset(kvp, 0, sizeof kvp[0]);
					kv->len--;
				}
				// put them back in
				for(k = 0; k < chlen; k++){
					kvp = kvtmp + k;
					if(keyval_put_locked(kv, kvp->key, kvp->keylen, kvp->val, kvp->vallen) == -1)
						abort();
				}
			}
			pthread_rwlock_unlock(&kv->lock);
			return 0;
		}
	}
	pthread_rwlock_unlock(&kv->lock);
	return -1;
}

int
keyval_list(Kvstore *kv, void ***keysp, int **keylensp, int *nkeysp)
{
	Keyval *kvp;
	void **keys;
	int *keylens;
	uint64_t i, nkeys;

	pthread_rwlock_rdlock(&kv->lock);
	keys = malloc(kv->len * sizeof keys[0]);
	keylens = malloc(kv->len * sizeof keylens[0]);
	nkeys = 0;
	for(i = 0; i < kv->cap; i++){
		kvp = kv->table + i;
		if(kvp->key != NULL){
			keys[nkeys] = kvp->key;
			keylens[nkeys] = kvp->keylen;
			nkeys++;
			if(nkeys > kv->len)
				abort();
		}
	}
	pthread_rwlock_unlock(&kv->lock);
	*keysp = keys;
	*keylensp = keylens;
	*nkeysp = nkeys;
	return 0;
}

#ifdef KEYVAL_TEST
#include "nsec.h"

enum {
	NumKeys = 100000,
};

int
main(int argc, char *argv[])
{
	Kvstore kv;
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
	keyval_init_Kvstore(&kv);
	if(keys == NULL || keylens == NULL)
		abort();
	for(i = 0; i < numkeys; i++){
		char *key;
		key = malloc(32);
		if(key == NULL)
			abort();
		snprintf(key, 32, "%d.%ld", i, random());
		keys[i] = key;
		keylens[i] = strlen(key);
	}

	for(j = 0; j < 10; j++){

		// insert keys, time the result
		start = nsec();
		for(i = 0; i < numkeys; i++){
			if(keyval_put(&kv, keys[i], keylens[i], NULL, 0) == -1){
				fprintf(stderr, "keyval_put %s kv->len %lld kv->cap %lld\n", keys[i], kv.len, kv.cap);
				abort();
			}
		}
		end = nsec();
		fprintf(stderr, "inserted %d keys in %lld nsec, %g keys/s\n", numkeys, end-start, numkeys / (1e-9*(end-start)));
		if(kv.len != (uint64_t)numkeys)
			abort();

		// fetch keys, time the result
		start = nsec();
		for(i = 0; i < numkeys; i++){
			if(keyval_get(&kv, keys[i], keylens[i], NULL, NULL, NULL, NULL) == -1){
				fprintf(stderr, "keyval_put %s kv->len %lld kv->cap %lld\n", keys[i], kv.len, kv.cap);
				abort();
			}
		}
		end = nsec();
		fprintf(stderr, "fetched %d keys in %lld nsec, %g keys/s\n", numkeys, end-start, numkeys / (1e-9*(end-start)));
		if(kv.len != (uint64_t)numkeys)
			abort();


		// delete keys, time the result
		start = nsec();
		for(i = 0; i < numkeys; i++){
			if(keyval_delete(&kv, keys[i], keylens[i], NULL, NULL, NULL, NULL) == -1){
				fprintf(stderr, "fail keyval_delete %s kv->len %lld kv->cap %lld\n", keys[i], kv.len, kv.cap);
				abort();
			}
		}
		end = nsec();
		fprintf(stderr, "deleted %d keys in %lld nsec, %g keys/s\n", numkeys, end-start, numkeys / (1e-9*(end-start)));

		if(kv.len != 0)
			abort();
	}
}
#endif
