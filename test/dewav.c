
/* dewav
   Demo of libburn extension libdax_audioxtr
   Audio track data extraction facility of libdax and libburn.
   Copyright (C) 2006 Thomas Schmitt <scdbackup@gmx.net>, provided under GPL
*/

#include <stdio.h> 
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


/* The API of libburn */
#include "../libburn/libburn.h"

/* The API extension for .wav extraction */
#include "../libburn/libdax_audioxtr.h"


int main(int argc, char **argv)
{
 /* This program acts as filter from in_path to out_path */
 char *in_path= "", *out_path= "-";

 /* The read-and-extract object for use with in_path */
 struct libdax_audioxtr *xtr= NULL;

 /* Default output is stdout */
 int out_fd= 1;

 /* Inquired source parameters */
 char *fmt, *fmt_info;
 int num_channels, sample_rate, bits_per_sample;

 /* Auxiliary variables */
 int ret, i, be_strict= 1, buf_count;
 char buf[2048];

 if(argc < 2)
   goto help;
 for(i= 1; i<argc; i++) {
   if(strcmp(argv[i],"-o")==0) {
     if(i>=argc-1) {
       fprintf(stderr,"%s: option -o needs a file address as argument.\n",
               argv[0]);
       exit(1);
     }
     i++;
     out_path= argv[i];
   } else if(strcmp(argv[i],"--lax")==0) {
     be_strict= 0;
   } else if(strcmp(argv[i],"--strict")==0) {
     be_strict= 1;
   } else if(strcmp(argv[i],"--help")==0) {
help:;
     printf(
     "usage: %s [-o output_path|\"-\"] [--lax|--strict] [source_path|\"-\"]\n",
            argv[0]);
     exit(0);
   } else {
     if(in_path[0]!=0) {
       fprintf(stderr,"%s: only one input file is allowed.\n", argv[0]);
       exit(2);
     }
     in_path= argv[i];
   }
 }
 if(in_path[0] == 0)
   in_path= "-";


 /* Initialize libburn and set up messaging system */
 if(burn_initialize() == 0) {
   fprintf(stderr,"Failed to initialize libburn.\n");
   exit(3);
 }
 /* Print messages of severity NOTE or more directly to stderr */
 burn_msgs_set_severities("NEVER", "NOTE", "");


 /* Open audio source and create extractor object */
 ret= libdax_audioxtr_new(&xtr, in_path, 0);
 if(ret<=0)
   exit(4);
 if(strcmp(out_path,"-")!=0) {
   out_fd= open(out_path, O_WRONLY|O_CREAT|O_TRUNC);
   if(out_fd == -1) {
     fprintf(stderr, "Cannot open file: %s\n", out_path);
     fprintf(stderr, "Error reported: '%s'  (%d)\n",strerror(errno), errno);
     exit(5);
   }
 }
 /* Obtain and print parameters of audio source */
 libdax_audioxtr_get_id(xtr, &fmt, &fmt_info,
                        &num_channels, &sample_rate, &bits_per_sample, 0);
 fprintf(stderr, "Detected format: %s\n", fmt_info);
 if((num_channels!=2 && num_channels!=4) || 
    sample_rate!=44100 || bits_per_sample!=16) {
   fprintf(stderr,
         "%sAudio source parameters do not comply to cdrskin/README specs\n",
         (be_strict ? "" : "WARNING: "));
   if(be_strict)
     exit(6);
 }


 /* Extract and put out raw audio data */;
 while(1) {

   buf_count= libdax_audioxtr_read(xtr, buf, sizeof(buf), 0);
   if(buf_count < 0)
     exit(7);
   if(buf_count == 0)
 break;

   ret= write(out_fd, buf, buf_count);
   if(ret == -1) {
     fprintf(stderr, "Failed to write buffer of %d bytes to: %s\n",
                     buf_count, out_path);
     fprintf(stderr, "Error reported: '%s'  (%d)\n", strerror(errno), errno);
     exit(5);
   }

 }

 /* Shutdown */
 if(out_fd>2)
   close(out_fd);
 libdax_audioxtr_destroy(&xtr, 0);
 burn_finish();
 exit(0);
} 
