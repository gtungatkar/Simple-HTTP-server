/*
 * File Name : http_config.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 14-01-2011
 * Description :
 *
 */
/*Read http server configuration from the config file and store it in memory
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "tokenize.h"
#include "httpconf.h"
#include "fileparser.h"

int cfg_reader(void *c, char *line)
{
        assert(c);
        assert(line);
        char token[MAX_TOKEN_SIZE];
        int next_token = DEFAULT; 
        struct http_server_config *cfg = (struct http_server_config *)c;
        int indx = 0;
        int config_cnt = 0;
        /* From the given config file format, we assume we have 4 distinct
         * config objects - port, DocumentRoot, DirectoryIndex, supported files
         * and all are mandatory. If one of them is missing, we should give an
         * error and return.
         * This function gets one line of the config file at a time, which is
         * further parses.
         */
        
        while(tokenize(&line, token) > 0)
        {
                if(token[0] == '#')
                {
                        //This line must be a Comment. Ignore and return
                        return 0; 
                }
                if(strcmp(token, "DocumentRoot") == 0)
                {
                        next_token = DOCUMENT_ROOT;
                        config_cnt++;

                
                }
                else if(strcmp(token, "Listen") == 0)
                {
                        next_token = LISTEN_PORT;
                        config_cnt++;
                
                }
                else if(strcmp(token, "DirectoryIndex") == 0)
                {
                        next_token = DIR_INDEX;
                        config_cnt++;
                }
                else //list of valid files
                {
                        int len;
                        char *p;
                        if(token[0] == '.')
                        {
                                next_token = FILE_TYPE;
                        }
                        len = strlen(token);
                        /* next_token was set previously based on the type of
                         * config object. Based on that type, we now store its
                         * value 
                         */
                        switch(next_token)
                        {
                                case DOCUMENT_ROOT:
                                        if(len > sizeof(cfg->document_root))
                                        {
                                                LOG(stdout, 
                                                  "config:DocumentRoot size exceeded\n");
                                                return -1;
                                        
                                        }
                                        strcpy(cfg->document_root, token);
                                        config_cnt++;
                                        next_token = DEFAULT;
                                        break;
                                case LISTEN_PORT:
                                        cfg->listen_port = strtol(token, &p, 10);
                                        if(cfg->listen_port > 65535)
                                        {
                                                LOG(stdout, 
                                                   "Port value invalid\n");
                                                return -1;
                                        }
                                        config_cnt++;
                                        next_token = DEFAULT;
                                        break;
                                case DIR_INDEX:
                                        if(len >
                                            sizeof(cfg->dir_index[0].filename))
                                        {
                                                //LOG
                                                LOG(stdout, 
                                                  "config:DirectoryIndex size exceeded\n");
                                                return -1;
                                        }
                                        strcpy(cfg->dir_index[indx++].filename,
                                                        token);
                                        config_cnt++;
                                        next_token = DIR_INDEX;
                                        break;
                                case FILE_TYPE:
                                        if(len >
                                                 sizeof(cfg->filetypes[0].extension))
                                        {
                                                
                                                LOG(stdout, 
                                                  "config:File type size exceeded\n");
                                                return -1;
                                        
                                        }
                                        strcpy(cfg->filetypes[cfg->f_type_cnt].extension,token);
                                        next_token = FILE_TYPE_EXT;
                                        break;
                                case FILE_TYPE_EXT:
                                        if(len >
                                                 sizeof(cfg->filetypes[0].type))
                                        {
                                                //LOG
                                                LOG(stdout, 
                                                  "config:Extension size exceeded\n");
                                                return -1;
                                        
                                        }
                                        strcpy(cfg->filetypes[cfg->f_type_cnt].type,token);
                                        cfg->f_type_cnt++;
                                        next_token = DEFAULT;
                                        break;
                                default:
                                        LOG(stdout, 
                                            "Error in config file.Exiting...\n");
                                        return -1;
                                                
                        }
                        
                }

        }
        /* config_cnt counts how many config types or values we have seen. 
         * if it is 1, it means we just got the type and not the value.*/
        if(config_cnt == 1)
        {
                LOG(stdout, "Error in config file\n");
                return -1;
        
        }
        return 0;

}
