#include <stdio.h>
#include <stdlib.h>

int main()
{
	FILE *archivo;
	
	archivo = fopen("ticket-arriba.txt","a");
	fputc(27,archivo);
	fputc(33,archivo);
	fputc(1,archivo);
/*	fputc(27,archivo);
	fputc(33,archivo);
	fputc(32,archivo);

	fputc(10,archivo);

	fputc(27,archivo);
	fputc(33,archivo);
	fputc(0,archivo);
	fputc(27,archivo);
	fputc(33,archivo);
	fputc(0,archivo);*/

	fclose(archivo);
}
