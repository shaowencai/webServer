
#ifndef __WEBNET_H__
#define __WEBNET_H__

#ifdef __cplusplus
extern "C" {
#endif

#define  WEBNET_USING_CGI              1
#define  WEBNET_USING_ASP              0
#define  WEBNET_USING_ALIAS            0
#define  WEBNET_USING_AUTH             0
#define  WEBNET_USING_UPLOAD           0
#define  WEBNET_USING_INDEX            0
#define  WEBNET_USING_SSI              0
#define  WEBNET_USING_HTTPS            1


#ifndef WEBNET_USING_RANGE
#define WEBNET_USING_RANGE
#endif
    
#ifndef WEBNET_USING_KEEPALIVE
#define WEBNET_USING_KEEPALIVE
#endif

#ifndef WEBNET_USING_COOKIE
#define WEBNET_USING_COOKIE
#endif

#define TRUE                            (1)
#define FALSE                           (0)

#define WEBNET_PATH_MAX                 (512)
#define POST_BUFSZ_MAX      			(8 * 1024)
#define WEBNET_VERSION                  "1.0.0"
#define WEBNET_SERVER					"Server: CET_Server "WEBNET_VERSION"\r\n"
#define WEBNET_ROOT						"/run/app"
#define WEBNET_PORT						(80)
#define ALIGN_DOWN(size, align)         ((size) & ~((align) - 1))


struct webnet_session;
struct webnet_query_item
{
    char* name;
    char* value;
};


const char* mime_get_type(const char* url);
void        webnet_set_port(int port);
int         webnet_get_port(void);
void        webnet_set_root(const char* webroot_path);
const char* webnet_get_root(void);
void        webnet_start(char *port,void *user_data);

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <wn_timer.h>
#include <wn_session.h>
#include <wn_module.h>
#include <wn_utils.h>
#include <wn_request.h>
#include <wn_malloc.h>

#if  WEBNET_USING_HTTPS
#include "libOpenSSL.h"
#endif

#ifndef ASSERT
#define ASSERT(EX)          if (!(EX)){printf("File:\\ %s Line: %d: ",__FILE__, __LINE__);}
#endif

#ifdef  __cplusplus
    }
#endif

#endif /* __WEBNET_H__ */
