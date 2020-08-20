#include <stdio.h>
#include <string.h>
#include <time.h>
#include <webnet.h>


static int _webnet_module_system_uri_physical(struct webnet_session *session)
{
    int result;
    result = WEBNET_MODULE_CONTINUE;

#if WEBNET_USING_ALIAS
    result = webnet_module_alias(session);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#if WEBNET_USING_AUTH
    result = webnet_module_auth(session);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#if WEBNET_USING_CGI
    result = webnet_module_cgi(session);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#if WEBNET_USING_UPLOAD
    result = webnet_module_upload(session);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

    return result;
}

static void _webnet_dofile_handle(struct webnet_session *session, int event)
{
    int fd = session->user_data;

    if (event & WEBNET_EVENT_WRITE)
    {
        size_t readbytes;
        size_t length = ALIGN_DOWN(WEBNET_SESSION_BUFSZ, 4);
#ifdef WEBNET_USING_RANGE
        if(session->request->Range)
        {
            length = session->request->pos_end - session->request->pos_start;
            if(length == 0)
            {
                goto __exit;
            }
       
            if(length > WEBNET_SESSION_BUFSZ)
            {
                length = WEBNET_SESSION_BUFSZ;
            }
           lseek(fd, session->request->pos_start, SEEK_SET);
           session->request->pos_start += length;
        }
#endif            
        readbytes = read(fd, session->buffer, length);
        if (readbytes <= 0) /* end of file */
            goto __exit;

        if (webnet_session_write(session, session->buffer, readbytes) == 0)
            goto __exit;
        return;
    }

__exit:
    close(fd);
    session->user_data = 0;
    session->session_event_mask = 0; /* clean event */
    /* destroy session */
    session->session_phase = WEB_PHASE_CLOSE;

    return;
}

static const struct webnet_session_ops _dofile_ops =
{
    _webnet_dofile_handle,
    NULL
};

/* send a file to http client */
int webnet_module_system_dofile(struct webnet_session *session)
{
    int fd = -1;    /* file descriptor */
    struct stat file_stat;
    const char *mimetype;
    size_t file_length;
    struct webnet_request *request;

#if WEBNET_CACHE_LEVEL > 0
    char ctime_str[32];
    char gmtime_str[32];
    struct tm* info;
    int stat_result = -1;
#endif /* WEBNET_CACHE_LEVEL */

    ASSERT(session != NULL);
    request = session->request;
    ASSERT(request != NULL);

#if WEBNET_CACHE_LEVEL > 0
#ifdef WEBNET_USING_GZIP
    /* get .gz Last-Modified. */
    if (request->suppogzip)
    {
        struct stat file_stat;

        char *path_gz = wn_malloc(strlen(request->path) + 4);  /* ".gz\0" */

        if (path_gz != NULL)
        {
            sprintf(path_gz, "%s.gz", request->path);
            stat_result = stat(request->path, &file_stat);
            wn_free(path_gz);
        }

        if (stat_result == 0)
        {
            enter_critical();
            strcpy(ctime_str, ctime((time_t *)&file_stat.st_mtime));
            exit_critical();

            ctime_str[strlen(ctime_str) - 1] = '\0'; /* clear the end \n */

            if ((request->modified != NULL)
                    && (strcmp(request->modified, ctime_str) == 0))
            {
                request->result_code = 304;
                return WEBNET_MODULE_FINISHED;
            }
        }
    }

    /* .gz not exist, use raw. */
#endif /* WEBNET_USING_GZIP */
    /* get Last-Modified. */
    if (stat_result != 0)
    {
        struct stat file_stat;
        stat_result = stat(request->path, &file_stat);

        if (stat_result == 0)
        {
            enter_critical();
            info = localtime((time_t *)&file_stat.st_mtime);
            memset(gmtime_str,0,32);
            strftime(gmtime_str,sizeof(ctime_str),"%a, %d %b %Y %H:%M:%S GMT",info);
            
            strcpy(ctime_str, ctime((time_t *)&file_stat.st_mtime));
            exit_critical();

            ctime_str[strlen(ctime_str) - 1] = '\0'; /* clear the end \n */
            gmtime_str[strlen(gmtime_str)] = '\0'; /* clear the end \n */

            if ((request->modified != NULL)
                    && ((strcmp(request->modified, ctime_str) == 0)||strcmp(request->modified, gmtime_str) == 0))
            {
                request->result_code = 304;
                return WEBNET_MODULE_FINISHED;
            }
        }
    }
#endif /* WEBNET_CACHE_LEVEL > 0 */

    /* get mime type */
    mimetype = mime_get_type(request->path);

#ifdef WEBNET_USING_GZIP
    if (request->suppogzip)
    {
        char *path_gz = wn_malloc(strlen(request->path) + 4);  /* ".gz\0" */

        if (path_gz != NULL)
        {
            sprintf(path_gz, "%s.gz", request->path);
            fd = open(path_gz, O_RDONLY, 0);
            wn_free(path_gz);

            if (fd < 0)
            {
                /* .gz not exist, use raw. */
                request->suppogzip = FALSE;
            }
        }
    }

    /* .gz not exist, use raw. */
#endif /* WEBNET_USING_GZIP */
    if (fd < 0 && stat(request->path, &file_stat) >= 0 && !S_ISDIR(file_stat.st_mode))
    {
        fd = open(request->path, O_RDONLY, 0);
    }

    if (fd < 0)
    {
        request->result_code = 404;
        return WEBNET_MODULE_FINISHED;
    }

    /* get file size */
    file_length = lseek(fd, 0, SEEK_END);
    /* seek to beginning of file */
    lseek(fd, 0, SEEK_SET);

    /*************todo**********************/
#ifdef WEBNET_USING_RANGE
    if (request->Range)
    {
        char *range_start, *range_end;
        int32_t pos_start = 0;
        uint32_t pos_end = file_length - 1;
        range_start = strstr(request->Range, "bytes=");
        if (range_start)
        {
            range_start += 6;
            range_end = strstr(range_start, "-");
            if (range_start == range_end)
                pos_start = 0;
            else
                pos_start = atoi(range_start);

            /* send file to remote */
            if ((!range_end) || (strstr(range_start, ",")))
            {
                request->result_code = 400;
                goto _error_exit;
            }
            if (range_end)
            {
                *range_end = '\0';
                range_end += 1;
                pos_end = atoi(range_end);
            }
        }
#ifdef WEBNET_USING_GZIP
        if (request->suppogzip)
        {
            pos_start = 0; /*  */
        }
#endif /* WEBNET_USING_GZIP */
        if ((pos_start >= file_length) || (pos_end >= file_length))
        {
            request->result_code = 416;
            webnet_session_set_header_status_line(session, request->result_code, "Requested Range Not Satisfiable");
            goto _error_exit;
        }
        if (lseek(fd, pos_start, SEEK_SET) != pos_start)
        {
            request->result_code = 500;
            goto _error_exit;
        }
        if (pos_end == 0)
        {
            pos_end = file_length - 1;
        }
        file_length = pos_end - pos_start + 1;
        request->result_code = 216;
        request->pos_start = pos_start;
        request->pos_end = pos_end;
        webnet_session_set_header_status_line(session, request->result_code, "Partial Content");
        webnet_session_printf(session, "Content-Range: %d-%d/%d\r\n", pos_start, pos_end, file_length);
    }else

#endif /* WEBNET_USING_RANGE */

    /*************todo**********************/
    {
        /* send file to remote */
        request->result_code = 200;
        webnet_session_set_header_status_line(session, request->result_code, "OK");
    }
#if WEBNET_CACHE_LEVEL > 0
    /* send Last-Modified. */
    webnet_session_printf(session,
                          "Last-Modified: %s\r\n",
                          ctime_str);
#endif /* WEBNET_CACHE_LEVEL > 0 */

#if WEBNET_CACHE_LEVEL > 1
    /* Cache-Control. */
    webnet_session_printf(session,
                          "Cache-Control: max-age=%d\r\n",
                          WEBNET_CACHE_MAX_AGE);
#endif /* WEBNET_CACHE_LEVEL > 1 */

    /* send Content-Type. */
    webnet_session_printf(session,
                          "Content-Type: %s\r\n",
                          mimetype);

    /* send Content-Length. */
    webnet_session_printf(session,
                          "Content-Length: %ld\r\n",
                          file_length);

#ifdef WEBNET_USING_KEEPALIVE
	if(session->request->connection == WEBNET_CONN_KEEPALIVE)
	{
		webnet_session_printf(session,
                          "Connection: %s\r\n",
                          "Keep-Alive");
	}
	else
	{
		webnet_session_printf(session,
                          "Connection: %s\r\n",
                          "close");
	}
#else
	webnet_session_printf(session,
                        "Connection: %s\r\n",
                        "close");
#endif

#ifdef WEBNET_USING_GZIP
    if (request->suppogzip)
    {
        /* gzip deflate. */
        webnet_session_printf(session, "Content-Encoding: gzip\r\n");
    }
#endif /* WEBNET_USING_GZIP */

    /* send Access-Control-Allow-Origin. */
    webnet_session_printf(session, "Access-Control-Allow-Origin:*\r\n");

    /* send http header end. */
    webnet_session_printf(session, "\r\n");

    if (file_length <= 0)
    {
        close(fd);
        return WEBNET_MODULE_FINISHED;
    }

    /*
     * set session write context
     */
    if (request->method != WEBNET_HEADER)
    {
        /* set dofile session ops */
        session->session_event_mask = WEBNET_EVENT_WRITE;
        session->user_data = (uint32_t)fd;
        session->session_ops = &_dofile_ops;
    }
    return WEBNET_MODULE_FINISHED;

_error_exit:
    if (fd >= 0)
    {
        close(fd);
    }

    return WEBNET_MODULE_FINISHED;
}

static int _webnet_module_system_uri_post(struct webnet_session *session)
{
    int result;
    result = WEBNET_MODULE_CONTINUE;

#if WEBNET_USING_ASP
    result = webnet_module_asp(session);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#if WEBNET_USING_SSI
    result = webnet_module_ssi(session);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

#if WEBNET_USING_INDEX
    result = webnet_module_dirindex(session);
    if (result == WEBNET_MODULE_FINISHED) return result;
#endif

    /* always module finished in dofile */
    result = webnet_module_system_dofile(session);
    if (result == WEBNET_MODULE_FINISHED) return result;

    return WEBNET_MODULE_CONTINUE;
}


/* default index file */
static const char *default_files[] =
{
    "",
    "/index.html",
    "/index.htm",
    NULL
};

/**
 * handle uri
 * there are two phases on uri handling:
 * - map url to physical
 * - url handling
 */
int webnet_module_handle_uri(struct webnet_session *session)
{
    int result;
    char *full_path;
    uint32_t index;
    struct webnet_request *request;

    ASSERT(session != NULL);
    /* get request */
    request = session->request;
    ASSERT(request != NULL);

    /* map uri to physical */
    result = _webnet_module_system_uri_physical(session);
    if (result == WEBNET_MODULE_FINISHED) return result;

    /* made a full physical path */
    full_path = (char *) wn_malloc(WEBNET_PATH_MAX);
    ASSERT(full_path != NULL);

    /* only GET or POST need try default page. */
    if ((session->request->method != WEBNET_GET)
            && (session->request->method != WEBNET_POST))
    {
        index = sizeof(default_files) / sizeof(default_files[0]);
        index -= 1;

        goto _end_default_files;
    }

    index = 0;
    while (default_files[index] != NULL)
    {
        struct stat file_stat;
        
        /* made a full path */
        snprintf(full_path, WEBNET_PATH_MAX, "%s/%s%s",
                    webnet_get_root(), request->path, default_files[index]);
        /* normalize path */
        str_normalize_path(full_path);

        if (stat(full_path, &file_stat) >= 0 && !S_ISDIR(file_stat.st_mode))
        {
            break;
        }

        index ++;
    }
_end_default_files:

    /* no this file */
    if (default_files[index] == NULL)
    {
        /* use old full path */
        snprintf(full_path, WEBNET_PATH_MAX, "%s/%s", webnet_get_root(), request->path);
        /* normalize path */
        str_normalize_path(full_path);
    }

    /* mark path as full physical path */
    wn_free(request->path);
    request->path = full_path;

    /* check uri valid */
    if (!str_begin_with(request->path, webnet_get_root()))
    {
        /* not found */
        request->result_code = 404;
        return WEBNET_MODULE_FINISHED;
    }

    /* uri post handle */
    return _webnet_module_system_uri_post(session);
}