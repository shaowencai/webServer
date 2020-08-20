#ifndef __WN_SESSION_H__
#define __WN_SESSION_H__

#include <webnet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WEBNET_SESSION_BUFSZ	(4 * 1024)

/* close session */
#define WEBNET_EVENT_CLOSE			(1 << 5)
/* read  session */
#define WEBNET_EVENT_READ			(1 << 6)
/* write session */
#define WEBNET_EVENT_WRITE			(1 << 7)


struct webnet_session_ops
{
    void (*session_handle)(struct webnet_session* session, int event);
    void (*session_close) (struct webnet_session* session);
};

enum webnet_session_phase
{
	WEB_PHASE_METHOD = 0, 	/* parse method */
	WEB_PHASE_HEADER, 		/* handle web request header */
	WEB_PHASE_QUERY,        /* handle web query */
	WEB_PHASE_RESPONSE,		/* handle web response */
	WEB_PHASE_CLOSE,		/* to close session */
};

struct webnet_session
{
    struct webnet_session *next;

    int socket;
    struct sockaddr_in cliaddr;
    struct webnet_request* request;
    Timer  timer;
    uint16_t buffer_length;
    uint16_t buffer_offset;
    uint8_t  buffer[WEBNET_SESSION_BUFSZ];
	uint32_t  session_phase;
	uint32_t  session_event_mask;
    const struct webnet_session_ops* session_ops;
    uint32_t user_data;
    uint32_t app_user_data;
    char ClientIP[32];
};

struct webnet_session* webnet_session_create(int listenfd);

int  webnet_session_read(struct webnet_session *session, char *buffer, int length);
void webnet_session_close(struct webnet_session *session);

void webnet_session_printf(struct webnet_session *session, const char* fmt, ...);
int  webnet_session_write(struct webnet_session *session, const uint8_t* data, size_t size);
int  webnet_session_redirect(struct webnet_session *session, const char* url);
int  webnet_session_get_physical_path(struct webnet_session *session, const char* virtual_path, char* full_path);
void webnet_session_set_header(struct webnet_session *session, const char* mimetype, int code, const char* status, int length);
void webnet_session_set_header_status_line(struct webnet_session *session, int code, const char * reason_phrase);

int webnet_sessions_set_fds(fd_set *readset, fd_set *writeset);
void webnet_sessions_handle_fds(fd_set *readset, fd_set *writeset);

void webnet_sessions_set_err_callback(void (*callback)(struct webnet_session *session));

#ifdef  __cplusplus
    }
#endif

#endif /* __WN_SESSION_H__ */

