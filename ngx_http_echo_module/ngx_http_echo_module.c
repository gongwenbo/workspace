
/*
 * Copyright (C) Liuzhaodong
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
	ngx_str_t ed;
} ngx_http_echo_loc_conf_t;


static char *ngx_http_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_echo_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_echo_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_echo_handler(ngx_http_request_t *r);


static ngx_command_t ngx_http_echo_commands[] = {
	{ ngx_string("echo"),
	  NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_http_echo,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(ngx_http_echo_loc_conf_t, ed),
	  NULL },

	  ngx_null_command
};


static ngx_http_module_t ngx_http_echo_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */
    ngx_http_echo_create_loc_conf,         /* create location configration */
    ngx_http_echo_merge_loc_conf           /* merge location configration */
};


ngx_module_t ngx_http_echo_module = {
    NGX_MODULE_V1,
    &ngx_http_echo_module_ctx,             /* module context */
    ngx_http_echo_commands,                /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static void *
ngx_http_echo_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_echo_loc_conf_t *conf;
	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_echo_loc_conf_t));
	if (conf == NULL) {
		return NULL;	
	}
	return conf;
}


static char *
ngx_http_echo_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_echo_loc_conf_t *prev = parent;
	ngx_http_echo_loc_conf_t *conf = child;
	ngx_conf_merge_str_value(conf->ed, prev->ed, "");
	return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_echo_handler(ngx_http_request_t *r)
{
	ngx_int_t rc;
	ngx_buf_t *b;
	ngx_chain_t out;
	ngx_unit_t content_length = 0;
	ngx_http_echo_loc_conf_t *elcf;
	
	/* get the module conf */
	elcf = ngx_http_get_module_loc_conf(r, ngx_http_echo_module);
	
	/* we response to 'GET' and 'HEAD' requests only */
	if (!(r->method & (NGX_HTTP_HEAD|NGX_HTTP_GET))) {
		return NGX_HTTP_NOT_ALLOWED;	
	}
	
	/* discard request body, since we don't need it here */
	rc = ngx_http_discard_request_body(r);	
	if (rc != NGX_OK) {
		return rc;
	}
	
	/* set the 'Content-type' header */
	ngx_str_set(&r->headers_out.content_type, "text/html");
	content_length = ngx_strlen(elcf->ed);
	
	/* send the header only, if the request type is http 'HEAD' */
	if (r->method == NGX_HTTP_HEAD) {
		r->headers_out.status = NGX_HTTP_OK;	
		r->headers_out.content_length_n = content_length;
		
		return ngx_http_send_header(r);
	}
	
	/* allocate a buffer to the response body */
	b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if (b == NULL) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	
	/* attach this buffer to the buffer chain */
	out.buf = b;
	out.next = NULL;

	/* adjust the pointers of the buffer */
	b->pos = elcf->ed.data;
	b->last = elcf->ed.data + content_length;
	b->memory = 1;
	b->last_buf = 1;
	
	/* set the status line */
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = content_length;
	
	/* send the headers of the response */
	rc = ngx_http_send_header(r);
	
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		return rc;	
	}
	
	/* send the buffer chain of the response */
	return ngx_http_output_filter(r, &out);
}

static char *
ngx_http_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t *clcf;
	
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_echo_handler;
	ngx_conf_set_str_slot(cf, cmd, conf);
	
	return NGX_CONF_OK;
}
