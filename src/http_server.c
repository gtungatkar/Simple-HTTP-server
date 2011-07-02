/*
 * File Name : http_server.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 17-01-2011
 * Description :
 *
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include "httpconf.h"
#include "tokenize.h"
#include "fileparser.h"
#define BUFFSIZE (4*1024)
#define MAX_FILENAME 512
#define HTTP_HDR_SIZE 512
#define HTTP_URI_SIZE 1024
#define HTTP_STATUS_SIZE 1024
#define HTTP_GET 11
#define SP 0x20 
#define CRLF "\r\n"
void http_server(struct http_server_config *cfg, int sockfd)
{

        char request[BUFFSIZE+1];
        int numbytes;
        int errflag = 0;
        char request_method[5];
        char version[10];
        int method;
        int ftype;;
        int blksize = 0;
        int fd;
        char uri[HTTP_URI_SIZE];
        char filepath[HTTP_URI_SIZE];
        char status[HTTP_STATUS_SIZE];
        char header[HTTP_HDR_SIZE], buffer[BUFFSIZE];
        if((numbytes = read(sockfd, (void *)request, BUFFSIZE)) <= 0)
        {
                LOG(stdout, "read from socket failed");
                return;
        }
        char *requestptr = request;
        if((method = valid_method_string(&requestptr, request_method)) == -1)
        {
                //ERROR in Request
                snprintf(status, 
                        HTTP_STATUS_SIZE,
                        "HTTP/1.0 400 Bad Request: Invalid Method: %s\r\n", 
                        request_method);
                LOG(stdout, status);
                errflag = 1;
        }
        if(!errflag && (method == HTTP_GET))
        {
                //tokenize URI
                //check that the method name and URI are separated by exactly 1
                //SP character
                //requestptr should now be pointing at a SP character. If the
                //next character is SP as well, invalid request
                if(valid_uri(&requestptr, cfg, uri) == -1)
                {
                        snprintf(status, 
                                HTTP_STATUS_SIZE,
                                "HTTP/1.0 400 Bad Request: Invalid URI: %s\r\n", 
                                uri);
                        LOG(stdout, status);
                        //ERROR in request
                        errflag = 1;
                
                }
                
        
        }
        if(!errflag)
        {
                if(valid_version(&requestptr, cfg, version) == -1)
                {
                        //ERROR
                        //HTTP/1.0 400 Bad Request: Invalid HTTP-Version:
                        //<requested HTTP version>
                        snprintf(status, 
                                HTTP_STATUS_SIZE,
                                "HTTP/1.0 400 Bad Request: Invalid HTTP-Version: %s\r\n",
                                version);
                        LOG(stdout, status);
                        errflag = 1;
                }

        }
        if(!errflag)
        {
                if((ftype = valid_filetype(&requestptr, cfg, uri)) == -1)
                {
                        //ERROR
                        snprintf(status, 
                                HTTP_STATUS_SIZE,
                                "HTTP/1.0 501 Not Implemented: %s\r\n",
                                 uri);
                        LOG(stdout, status);
                        errflag = 1;
                }

        }
        //seems like request came up fine! Now lets see if we can read the file
        if(!errflag)
        {
                /*all file paths relative to document root */
                strncat(filepath, cfg->document_root, HTTP_URI_SIZE);
                strncat(filepath, uri, HTTP_URI_SIZE);
                fd = open(filepath, O_RDONLY);
                if(fd == -1)
                {
                        snprintf(status,HTTP_STATUS_SIZE,
                                "HTTP/1.0 404 Not Found: %s\r\n", 
                                 uri);
                        LOG(stdout, status);
                        errflag = 1;
                        //file not found..
                
                }

        }
        if(!errflag)
        {
                int filelen;
                //find file size
                if((filelen = lseek(fd, 0, SEEK_END)) < 0)
                {
                        //error
                        LOG(stdout, "lseek() failed\n");
                }
                snprintf(status, HTTP_STATUS_SIZE, 
                                "HTTP/1.0 200 Document Follows\r\n");
                LOG(stdout, status);
                LOG(stdout, filepath);
                snprintf(header, HTTP_HDR_SIZE, 
                                "Content-Type: %s\r\nContent-Length: %d\r\n\n",
                                cfg->filetypes[ftype].type,filelen );
                lseek(fd, 0, SEEK_SET);
                /*send status and header */
                write(sockfd, status, strlen(status));
                write(sockfd, header, strlen(header));
                /*write the file contents to socket */
                while ( (blksize = read(fd, buffer, BUFFSIZE)) > 0 ) {
                        write(sockfd,buffer,blksize);
                }


        
        }
        else
        {
                /*Request had error. Send the appropriate status*/
                write(sockfd, status, strlen(status));
                LOG(stdout, "Error in processing request\n");

        }




        return;
}
int valid_method_string(char **request, char *request_method)
{
        /*only GET method supported */
        if((tokenize(request, request_method) != 3)
                        ||(strcmp(request_method, "GET") != 0))
        {
                LOG(stdout, "Invalid method\n");
                return -1;
        }
        else
        {
                return HTTP_GET;
        }

}
int valid_version(char **request, struct http_server_config *cfg, 
                char *version)
{
        /* HTTP versions 1.0 and 1.1 messages are accepted
         */
        if((tokenize(request, version) <= 0)
                        ||((strcmp(version, "HTTP/1.1") != 0) && (strcmp(version,
                                                "HTTP/1.0") != 0)))
        {
                LOG(stdout, "Version not supported\n");
                return -1;
        }
        else
        {
                return 0;
        }


}
int valid_uri(char **request, struct http_server_config *cfg, 
                char *uri)
{
        /*if it sees 2 or more leading spaces(SP) - thats invalid URI*/
        if(*(*(request)+1) == SP)
        {
                LOG(stdout, "Invalid URI\n");
                return -1;
        }
        
        if((tokenize(request, uri) <= 0))
        {
                LOG(stdout, "Invalid URI\n");
                return -1;
        }
        else
        {
                //cannot refer to the parent directory
                if(uri[0] == '.' && uri[1] == '.')
                {
                        LOG(stdout, "Invalid URI\n");
                        return -1;
                }
                //if just '/' , append the default index file name
                if((uri[0] == '/') && (uri[1] == '\0'))
                        strcat(uri,
                           cfg->dir_index[0].filename);
        }
        return 0;

}

int valid_filetype(char **request, struct http_server_config *cfg, 
                char *uri)
{
        int i = 0, validstr = -1;  
        int j;
        /* entry in the form of
         * .html text/html
         * So get the extension. */
        while(uri[i] != '.') 
        { 
                if(uri[i] != '\0')
                        i++;
                else 
                        return 0;

        }
        /*Check if this extension is present in supported extensions*/
        for(j = 0; j < cfg->f_type_cnt; j++)
        {
                if(!strcmp((uri+i), cfg->filetypes[j].extension))
                        validstr = j;
        }
        if(validstr < 0)
        {
                LOG(stdout, "Invalid filetype\n");
                return -1;
        }
        return validstr;

}
int main(int argc, char *argv[])
{
        struct http_server_config cfg ;
        memset(&cfg, 0, sizeof(cfg));
        char filename[MAX_FILENAME];
        if(argc == 2)
        {
                if(strlen(argv[1]) < MAX_FILENAME)
                        strcpy(filename, argv[1]);
        }
        else
                strcpy(filename, "config.txt");

        if(file_parser(filename, cfg_reader, &cfg)== -1)
        {
                LOG(stdout, "Configuration Error.Exiting...\n");
                exit(1);
        
        }
        /*Ignore child death and avoid zombies!*/
        signal(SIGCLD, SIG_IGN);
        LOG(stdout, 
           "Starting primitive HTTP web server implemented for CSC573\n");
        if(connection_handler(&cfg) == -1)
        {
                LOG(stdout, "Error!Exiting..\n");
                exit(1);
        }
        return 0;
}
