/* bin2header.c - program to convert binary file into a C structure
 * definition to be included in a header file.
 *
 * written by Maxime Perrotin
 * Based on the original bin2header.c written by Harald Welte
 *  
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char **argv)
{
       int c=0;
       int count=0;

       FILE *f_in=NULL, *f_out=NULL;

	if (argc < 4 ) {
               fprintf(stderr, "%s needs three arguments: input file, output file, structure name \n",
                       argv[0]);
               exit(1);
       }

       f_in=fopen(argv[1], "rb");
       f_out=fopen(argv[2], "w");

       assert (NULL != f_in && NULL != f_out);

       fprintf(f_out, "/* bin2header generated output - do not modify manually */\n\n");

       fprintf(f_out, "unsigned char %s[] = {\n", argv[3]);

       while (EOF != (c=getc(f_in))) {	
	fprintf(f_out, "%s%s0x%02x", count?", ":"", !(count%20)?"\n": "", c);
	count++;
       }

       fprintf(f_out, "};\n");
       fprintf(f_out, "#define %s_len %d\n", argv[3], count);
       
       fclose(f_in);
       fclose(f_out);

       exit(0);
}
