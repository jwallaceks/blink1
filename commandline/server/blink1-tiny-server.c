/*
 *
 * blink1-tiny-server -- a small cross-platform REST/JSON server for
 *                       controlling a blink(1) device
 *
 *
 * Supported URLs:
 *
 *	PRIMARY STATUS LIGHT
 *  localhost:8080/blink1/1/on
 *  localhost:8080/blink1/1/off
 *  localhost:8080/blink1/1/fadeToRGB?rgb=%23ff00ff&time=1.0
 *  localhost:8080/blink1/1/setToRGB?rgb=%23ff00ff
 *  localhost:8080/blink1/1/toggle
 *  localhost:8080/blink1/1/here
 *  localhost:8080/blink1/1/out
 *  localhost:8080/blink1/1/lunch
 *  localhost:8080/blink1/1/meeting
 *  localhost:8080/blink1/1/remote
 *  localhost:8080/blink1/1/busy
 *  localhost:8080/blink1/1/status
 *
 *	Status codes:
 * 		here=1
 * 		out=0
 * 		meeting=2
 * 		lunch=3
 * 		busy=4
 * 		remote=5
 */

#include "mongoose.h"

#include "blink1-lib.h"

const char* blink1_server_version = "0.99";

static const char *s_http_port = "8080";
static struct mg_serve_http_opts s_http_server_opts;

static const char *serial_1 = "20002C01";
int status;
	
// parse a comma-delimited string containing numbers (dec,hex) into a byte arr
// FIXME: copy of same func in blink1-tool.c
static int  hexread(uint8_t *buffer, char *string, int buflen)
{
    char    *s;
    int     pos = 0;

    memset(buffer,0,buflen);  // bzero() not defined on Win32?
    while((s = strtok(string, ", ")) != NULL && pos < buflen){
        string = NULL;
        buffer[pos++] = (char)strtol(s, NULL, 0);
    }
    return pos;
}

// given a string of hex color code ("#FF3322") or rgb triple ("255,0,0" or
// "0xff,0x23,0x00"), produce a parsed byte array
static void parse_rgbstr(uint8_t* rgb, char* rgbstr)
{
    if( rgbstr != NULL && strlen(rgbstr) ) {
        if( rgbstr[0] == '#' ) {
            uint32_t rgbval = strtoul(rgbstr+1,NULL,16); // FIXME: hack
            rgb[0] = ((rgbval >> 16) & 0xff);
            rgb[1] = ((rgbval >>  8) & 0xff);
            rgb[2] = ((rgbval >>  0) & 0xff);
        }
        else {
            hexread(rgb, rgbstr, 3);
        }
    }
}

// used in ev_handler below
#define do_blink1_color(selected) \
    blink1_device* dev = blink1_open(); \
    if ( selected == 1) { \
	blink1_device* dev = blink1_openBySerial(serial_1); \
    } \
    if( blink1_fadeToRGB( dev, millis, r,g,b ) == -1 ) { \
        fprintf(stderr, "off: blink1 device error\n"); \
        sprintf(result, "%s; couldn't find blink1", result); \
    } \
    else { \
        sprintf(result, "blink1 set color #%2.2x%2.2x%2.2x", r,g,b);  \
    } \
    blink1_close(dev); 


static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;

    if( ev != MG_EV_HTTP_REQUEST ) {
        return;
    }

    char result[1000];  result[0] = 0;
    char uristr[1000];
    char tmpstr[1000];
    int rc;
    uint16_t millis = 100;
    uint8_t rgb[3] = {0,0,0};
    uint8_t r = rgb[0], g = rgb[1], b = rgb[2];
    uint8_t count = 1;

    struct mg_str* uri = &hm->uri;
    struct mg_str* querystr = &hm->query_string;

    snprintf(uristr, uri->len+1, "%s", uri->p);

    if( mg_get_http_var(querystr, "millis", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "time", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = 1000 * strtof(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "rgb", tmpstr, sizeof(tmpstr)) > 0 ) {
        parse_rgbstr( rgb, tmpstr);
        r = rgb[0]; g = rgb[1]; b = rgb[2];
    }
    if( mg_get_http_var(querystr, "count", tmpstr, sizeof(tmpstr)) > 0 ) {
        count = strtod(tmpstr,NULL);
    }

    if( mg_vcmp( uri, "/") == 0 ||
        mg_vcmp( uri, "/blink1") == 0 ||
        mg_vcmp( uri, "/blink1/") == 0  ) {
        sprintf(result, "welcome to blink1-tiny-server api server. All URIs start with '/blink1', e.g. '/blink1/red', '/blink1/off', '/blink1/fadeToRGB?rgb=%%23FF00FF'");
    }
    else if( mg_vcmp( uri, "/blink1/1/off") == 0 ) {
        sprintf(result, "blink1 off");
        r = 0; g = 0; b = 0;
        do_blink1_color(1);
    }
	else if( mg_vcmp( uri, "/blink1/1/setToRGB") == 0 ) {
		sprintf(result, "blink1 setToRGB");
		do_blink1_color(1);
	}
	else if( mg_vcmp( uri, "/blink1/1/toggle") == 0 ) {
        sprintf(result, "blink1 toggle");
		if (status == 1) {
			r = 255; g = 0; b = 0;
		}
		else {
			r = 0; g = 255; b = 0;
		}
			do_blink1_color(1);
	}
	
    else if( mg_vcmp( uri, "/blink1/1/on") == 0 ) {
        sprintf(result, "blink1 on");
        r = 255; g = 255; b = 255;
        do_blink1_color(1);
    }
    else if( mg_vcmp( uri, "/blink1/1/out") == 0 ) {
        sprintf(result, "blink1 out");
        r = 255; g = 0; b = 0;
		status = 0;
        do_blink1_color(1);
    }
    else if( mg_vcmp( uri, "/blink1/1/here") == 0 ) {
        sprintf(result, "blink1 here");
        r = 0; g = 255; b = 0;
		status = 1;
        do_blink1_color(1);
    }
    else if( mg_vcmp( uri, "/blink1/1/lunch") == 0 ) {
        sprintf(result, "blink1 lunch");
        r = 0; g = 0; b = 255;
		status = 3;
        do_blink1_color(1);
    }
	else if( mg_vcmp( uri, "/blink1/1/meeting") == 0 ) {
        sprintf(result, "blink1 meeting");
        r = 255; g = 255; b = 0;
		status = 2;
        do_blink1_color(1);
    }
	else if( mg_vcmp( uri, "/blink1/1/busy") == 0 ) {
        sprintf(result, "blink1 busy");
        r = 255; g = 0; b = 255;
		status = 4;
        do_blink1_color(1);
    }
	else if( mg_vcmp( uri, "/blink1/1/remote") == 0 ) {
        sprintf(result, "blink1 remote");
        r = 255; g = 140; b = 0;
		status = 5;
        do_blink1_color(1);
    }
	else if( mg_vcmp(uri, "/blink1/1/status") == 0) {
		switch(status){
			case 0:
				sprintf(result,"Out");
				break;
			case 1:
				sprintf(result,"Here");
				break;
			case 2:
				sprintf(result,"Meeting");
				break;
			case 3:
				sprintf(result,"Lunch");
				break;
			case 4:
				sprintf(result,"Busy");
				break;
			case 5:
				sprintf(result,"Remote");
				break;
		}
	}
	
    else if( mg_vcmp( uri, "/blink1/1/fadeToRGB") == 0 ) {
        sprintf(result, "blink1 fadeToRGB");
        do_blink1_color(1);
    }
    else if( mg_vcmp( uri, "/blink1/blink") == 0 ) {
        sprintf(result, "blink1 blink");
        if( r==0 && g==0 && b==0 ) { r = 255; g = 255; b = 255; }
        blink1_device* dev = blink1_open();
        for( int i=0; i<count; i++ ) {
            blink1_fadeToRGB( dev, millis/2, r,g,b );
            blink1_sleep( millis/2 ); // fixme
            blink1_fadeToRGB( dev, millis/2, 0,0,0 );
            blink1_sleep( millis/2 ); // fixme
        }
        blink1_close(dev);
    }
    else if( mg_vcmp( uri, "/blink1/random") == 0 ) {
        sprintf(result, "blink1 random");
        srand( time(NULL) * getpid() );
        blink1_device* dev = blink1_open();
        for( int i=0; i<count; i++ ) {
            r = rand() % 255;
            g = rand() % 255;
            b = rand() % 255 ;
            blink1_fadeToRGB( dev, millis/2, r,g,b );
            blink1_sleep( millis/2 ); // fixme
        }
        blink1_close(dev);
    }
    else {
        sprintf(result, "%s; unrecognized uri", result);
        //mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
    }

    if( result[0] != '\0' ) {
        sprintf(tmpstr, "#%2.2x%2.2x%2.2x", r,g,b );
        mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(nc,
                             "{\n"
                             "\"uri\":  \"%s\",\n"
                             "\"result\":  \"%s\",\n"
                             "\"millis\": \"%d\",\n"
                             "\"rgb\": \"%s\",\n"
                             "\"version\": \"%s\"\n"
                             "}\n",
                             uristr,
                             result,
                             millis,
                             tmpstr,
                             blink1_server_version
                             );
        mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
    }

}

int main(int argc, char *argv[]) {
    status = 0;
    struct mg_mgr mgr;
    struct mg_connection *nc;
    struct mg_bind_opts bind_opts;
    int i;
    char *cp;
    const char *err_str;

    mg_mgr_init(&mgr, NULL);

  /* Process command line options to customize HTTP server */
  for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
          s_http_port = argv[++i];
      }
  }

  /* Set HTTP server options */
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.error_string = &err_str;

    nc = mg_bind_opt(&mgr, s_http_port, ev_handler, bind_opts);
    if (nc == NULL) {
        fprintf(stderr, "Error starting server on port %s: %s\n", s_http_port,
                *bind_opts.error_string);
        exit(1);
    }

    mg_set_protocol_http_websocket(nc);

    s_http_server_opts.enable_directory_listing = "no";

    printf("blink1-server: running on port %s\n", s_http_port);

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return 0;
}
