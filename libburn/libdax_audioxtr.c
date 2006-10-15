
/* libdax_audioxtr 
   Audio track data extraction facility of libdax and libburn.
   Copyright (C) 2006 Thomas Schmitt <scdbackup@gmx.net>, provided under GPL
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


#include "libdax_msgs.h"
extern struct libdax_msgs *libdax_messenger;


/* Only this single source module is entitled to do this */
#define LIBDAX_AUDIOXTR_H_INTERNAL 1

/* All clients of the extraction facility must do this */
#include "libdax_audioxtr.h"


int libdax_audioxtr_new(struct libdax_audioxtr **xtr, char *path, int flag)
{
 int ret;
 struct libdax_audioxtr *o;

 o= *xtr= (struct libdax_audioxtr *) malloc(sizeof(struct libdax_audioxtr));
 if(o==NULL)
   return(-1);
 strncpy(o->path,path,LIBDAX_AUDIOXTR_STRLEN-1);
 o->path[LIBDAX_AUDIOXTR_STRLEN]= 0;
 o->fd= -1;
 strcpy(o->fmt,"unidentified");
 o->fmt_info[0]= 0;

 o->wav_num_channels= 0;
 o->wav_sample_rate= 0;
 o->wav_bits_per_sample= 0;

 ret= libdax_audioxtr_open(o,0);
 if(ret<=0)
   goto failure;

 return(1);
failure:
 libdax_audioxtr_destroy(xtr,0);
 return(-1);
}


int libdax_audioxtr_destroy(struct libdax_audioxtr **xtr, int flag)
{
 struct libdax_audioxtr *o;

 o= *xtr;
 if(o==NULL)
   return(0);
 if(o->fd>=0 && strcmp(o->path,"-")!=0)
   close(o->fd);
 free((char *) o);
 *xtr= NULL;
 return(1);
}


static int libdax_audioxtr_open(struct libdax_audioxtr *o, int flag)
{
 int ret;
 char msg[LIBDAX_AUDIOXTR_STRLEN+80];
 
 if(strcmp(o->path,"-")==0)
   o->fd= 0;
 else
   o->fd= open(o->path, O_RDONLY);
 if(o->fd<0) {
   sprintf(msg,"Cannot open audio source file : %s",o->path);
   libdax_msgs_submit(libdax_messenger,-1,0x00020200,
                      LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
                      msg, errno, 0);
   return(-1);
 }
 ret= libdax_audioxtr_identify(o,0);
 if(ret<=0) {
   sprintf(msg,"Audio source file has unsuitable format : %s",o->path);
   libdax_msgs_submit(libdax_messenger,-1,0x00020201,
                      LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
                      msg, 0, 0);
   return(0);
 }
 ret= libdax_audioxtr_init_reading(o,0);
 if(ret<=0) {
   sprintf(msg,"Failed to prepare reading of audio data : %s",o->path);
   libdax_msgs_submit(libdax_messenger,-1,0x00020202,
                      LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
                      msg, 0, 0);
   return(0);
 }
 return(1);
}


static int libdax_audioxtr_identify(struct libdax_audioxtr *o, int flag)
{
 int ret;
 char buf[45];

 /* currently this only works for MS WAVE files .wav */
 /* info used: http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

 ret= read(o->fd, buf, 44);
 if(ret<44)
   return(0);
 buf[44]= 0; /* as stopper for any string operations */

 if(strncmp(buf,"RIFF",4)!=0)                                     /* ChunkID */
   return(0);
 if(strncmp(buf+8,"WAVE",4)!=0)                                    /* Format */ 
   return(0);
 if(strncmp(buf+12,"fmt ",4)!=0)                              /* Subchunk1ID */
   return(0);
 if(buf[16]!=16 || buf[17]!=0 || buf[18]!=0 || buf[19]!=0)  /* Subchunk1Size */
   return(0);
 if(buf[20]!=1 || buf[21]!=0)  /* AudioFormat must be 1 (Linear quantization) */
   return(0);

 strcpy(o->fmt,".wav");
 o->wav_num_channels=  libdax_audioxtr_to_int(o,(unsigned char *) buf+22,2,0);
 o->wav_sample_rate= libdax_audioxtr_to_int(o,(unsigned char *) buf+24,4,0);
 o->wav_bits_per_sample= libdax_audioxtr_to_int(o,(unsigned char *)buf+34,2,0);
 sprintf(o->fmt_info,
         ".wav , num_channels=%d , sample_rate=%d , bits_per_sample=%d",
         o->wav_num_channels,o->wav_sample_rate,o->wav_bits_per_sample);

 return(1);
}


static unsigned libdax_audioxtr_to_int(struct libdax_audioxtr *o,
                                      unsigned char *bytes, int len, int flag)
{
 unsigned int ret= 0;
 int i;

 for(i= len-1; i>=0; i--)
   ret= ret*256+bytes[i];
 return(ret);
}


static int libdax_audioxtr_init_reading(struct libdax_audioxtr *o, int flag)
{
 int ret;

 /* currently this only works for MS WAVE files .wav */;
 if(o->fd==0) /* stdin: hope no read came after libdax_audioxtr_identify() */
   return(1);

 ret= lseek(o->fd,44,SEEK_SET);
 if(ret==-1)
   return(0);

 return(1);
}


int libdax_audioxtr_get_id(struct libdax_audioxtr *o,
                     char **fmt, char **fmt_info,
                     int *num_channels, int *sample_rate, int *bits_per_sample,
                     int flag)
{
 *fmt= o->fmt;
 *fmt_info= o->fmt_info;
 if(strcmp(o->fmt,".wav")==0) {
   *num_channels= o->wav_num_channels;
   *sample_rate= o->wav_sample_rate;
   *bits_per_sample= o->wav_bits_per_sample;
 } else
   *num_channels= *sample_rate= *bits_per_sample= 0;
 return(1);
}


int libdax_audioxtr_read(struct libdax_audioxtr *o,
                         char buffer[], int buffer_size, int flag)
{
 int ret;

 if(buffer_size<=0 || o->fd<0)
   return(-2);
 ret= read(o->fd,buffer,buffer_size);
 return(ret);
}

