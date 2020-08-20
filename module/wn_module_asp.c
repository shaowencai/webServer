#include <string.h>
#include <webnet.h>


#if WEBNET_USING_ASP

struct webnet_asp_variable
{
    char* name;
    void (*handler)(struct webnet_session* session);
};

static struct webnet_asp_variable* _webnet_asp_vars = NULL;
static uint32_t _webnet_asp_vars_count = 0;

void webnet_asp_add_var(const char* name, void (*handler)(struct webnet_session* session))
{
    if (_webnet_asp_vars == NULL)
    {
        _webnet_asp_vars_count = 1;
        _webnet_asp_vars = (struct webnet_asp_variable*)wn_malloc (sizeof(struct webnet_asp_variable) *
                           _webnet_asp_vars_count);
    }
    else
    {
        _webnet_asp_vars_count += 1;
        _webnet_asp_vars = (struct webnet_asp_variable*) wn_realloc (_webnet_asp_vars, sizeof(struct webnet_asp_variable) *
                           _webnet_asp_vars_count);
    }

    ASSERT(_webnet_asp_vars != NULL);

    _webnet_asp_vars[_webnet_asp_vars_count - 1].name = wn_strdup(name);
    _webnet_asp_vars[_webnet_asp_vars_count - 1].handler = handler;
}

static void _webnet_asp_dofile(struct webnet_session* session, int fd)
{
    char *asp_begin, *asp_end;
    char *offset, *end;
    char *buffer;
    uint32_t length, index;

    /* get file length */
    length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    /* allocate read buffer */
    buffer = (char*) wn_malloc (length);
    if (buffer == NULL)
    {
        session->request->result_code = 500;
        return;
    }

    /* write page header */
    webnet_session_set_header(session, "text/html", 200, "OK", -1);
    /* read file to buffer */
    if (read(fd, buffer, (size_t)length) != length) /* read failed */
    {
        wn_free(buffer);
        session->request->result_code = 500;
        return;
    }

    offset = buffer;
    end = buffer + length;
    while (offset < end)
    {
        /* get beginning of asp variable */
        asp_begin = strstr(offset, "<%");
        if (asp_begin == NULL)
        {
            /* write content directly */
            webnet_session_write(session, (const uint8_t*)offset, end - offset);
            break;
        }

        /* get end of aps variable */
        asp_end = strstr(asp_begin, "%>");
        if (asp_end == NULL)
        {
            /* write content directly */
            webnet_session_write(session, (const uint8_t*)offset, end - offset);
            break;
        }
        else
        {
            /* write content */
            webnet_session_write(session, (const uint8_t*)offset, asp_begin - offset);

            offset = asp_begin + 2;
            while ((*offset == ' ') || (*offset == '\t')) offset ++;

            /* extrace asp variable */
            for (index = 0; index < _webnet_asp_vars_count; index ++)
            {
                if (str_begin_with(offset, _webnet_asp_vars[index].name))
                {
                    /* found asp variable */
                    _webnet_asp_vars[index].handler(session);
                    break;
                }
            }

            /* move to the end of asp varaialbe */
            offset = asp_end + 2;
        }
    }

    /* release read buffer */
    wn_free(buffer);
}

int webnet_module_asp(struct webnet_session* session)
{
    struct webnet_request* request;
    int fd;

    ASSERT(session != NULL);
    request = session->request;
    ASSERT(request != NULL);

    /* check whether a asp file */
    if ((strstr(request->path, ".asp") != NULL) ||
            (strstr(request->path, ".ASP") != NULL))
    {
        /* try to open this file */
        fd = open(request->path, O_RDONLY, 0);
        if ( fd >= 0)
        {
            _webnet_asp_dofile(session, fd);
            close(fd);
            return WEBNET_MODULE_FINISHED;
        }
        else
        {
            /* no this file */
            request->result_code = 404;
            return WEBNET_MODULE_FINISHED;
        }
    }
    else
    {
        /* try index.asp */
        char *asp_filename;

        asp_filename = (char*) wn_malloc (WEBNET_PATH_MAX);
        if (asp_filename != NULL)
        {
            snprintf(asp_filename, WEBNET_PATH_MAX, "%s/index.asp", request->path);
            fd = open(asp_filename, O_RDONLY, 0);

            if (fd >= 0)
            {
                wn_free(asp_filename);
                _webnet_asp_dofile(session, fd);
                close(fd);
                return WEBNET_MODULE_FINISHED;
            }
        }
        wn_free(asp_filename);
    }

    return WEBNET_MODULE_CONTINUE;
}

#endif /* WEBNET_USING_ASP */
