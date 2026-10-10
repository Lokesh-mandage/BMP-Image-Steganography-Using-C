#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv[2] != NULL && strcmp(strstr(argv[2], "."), ".bmp") == 0)
    {
        decInfo->stego_image_fname = argv[2];
    }
    else
    {
        printf("ERROR: Stego image file should be a .bmp file\n");
        return e_failure;
    }

    if (argv[3] != NULL)
    {
        strcpy(decInfo->output_fname, argv[3]);
    }
    else
    {
        printf("ERROR: Output file name not provided\n");
        return e_failure;
    }

    return e_success;
}

Status skip_bmp_header(FILE *fptr_stego_image)
{
    fseek(fptr_stego_image, 54, SEEK_SET);

    return e_success;
}

Status decode_byte_to_lsb(char *ch, char *image_buffer)
{
    *ch = 0;

    for (int i = 0; i < 8; i++)
    {
        *ch = (*ch << 1) | (image_buffer[i] & 1);
    }

    return e_success;
}

Status decode_image_to_data(char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char image_buffer[8];

    for (int i = 0; i < size; i++)
    {
        fread(image_buffer, 1, 8, fptr_stego_image);

        decode_byte_to_lsb(&data[i], image_buffer);
    }

    return e_success;
}

/* Decode and verify the magic string
 * The magic string was encoded first after the BMP header.
 * We know its size from MAGIC_STRING.
 */
Status decode_magic_string(char *magic_string, DecodeInfo *decInfo)
{
    // Temporary array to store the decoded magic string
    char decoded_magic_string[strlen(MAGIC_STRING) + 1];

    // Decode the magic string from the stego image
    if (decode_image_to_data(decoded_magic_string, strlen(MAGIC_STRING), NULL, decInfo->fptr_stego_image) == e_success)
    {
        // Add null character at the end
        decoded_magic_string[strlen(MAGIC_STRING)] = '\0';

        // Compare decoded string with original MAGIC_STRING
        if (strcmp(decoded_magic_string, MAGIC_STRING) == 0)
        {
            printf("Magic String decoded successfully\n");

            // Copy decoded magic string to output variable
            strcpy(magic_string, decoded_magic_string);

            return e_success;
        }
        else
        {
            printf("ERROR: Magic String mismatch\n");
            return e_failure;
        }
    }

    return e_failure;
}

/* Decode the size of the secret file extension*/
Status decode_output_file_extn_size(DecodeInfo *decInfo)
{
    char image_buffer[32];

    // Read 32 image bytes because extension size is stored as 32 bits
    fread(image_buffer, 1, 32, decInfo->fptr_stego_image);

    // Initially set extension size to 0
    decInfo->extn_size = 0;

    // Extract one LSB from each image byte
    for (int i = 0; i < 32; i++)
    {
        decInfo->extn_size =
            (decInfo->extn_size << 1) | (image_buffer[i] & 1);
    }

    printf("Extension size = %u\n", decInfo->extn_size);

    return e_success;
}

/* Decode the secret file extension*/
Status decode_output_file_extn(DecodeInfo *decInfo)
{
    // Temporary array to store decoded extension
    char extension[decInfo->extn_size + 1];

    // Decode extension characters from the stego image
    if (decode_image_to_data(extension, decInfo->extn_size, NULL, decInfo->fptr_stego_image) == e_success)
    {
        // Add null character at the end
        extension[decInfo->extn_size] = '\0';

        // Store decoded extension in DecodeInfo structure
        strcpy(decInfo->extn, extension);

        printf("Secret file extension decoded successfully\n");
        printf("Extension = %s\n", decInfo->extn);

        return e_success;
    }

    printf("Error in decoding secret file extension\n");

    return e_failure;
}

/* Decode the size of the secret file
 *
 * The encoder stores the secret file size
 * using 32 bits.
 */
Status decode_output_file_size(DecodeInfo *decInfo)
{
    char image_buffer[32];

    // Read 32 image bytes from the stego image
    // because file size is stored as 32 bits
    fread(image_buffer, 1, 32, decInfo->fptr_stego_image);

    // Initially set file size to 0
    decInfo->file_size = 0;

    // Extract one LSB from each image byte
    for (int i = 0; i < 32; i++)
    {
        decInfo->file_size =
            (decInfo->file_size << 1) | (image_buffer[i] & 1);
    }

    printf("Secret file size = %u\n", decInfo->file_size);

    return e_success;
}

Status decode_secret_file_data(DecodeInfo *decodeInfo)
{
    char ch;
    char image_buffer[8];

    //Decode one byte at a time
    for(int i = 0; i < decodeInfo->file_size; i++)
    {
        //Read 8 bytes from the Stego image
        fread(image_buffer, 1, 8, decodeInfo->fptr_stego_image);

        //Decode one secret chaaracter from 8 image bytes
        decode_byte_to_lsb(&ch, image_buffer);

        //Write decoded character to output file
        fputc(ch, decodeInfo->fptr_output_file);
    }
    return e_success;
}

Status open_files_decode(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n",
                decInfo->stego_image_fname);
        return e_failure;
    }

    decInfo->fptr_output_file = fopen(decInfo->output_fname, "w");

    if (decInfo->fptr_output_file == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n",
                decInfo->output_fname);
        return e_failure;
    }

    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    // Open the stego image and output file
    if (open_files_decode(decInfo) == e_success)
    {
        printf("All decode files opened successfully\n");

        // Skip the 54-byte BMP header
        if (skip_bmp_header(decInfo->fptr_stego_image) == e_success)
        {
            printf("BMP header skipped successfully\n");

            // Array to store the decoded magic string
            char magic_string[10];

            // Decode and verify the magic string
            if (decode_magic_string(magic_string, decInfo) == e_success)
            {
                printf("Magic String verified successfully\n");

                // Decode secret file extension size
                if (decode_output_file_extn_size(decInfo) == e_success)
                {
                    printf("Secret file extension size decoded successfully\n");

                    // Decode secret file extension
                    if (decode_output_file_extn(decInfo) == e_success)
                    {
                        printf("Secret file extension decoded successfully\n");
                        
                        if (decode_output_file_size(decInfo) == e_success)
                        {
                            printf("Secret file size decoded successfully\n");

                            //Decode the actual secret file data
                            if (decode_secret_file_data(decInfo) == e_success)
                            {
                                printf("Secret file size decoded successfully\n");
                            }
                            else{
                                printf("Error in decode secret file data\n");
                                return e_failure;
                            }
                        }
                        else
                        {
                            printf("Error in decoding secret file size\n");
                            return e_failure;
                        }
                    }
                    else
                    {
                        printf("Error in decoding secret file extension\n");
                        return e_failure;
                    }
                }
                else
                {
                    printf("Error in decoding secret file extension size\n");
                    return e_failure;
                }
            }
            else
            {
                printf("Magic String verification failed\n");
                return e_failure;
            }
        }
        else
        {
            printf("Error in skipping BMP header\n");
            return e_failure;
        }
    }
    else
    {
        printf("Error in opening decode files\n");
        return e_failure;
    }

    return e_success;
}