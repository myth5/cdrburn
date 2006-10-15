
/* libdax_audioxtr
   Audio track data extraction facility of libdax and libburn.
   Copyright (C) 2006 Thomas Schmitt <scdbackup@gmx.net>, provided under GPL
*/

#ifndef LIBDAX_AUDIOXTR_H_INCLUDED
#define LIBDAX_AUDIOXTR_H_INCLUDED 1

                            /* Public Macros */

#define LIBDAX_AUDIOXTR_STRLEN 4096


                          /* Public Opaque Handles */

/** Extractor object encapsulating intermediate states of extraction.
    The clients of libdax_audioxtr shall only allocate pointers to this
    struct and get a storage object via libdax_audioxtr_new().
    Appropriate initial value for the pointer is NULL.
*/
struct libdax_audioxtr;


                            /* Public Functions */

               /* Calls initiated from inside libdax/libburn */


     /* Calls from applications (to be forwarded by libdax/libburn) */


/** Open an audio file, check wether suitable, create extractor object.
    @param xtr Opaque handle to extractor. Gets attached extractor object.
    @param path Address of the audio file to extract. "-" is stdin (but might
                be not suitable for all futurely supported formats).
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return >0 success, <=0 failure
*/
int libdax_audioxtr_new(struct libdax_audioxtr **xtr, char *path, int flag);


/** Obtain identification parameters of opened audio source.
    @param xtr Opaque handle to extractor
    @param fmt Gets pointed to the audio file format id text: ".wav"
    @param fmt_info Gets pointed to a format info text telling parameters
    @param num_channels     e.g. 1=mono, 2=stereo, etc
    @param sample_rate      e.g. 11025, 44100 
    @param bits_per_sample  e.g. 8= 8 bits per sample, 16= 16 bits ...
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return >0 success, <=0 failure
*/
int libdax_audioxtr_get_id(struct libdax_audioxtr *xtr,
                           char **fmt, char **fmt_info,
                           int *num_channels, int *sample_rate,
                           int *bits_per_sample, int flag);


/** Obtain next buffer full of extracted data in desired format (only raw audio
    for now).
    @param xtr Opaque handle to extractor
    @param buffer Gets filled with extracted data
    @param buffer_size Maximum number of bytes to be filled into buffer
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return >0 number of valid buffer bytes,
             0 End of file
            -1 operating system reports error
            -2 usage error by application
*/
int libdax_audioxtr_read(struct libdax_audioxtr *xtr,
                         char buffer[], int buffer_size, int flag);


/** Clean up after extraction and destroy extractor object.
    @param xtr Opaque handle to extractor, *xtr is allowed to be NULL,
               *xtr is set to NULL by this function
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 = destroyed object, 0 = was already destroyed
*/
int libdax_audioxtr_destroy(struct libdax_audioxtr **xtr, int flag);



#ifdef LIDBAX_AUDIOXTR________________


-- place documentation text here ---


#endif /* LIDBAX_AUDIOXTR_________________ */



/*
  *Never* set this macro outside libdax_audioxtr.c !
  The entrails of this facility are not to be seen by
  the other library components or the applications.
*/
#ifdef LIBDAX_AUDIOXTR_H_INTERNAL

                       /* Internal Structures */

/** Extractor object encapsulating intermediate states of extraction */
struct libdax_audioxtr {

 /* Source of the encoded audio data */
 char path[LIBDAX_AUDIOXTR_STRLEN];

 /* File descriptor to path */
 int fd;

 /* Format identifier. E.g. ".wav" */
 char fmt[80];

 /* Format parameter info text */
 char fmt_info[LIBDAX_AUDIOXTR_STRLEN];


 /* Format dependent parameters */

 /* MS WAVE Format */
 /* info used: http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

 /* 1= mono, 2= stereo, etc. */
 int wav_num_channels;

 /* 8000, 44100, etc. */
 int wav_sample_rate;

 /* 8 bits = 8, 16 bits = 16, etc. */
 int wav_bits_per_sample;

};


                             /* Internal Functions */

/** Open the audio source pointed to by .path and evaluate suitability.
    @return -1 failure to open, 0 unsuitable format, 1 success
*/
static int libdax_audioxtr_open(struct libdax_audioxtr *o, int flag);


/** Identify format and evaluate suitability.
    @return 0 unsuitable format, 1 format is suitable
*/
static int libdax_audioxtr_identify(struct libdax_audioxtr *o, int flag);


/** Convert a byte string into a number (currently only little endian)
    @return The resulting number
*/
static unsigned libdax_audioxtr_to_int(struct libdax_audioxtr *o,
                                      unsigned char *bytes, int len, int flag);


/** Prepare for reading of first buffer.
    @return 0 error, 1 success
*/
static int libdax_audioxtr_init_reading(struct libdax_audioxtr *o, int flag);



#endif /* LIBDAX_AUDIOXTR_H_INTERNAL */


#endif /* ! LIBDAX_AUDIOXTR_H_INCLUDED */

