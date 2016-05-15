typedef struct Kvstore Kvstore;
typedef struct Keyval Keyval;

struct Keyval {
	void *key;
	void *val;
	int keylen;
	int vallen;
};

struct Kvstore {
	pthread_rwlock_t lock;
	Keyval *table;
	uint64_t cap;
	uint64_t len;
};

void keyval_init_bucket(Kvstore *kv);
int keyval_put(Kvstore *kv, void *key, int keylen, void *val, int vallen);
int keyval_get(Kvstore *kv, void *key, int keylen, void **keyp, int *keylenp, void **valp, int *vallenp);
int keyval_delete(Kvstore *kv, void *key, int keylen, void **keyp, int *keylenp, void **valp, int *vallenp);
int keyval_list(Kvstore *kv, void ***keysp, int **keylensp, int *nkeysp);
