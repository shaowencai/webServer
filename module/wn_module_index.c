#include <webnet.h>

#if WEBNET_USING_INDEX

int webnet_module_dirindex(struct webnet_session* session)
{
    if( (session->request->method != WEBNET_GET)
            && (session->request->method != WEBNET_POST) )
    {
        return WEBNET_MODULE_CONTINUE;
    }

    DIR *dir;
    struct stat file_stat;
    struct webnet_request *request;
    static const char* header = "<html><head><title>Index of %s</title></head><body bgcolor=\"white\"><h1>Index of %s</h1><hr><pre>";
    static const char* foot = "</pre><hr>WebNet/%s (CET)</body></html>";

    ASSERT(session != NULL);
    request = session->request;
    ASSERT(request != NULL);

    if (stat(request->path, &file_stat) < 0 || !S_ISDIR(file_stat.st_mode))
    {
        return WEBNET_MODULE_CONTINUE;
    }

    dir = opendir(request->path);
    if (dir != NULL)
    {
        struct stat s;
        struct dirent* dirent;
        const char* sub_path;
        char *fullpath;
        char *delim;

        dirent = NULL;
        fullpath = wn_malloc (WEBNET_PATH_MAX);
        if (fullpath == NULL)
        {
            request->result_code = 500;
            return WEBNET_MODULE_FINISHED;
        }

        webnet_session_set_header(session, "text/html", 200, "OK", -1);
        /* get sub path */
        sub_path = request->path + strlen(webnet_get_root());
        delim = strrchr(sub_path, '/');
        snprintf(fullpath, delim - sub_path + 1, "%s", sub_path);
        webnet_session_printf(session, header, sub_path, sub_path);
        /* display parent directory */
        webnet_session_printf(session, "<a href=\"../\">..</a>\n");

        /* list directory */
        do
        {
            dirent = readdir(dir);
            if (dirent == NULL) break;

            memset(&s, 0, sizeof(struct stat));

            /* build full path for each file */
            sprintf(fullpath, "%s/%s", request->path, dirent->d_name);
            str_normalize_path(fullpath);

            stat(fullpath, &s);
            sprintf(fullpath, "%s/%s", sub_path, dirent->d_name);
            if ( s.st_mode & S_IFDIR )
            {
                webnet_session_printf(session, "<a href=\"%s/\">%s/</a>\n", fullpath, dirent->d_name);
            }
            else
            {
                webnet_session_printf(session, "<a href=\"%s\">%s</a>\t\t\t\t\t%d\n", fullpath, dirent->d_name, s.st_size);
            }
        }
        while (dirent != NULL);

        closedir(dir);
        wn_free(fullpath);

        /* set foot */
        webnet_session_printf(session, foot, WEBNET_VERSION);

        return WEBNET_MODULE_FINISHED;
    }

    return WEBNET_MODULE_CONTINUE;
}

#endif /* WEBNET_USING_INDEX */

