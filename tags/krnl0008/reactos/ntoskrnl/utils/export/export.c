/*
 * This program takes a list of export symbols and builds a c source
 * file which when linked will contain a symbol table
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_line(char* buf)
{
        char ch;
        while ((ch=fgetc(stdin))!='\n'&& !feof(stdin))
        {
                *buf=ch;
                buf++;
        }
        *buf=0;
        return(1);
}

int main(int argc, char* argv[])
{
        char buffer[12000];
        char* sym[4096];
        int i=0,j=0;
        char ch;
   
        if (argc!=1)
        {
                fprintf(stderr,"Usage: export < export_list > symbol table\n");
                exit(0);
        }

        /*
         * Output preamble
         */
        fprintf(stdout,"/*\n");
        fprintf(stdout," * This file was machine generated by export\n");
        fprintf(stdout," * Don't edit\n");
        fprintf(stdout," *\n");
        fprintf(stdout," *\n");
        fprintf(stdout,"*/\n");

        fprintf(stdout,"#include <internal/symbol.h>\n");        

        while (!feof(stdin))
        {
                read_line(buffer);
//                printf("reading line %s\n",buffer);
                if (buffer[0]!='#' && strlen(buffer) >= 1)
                {
                        sym[i]=strdup(buffer);
                        i++;
                }
                
        }
        fprintf(stdout,"#ifdef __cplusplus\n");
        fprintf(stdout,"extern \"C\" {\n");
        fprintf(stdout,"#endif\n");
        for (j=0;j<i;j++)
        {
                fprintf(stdout,"void %s(void);\n",sym[j]);
        }
        fprintf(stdout,"#ifdef __cplusplus\n");
        fprintf(stdout,"}\n");
        fprintf(stdout,"#endif\n");
        fprintf(stdout,"export symbol_table[]={\n");
        for (j=0;j<i;j++)
        {
                fprintf(stdout,"{\"_%s\",(unsigned int)%s},\n",sym[j],sym[j]);
        }

        fprintf(stdout,"{NULL,NULL},\n");
        fprintf(stdout,"};\n");
   
   return(0);
}
