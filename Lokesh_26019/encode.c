#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"


/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if(argv[2] != NULL && strcmp(strstr(argv[2], "."), ".bmp") == 0)
    {
        encInfo->src_image_fname = argv[2];
    }
    else
    {
        printf("ERROR: Source image file should be a .bmp file\n");
        return e_failure;
    }
    //validate the .txt file 
    if(argv[3] != NULL && strcmp(strstr(argv[3], "."), ".txt") == 0)
    {
        encInfo->secret_fname = argv[3];
    }
    else
    {
        printf("ERROR: Secret file should be a .text file\n");
        return e_failure;
    }
    //check whether user have passed the output file name or not
    if(argv[4] != NULL)
    {
        encInfo->stego_image_fname = argv[4];
    }
    else
    {
        encInfo->stego_image_fname = "stego.bmp";
    }
    return e_success;
}
//get the size of secret file
uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    uint size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    return size;
}                     //return the current file pointer, Size of file

//check the capacity of source image
Status check_capacity(EncodeInfo *encInfo)
{
    //get the image size
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);
    //header + magic string 2bytes + secret file extension size 4bytes + secret file extension 4 bytes + secret file size 4bytes + secret file 
    if(encInfo->image_capacity >= ((2 + 4 + 4 + 4 + encInfo->size_secret_file) * 8))
    {
        return e_success;
    }
    else
    {
        printf("ERROR: Source image does not have enough capacity to hold the secret file\n");
        return e_failure;
    }
}
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    //copy first 54 bytes (header) from source to destination
    char header[54];
    fseek(fptr_src_image, 0, SEEK_SET);
    fread(header, sizeof(char), 54, fptr_src_image);
    fwrite(header, sizeof(char), 54, fptr_dest_image);
    return e_success;
}
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    unsigned mask = 1 << 7;
    for (int i = 0; i < 8; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data & mask) >> (7 - i));
        mask = mask >> 1;
    }
    return e_success; 
}
Status encode_data_to_image(const char *data, int size, EncodeInfo *encInfo)
{
    //take a character from a data and fetch 8bytes of RGB from source image to encode
    for(int i = 0; i < size; i++)
    {
        fread(encInfo->image_data, 8, sizeof(char), encInfo->fptr_src_image);
        //encode byte to lsb will do the real encoding of data
        encode_byte_to_lsb(data[i], encInfo->image_data);
        //write the encoded data back to the destination image
        fwrite(encInfo->image_data, 8, sizeof(char), encInfo->fptr_stego_image);
    }
    return e_success;
}
Status encode_magic_string(const char *magic_str, EncodeInfo *encInfo)
{
    //every character encoded needs to call encode_data_to_image
    encode_data_to_image(magic_str, strlen(magic_str), encInfo);
    return e_success;
}
Status encode_size_to_lsb(char *image_buffer, int size)
{
    unsigned mask = 1 << 31;
    for (int i = 0; i < 32; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((size & mask) >> (31 - i));
        mask = mask >> 1;
    }
    return e_success; 
}
Status encode_secret_file_extn_size(int size, FILE *fptr_src, FILE *fptr_stego)
{
    //to encode size of 4 bytes i need 32 bytes rgb
    char rgb[32];
    fread(rgb, 32, sizeof(char), fptr_src);
    encode_size_to_lsb(rgb, size);
    fwrite(rgb, 32, sizeof(char), fptr_stego);
    return e_success;
}

Status encode_secret_file_extn(const char *file_ext, EncodeInfo *encInfo)
{
    file_ext = ".txt";
    encode_data_to_image(file_ext, strlen(file_ext), encInfo);
    return e_success;
}
Status encode_secret_file_size(long int size, EncodeInfo *encInfo)
{
    //to encode size of 4 bytes i need 32 bytes rgb
    char rgb[32];
    fread(rgb, 32, sizeof(char), encInfo->fptr_src_image);
    encode_size_to_lsb(rgb, size);
    fwrite(rgb, 32, sizeof(char), encInfo->fptr_stego_image);
    return e_success;
}
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch;
    // bring the secret file pointer to 8th position
    fseek(encInfo->fptr_secret, 0, SEEK_SET);
    for(int i = 0; i < encInfo->size_secret_file; i++)
    {
        fread(&ch, 1, sizeof(char), encInfo->fptr_secret);
        //read 8 bytes of rgb
        fread(encInfo->image_data, sizeof(char), 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(ch, encInfo->image_data);
        fwrite(encInfo->image_data, 8, sizeof(char), encInfo->fptr_stego_image);
    }
    return e_success;
}
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_stego)
{
    char ch;
    while(fread(&ch, 1, 1, fptr_src) > 0)
    {
        fwrite(&ch, 1, 1, fptr_stego);
    }
    return e_success;
}

//all the reset of encoding is called here
Status do_encoding(EncodeInfo *encInfo)
{
    //open all the required files
    if(open_files(encInfo) == e_success)
    {
        printf("All files opened successfully\n");
        if (check_capacity(encInfo) == e_success)
        {
            printf("Capacity check passed\n");
            if(copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
            {
                printf("BMP header copied successfully\n");
                if(encode_magic_string(MAGIC_STRING, encInfo) == e_success)
                {
                    printf("Magic String encoded successfully\n");
                    if(encode_secret_file_extn_size(strlen(".txt"), encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
                    {
                        printf("Secret file extension size is encoded successfully\n");
                        if(encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_success)
                        {
                            printf("Success!! Encoding the secret file extension\n");
                            if(encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_success)
                            {
                                printf("Encoded secret file size successfully\n");
                                if (encode_secret_file_data(encInfo) == e_success)
                                {
                                    printf("Encoded secret data successfully\n");
                                    if(copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
                                    {
                                        printf("Copied remaining RGB data\n");
                                    }
                                    else
                                    {
                                        printf("Failure: Copying remaining RGB\n");
                                        return e_failure;
                                    }
                                }
                                else
                                {
                                    printf("Failure: Encoding the secret file data\n");
                                    return e_failure;
                                }
                            }
                            else
                            {
                                printf("Failure: Encoding the secret file size\n");
                                return e_failure;
                            }
                        }
                        else
                        {
                            printf("Failure: Encoding the secret file extension\n");
                            return e_failure;
                        }
                    }
                    else
                    {
                        printf("Failed to encode extension size\n");
                        return e_failure;
                    }
                }
                else
                {
                    printf("Encoding Magic String failure\n");
                    return e_failure;
                }
            }
            else
            {
                printf("Error in copying BMP header\n");
                return e_failure;
            }
        }
        else
        {
            printf("Capacity check failed\n");
            return e_failure;
        }
    }
    else
    {
        printf("Error in opening files\n");
        return e_failure;
    }
    return e_success;
}
