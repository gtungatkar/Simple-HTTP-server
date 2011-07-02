#ifndef __FILE_PARSER_HDR__
#define __FILE_PARSER_HDR__
int file_parser(char *filename, int (*reader)(void *, char *line), void *c);
#define MAX_LINE 1024
#endif
