#include <stdio.h>
#include <stdlib.h>

int main (int argv, char *argc[])
{
	FILE *f ;
	
	if  ( ! (f = fopen ("/dev/lp0","w"))  )
	{
	
		printf ("Error\n");
		exit (0);
	}
	
	/*fputc (27,f);
	fputc ('-',f);
	fputc (0,f);*/
	
	
	fputc (27,f);
	fputc ('r',f);
	fputc (1,f);
	
	/*fprintf (f,"%x",0x0001b);
	fprintf (f,"%x",0x0002d);
	fprintf (f,"%x",0x00001);*/
	
	fprintf (f,"COLOR ROJO\n");
	fprintf (f,"\n\n\n\n\n\n\n");
	
	fclose (f);
	
	return 1;
}
