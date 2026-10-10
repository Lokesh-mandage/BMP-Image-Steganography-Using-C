#ifndef DECODE_H
#define DECODE_H

#include "types.h" // constants user defined types

// struct to store info req

typedef struct decodeInfo
{
    //stego Image info
    char *stego_image_fname;
    FILE *fptr_stego_image;
    uint extn_size;
    char extn[5];
    uint file_size;

    //Output file info
    char output_fname[20];
    FILE *fptr_output_file;

} DecodeInfo;


/* Read and validate Decode args from argv */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* Perform the decoding */
Status do_decoding(DecodeInfo *decInfo);

/* Get File pointers for i/p and o/p files */
Status open_files_decode(DecodeInfo *decInfo);

/*Skip imp image header*/
Status skip_bmp_header(FILE *fptr_stego_image);

/* Store Magic String */
Status decode_magic_string( char *magic_string, DecodeInfo *decInfo);

/* Decode secret file extenstion */
Status decode_output_file_extn_size(DecodeInfo *decInfo);

/*decode output file extension*/
Status decode_output_file_extn(DecodeInfo *decInfo);

/*decode output file data*/
Status decode_output_file_size(DecodeInfo *decInfo);

/*decode secret file data*/
Status decode_secret_file_data(DecodeInfo *decInfo);

/*decode function, which does the real decoding*/
Status decode_image_to_data(char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image);

/*decode a byte into LSB of image data array*/
Status decode_byte_to_lsb(char *ch, char *image_buffer);

/*decode a size into LSB of image data array*/
Status decode_size_to_lsb(DecodeInfo *decInfo, char *size);


#endif

