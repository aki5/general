#define base64len(len) (((len)*4+2)/3)
int base64enc(char *out, char *buf, int len);

#define base64cap(len) (((len)*3+3)/4)
int base64dec(char *out, char *buf, int len);
