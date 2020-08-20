#include <string.h>
#include <stdlib.h>

#include <webnet.h>

/**
 * parse a query
 */
static void _webnet_request_parse_query(struct webnet_request* request)
{
    char *ptr, *end_ptr;
    uint32_t index;

    if ((request->query == NULL) || (*request->query == '\0')) return; /* no query */

    /* get the query counter */
    ptr = request->query;
    end_ptr = request->query + strlen(request->query);

    request->query_counter = 1;
    while (*ptr && ptr <= end_ptr)
    {
        if (*ptr == '&')
        {
            while ((*ptr == '&') && (*ptr != '\0')) ptr ++;
            if (*ptr == '\0') break;

            request->query_counter ++;
        }
        ptr ++;
    }
    if (request->query_counter == 0) return; /* no query */

    /* allocate query item */
    request->query_items = (struct webnet_query_item*) wn_malloc (sizeof(struct webnet_query_item)
                           * request->query_counter);
    if (request->query_items == NULL)
    {
        request->result_code = 500;
        return;
    }

    /* parse the query */
    ptr = request->query;
    for (index = 0; index < request->query_counter; index ++)
    {
        request->query_items[index].name = ptr;
        request->query_items[index].value = NULL;

        /* get value or goto next item */
        while ((*ptr != '&') && (*ptr != '\0'))
        {
            /* get value */
            if (*ptr == '=')
            {
                *ptr = '\0';

                ptr ++;
                request->query_items[index].value = ptr;
            }
            else ptr ++;
        }

        if (*ptr == '\0')
        {
            urldecode(request->query_items[index].name, strlen(request->query_items[index].name));
            urldecode(request->query_items[index].value, strlen(request->query_items[index].value));
            break;
        }

        if (*ptr == '&')
        {
            /* make a item */
            *ptr = '\0';
            urldecode(request->query_items[index].name, strlen(request->query_items[index].name));
            urldecode(request->query_items[index].value, strlen(request->query_items[index].value));
            ptr ++;
            while (*ptr == '&' && *ptr != '\0' && ptr <= end_ptr)ptr ++;
            if (*ptr == '\0') break;
        }
    }
}

/**
 * to check whether a query on the http request.
 */
char webnet_request_has_query(struct webnet_request* request, char* name)
{
    uint32_t index;

    for (index = 0; index < request->query_counter; index ++)
    {
        if (strncmp(request->query_items[index].name, name, strlen(name)) == 0)
            return TRUE;
    }

    return FALSE;
}

/**
 * get query value according to the name
 */
const char* webnet_request_get_query(struct webnet_request* request, char* name)
{
    uint32_t index;

    for (index = 0; index < request->query_counter; index ++)
    {
        if (strncmp(request->query_items[index].name, name, strlen(name)) == 0 &&
            strlen(name) == strlen(request->query_items[index].name))
            return request->query_items[index].value;
    }

    return NULL;
}

int webnet_get_number(struct webnet_request* request, char* name)
{
    const char *s = NULL;

    if(request == NULL || name == NULL)
    {
        return 0;
    }

    s = webnet_request_get_query(request, name);
    if(s == NULL)
    {
        return 0;
    }
    return atoi(s);
}

float webnet_get_float(struct webnet_request* request, char* name)
{
    const char *s = NULL;

    if(request == NULL || name == NULL)
    {
        return 0;
    }

    s = webnet_request_get_query(request, name);
    if(s == NULL)
    {
        return 0;
    }
    return atof(s);
}

int webnet_get_str(struct webnet_request* request, char* name,char *des,size_t n)
{
    int re = -1;
    const char *s = NULL;

    if(request == NULL || name == NULL || des == NULL || n == 0)
    {
        return re;
    }

    s = webnet_request_get_query(request, name);
    if(s)
    {
        snprintf(des,n,"%s",s);
        re = 1;
    }
    else
    {
        des[0]= '\0';
    }
    return re;
}

struct web_method
{
    const char *method_str;
    enum webnet_method method_value;
};

const struct web_method methods [] = {
    {"GET ",        WEBNET_GET},
    {"POST ",       WEBNET_POST},
    {"HEADER ",     WEBNET_HEADER},
    {"HEAD ",       WEBNET_HEAD},
    {"PUT ",        WEBNET_PUT},
    {"OPTIONS ",    WEBNET_OPTIONS},
    {"PROPFIND ",   WEBNET_PROPFIND},
    {"PROPPATCH ",  WEBNET_PROPPATCH},
    {"DELETE ",     WEBNET_DELETE},
    {"CONNECT ",    WEBNET_CONNECT},
    {"MKCOL ",      WEBNET_MKCOL},
    {"MOVE ",       WEBNET_MOVE},
    {"SUBSCRIBE ",  WEBNET_SUBSCRIBE},
    {"UNSUBSCRIBE ", WEBNET_UNSUBSCRIBE},
    {"NOTIFY ",     WEBNET_NOTIFY},
    {NULL, WEBNET_UNKNOWN},
};

int webnet_request_parse_method(struct webnet_request *request, char* buffer, int length)
{
    char                    *ch = NULL;
    int                     index = 0;
    char                    *request_buffer = NULL;
    const struct web_method *method = NULL;

    if( request == NULL || request->session == NULL )
    {
        return 0;
    }

    request_buffer = buffer;

    if (strstr(request_buffer, "\r\n") == NULL) return 0;

    /* parse method */
    for (index = 0; ; index ++)
    {
        method = &methods[index];
        if (method->method_value == WEBNET_UNKNOWN)
        {
            /* Not implemented for other method */
            request->result_code = 501;
            return 0;
        }

        if (str_begin_with(request_buffer, method->method_str))
        {
            request->method = method->method_value;
            request_buffer += strlen(method->method_str);
            break;
        }
    }

    /* get path */
    ch = strchr(request_buffer, ' ');
    if (ch == NULL)
    {
        /* Bad Request */
        request->result_code = 400;
        return request_buffer - buffer;
    }
    *ch++ = '\0';
    request->path = wn_strdup(request_buffer);
    request_buffer = ch;

    /* check path, whether there is a query */
    ch = strchr(request->path, '?');
    if (ch != NULL)
    {
        *ch++ = '\0';
        while (*ch == ' ') ch ++;

        /* copy query and parse query */
        request->query = wn_strdup(ch);
        /* copy query and parse parameter */
        _webnet_request_parse_query(request);
    }

    /* check protocol */
    if (!str_begin_with(request_buffer, "HTTP/1"))
    {
        /* Not implemented, webnet does not support HTTP 0.9 protocol */
        request->result_code = 501;
        return request_buffer - buffer;
    }

    ch = strstr(request_buffer, "\r\n");
    *ch ++ = '\0';
    *ch ++ = '\0';
    request_buffer = ch;

    /* move to next phase */
    request->session->session_phase = WEB_PHASE_HEADER;

    return request_buffer - buffer;
}

int webnet_request_parse_header(struct webnet_request *request, char* buffer, int length)
{
    char *ch;
    char *request_buffer;
    struct webnet_session *session;

    ASSERT(request != NULL);
    ASSERT(request->session != NULL);

    session = request->session;
    request_buffer = buffer;

    for (;;)
    {
        if (str_begin_with(request_buffer, "\r\n"))
        {
            /* end of http request */
            request_buffer += 2;

            if (request->content_length && request->method == WEBNET_POST)
            {
                session->session_phase = WEB_PHASE_RESPONSE;
                if (!str_begin_with(request->content_type, "multipart/form-data") &&
                    request->content_length < POST_BUFSZ_MAX)
                {
                    session->session_phase = WEB_PHASE_QUERY;

                    /* allocate query buffer */
                    if(request->query != NULL) {
                       wn_free(request->query);
                    }
                    request->query = (char*) wn_malloc(request->content_length + 1);
                    memset(request->query, 0, request->content_length + 1);
                    request->query_offset = 0;
                }
            }
            else
            {
                /* end of http request */
                request->result_code = 200;
                /* move to next phase */
                session->session_phase = WEB_PHASE_RESPONSE;
            }
            break;
        }

        if (*request_buffer == '\0')
        {
            /* not the end of http request */
            return request_buffer - buffer;
        }

        ch = strstr(request_buffer, "\r\n");
        if (ch == NULL)
        {
            /* not the end of http header request line */
            return request_buffer - buffer;
        }
        /* set terminal field */
        *ch ++ = '\0';
        *ch ++ = '\0';

        if (str_begin_with(request_buffer, "Host:"))
        {
            /* get host */
            request_buffer += 5;
            while (*request_buffer == ' ') request_buffer ++;
            request->host = wn_strdup(request_buffer);
        }
        else if (str_begin_with(request_buffer, "User-Agent:"))
        {
            /* get user agent */
            request_buffer += 11;
            while (*request_buffer == ' ') request_buffer ++;
            request->user_agent = wn_strdup(request_buffer);
        }
        else if (str_begin_with(request_buffer, "Accept-Language:"))
        {
            /* get accept language */
            request_buffer += 16;
            while (*request_buffer == ' ') request_buffer ++;
            request->accept_language = wn_strdup(request_buffer);
        }
        else if (str_begin_with(request_buffer, "Content-Length:"))
        {
            /* get content length */
            request_buffer += 15;
            while (*request_buffer == ' ') request_buffer ++;
            request->content_length = atoi(request_buffer);
        }
        else if (str_begin_with(request_buffer, "Content-Type:"))
        {
            /* get content type */
            request_buffer += 13;
            while (*request_buffer == ' ') request_buffer ++;
            request->content_type = wn_strdup(request_buffer);
        }
        else if (str_begin_with(request_buffer, "Referer:"))
        {
            /* get referer */
            request_buffer += 8;
            while (*request_buffer == ' ') request_buffer ++;
            request->referer = wn_strdup(request_buffer);
        }
#ifdef WEBNET_USING_RANGE
        else if (str_begin_with(request_buffer, "Range:"))
        {
            /* get range */
            request_buffer += 6;
            while (*request_buffer == ' ') request_buffer ++;
            request->Range = wn_strdup(request_buffer);
        }
#endif /* WEBNET_USING_RANGE */
#ifdef WEBNET_USING_DAV
        else if(str_begin_with(request_buffer, "Depth:"))
        {
            request_buffer += 6;
            while (*request_buffer == ' ') request_buffer ++;
            request->depth = wn_strdup(request_buffer);
        }
        else if (str_begin_with(request_buffer, "Destination:"))
        {
            request_buffer += 12;
            while (*request_buffer == ' ') request_buffer ++;
            request->destination = wn_strdup(request_buffer);
        }
#endif /* WEBNET_USING_DAV */
#ifdef WEBNET_USING_KEEPALIVE
        else if (str_begin_with(request_buffer, "Connection:"))
        {
            /* set default connection to keep alive */
            request->connection = WEBNET_CONN_KEEPALIVE;

            /* get connection */
            request_buffer += 11;
            while (*request_buffer == ' ') request_buffer ++;

            if (str_begin_with(request_buffer, "close"))
                request->connection = WEBNET_CONN_CLOSE;
            else if (str_begin_with(request_buffer, "Keep-Alive"))
                request->connection = WEBNET_CONN_KEEPALIVE;
        }
#endif
#ifdef WEBNET_USING_COOKIE
        else if (str_begin_with(request_buffer, "Cookie:"))
        {
            /* get cookie */
            request_buffer += 7;
            while (*request_buffer == ' ') request_buffer ++;
            request->cookie = wn_strdup(request_buffer);
        }
#endif /* WEBNET_USING_COOKIE */
#ifdef WEBNET_USING_AUTH
        else if (str_begin_with(request_buffer, "Authorization: Basic"))
        {
            /* get authorization */
            request_buffer += 20;
            while (*request_buffer == ' ') request_buffer ++;
            request->authorization = wn_strdup(request_buffer);
        }
#endif /* WEBNET_USING_AUTH */
#if WEBNET_CACHE_LEVEL > 0
        else if (str_begin_with(request_buffer, "If-Modified-Since:"))
        {
            /* get If-Modified-Since */
            request_buffer += 18;
            while (*request_buffer == ' ') request_buffer ++;
            request->modified = wn_strdup(request_buffer);
        }
#endif /* WEBNET_CACHE_LEVEL > 0 */
#ifdef WEBNET_USING_GZIP
        else if (str_begin_with(request_buffer, "Accept-Encoding:"))
        {
            const char *gzip = strstr(request_buffer, "gzip");

            if( (gzip != NULL))
            {
                request->suppogzip = TRUE;
            }
        }
#endif /* WEBNET_USING_GZIP */
        else if (str_begin_with(request_buffer, "SOAPACTION:"))
        {
            request_buffer += 11;
            while (*request_buffer == ' ') request_buffer ++;

            request->soap_action = wn_strdup(request_buffer);
        }
        else if (str_begin_with(request_buffer, "CALLBACK:"))
        {
            request_buffer += 9;
            while (*request_buffer == ' ') request_buffer ++;

            request->callback = wn_strdup(request_buffer);
        }

        request_buffer = ch;
    }

    return request_buffer - buffer;
}

int webnet_request_parse_post(struct webnet_request* request, char* buffer, int length)
{
    struct webnet_session* session = request->session;

    if (request->query && length)
    {
        if (request->query_offset + length > request->content_length)
            length = request->content_length - request->query_offset;

        memcpy(&request->query[request->query_offset], buffer, length);
        request->query_offset += length;

        if (request->query_offset == request->content_length)
        {
            /* set terminal charater */
            buffer[request->content_length] = '\0';

            /* parse query */
            if (str_begin_with(request->content_type, "application/x-www-form-urlencoded"))
            {
                _webnet_request_parse_query(request);
            }

            /* set to http response phase */
            request->result_code = 200;
            session->session_phase = WEB_PHASE_RESPONSE;
        }
    }

    return length;
}

struct webnet_request* webnet_request_create()
{
    struct webnet_request* request;

    request = (struct webnet_request*) wn_malloc (sizeof(struct webnet_request));
    if (request != NULL)
    {
        memset(request, 0, sizeof(struct webnet_request));
        request->field_copied = FALSE;
    }

    return request;
}

void webnet_request_destory(struct webnet_request* request)
{
    if (request != NULL)
    {
        // if (request->field_copied == TRUE)
        {
            if (request->path != NULL) wn_free(request->path);
            if (request->host != NULL) wn_free(request->host);
            if (request->cookie != NULL) wn_free(request->cookie);
            if (request->user_agent != NULL) wn_free(request->user_agent);
            if (request->authorization != NULL) wn_free(request->authorization);
            if (request->accept_language != NULL) wn_free(request->accept_language);
            if (request->referer != NULL) wn_free(request->referer);
            if (request->content_type != NULL) wn_free(request->content_type);
            if (request->query != NULL) wn_free(request->query);
            if (request->query_items != NULL) wn_free(request->query_items);

            if (request->callback) wn_free(request->callback);
            if (request->soap_action) wn_free(request->soap_action);
            if (request->sid) wn_free(request->sid);
#if (WEBNET_CACHE_LEVEL > 0)
            if(request->modified) wn_free(request->modified);
#endif
#ifdef WEBNET_USING_RANGE
            if (request->Range) wn_free(request->Range);
#endif /* WEBNET_USING_RANGE */
#ifdef WEBNET_USING_DAV
            if (request->depth) wn_free(request->depth);
#endif
        }

        /* free request memory block */
        wn_free(request);
    }
}
