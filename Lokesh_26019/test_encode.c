 /**************************************************
  * Name: Lokesh Mandage
  * Date: 2/10/2026
  * Description: This file contains the main function for the
  * steganography project. It handles the command line arguments
  * and calls the appropriate functions for encoding or decoding.
  ***************************************************/

#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("ERROR: Please pass the operation type\n");
        printf("Usage: ./a.out -e beautiful.bmp secret.txt stego.bmp\n");
        printf("Usage: ./a.out -d stego.bmp output.txt\n");
        return e_failure;
    }

    
    if(check_operation_type(argv) == e_encode)
    {
        EncodeInfo encInfo;
        printf("Select Encoding\n");
         // Read and validate encode args for encoding
        if(read_and_validate_encode_args(argv, &encInfo) == e_success)
        {
            printf("Encode args read and validated successfully\n");
            if(do_encoding(&encInfo) == e_success)
            {
                printf("Encoding done successfully\n");
            }
            else
            {
                printf("Error in encoding\n");
            }
        }
        else
        {
            printf("Error in reading and validating encode args\n");
        }
    }
    else if (check_operation_type(argv) == e_decode)
    {
        DecodeInfo decInfo;
        
        printf("Selected Decoding\n");

        if(read_and_validate_decode_args(argv, &decInfo) == e_success)
        {
            printf("Decode args read and validated successfully\n");

            if (do_decoding(&decInfo) == e_success)
            {
                printf("Decoding done successfully\n");
            }
            else
            {
                printf("Error in decoding\n");
            }
        }
        
    }
    else{
        printf("Unsupported operation type\n");
        printf("*************************Usage****************************\n");
        printf("To Encode: ./a.out -e beautiful.bmp secret.txt stego.bmp\n");
        printf("To Decode: ./a.out -d stego.bmp output.txt\n");
        printf("*************************Usage****************************\n");
    }

    return 0;
}
OperationType check_operation_type(char *argv[])
{
    if(strcmp(argv[1], "-e") == 0)
    {
        return e_encode;
    }
    else if(strcmp(argv[1], "-d") == 0)
    {
        return e_decode;
    }
    else{
        return e_unsupported;
    }
}
