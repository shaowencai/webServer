#ifndef __WN_MODULE_H__
#define __WN_MODULE_H__

#include "webnet.h"

#ifdef __cplusplus
extern "C" {
#endif

/* continue other modules */
#define WEBNET_MODULE_CONTINUE		0
/* this session is finished */
#define WEBNET_MODULE_FINISHED		1

int webnet_module_system_dofile(struct webnet_session* session);
int webnet_module_handle_uri(struct webnet_session* session);

/* module function pre-declaration */
int webnet_module_alias(struct webnet_session* sesion);
int webnet_module_auth(struct webnet_session* session);
int webnet_module_asp(struct webnet_session* session);
int webnet_module_cgi(struct webnet_session* session);
int webnet_module_dirindex(struct webnet_session* session);
int webnet_module_ssi(struct webnet_session* session);

/* add ASP variable */
void webnet_asp_add_var(const char* name, void (*handler)(struct webnet_session* session));
void webnet_cgi_set_hook_before_handle(int (*fun)(void*,void*));
void webnet_cgi_set_hook_after_handle (int (*fun)(void*,void*));
/* register CGI event */
void webnet_cgi_register(const char* name,uint16_t level, void (*handler)(struct webnet_session* session));
void webnet_cgi_set_root(const char* root);
/* set basic authentication configure */
void webnet_auth_set(const char* path, const char* username_password);
/* set directory alias */
void webnet_alias_set(char* old_path, char* new_path);

struct webnet_cgi_item
{
    const char* name;
    uint16_t level;
    void (*handler)(struct webnet_session* session);
};

/* upload module */
struct webnet_module_upload_entry
{
    const char* url;

    int (*upload_open) (struct webnet_session* session);
    int (*upload_close)(struct webnet_session* session);
    int (*upload_write)(struct webnet_session* session, const void* data, size_t length);
    int (*upload_done) (struct webnet_session* session);
};
int webnet_module_upload(struct webnet_session* session);
void webnet_upload_add(const struct webnet_module_upload_entry* entry);

const char* webnet_upload_get_filename(struct webnet_session* session);
const char* webnet_upload_get_content_type(struct webnet_session* session);
const char* webnet_upload_get_nameentry(struct webnet_session* session, const char* name);
const void* webnet_upload_get_userdata(struct webnet_session* session);

int webnet_upload_file_open(struct webnet_session* session);
int webnet_upload_file_close(struct webnet_session* session);
int webnet_upload_file_write(struct webnet_session* session, const void* data, size_t length);

#ifdef  __cplusplus
    }
#endif

#endif /* __WN_MODULE_H__ */
