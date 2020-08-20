#ifndef __WN_REQUEST_H__
#define __WN_REQUEST_H__

#include "webnet.h"

#ifdef __cplusplus
extern "C" {
#endif

/* http request method */
enum webnet_method
{
    WEBNET_UNKNOWN = 0,
    WEBNET_GET,
    WEBNET_POST,
    WEBNET_HEADER,
    WEBNET_HEAD,
	WEBNET_PUT,
	WEBNET_OPTIONS,
	WEBNET_PROPFIND,
	WEBNET_PROPPATCH,
	WEBNET_DELETE,
	WEBNET_CONNECT,
	WEBNET_MKCOL,
	WEBNET_MOVE,
    WEBNET_SUBSCRIBE,
    WEBNET_UNSUBSCRIBE,
    WEBNET_NOTIFY,
};

/* http connection status */
enum webnet_connection
{
    WEBNET_CONN_CLOSE,
    WEBNET_CONN_KEEPALIVE,
};

/* http request structure */
struct webnet_request
{
    enum webnet_method method;
    int result_code;
    int content_length;

    /* the corresponding session */
    struct webnet_session *session;

    /* path and authorization */
    char* path;
    char* host;
    char* authorization;
#if WEBNET_CACHE_LEVEL > 0
    char* modified;
#endif /* WEBNET_CACHE_LEVEL */
#ifdef WEBNET_USING_GZIP
    bool_t suppogzip;
#endif /* WEBNET_USING_GZIP */

    char* user_agent;
    char* accept_language;
    char* cookie;
    char* referer;
#ifdef WEBNET_USING_RANGE
    char *Range;
    int pos_start;
    int pos_end;
#endif /* WEBNET_USING_RANGE */
#ifdef WEBNET_USING_DAV
	char* depth;
	char* destination;
#endif /* WEBNET_USING_DAV */

    /* DMR */
    char *soap_action;
    char *callback;
    char *sid;

    /* Content-Type */
    char* content_type;

    /* query information */
    char* query;
	int query_offset;
    struct webnet_query_item* query_items;
    uint16_t query_counter;

    enum webnet_connection connection;

    /* whether the string filed is copied */
    char field_copied;
};

struct webnet_request* webnet_request_create(void);
void webnet_request_destory(struct webnet_request* request);

int webnet_request_parse_method(struct webnet_request *request, char* buffer, int length);
int webnet_request_parse_header(struct webnet_request *request, char* buffer, int length);
int webnet_request_parse_post(struct webnet_request* request, char* buffer, int length);

void webnet_request_parse(struct webnet_request* request, char* buffer, int length);

char webnet_request_has_query(struct webnet_request* request, char* name);
const char* webnet_request_get_query(struct webnet_request* request, char* name);
int webnet_get_number(struct webnet_request* request, char* name);
float webnet_get_float(struct webnet_request* request, char* name);
int webnet_get_str(struct webnet_request* request, char* name,char *des,size_t n);

#ifdef  __cplusplus
    }
#endif

#endif /* __WN_REQUEST_H__ */
