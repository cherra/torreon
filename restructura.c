#include <stdio.h>
#include <mysql/mysql.h>
#include <string.h>

MYSQL mysql;
MYSQL_ROW row,row2;
MYSQL_RES *res,*res2;

int conecta_bd()
{
	FILE *fconfiguracionbd;
	int i=0;
	int j=0;
	char tipo[50]="";
	char dato[50]="";
	char tmp;
	char ip[20]="";
	char usuario[20]="";
	char contrasena[20]="";
	char bd[30]="";

	if((fconfiguracionbd = fopen("configuracionbd.dat","r")))
	{
		while(!feof(fconfiguracionbd))
		{
			tmp = fgetc(fconfiguracionbd);
			if(tmp != ' ')
			{
				tipo[i] = tmp;
				i++;
			}
			else
			{
				while(tmp != '\n')
				{
					tmp = fgetc(fconfiguracionbd);
					dato[j]=tmp;
					j++;
				}
				dato[j-1] = '\0';
				j=0;
				if(strcmp(tipo,"ip") == 0)
					strcpy(ip, dato);
				if(strcmp(tipo,"usuario") == 0)
					strcpy(usuario, dato);
				if(strcmp(tipo,"contrasena") == 0)
					strcpy(contrasena, dato);
				if(strcmp(tipo,"bd") == 0)
					strcpy(bd, dato);
				strcpy(tipo,"");
				for(i=0;i<30;i++)
					tipo[i]='\0';
				i=0;
			}
		}
		fclose(fconfiguracionbd);

		mysql_init(&mysql);
		if(!(mysql_real_connect(&mysql, ip, usuario, contrasena, bd, 0, NULL, 0)))
			return -1;
		else
			return 1;
	}
	else
	{
		printf("No se pudo abrir el archivo....\n");
		return -1;
	}
}

int main(int argc, char *argv[])
{
	char sql[200],sql2[200],sql3[200];
	int er;
	double registros,contador;
	char temporal[7],actual[7];
		
	if(conecta_bd() != -1)
	{
		sprintf(sql,"SELECT id_venta FROM Venta WHERE num_factura=0 ORDER BY id_venta");
		er = mysql_query(&mysql, sql);
		if(er == 0)
		{
			if((res = mysql_store_result(&mysql)))
			{
				registros = mysql_num_rows(res);
				printf("\nActualizando...\n");
				contador=0;
				while((row = mysql_fetch_row(res)))
				{
					contador++;
					sprintf(sql2,"SELECT Venta_Factura_Relacion.num_factura, Venta_Factura.fecha, Venta_Factura.hora, Venta_Factura.observacion FROM Venta INNER JOIN Venta_Factura_Relacion ON Venta.id_venta = Venta_Factura_Relacion.id_venta INNER JOIN Venta_Factura ON Venta_Factura_Relacion.id_factura = Venta_Factura.id_factura WHERE Venta.id_venta=%s",row[0]);
					er = mysql_query(&mysql, sql2);
					if(er == 0)
					{
						if((res2 = mysql_store_result(&mysql)))
						{
							if((row2 = mysql_fetch_row(res2)))
							{
								sprintf(sql3,"UPDATE Venta SET num_factura=%s, fecha_factura='%s', hora_factura='%s', observacion='%s' WHERE id_venta=%s",row2[0],row2[1],row2[2],row2[3],row[0]);
								er = mysql_query(&mysql, sql3);
								if(er != 0)
								{
									printf("Error: %s\n", mysql_error(&mysql));
									sleep(1);
								}
								else
								{	
									sprintf(actual,"%.2f",(contador*100)/registros);
									if(strcmp(temporal,actual)!=0)
										printf("%s%% - %s\n",actual,row[0]);
									//fseek(stdout,0,SEEK_CUR-3);
									sprintf(temporal,"%.2f",(contador*100)/registros);
								}
							}
						}
					}
					else
						printf("Error: %s\n", mysql_error(&mysql));
				}
				printf("\nTerminado");
			}
		}
		else
			printf("Error: %s\n", mysql_error(&mysql));
	}
	else
		printf("Error: %s\n", mysql_error(&mysql));
	mysql_close(&mysql);
}
