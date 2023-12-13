#include "dlog.h"

#define DLOG_SIZE_LINE  0
#define DLOG_COUNT_LINE 1
#define DLOG_TAIL_LINE  2
#define DLOG_HEAD_LINE  3

#define DLOG_FIRST_ENTRY_LINE 5

#define DLOG_FULL_LINE_SIZE (DLOG_LINE_MAX_SIZE + 1)
#ifdef _____fpos_t_defined
#define UPDATE_QUEUE_POS(pos, index)  pos.__pos = ptr->log_begin_pos.__pos + ( index * DLOG_FULL_LINE_SIZE );
#else
#define UPDATE_QUEUE_POS(pos, index)  pos = ptr->log_begin_pos + ( index * DLOG_FULL_LINE_SIZE );
#endif    

const unsigned char DLOG_OPT_DEFAULT_ON = DLOG_OPT_AUTO_CLR | DLOG_OPT_OVERWRITE;

static char string_terminator = ';';
static char alignment_character = '.';

static int open_log_file(dlog_t* ptr);
static int create_log_file(dlog_t* ptr);
static int read_header(dlog_t* ptr);
static int update_header(dlog_t* ptr);
static int seek_position(FILE* pfile, unsigned int line, fpos_t* pos);

int dlog_open(dlog_t* ptr, char* filename, unsigned int size)
{
    if(ptr == NULL)
        return DLOG_NULL_PTR;

    memset(ptr, 0, sizeof(dlog_t));
    ptr->qcount = 0;
    ptr->qtail = 0;
    ptr->qhead = 0;
    ptr->qsize = size;
    dlog_set_opt(ptr, DLOG_OPT_DEFAULT_ON, 1);

    if(size == 0)
        return DLOG_BAD_PARAM;

    if(filename != NULL) 
    {
        if(strlen(filename) <= DLOG_NAME_MAX_SIZE) 
        {
            snprintf(ptr->filename, DLOG_NAME_MAX_SIZE, "%s", filename);
        }
        else
        {
            return DLOG_BAD_PARAM;
        }
    }
    else
    {
        snprintf(ptr->filename, DLOG_NAME_MAX_SIZE, "dlog.log");
    }

    if( !open_log_file(ptr) ) 
    {
        if( !create_log_file(ptr) )
            return DLOG_FILESYSTEM_ERR;
    }

    //initialize all file positions to 0
    fgetpos(ptr->file_ptr, &ptr->tail_pos);
    fgetpos(ptr->file_ptr, &ptr->head_pos);
    fgetpos(ptr->file_ptr, &ptr->header_size_pos);
    fgetpos(ptr->file_ptr, &ptr->header_count_pos);
    fgetpos(ptr->file_ptr, &ptr->header_tail_pos);
    fgetpos(ptr->file_ptr, &ptr->header_head_pos);

    //set header positions
    seek_position(ptr->file_ptr, DLOG_SIZE_LINE, &ptr->header_size_pos);
    seek_position(ptr->file_ptr, DLOG_COUNT_LINE, &ptr->header_count_pos);
    seek_position(ptr->file_ptr, DLOG_TAIL_LINE, &ptr->header_tail_pos);
    seek_position(ptr->file_ptr, DLOG_HEAD_LINE, &ptr->header_head_pos);

    if( !read_header(ptr) )
        return DLOG_INTERNAL_ERR;

    //set the log begin position
    seek_position(ptr->file_ptr, DLOG_FIRST_ENTRY_LINE, &ptr->log_begin_pos);

    //set the tail and head position
    UPDATE_QUEUE_POS(ptr->tail_pos, ptr->qtail);
    UPDATE_QUEUE_POS(ptr->head_pos, ptr->qhead);


    return DLOG_OK;
}

int dlog_set_opt(dlog_t* ptr, unsigned char opt, unsigned char value)
{
    if( ptr == NULL)
        return DLOG_NULL_PTR;

    if ( opt & DLOG_OPT_AUTO_CLR )  { ptr->en_auto_clr   =  value; }
    if ( opt & DLOG_OPT_OVERWRITE ) { ptr->en_overwrite  =  value; }
    if ( opt & DLOG_OPT_CRC )       { ptr->en_crc        =  value; }

    return DLOG_OK;
}

int read_header(dlog_t* ptr)
{

    fsetpos(ptr->file_ptr, &ptr->header_size_pos);
	if (fscanf(ptr->file_ptr, "size: %010u\n", &ptr->qsize) < 0)
		return 0;

    fsetpos(ptr->file_ptr, &ptr->header_count_pos);
	if (fscanf(ptr->file_ptr, "count: %010u\n", &ptr->qcount) < 0)
		return 0;

    if( ptr->qcount > ptr->qsize )
        ptr->qcount = 0;

    fsetpos(ptr->file_ptr, &ptr->header_tail_pos);
	if (fscanf(ptr->file_ptr, "tail: %010u\n", &ptr->qtail) < 0)
		return 0;

    if( ptr->qtail > ptr->qsize )
        ptr->qtail = 0;
    
    fsetpos(ptr->file_ptr, &ptr->header_head_pos);
	if (fscanf(ptr->file_ptr, "head: %010u\n", &ptr->qhead) < 0)
		return 0;

    if( ptr->qhead > ptr->qsize )
        ptr->qhead = 0;

    return 1;
}

int update_header(dlog_t* ptr)
{
	rewind(ptr->file_ptr);

    fsetpos(ptr->file_ptr, &ptr->header_size_pos);
	fprintf(ptr->file_ptr, "size: %010u\n", ptr->qsize);

    fsetpos(ptr->file_ptr, &ptr->header_count_pos);
	fprintf(ptr->file_ptr, "count: %010u\n", ptr->qcount);

    fsetpos(ptr->file_ptr, &ptr->header_tail_pos);
	fprintf(ptr->file_ptr, "tail: %010u\n", ptr->qtail);

    fsetpos(ptr->file_ptr, &ptr->header_head_pos);
	fprintf(ptr->file_ptr, "head: %010u\n", ptr->qhead);

    return 1;
}

int open_log_file(dlog_t* ptr)
{
    //check if file exists
    ptr->file_ptr = fopen(ptr->filename, "r+");

    if ( ptr->file_ptr == NULL )
        return 0;

    //now lets check if it is empty
    rewind(ptr->file_ptr);
    fseek(ptr->file_ptr, 0L, SEEK_END);   

    if( ftell(ptr->file_ptr) == 0 ) 
    {
        //its empty, close and return error
        fclose(ptr->file_ptr);
        return 0;
    }

    //there is something in it, hopefully is not corrupted data
    return 1;
}


int create_log_file(dlog_t* ptr)
{
    //try to create a file
    ptr->file_ptr = fopen(ptr->filename, "w+");
    if ( ptr->file_ptr == NULL )
        return 0;

    // fill the header block
	fprintf(ptr->file_ptr, "size: %010u\n", ptr->qsize);
	fprintf(ptr->file_ptr, "count: %010u\n", ptr->qcount);
	fprintf(ptr->file_ptr, "tail: %010u\n", ptr->qtail);
	fprintf(ptr->file_ptr, "head: %010u\n", ptr->qhead);
	fprintf(ptr->file_ptr, "########### LOG BEGIN #############\n");

    // fill the log file with empty messages for alignment
    for(int j=0; j < ptr->qsize; j++) 
    {        
        for(int i=0; i < DLOG_LINE_MAX_SIZE; i++) 
            fputc(alignment_character, ptr->file_ptr);

        fputc('\n', ptr->file_ptr);     
    }
    return 1;
}

/**
 * @brief Find the file position according to the line. Each line is a string
 * followed by a newline character, so we must loop through the file counting
 * how many newline characters we find. This is very slow, so we should not overuse it.
*/
int seek_position(FILE* pfile, unsigned int line, fpos_t* pos)
{
    char c;
    rewind(pfile);

    for(int i=0; i < line; i++) {
        while ((c = fgetc(pfile)) != '\n'){;}
    }

    fgetpos(pfile, pos);
    return 1;
}

int dlog_get(dlog_t* ptr, char* msg, int size)
{
    if(ptr == NULL)
        return DLOG_NULL_PTR;

    if(ptr->qcount == 0)
        return DLOG_EMPTY_QUEUE;

    //set file to head position
    fsetpos(ptr->file_ptr , &ptr->head_pos);

    for(int i = 0; i < size; i++)
    {
        int c = fgetc(ptr->file_ptr);
        if( c == string_terminator ){
            msg[i] = '\0';
            break;
        }
        msg[i] = c;
    }

    ptr->qhead = (ptr->qhead + 1) % ptr->qsize;
    UPDATE_QUEUE_POS(ptr->head_pos, ptr->qhead);

    ptr->qcount--;

    if( !update_header(ptr) )   
        return DLOG_INTERNAL_ERR;

    return DLOG_OK;
}

int dlog_put(dlog_t* ptr, char* msg)
{
    unsigned char full = 0;

    if( ptr == NULL )
        return DLOG_NULL_PTR;

    if(ptr->qcount >= ptr->qsize)
        full = 1;

    if( !ptr->en_overwrite && full )
        return DLOG_FULL_QUEUE;

    size_t size = strlen(msg);

    if( size >= DLOG_LINE_MAX_SIZE )
        return DLOG_MSG_SIZE_ERR;

    // set file position to tail
    fsetpos(ptr->file_ptr , &ptr->tail_pos);

    // insert new message
    fputs(msg, ptr->file_ptr);
    fputc(string_terminator, ptr->file_ptr);

    // if we overrun the head index we must update it to keep the FIFO behaviour
    if ( ( ptr->qtail == ptr->qhead ) && full )
    {
        ptr->qhead = (ptr->qhead + 1) % ptr->qsize;
        UPDATE_QUEUE_POS(ptr->head_pos, ptr->qhead);                
    }

    if ( ptr->en_auto_clr )
    {
        for(int i=size+1; i < DLOG_LINE_MAX_SIZE; i++)
        {
            fputc(alignment_character, ptr->file_ptr);
        }
    }

    // update tail
    ptr->qtail = (ptr->qtail + 1) % ptr->qsize;
    UPDATE_QUEUE_POS(ptr->tail_pos, ptr->qtail);
 
    if( ptr->qcount < ptr->qsize )
        ptr->qcount++;

    if( !update_header(ptr) )   
        return DLOG_INTERNAL_ERR;

    fflush(ptr->file_ptr);

    return DLOG_OK;
}

int dlog_close(dlog_t* ptr)
{
    if( ptr == NULL )
        return DLOG_NULL_PTR;

    fclose(ptr->file_ptr);
    memset(ptr, 0, sizeof(dlog_t));

    return DLOG_OK;
}

int dlog_write_test(dlog_t* ptr, unsigned int n)
{
    char msg[DLOG_LINE_MAX_SIZE] = "";

    if( ptr == NULL )
        return DLOG_NULL_PTR;

    for(int i=1; i <= n; i++)
    {
        snprintf(msg, sizeof(msg), "Test message%d", i);

        if ( !dlog_put(ptr, msg) ){
            printf("dlog_put FAILED \n");
            return 0;
        }
    }

    return 1;
}

int dlog_read_test(dlog_t* ptr, unsigned int n)
{
    char readbuf[DLOG_LINE_MAX_SIZE] = "";
    int ret = DLOG_OK;

    if( ptr == NULL )
        return DLOG_NULL_PTR;

    if(n == 0) n = 1;

    do
    {
        n--;
        if(!n) { break; }

        ret = dlog_get(ptr, readbuf, sizeof(readbuf));                
        if ( ret == DLOG_OK )
        {
            printf("%s\n", readbuf);
            usleep(1000);              
        }
        else if (ret == DLOG_EMPTY_QUEUE)
        {
            printf("DLOG is empty\n");
        }
        else
        {
            printf("dlog_get error %d\n", ret);
        }        

    } while( ret == DLOG_OK );

    return 1;
}