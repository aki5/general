#include <event2/dns.h>
#include <event2/dns_struct.h>
#include <event2/util.h>
#include <event2/event.h>

#include <sys/socket.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>

#define nelem(x) (sizeof(x)/sizeof(x[0]))

#define ONE_HOUR 3600

typedef struct DnsHost DnsHost;
typedef struct DnsPtr DnsPtr;
typedef struct DnsSoa DnsSoa;

struct DnsHost {
	char *name;
	uint8_t ip4[4];
	uint8_t ip6[16];
};

struct DnsPtr {
	char *name;
	char *ptr;
};

struct DnsSoa {
	char *mname;
	char *rname;
	uint32_t serial;
	uint32_t refresh;
	uint32_t retry;
	uint32_t expire;
	uint32_t minimum;
};

DnsHost hostdb[] = {
	{
		.name = "localhost",
		.ip4 = { 127, 0, 0, 1 },
		.ip6 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	}
};

DnsPtr ptrdb[] = {
	{
		.name = "1.0.0.127.in-addr.arpa",
		.ptr = "localhost",
	},{
		.name = "1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa",
		.ptr = "localhost",
	}
};

static DnsHost *
hostlookup(char *name)
{
	int i;
	for(i = 0; i < nelem(hostdb); i++)
		if(!evutil_ascii_strcasecmp(hostdb[i].name, name))
			return hostdb+i;
	return NULL;
}

static DnsPtr *
ptrlookup(char *name)
{
	int i;
	for(i = 0; i < nelem(ptrdb); i++)
		if(!evutil_ascii_strcasecmp(ptrdb[i].name, name))
			return ptrdb+i;
	return NULL;
}

static void
dnsrequest(struct evdns_server_request *request, void *data)
{
	int i;
	int err = DNS_ERR_NONE;

	(void)data;
	for(i = 0; i < request->nquestions; ++i){
		DnsHost *dnshost;
		DnsPtr *dnsptr;
		struct evdns_server_question *q;
		int j;

		// request: class type name 
		q = request->questions[i];

		fprintf(stderr, "request: class %d type %d name %s\n", q->class, q->type, q->name);

		if(q->dns_question_class == EVDNS_CLASS_INET){
			switch(q->type){
			case EVDNS_TYPE_A:
				/*
				 *	uint32 ADDRESS  A 32 bit Internet address.
				 */
				if((dnshost = hostlookup(q->name)) != NULL){
					evdns_server_request_add_a_reply(request, q->name, 1, dnshost->ip4, ONE_HOUR);
				} else {
					err = DNS_ERR_NOTEXIST;
				}
				break;

			case EVDNS_TYPE_AAAA:
				/*
				 *	uint128 ADDRESS A 128 bit Internet address.
				 */
				if((dnshost = hostlookup(q->name)) != NULL){
					evdns_server_request_add_aaaa_reply(request, q->name, 1, dnshost->ip6, ONE_HOUR);
				} else {
					err = DNS_ERR_NOTEXIST;
				}
				break;

			case EVDNS_TYPE_CNAME:
				/*
				 *	string cname    A <domain-name> which specifies the canonical or primary name for the owner.  The owner name is an alias.
				 */
				//int evdns_server_request_add_cname_reply(struct evdns_server_request *req, const char *name, const char *cname, int ttl);
				err = DNS_ERR_SERVERFAILED;
				break;

			case EVDNS_TYPE_MX:
				/*
				 *	preference      A 16 bit integer which specifies the preference given to this RR among others at the same owner.  Lower values are preferred.
				 *	exchange        A <domain-name> which specifies a host willing to act as a mail exchange for the owner name.
				 */
			case EVDNS_TYPE_NS:
				/*
				 *	nsdname         A <domain-name> which specifies a host which should be authoritative for the specified class and domain.
				 */
				//int evdns_server_request_add_reply(struct evdns_server_request *req, int section, const char *name, int type, int class, int ttl, int datalen, int is_name, const char *data);
				err = DNS_ERR_SERVERFAILED;
				break;

			case EVDNS_TYPE_PTR:
				/*
				 *	string ptrdname A <domain-name> which points to some location in the domain name space.
				 */
				if((dnsptr = ptrlookup(q->name)) != NULL){
					evdns_server_request_add_ptr_reply(request, NULL, q->name, dnsptr->ptr, ONE_HOUR);
				} else {
					err = DNS_ERR_NOTEXIST;
				}
				break;

			case EVDNS_TYPE_SOA:
				/*
				 *	string  mname   The <domain-name> of the name server that was the original or primary source of data for this zone
				 *	string  rname   A <domain-name> which specifies the mailbox of the person responsible for this zone.
				 *	uint32	serial  Version number of the original copy of the zone. Zone transfers preserve this value. Wraps around.
				 *	uint32	refresh Time interval before the zone should be refreshed
				 *	uint32	retry   interval that should elapse before a failed refresh should be retried.
				 *	uint32	expire  The upper limit on the time interval that can elapse before the zone is no longer authoritative.
				 *	uint32	minimum The minimum TTL field that should be exported with any RR from this zone.
				 */
			case EVDNS_TYPE_TXT:
				/*
				 *	txt-data        One or more <character-string>s.
				 */
			default:
				//int evdns_server_request_add_reply(struct evdns_server_request *req, int section, const char *name, int type, int class, int ttl, int datalen, int is_name, const char *data);
				err = DNS_ERR_SERVERFAILED;
				break;
			}
		} else {
			// unsupported class
			err = DNS_ERR_SERVERFAILED;
		}
	}
	evdns_server_request_respond(request, err);
}

int
servedns(struct event_base *base, int port)
{
	struct evdns_server_port *server6;
	evutil_socket_t lfd6;
	struct sockaddr_in6 laddr6;

	if((lfd6 = socket(AF_INET6, SOCK_DGRAM, 0)) == -1){
		fprintf(stderr, "socket6: %s\n", strerror(errno));
		return -1;
	}
	/* use IPPROTO_IPV6 level socket option IPV6_V6ONLY if IPv6-only is desired */
	memset(&laddr6, 0, sizeof laddr6);
	laddr6.sin6_family = AF_INET6;
	laddr6.sin6_addr = in6addr_any;
	laddr6.sin6_port = htons(port);
	if(bind(lfd6, (struct sockaddr*)&laddr6, sizeof laddr6) == -1){
		fprintf(stderr, "bind6: %s\n", strerror(errno));
		return -1;
	}
	if(evutil_make_socket_nonblocking(lfd6) == -1){
		fprintf(stderr, "evutil_make_socket_nonblocking6: %s\n", strerror(errno));
		return -1;
	}
	server6 = evdns_add_server_port_with_base(base, lfd6, 0, dnsrequest, NULL);

	//evdns_close_server_port(server6);

	return 0;
}