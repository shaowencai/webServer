#include <webnet.h>

static uint32_t   webnet_port         = 80;
static char       webnet_root[64]     = "web";
static uint8_t    init_ok             = 0;

void webnet_set_port(int port)
{
    if(init_ok == 0)
    {
        webnet_port = port;
    }
}

int webnet_get_port(void)
{
    return webnet_port;
}

void webnet_set_root(const char* webroot_path)
{
    strncpy(webnet_root, webroot_path, sizeof(webnet_root) - 1);
    webnet_root[sizeof(webnet_root) - 1] = '\0';
}

const char* webnet_get_root(void)
{
    return webnet_root;
}


const char my_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIICMDCCAZkCCQCWv61peEI6oDANBgkqhkiG9w0BAQUFADBZMQswCQYDVQQGEwJD\r\n"
"TjESMBAGA1UECBMJZ3Vhbmdkb25nMREwDwYDVQQHEwhzaGVuemhlbjELMAkGA1UE\r\n"
"ChMCQ0ExFjAUBgNVBAMTDTE5Mi4xNjguMC4xMjcwHhcNMjAwMzIwMTExMzA5WhcN\r\n"
"MjAwNDE5MTExMzA5WjBgMQswCQYDVQQGEwJDTjESMBAGA1UECBMJZ3Vhbmdkb25n\r\n"
"MREwDwYDVQQHEwhzaGVuemhlbjESMBAGA1UEChQJU2VydmVyX0NBMRYwFAYDVQQD\r\n"
"Ew0xOTIuMTY4LjAuMTI3MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC58knQ\r\n"
"q56YYfbr2pp3jre46g8xIJOq9DVBlFYLpPy75AgAvWxM1UCR4Yig6jq5Ax4FYOQe\r\n"
"/KRWHPHyjMdU46wAnoNupD1NQ2sZup/ahKFcNNrpxkPb07cs+F3mIQ2qUL88I87S\r\n"
"CXpnPpNmlAA2ff1XKH2wGs03TKxP0uHgcN4NAwIDAQABMA0GCSqGSIb3DQEBBQUA\r\n"
"A4GBAM3xlxtb5RNKFYBixMR7DVLt2v1y2iB0oF+0w1VC2tCmdffpEYNwk0na6/m+\r\n"
"m9xZxNseypzTfcnI7Pwuxv6JkkreYBHmJA2O2wRiId1rNM7ulcpoU0KuoOX6vAwV\r\n"
"pHmxBxquyQalYtqQ3DaGCExqiVsXhXMYPuxgrBfUL3F0grdZ\r\n"
"-----END CERTIFICATE-----\r\n";

const size_t my_crt_len = sizeof( my_crt );


const char my_key[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIICXgIBAAKBgQC58knQq56YYfbr2pp3jre46g8xIJOq9DVBlFYLpPy75AgAvWxM\r\n"
"1UCR4Yig6jq5Ax4FYOQe/KRWHPHyjMdU46wAnoNupD1NQ2sZup/ahKFcNNrpxkPb\r\n"
"07cs+F3mIQ2qUL88I87SCXpnPpNmlAA2ff1XKH2wGs03TKxP0uHgcN4NAwIDAQAB\r\n"
"AoGANwPGbyS3saIaakGtPf5NwToO8JwQ1J2YPCTlKGDSHc0jyQRCTKEzj3XI9MMY\r\n"
"SLDxaun11G0vOgMqWnboaudJdgwj68Ljjb28jNdz2uNoPSvTHWl5tuiLhcbrr5/o\r\n"
"qMi3Xxzk9IDNGHiD3CVYzGKx5UpmcUOz5GcDlcZACDcw0+ECQQDmcGag7oEchsQ0\r\n"
"aYLejukx8rVGX0QDQKDECg+M5GEBi2TG0sNY8YziSaYRK6hhhXudDTumUfayQebQ\r\n"
"UrVD0nqLAkEAzpJ3AqgeVbRtV4S1gMyZdOvwNR/eNSQbGrisTyJHoK/5okV45Spv\r\n"
"33BtWDnZgVU9h/URKz5+ApXB5IMzbLieaQJBAIYm/4hG8UEzL7w3hKmeVyHt4xxx\r\n"
"z315PV6DYOQr/FFS3jtlbbY1AUiniZdLbD5B4GHg104PP2gtN3sl+0LdoNsCQQDK\r\n"
"qrQMbTjlyREFrnQMm69rIRgYZt2xsWzOOKUFNpKjukLmy9YisBH2W+1Lg+Y7l4+4\r\n"
"1d00WilzC86hDX/kiPURAkEAk4GlMtN5h0m7rEL/jKct3kfJ78tzAP9462h2cCgT\r\n"
"Z05yLbmo/iVdmMCRKKdqebjGzz1qlDT7upnoIuIpMkZFzQ==\r\n"
"-----END RSA PRIVATE KEY-----\r\n";


const size_t my_key_len = sizeof( my_key );

/**
 * webnet thread entry
 */
void webnet_start(char *port,void * user_data)
{
#if WEBNET_USING_HTTPS
    int handle;
    char *pers = "ssl_server";
#else
    int listenfd = -1;
    fd_set readset, tempfds;
    fd_set writeset, tempwrtfds;
    int sock_fd, maxfdp1;
    struct sockaddr_in webnet_saddr;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
#endif

    struct webnet_session* session;
    struct in_addr  cliaddr;
    
#if WEBNET_USING_HTTPS
	if ((handle = mbedtls_server_init((void *) pers, strlen(pers))) == 0)
	{
		return;
	}

	if ((mbedtls_server_context(handle,(unsigned const char *)my_crt,my_crt_len,(unsigned const char *)my_key,my_key_len)) != 0)
	{
		mbedtls_server_close(handle);
		return;
	}
	
	if ((mbedtls_server_listen(handle,port==NULL ?"443":port)) != 0)
	{
		mbedtls_server_close(handle);
		return;
	}
	
	for (;;)
	{
		if ((mbedtls_server_accept(handle,&cliaddr)) != 0)
		{
			cmTaskDelay(100);
			continue;
		}

		session = webnet_session_create(handle);
		session->app_user_data = (uint32_t)user_data;
		memset(session->ClientIP,0,sizeof(session->ClientIP));
		snprintf(session->ClientIP,sizeof(session->ClientIP),inet_ntoa(cliaddr));
		webnet_sessions_handle_fds(NULL, NULL);
	}

#else
    /* First acquire our socket for listening for connections */
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0)
    {
        printf("Create socket failed.");
        goto __exit;
    }

    memset(&webnet_saddr, 0, sizeof(webnet_saddr));
    webnet_saddr.sin_family = AF_INET;
    webnet_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    webnet_saddr.sin_port = htons(webnet_port); /* webnet server port */

    if (bind(listenfd, (struct sockaddr *) &webnet_saddr, sizeof(webnet_saddr)) == -1)
    {
        goto __exit;
    }

    /* Put socket into listening mode */
    if (listen(listenfd, 20) == -1)
    {
        goto __exit;
    }

    /* Wait forever for network input: This could be connections or data */
    for (;;)
    {
        /* Determine what sockets need to be in readset */
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(listenfd, &readset);

        /* set fds in each sessions */
        maxfdp1 = webnet_sessions_set_fds(&readset, &writeset);
        if (maxfdp1 < listenfd + 1)
        {
            maxfdp1 = listenfd + 1;
        }

        /* use temporary fd set in select */
        tempfds = readset;
        tempwrtfds = writeset;
        /* Wait for data or a new connection */
        sock_fd = select(maxfdp1, &tempfds, &tempwrtfds, 0, &timeout);
        timer_loop();//查询是否有连接超时，超时就取消此次会话 释放资源  断开连接
        if (sock_fd == 0)
        {
            continue;
        }

        /* At least one descriptor is ready */
        if (FD_ISSET(listenfd, &tempfds))
        {
            struct webnet_session* accept_session;
            /* We have a new connection request */
            accept_session = webnet_session_create(listenfd);

            if (accept_session == NULL)
            {
                /* create session failed, just accept and then close */
                int sock;
                struct sockaddr cliaddr;
                socklen_t clilen;

                clilen = sizeof(struct sockaddr_in);
                sock = accept(listenfd, &cliaddr, &clilen);
                if (sock >= 0)
                {
                    close(sock);
                }
            }
            else
            {
                /* add read fdset */
                FD_SET(accept_session->socket, &readset);
            }
        }

        webnet_sessions_handle_fds(&tempfds, &writeset);
    }

__exit:
    if (listenfd >= 0)
    {
        close(listenfd);
    }
#endif
    
}

