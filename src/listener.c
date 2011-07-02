/*
 * File Name : listener.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 16-01-2011
 * Description :
 *
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include "httpconf.h"


int connection_handler(struct http_server_config *cfg)
{
        int listen_fd, new_fd, set = 1;
        struct sockaddr_in listener_addr, client_addr;
        socklen_t addr_len = 0;
        pid_t pid;
        char p[10];
        assert(cfg != NULL);
        /* Standard server side socket sequence*/

        if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
                LOG(stdout, "socket() failure\n");
                return -1;
        }
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &set,
                                sizeof(int)) == -1) {
                LOG(stdout, "setsockopt() failed");
                return -1;
        }
        bzero(&listener_addr, sizeof(listener_addr));
        listener_addr.sin_family = AF_INET;
        listener_addr.sin_port = htons(cfg->listen_port);
        listener_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(listener_addr.sin_zero, '\0', sizeof(listener_addr.sin_zero));

        if(bind(listen_fd, (struct sockaddr*)&listener_addr, sizeof
                                listener_addr) == -1)
        {
                LOG(stdout, "bind() failed\n");
                return -1;
        
        }

        if(listen(listen_fd, PENDING_CONN) == -1)
        {
                LOG(stdout, "listen() failed\n");
                return -1;
        }
        sprintf(p, "HTTP server listening on port:%d\n", cfg->listen_port);
        LOG(stdout, p);
        while(1)
        {

                new_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
                if(new_fd == -1)
                {
                        //log
                        LOG(stdout, "accept() failed\n");
                        return -1;
                
                }
                LOG(stdout, "new connection accepted\n");
                //fork a new process to handle this request
                if((pid = fork()) == -1)
                {
                        //LOG Error
                        LOG(stdout, "Error in fork\n");

                }
                else if(pid == 0)
                {
                        /*This is the child process. This will service the
                         *request while parent goes back to listening.*/
                        LOG(stdout, "Servicing request\n");
                        http_server(cfg, new_fd);
                        return 0;
                }
                else
                {
                        close(new_fd);
                }


        }

        return 0;

}
