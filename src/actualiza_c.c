#include <stdio.h>
#include <mysql/mysql.h>

MYSQL mysql;
MYSQL_ROW row;
MYSQL_RES *res;

int main(int argc, char *argv[])
{
	char sqlsaca[100];
	char sqlmete[200];
	int er;

	sprintf(sqlsaca, "SELECT id_cliente FROM Cliente ORDER BY id_cliente");

	if(argc > 1)
	{
	mysql_init(&mysql);
	if(mysql_real_connect(&mysql, argv[1], argv[2], argv[3], argv[4], 0, NULL, 0))
	{
		er = mysql_query(&mysql, sqlsaca);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row = mysql_fetch_row(res)))
				{
					sprintf(sqlmete, "INSERT INTO Cliente_Envio (id_cliente_envio, id_cliente) VALUES(NULL, %s)", row[0]);
					er = mysql_query(&mysql, sqlmete);
					if(er == 0)
						printf("Cliente: %s\n", row[0]);
					else
						printf("Error: %s\n", mysql_error(&mysql));
				}
			}
		}
		else
			printf(mysql_error(&mysql));
	}

	mysql_close(&mysql);
	}
	else
		printf("USO: actualiza_c <IP> <usuario> <contraseña> <db>\n");
}
