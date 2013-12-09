#ifndef __SHTTPD_H__
#define __SHTTPD_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>   /* for sockaddr_in */
#include <netdb.h>        /* for hostent */ 
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>        /* we want to catch some of these after all */
#include <unistd.h>       /* protos for read, write, close, etc */
#include <dirent.h>       /* for MAXNAMLEN */
#include <limits.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stddef.h>


/*Ïß³ÌµÄ×´Ì¬Öµ*/
enum{WORKER_INITED, WORKER_RUNNING,WORKER_DETACHING, WORKER_DETACHED,WORKER_IDEL};

struct conf_opts{
	char CGIRoot[128];		/*CGI¿¿¿¿¿*/
	char DefaultFile[128];		/*¿¿¿¿¿¿*/
	char DocumentRoot[128];		/*¿¿¿¿¿*/
	char ConfigFile[128];		/*¿¿¿¿¿¿¿¿¿*/
	int ListenPort;			/*¿¿¿¿*/
	int MaxClient;			/*¿¿¿¿¿¿¿*/
	int TimeOut;			/*¿¿¿¿*/
	int InitClient;			/*¿¿¿¿¿¿¿*/
};



/* HTTPĞ­ÒéµÄ·½·¨ */
typedef enum SHTTPD_METHOD_TYPE{
	METHOD_GET, 		/*GET     ·½·¨*/
	METHOD_POST, 		/*POST   ·½·¨*/
	METHOD_PUT, 		/*PUT     ·½·¨*/
	METHOD_DELETE, 	/*DELETE·½·¨*/
	METHOD_HEAD,		/*HEAD   ·½·¨*/
	METHOD_CGI,		/**CGI·½·¨*/
	METHOD_NOTSUPPORT
}SHTTPD_METHOD_TYPE;

enum {HDR_DATE, HDR_INT, HDR_STRING};	/* HTTP header types		*/

typedef struct shttpd_method{
	SHTTPD_METHOD_TYPE type;
	int name_index;
	
}shttpd_method;

/*
 * For parsing. This guy represents a substring.
 */
typedef struct vec 
{
	char	*ptr;
	int			len;	/*request method len*/
	SHTTPD_METHOD_TYPE type;	/*char type*/
}vec;

/*
 * This thing is aimed to hold values of any type.
 * Used to store parsed headers' values.
 */
#define big_int_t long

struct http_header {
	int		len;		/* Header name length		*/
	int		type;		/* Header type			*/
	size_t		offset;		/* Value placeholder		*/
	char	*name;		/* Header name			*/
};

/*
 * This structure tells how HTTP headers must be parsed.
 * Used by parse_headers() function.
 */
#define	OFFSET(x)	offsetof(struct headers, x)

/*
 * This thing is aimed to hold values of any type.
 * Used to store parsed headers' values.
 */
union variant {
	char		*v_str;
	int		v_int;
	big_int_t	v_big_int;
	time_t		v_time;
	void		(*v_func)(void);
	void		*v_void;
	struct vec	v_vec;
};


#define	URI_MAX		16384		/* Default max request size	*/
/*
 * This guy holds parsed HTTP headers
 */
struct headers {
	union variant	cl;		/* Content-Length:		*/
	union variant	ct;		/* Content-Type:		*/
	union variant	connection;	/* Connection:			*/
	union variant	ims;		/* If-Modified-Since:		*/
	union variant	user;		/* Remote user name		*/
	union variant	auth;		/* Authorization		*/
	union variant	useragent;	/* User-Agent:			*/
	union variant	referer;	/* Referer:			*/
	union variant	cookie;		/* Cookie:			*/
	union variant	location;	/* Location:			*/
	union variant	range;		/* Range:			*/
	union variant	status;		/* Status:			*/
	union variant	transenc;	/* Transfer-Encoding:		*/
};

struct cgi{
	int iscgi;
	struct vec bin;
	struct vec para;	
};

struct worker_ctl;
struct worker_opts{
	pthread_t th;			/*Ïß³ÌµÄIDºÅ*/
	int flags;				/*Ïß³Ì×´Ì¬*/
	pthread_mutex_t mutex;/*Ïß³ÌÈÎÎñ»¥³â*/

	struct worker_ctl *work;	/*±¾Ïß³ÌµÄ×Ü¿Ø½á¹¹*/
};

struct worker_conn ;
/*ÇëÇó½á¹¹*/
struct conn_request{
	struct vec	req;		/*ÇëÇóÏòÁ¿*/
	char *head;			/*ÇëÇóÍ·²¿\0'½áÎ²*/
	char *uri;			/*ÇëÇóURI,'\0'½áÎ²*/
	char rpath[URI_MAX];	/*ÇëÇóÎÄ¼şµÄÕæÊµµØÖ·\0'½áÎ²*/

	int 	method;			/*ÇëÇóÀàĞÍ*/

	/*HTTPµÄ°æ±¾ĞÅÏ¢*/
	unsigned long major;	/*Ö÷°æ±¾*/
	unsigned long minor;	/*¸±°æ±¾*/

	struct headers ch;	/*Í·²¿½á¹¹*/

	struct worker_conn *conn;	/*Á¬½Ó½á¹¹Ö¸Õë*/
	int err;
};

/* ÏìÓ¦½á¹¹ */
struct conn_response{
	struct vec	res;		/*ÏìÓ¦ÏòÁ¿*/
	time_t	birth_time;	/*½¨Á¢Ê±¼ä*/
	time_t	expire_time;/*³¬Ê±Ê±¼ä*/

	int		status;		/*ÏìÓ¦×´Ì¬Öµ*/
	int		cl;			/*ÏìÓ¦ÄÚÈİ³¤¶È*/

	int 		fd;			/*ÇëÇóÎÄ¼şÃèÊö·û*/
	struct stat fsate;		/*ÇëÇóÎÄ¼ş×´Ì¬*/

	struct worker_conn *conn;	/*Á¬½Ó½á¹¹Ö¸Õë*/	
};
struct worker_conn 
{
#define K 1024
	char		dreq[16*K];	/*ÇëÇó»º³åÇø*/
	char		dres[16*K];	/*ÏìÓ¦»º³åÇø*/

	int		cs;			/*¿Í»§¶ËÌ×½Ó×ÖÎÄ¼şÃèÊö·û*/
	int		to;			/*¿Í»§¶ËÎŞÏìÓ¦Ê±¼ä³¬Ê±ÍË³öÊ±¼ä*/

	struct conn_response con_res;
	struct conn_request con_req;

	struct worker_ctl *work;	/*±¾Ïß³ÌµÄ×Ü¿Ø½á¹¹*/
};

struct worker_ctl{
	struct worker_opts opts;
	struct worker_conn conn;
};
struct mine_type{
	char	*extension;
	int 			type;
	int			ext_len;
	char	*mime_type;
};
void Para_Init(int argc, char *argv[]);

int Request_Parse(struct worker_ctl *wctl);
int Request_Handle(struct worker_ctl* wctl);


int Worker_ScheduleRun();
int Worker_ScheduleStop();
void Method_Do(struct worker_ctl *wctl);
void uri_parse(char *src, int len);

struct mine_type* Mine_Type(char *uri, int len, struct worker_ctl *wctl);



#define DBGPRINT printf


#endif /*__SHTTPD_H__*/

