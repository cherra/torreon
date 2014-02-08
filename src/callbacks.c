#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>

#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
//#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include <time.h>
#include <unistd.h>
#define SIZE_BUFFER 256 //Buffer para el time.h
#include <sys/types.h>
#include <regex.h> //Expresion regular

#include "callbacks.h"
#include "interface.h"
#include "support.h"

gboolean corte_final=FALSE;
gboolean focus_txtbarcode=FALSE;
gboolean aplicar_observaciones = TRUE;
gboolean cambiar_observaciones = TRUE;
int sport; /* Variable para el descriptor del puerto serial */

//////Funciones Eder////////
#include "dialogos2.h" //Libreria para mensajes
#include "dialogos.h" //Libreria para mensajes
#include "funciones.h"
#include "impresion_ticket.h"

char num_vendedor[3];

int puerto_serie=2;
GtkWidget *caja; 
GtkWidget *win_cajon_abierto;
GtkWidget *V_pedidos;

/** FUNCIONES PARA EL MANEJO DEL PUERTO SERIAL **/

int open_port(int port)
{  
	int fd; /* File descriptor for the port */
	if(port == 2){
	  //printf("puerto COM2\n");
	  fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
	 } else{
	  //printf("Por default puerto COM1\n");
	  fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
	  }
	  
	  if (fd == -1)
	  {
   /*
    * Could not open the port.
    */
	    perror("open_port: Unable to open Port - ");
	  }
	  else
	    fcntl(fd, F_SETFL, 0);
  return (fd);
}

int initport(int puerto){

	if((sport = open_port(puerto)) ==-1){
	        perror("open");
	        return (-1);
	}
	else
	//printf("Puerto abierto Good\n");
	//printf ("Puerto = %d\n",sport);

	tcgetattr(sport, &last);
	/*Colocando opciones propias*/
	bzero(&options, sizeof(options)); /*limpiar la estructura para nuevos valores(ceros)*/
	
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	
	/*8N1*/
	options.c_cflag &= ~CBAUD;
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag |= CLOCAL;
	options.c_cflag |= CREAD;	/*habilitar	 lectura del puerto*/
	
	options.c_iflag = IGNPAR | ICRNL; /*ignora errores de paridad*/
    	options.c_oflag = 0;
	 /* set input mode (non-canonical, no echo,...) */
    	options.c_lflag = 0;
	options.c_cc[VMIN]=1;
        options.c_cc[VTIME]=30;	

	tcflush(sport, TCIFLUSH);
	
	tcsetattr(sport, TCSANOW, &options);
	
//	signal(SIGIO,signal_handler_IO);
//	fcntl(sport, F_SETOWN, getpid()); /*Permitir recibir la senal SIGIO*/
	//fcntl(sport, F_SETFL, 0);
//	fcntl(sport, F_SETFL, FNDELAY);
//	fcntl(sport, F_SETFL, FASYNC);
	
//	signal(SIGINT,SIG_INTERRUPT);
	return 0;
}

void SIG_INTERRUPT( ){
	//printf("Interrumpido por usuario\n");
	tcsetattr(sport, TCSANOW, &last);
	close(sport);
	//exit(1);
}

void signal_handler_IO( int status){
	char input;
	int n;
	GtkWidget *txtbarcode;
	
	txtbarcode = lookup_widget(GTK_WIDGET (caja),"txtbarcode");
	
	if((n=read(sport,&input,1)) == 1){
		//printf("recibido %d	\n",input);
		if(input == 0 ){
			//g_print("cajon abierto!!\n");
			//Info("Por favor cierre el cajon antes de continuar!");
			gtk_widget_show(win_cajon_abierto);
			gtk_widget_set_sensitive (caja,FALSE);
			focus_txtbarcode=TRUE;
		}else if(input == 1){
			//g_print("cajon cerrado!!\n");
			gtk_widget_set_sensitive (caja,TRUE);
			gtk_widget_hide(win_cajon_abierto);
			gtk_widget_grab_focus (caja);
			if (focus_txtbarcode)
			{
				focus_txtbarcode=FALSE;
				gtk_widget_grab_focus(GTK_WIDGET(txtbarcode));
			}
		}
	}else{printf(" %d ?\n",n);}
	
}

/**** EDER FUNCIONES PARA CAJON ***/

int id_checa_cajon;

gboolean checa_cajon();

gboolean checa_cajon()
{
	int fd, n; /* File descriptor for the port */
	FILE *ftty;
	char abierto=66;
	static char peticion[3];
	static char seleccion_dispositivo[3];
	char uno = 1;
	static char reiniciar[2];
if( ! imprimiendo){
	initport(puerto_serie);
	
	reiniciar[0]=27;
	reiniciar[1]=64;
	
	peticion[0]=29;
	peticion[1]=114;
	peticion[2]=2;
	
	//peticion[0]=10;
	//peticion[1]=04;
	//peticion[2]=1;
	
	seleccion_dispositivo[0]=27;
	seleccion_dispositivo[1]=61;
	seleccion_dispositivo[2]=3;

	//printf ("Puerto = %d\n",sport);

	//write(sport, reiniciar, 2);
	n = write(sport, peticion, 3);
	//g_print(" escritos %d \n",n);
	signal_handler_IO( 29);
	
	//printf ("\nBytes escritos = %d \n",n);
	if(n <1)
		Info("Por favor verifique que la impresora este bien conectada !!");
	

	SIG_INTERRUPT( );
	
	}
	return TRUE;
}


//VARIABLES PARA MODULOS

///// Modifcar clientesd
GModule *modulo_clientes;
GtkWidget *(*funcion)(void);

//VARIABLES PARA MODULOS


struct cliente
{
  gchar *id_cliente;
  gchar *nombre;
  gchar *domicilio;
  gchar *telefono;
  gchar *telefono2;
  gchar *telefono3;
  gchar *colonia;
  gchar *CPostal;
  gchar *ciudad;
  gchar *estado;
  gchar *RFC;
  gchar *contacto;
  gchar *nombre_de_lista;
  gchar *calles;
  gchar *pago;
  gchar *formato;
  gchar *vencimiento;
}modifica_cliente, alta_cliente;
 gchar *criterio_bak;
 gchar *lista2;
 int numerolistas;
 gint id_lista_de_precios = 0;


/*** MODIFICAR CLIENTES ***/
char id_cliente_modifica[10];
char id_lista_modifica[10];

int er = 0;

//****Faturacion de varios folios en uno solo a nombre de alguien**//

/*#include "facturacion_no_facturado/interface.h"
#include "facturacion_no_facturado/callbacks.h"
#include "facturacion_no_facturado/interface.c"
#include "facturacion_no_facturado/callbacks.c"*/

//****CONFIGURACION DE IMPRESORAS**//

#include "conf_impresio/interface.h"
#include "conf_impresio/callbacks.h"
#include "conf_impresio/interface.c"
#include "conf_impresio/callbacks.c"

//////////*******************************Estas las declaro Jorge******************************//////////////

MYSQL mysql,mysql2;
MYSQL_ROW row,row2,row3;
MYSQL_RES *res,*res2;
MYSQL_FIELD *field;


//** VARIABLE PARA CAMBIAR EL LIMITE DE LA CANTIDAD **/
gboolean limite_piezas = TRUE;
gboolean guarda_iva = FALSE;

int errno=0;
char Errores[200];
/*GtkWidget *caja; DECLARADA MAS ARRIBA !!!!*/
GtkWidget *Alta_clientes_caja;
GtkWidget *Buscar_venta;
GtkWidget *Buscar_articulo;
GtkWidget *Buscar_cliente;
GtkWidget *entry_global;
GtkWidget *winbascula = NULL;
char folio_global[20];
char codigo_articulo_global[20];
char codigo_cliente_global[20];
char telefono_cliente_global[20];
char bascula[3];

/*VARIABLES PARA EL LOGIN*/
struct inicio_sesion
{
	gchar *login;
	gchar *contrasena;
}vcaja;
char id_sesion_usuario[5]="";
char id_sesion_caja[5]="";
char tipo_sesion_usuario[10]="";
char usuario_login[20]="";
char password_login[20]="";

/* VARIABLE NECESARIAS PARA LA VENTA */
char codigo[5], peso[10], tipo_art[10];
gboolean yacodigo = FALSE;
int fila_clientes=-1;
int fila_articulo, columna_articulo;
char codigo_cliente[10];
double total_venta = 0;
double total_kilos = 0;
double total_piezas = 0;
double total_descuento = 0;
float ya_se_marco[50];
/*int radiobtnticket = -1;
int radiobtnfactura = -1;
int radiobtncontado = -1;		//OPCIONES DE VENTA
int radiobtncredito = -1;*/
int id_pedido = 0;
int cliente_credito = 1;
gboolean es_pedido=FALSE;

gboolean es_factura=FALSE;

int num_articulos_venta=-1;		//NUMERO DE ARTICULOS EN LA LISTA

gchar *efectivo_venta=NULL;			//EL PAGO DEL CLIENTE

gboolean vendedorbien=FALSE;
char id_empleado[4];

/***VARIABLES PARA LISTAS DE PRECIOS */
struct lista_precios
{
	char clave[6];
	char nombre[50];
}lista1;

char busqueda_lista_criterio[50];
char id_articulo_precio[10];
int fila_selec;

/***VARIABLES PARA DESCUENTOS*/
char id_cliente_descuento[10]="";
char codigo_articulo_descuento[10]="";
char articulo_descuento[10]="";
char busqueda_precio_descuento[50]="";
char busqueda_cliente_descuento[50]="";

/***VARIABLES NECESARIAS PARA LA SEGURIDAD */
char ventana[20]="";
GList *lista_items_menu;


/***VARIABLE GLOBALES NECESARIA PARA EVITAR QUE SE ABRAN DOS VENTANAS IGUALES */
GtkWidget *Datos_cliente;
gboolean bDatos_cliente = FALSE;

/*VARIABLES PARA LA CONFIGURACION*/
struct cbd					//Estructura para la configuracion de la base de datos
{
	gchar *ip;
	gchar *usuario;
	gchar *contrasena;
	gchar *bd;
	gchar *caja;
}vcdb;

struct cbd2
{
	char ip[20];
	char usuario[20];
	char contrasena[20];
	char bd[20];
	char caja[20];
}vcdb2;

/* PARA LOS ABONOS DE LOS CLIENTES */
char id_cliente_abono[10];
char id_venta_abono[20];
gchar *criterio_abono;
char total_abono[15];
char total_abono_abonos[15];
int fila_abono=-1;
char monto_quita_abono[12];


/*PARA LA MANIPULACION DE ARTICULOS*/
char id_linea_articulo[5]="";
char criterio_articulo[100]="";
char prefijo_linea[10]="";
int fila_linea_articulo=-1;
char id_nuevo_articulo[10]="";
char id_actualiza_articulo[10]="";


/* PARA EL CIERRE DE FACTURAS */
long int cierre_num_factura;

/*PARA LOS PEDIDOS*/
int  es_fecha(char *cadena);
int es_bisiesto(int anio);
/*********************Agregar estas variables como globales************************/
gchar *listaX[6];
int row_counter;
gint selected_row;
gint selected_row_clientes;
GtkWidget *pedidos;
 
 //Variable de ERROR 0 NO ERROR 1 SI ERROR
GtkWidget *ventana_oviedo;

struct Articulo
	{
		gchar *codigo;
		gchar *nombre;
		gchar *precio;
		gchar *observaciones;
		gchar *tipo;
	}articulo;

//Para que no se abran mas de dos ventanas en observaciones
GtkWidget *V_observaciones;
gboolean bV_observaciones = FALSE;
GtkWidget *Cierre_facturas;
int mes_numero(gchar *mes);
void mes_caracter(int mes, char mes_char[15]);
/**********************************************************************************/




//////////////////////////////////
////esta variable es usada para tener un cliente para llenar la lista
	char id_cliente_pedido[6] = "1863";
//////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////
///////Esta la declaro Jose Juan
/////////////////////////////////////////////////////////////////////////////////////////////////////////

struct producto
{
 gchar *id;
 gchar *nombre;
 gchar *tipo;
 gchar *imagen;
 gchar *precio;
}alta_producto, precio_producto;

struct usuario
{
 gchar *id_usuario;
 gchar *nombre;
 gchar *apellido;
 gchar *usuario;
 gchar *contrasena;
 gchar *contrasena2;
 gchar *tipo;
}alta_usuario;

struct empleado
{
 gchar *id_empleado;
 gchar *nombre;
 gchar *apellido;
 gchar *domicilio;
 gchar *colonia;
 gchar *cp;
 gchar *ciudad;
 gchar *estado;
 gchar *telefono;
 gchar *puesto;
 gchar *noseguro;
 gchar *sueldobase;
}alta_empleado;

/*****************************************TERMINAN VARIABLES***********************************/


///////////////////////////////////////////////////////////////////////////////////////////////
//                                Estas son de Eder                                             >>
///////////////////////////////////////////////////////////////////////////////////////////////

struct articulo
{
	gchar *id_articulo;
	gchar *nombre;
	gchar *tipo;
	gchar *imagen;
	gchar *codigo;
}str_articulo;

GtkCList *lista_borrada;

int indice_articulo;
char *id_venta;
char *id_articulo_arreglo[10000];
char *total_venta_cancelada="";

GtkEntry *art_nombre;
GtkWidget *widget_scroll_obs;
GtkEntry *art_codigo;
GtkEntry *art_tipo;
GtkScrolledWindow *scroll_desc; //Scroll descripcion
GtkScrolledWindow *scroll_obs; //Scroll observaciones

//<<///////////////////////////////////////////////////////////////////////////////////////////// (EDER)


/////////////////////////////////////	OVIEDO	///////////////////////////////////////////
gchar *Retiro_Efectivo_Usuario;
gchar *Retiro_Efectivo_Password;
int Retiro_Autoriza = 0;
int Retiro_Bandera = 0;
int GBL_Numero_Caja = 1;
int GBL_id_cajero = 1;

//OVIEDO <-- Lo mismo para que no se abran mas de dos ventanas
GtkWidget *Retiro_Efectivo;
gboolean bRetiro_Efectivo = FALSE;

///////////////////////////////////////////////////////////////////////////////////////////





/*************************************************************************************************
						JUAN
**************************************************************************************************/


void
on_btn_aceptaralta_clicked             (GtkWidget       *Ventana_altas,
                                        gpointer         user_data)
{
 GtkWidget *nombre, *direccion, *colonia, *rfc, *ciudad, *estado, *cp, *telefono, *telefono2, *telefono3, *contacto, *lista, *pago, *formato, *entre_calles;
 int bandera=0;
 int er;
 long int ultimo_cliente;
 char lista_igual[1000];
 char cadenilla[2000];
 char sqlcontpaq[100], sql_articulos[100], sql_clientearticulo[300];
 char cuenta_contable[10]="0";
 gsize algo=-1;
 gchar *cnombre, *cdireccion, *ccolonia, *crfc, *cciudad, *cestado, *ccp, *ctelefono, *ctelefono2, *ctelefono3, *ccontacto, *clista, *cpago, *cformato, *centre_calles;

  nombre = lookup_widget(Alta_clientes_caja, "entry_Altanuevonombre");
  direccion = lookup_widget(Alta_clientes_caja, "entry_Altanuevodireccion");
  colonia = lookup_widget(Alta_clientes_caja, "entry_Altanuevocolonia");
  rfc = lookup_widget(Alta_clientes_caja, "entry_AltanuevoRFC");
  ciudad = lookup_widget(Alta_clientes_caja, "entry_Altanuevociudad");
  estado = lookup_widget(Alta_clientes_caja, "entry_Altanuevoestado");
  cp = lookup_widget(Alta_clientes_caja, "entry_AltanuevoCP");
  telefono = lookup_widget(Alta_clientes_caja, "entry_Altanuevotelefono");
  telefono2 = lookup_widget(Alta_clientes_caja, "entry_Altanuevotelefono2");
  telefono3 = lookup_widget(Alta_clientes_caja, "entry_Altanuevotelefono3");
  contacto = lookup_widget(Alta_clientes_caja, "entry_Altanuevocontacto");
  entre_calles = lookup_widget(Alta_clientes_caja, "entry_Altanuevocalles");
  lista = lookup_widget(Alta_clientes_caja, "CBOX_Seleccionelista");
  formato = lookup_widget(Alta_clientes_caja, "combo_entry_Altanuevoclienteformato");
  pago = lookup_widget(Alta_clientes_caja, "combo_entry_Altanuevoclientepago");

  cnombre = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(nombre), 0, -1),-1,&algo,&algo,NULL);
  cdireccion = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(direccion), 0, -1),-1,&algo,&algo,NULL);
  ccolonia = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(colonia), 0, -1),-1,&algo,&algo,NULL);
  crfc = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(rfc), 0, -1),-1,&algo,&algo,NULL);
  cciudad = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(ciudad), 0, -1),-1,&algo,&algo,NULL);
  cestado = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(estado), 0, -1),-1,&algo,&algo,NULL);
  ccp = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(cp), 0, -1),-1,&algo,&algo,NULL);
  ctelefono = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(telefono), 0, -1),-1,&algo,&algo,NULL);
  ctelefono2 = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(telefono2), 0, -1),-1,&algo,&algo,NULL);
  ctelefono3 = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(telefono3), 0, -1),-1,&algo,&algo,NULL);
  ccontacto = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(contacto), 0, -1),-1,&algo,&algo,NULL);
  centre_calles = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(entre_calles), 0, -1),-1,&algo,&algo,NULL);
  clista = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(lista), 0, -1),-1,&algo,&algo,NULL);
  cformato = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(formato), 0, -1),-1,&algo,&algo,NULL);
  cpago = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(pago), 0, -1),-1,&algo,&algo,NULL);

  printf("El RFC: %d\n", crfc[0]);

  printf("El nombre: %s\nEl domicilio: %s\n", cnombre, cdireccion);

  if(strlen(crfc)>0)
  	bandera = arreglar_rfc(crfc);

  if((strlen(cnombre) == 0) || (strlen(ctelefono) == 0) ||(strlen(cdireccion) == 0))
  	Err_Info("Faltan algunos datos");
  else if(bandera == 1)
  {
	Err_Info("El RFC es incorrecto");
	gtk_widget_grab_focus(rfc);
  }
  else
  {

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		sprintf(sqlcontpaq, "SELECT MAX(cuenta_contable)+1 FROM Cliente");
		er = mysql_query(&mysql, sqlcontpaq);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
				{
					if(strlen(crfc)>0)
						strcpy(cuenta_contable,row[0]);
					sprintf(lista_igual,"SELECT nombre, id_lista FROM Lista WHERE nombre = '%s'",clista);
					er = mysql_query(&mysql, lista_igual);
					if(er == 0)
					{
						res = mysql_store_result(&mysql);
						if(res)
						{
							while((row=mysql_fetch_row(res)))
							{
								if(strcmp(clista,row[0])==0)
								{
									printf("Estoy en la funcion de mete valores en caja\n");
									sprintf(cadenilla, "INSERT INTO Cliente (id_cliente, nombre, domicilio, colonia, ciudad_estado, cp, telefono, telefono2, telefono3, rfc, entre_calles, contacto, id_lista, tipo_pago, tipo_impresion, cuenta_contable) VALUES(NULL, '%s', '%s', '%s', '%s, %s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %s, '%s', '%s', %s)", cnombre, cdireccion, ccolonia, cciudad, cestado, ccp, ctelefono, ctelefono2, ctelefono3, crfc, centre_calles, ccontacto, row[1], cpago, cformato, cuenta_contable);
							      		printf("Mysql: %s\n",cadenilla);
			      						er = mysql_query(&mysql,cadenilla);
      									if (er!=0)
									{
										sprintf(Errores, "No se pudo registrar al cliente: %s\nTome nota del error y llame a Matica\n para solucionar el problema", mysql_error(&mysql));
										Err_Info(Errores);
									}
									else
									{
										ultimo_cliente = mysql_insert_id(&mysql);
										sprintf(sql_articulos,"SELECT id_articulo FROM Articulo ORDER BY id_articulo");
										er = mysql_query(&mysql, sql_articulos);
										if(er == 0)
										{
											if((res = mysql_store_result(&mysql)))
											{
												while((row = mysql_fetch_row(res)))
												{
													sprintf(sql_clientearticulo,"INSERT INTO Cliente_Articulo (id_cliente, id_articulo, bloqueado) VALUES(%d,%s,'n')",ultimo_cliente,row[0]);
													er = mysql_query(&mysql, sql_clientearticulo);
													if(er == 0)
														printf("Cliente: %d\tArticulo: %s\n",ultimo_cliente, row[0]);
													else
														printf("Error al insertar bloqueos: %s\n%s\n", mysql_error(&mysql), sql_clientearticulo);
												}
											}
										}
										else
											printf("Error en el registro de bloqueos\n%s\n",mysql_error(&mysql));
									}
								}
							}
						}
					}
					else
						printf("Error: %s", mysql_error(&mysql));
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	mysql_close(&mysql);

	gtk_widget_destroy(Ventana_altas);
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Funcion para meter valores en la tabla de producto
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void
mete_valores_entabla_producto()
{

  int error;
  gchar cadena_articulo[500];
  if(conecta_bd() == -1)
    {
      printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
    }
  else
    {

      strcpy(cadena_articulo, "INSERT INTO Articulo VALUES(");
      strcat(cadena_articulo, "NULL,");
      strcat(cadena_articulo, "\"");

	if(alta_producto.nombre)      
      strcat(cadena_articulo, alta_producto.nombre);
      strcat(cadena_articulo,"\",\"");

	if(strcmp(alta_producto.tipo,"pieza")== 0)
      strcat(cadena_articulo, "pieza");


	if(strcmp(alta_producto.tipo,"peso") == 0)
      strcat(cadena_articulo, "peso");

      strcat(cadena_articulo,"\",\"\")");
      printf("Mysql: %s\n",cadena_articulo);
      error = mysql_query(&mysql,cadena_articulo);
      if (error!=0)
	printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));

      
    }
  mysql_close(&mysql);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//  termina la insercion a la tabla Articulo
/////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Funcion para meter valores en la tabla de usuario
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void
mete_valores_entabla_usuario()
{
  int error;
  gchar cadena_usuario[500];
  if(conecta_bd() == -1)
    {
      printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
    }
  else
    {

      strcpy(cadena_usuario, "INSERT INTO Usuario VALUES(");
      strcat(cadena_usuario, "NULL,");
      strcat(cadena_usuario, "\"");

	if(alta_usuario.nombre)
      strcat(cadena_usuario, alta_usuario.nombre);
      strcat(cadena_usuario,"\",\"");

	if(alta_usuario.apellido)
      strcat(cadena_usuario, alta_usuario.apellido);
      strcat(cadena_usuario,"\",\"");

	if(alta_usuario.usuario)
      strcat(cadena_usuario, alta_usuario.usuario);
      strcat(cadena_usuario,"\",\"");

	if(alta_usuario.contrasena)
      strcat(cadena_usuario, alta_usuario.contrasena);
      strcat(cadena_usuario,"\",\"");

    	if(alta_usuario.tipo)
      strcat(cadena_usuario, alta_usuario.tipo);


      strcat(cadena_usuario,"\")");
      printf("Mysql: %s\n",cadena_usuario);
	if(strcmp(alta_usuario.contrasena , alta_usuario.contrasena2) == 0)
	{
      		error = mysql_query(&mysql,cadena_usuario);
      		printf("Paso la validacion de password\n");
		if (error!=0)
		printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
        }
	else
	{
	 Info("Faltan algunos datos");
	}

    }
  mysql_close(&mysql);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//  termina la insercion a la tabla usuario
/////////////////////////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Funcion para meter valores en la tabla de empleado
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void
mete_valores_entabla_empleado()
{


  int error;
  gchar cadena_empleado[500];
  if(conecta_bd() == -1)
    {
      printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
    }
  else
    {

      strcpy(cadena_empleado, "INSERT INTO Empleado VALUES(");
      strcat(cadena_empleado, "NULL,");
      strcat(cadena_empleado, "\"");

	if(alta_empleado.nombre)      
      strcat(cadena_empleado, alta_empleado.nombre);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.apellido)
      strcat(cadena_empleado, alta_empleado.apellido);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.domicilio)      
      strcat(cadena_empleado, alta_empleado.domicilio);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.colonia)      
      strcat(cadena_empleado, alta_empleado.colonia);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.cp)      
      strcat(cadena_empleado, alta_empleado.cp);
      strcat(cadena_empleado,"\",\"");
      
	if(alta_empleado.ciudad)
      strcat(cadena_empleado, alta_empleado.ciudad);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.estado)      
      strcat(cadena_empleado, alta_empleado.estado);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.telefono)      
      strcat(cadena_empleado, alta_empleado.telefono);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.puesto)      
      strcat(cadena_empleado, alta_empleado.puesto);
      strcat(cadena_empleado,"\",\"");

	if(alta_empleado.noseguro)
      strcat(cadena_empleado, alta_empleado.noseguro);
      strcat(cadena_empleado,"\",");

	if(alta_empleado.sueldobase)
      strcat(cadena_empleado, alta_empleado.sueldobase);

      strcat(cadena_empleado,")");
      printf("Mysql: %s\n",cadena_empleado);
      error = mysql_query(&mysql,cadena_empleado);
      if (error!=0)
	printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));


    }
  mysql_close(&mysql);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//  termina la insercion a la tabla Empleado
/////////////////////////////////////////////////////////////////////////////////////////////////


void
on_nuevo_articulo_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *Articulos;
	Articulos = create_Articulos();
	gtk_widget_show(Articulos);
}


//////////////////////////////////////////////////////////////////////////////////////////////
//meter productos en listas de precios(un producto en todas las listas con el mismo precio) //
//////////////////////////////////////////////////////////////////////////////////////////////

void
mete_valores_en_lista()
{
  char sentencia[500];
  char sentencia_saca_id[500] = "SELECT id_articulo, nombre FROM Articulo GROUP BY id_articulo DESC LIMIT 1";

  int i, error;
  char id_lista[10];
	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
    	}
        else
	 {
		error = mysql_query(&mysql,"SELECT id_lista, nombre FROM Lista");
		if(error == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
				numerolistas = mysql_num_rows(res);
				error = mysql_query(&mysql,sentencia_saca_id);
			if(error == 0)	
			{
				res = mysql_store_result(&mysql);
				if(res)
				{
					if((row = mysql_fetch_row(res)))
					{
						if(strcmp(row[1], alta_producto.nombre) == 0)
						{
							for (i = 1; i <= numerolistas; i++)
							{printf("Entra al for");
								strcpy(sentencia,"INSERT INTO Articulo_Lista VALUES(NULL, ");
								strcat(sentencia, row[0]);
								strcat(sentencia, ", ");
								sprintf(id_lista, "%d", i);
								strcat(sentencia, id_lista);
								strcat(sentencia, ", ");
								strcat(sentencia, alta_producto.precio);
								strcat(sentencia, ")");
								//altera los precios
								error = mysql_query(&mysql,sentencia);
								sentencia[0] = '\0';
							}
						}
					}
				}
			
			}
			else
				printf("Error: %s", mysql_error(&mysql));
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	 }


mysql_close(&mysql);


}



void
on_btn_Altaempleadoaceptar_clicked_nombre
                                        (GtkEntry       *entry_nombre,
                                        gpointer         user_data)
{
	alta_empleado.nombre = gtk_editable_get_chars(GTK_EDITABLE(entry_nombre),0,-1);
}


void
on_btn_Altaempleadoaceptar_clicked_apellido
                                        (GtkEntry       *entry_apellido,
                                        gpointer         user_data)
{
	alta_empleado.apellido = gtk_editable_get_chars(GTK_EDITABLE(entry_apellido),0,-1);
}


void
on_btn_Altaempleadoaceptar_clicked_domicilio
                                        (GtkEntry       *entry_domicilio,
                                        gpointer         user_data)
{
	alta_empleado.domicilio = gtk_editable_get_chars(GTK_EDITABLE(entry_domicilio),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked_colonia
                                        (GtkEntry       *entry_colonia,
                                        gpointer         user_data)
{
	alta_empleado.colonia = gtk_editable_get_chars(GTK_EDITABLE(entry_colonia),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked_cp  (GtkEntry       *entry_cp,
                                        gpointer         user_data)
{
	alta_empleado.cp = gtk_editable_get_chars(GTK_EDITABLE(entry_cp),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked_ciudad
                                        (GtkEntry       *entry_ciudad,
                                        gpointer         user_data)
{
	alta_empleado.ciudad = gtk_editable_get_chars(GTK_EDITABLE(entry_ciudad),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked_estado
                                        (GtkEntry       *entry_estado,
                                        gpointer         user_data)
{
	alta_empleado.estado = gtk_editable_get_chars(GTK_EDITABLE(entry_estado),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked_telefono
                                        (GtkEntry       *entry_telefono,
                                        gpointer         user_data)
{
	alta_empleado.telefono = gtk_editable_get_chars(GTK_EDITABLE(entry_telefono),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked_puesto
                                        (GtkEntry       *entry_puesto,
                                        gpointer         user_data)
{
	alta_empleado.puesto = gtk_editable_get_chars(GTK_EDITABLE(entry_puesto),0,-1);
}


void
on_btn_Altaempleadoaceptar_clicked_seguro
                                        (GtkEntry       *entry_seguro,
                                        gpointer         user_data)
{
	alta_empleado.noseguro = gtk_editable_get_chars(GTK_EDITABLE(entry_seguro),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked_sueldo
                                        (GtkEntry       *entry_sueldo,
                                        gpointer         user_data)
{
	alta_empleado.sueldobase = gtk_editable_get_chars(GTK_EDITABLE(entry_sueldo),0,-1);	
}


void
on_btn_Altaempleadoaceptar_clicked     (GtkWidget       *widget_ventana,
                                        gpointer         user_data)
{
///////validar y llamar a funcion de meter datos, destruir widget_ventana

 gint bandera;


  bandera = 0;


  if (alta_empleado.nombre == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.nombre,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.apellido == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.apellido,"") == 0)
	{
		bandera = 1;
	}
  if (alta_empleado.domicilio == NULL)
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.domicilio,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.colonia == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.colonia,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.cp == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.cp,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.ciudad == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.ciudad,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.estado == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.estado,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.telefono == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.telefono,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.puesto == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.puesto,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.noseguro == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.noseguro,"") == 0) 
	{
		bandera = 1;
	}
  if (alta_empleado.sueldobase == NULL) 
	{
		bandera = 1;
	}
   else if (strcmp(alta_empleado.sueldobase,"") == 0)
	{
		bandera = 1;
	}

  if(bandera == 0)
  	{
  		mete_valores_entabla_empleado();
		gtk_widget_destroy(widget_ventana);
	}
  if(bandera == 1)
	{
		Info("Faltan algunos datos");
	}

}

/*void
on_Modificar_cliente_show_lista        (GtkCList       *lista_clientes,
                                        gpointer         user_data)
{
	char sqlclientes[200] = "SELECT Cliente.id_cliente, Cliente.nombre, Lista.nombre FROM Cliente INNER JOIN Lista ON Cliente.id_lista =  Lista.id_lista";
	gchar *lista[3];
	int er;
	
	gtk_clist_clear(lista_clientes);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					gtk_clist_append(lista_clientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	printf("DesConecta en 2253 \n");
	mysql_close(&mysql);
	
	gtk_clist_select_row(lista_clientes, 0, 0);
}*/


/*void
on_clist_modificarcliente_select_row_codigo
                                        (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  int er;
  char lista[10];
  gchar *apuntalista; 
  gchar *id_cliente_lista;
   /////////consultassql///////////////////////////
  char sqllistas[300] = "SELECT id_cliente, nombre, domicilio, colonia, ciudad_estado, cp, telefono, telefono2, telefono3, rfc, entre_calles, contacto, id_lista, tipo_pago, tipo_impresion, vencimiento FROM Cliente WHERE id_cliente =";

  	apuntalista=lista;
  	gtk_clist_get_text(clist,row,0,&apuntalista);
	
	gtk_clist_get_text(clist,row,0,&id_cliente_lista);
	strcpy(id_cliente_modifica, id_cliente_lista);

	strcat(sqllistas, apuntalista);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqllistas);
		if(er != 0)
			printf("Error en el query de listas: %s\n", mysql_error(&mysql));
		else
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row2=mysql_fetch_row(res)))
				{
					modifica_cliente.id_cliente = row2[0];
					modifica_cliente.nombre = row2[1];
					printf("Cliente: %s\n",modifica_cliente.id_cliente);
					modifica_cliente.domicilio = row2[2];
					modifica_cliente.colonia = row2[3];
					modifica_cliente.ciudad = row2[4];
					modifica_cliente.CPostal = row2[5];
					modifica_cliente.telefono = row2[6];
					modifica_cliente.telefono2 = row2[7];
					modifica_cliente.telefono3 = row2[8];
					modifica_cliente.RFC = row2[9];	
					modifica_cliente.calles = row2[10];
					modifica_cliente.contacto = row2[11];
					obtiene_nombre_de_lista(modifica_cliente.id_cliente);
					modifica_cliente.nombre_de_lista = lista2;
					modifica_cliente.pago = row2[13];
					modifica_cliente.formato = row2[14];
					modifica_cliente.vencimiento = row2[15];
					strcpy(id_lista_modifica,row2[12]);
				}
			}
		}
	}
	printf("Des Conecta en 2304\n");
 mysql_close(&mysql);
}*/


/*void
on_clist_modificarcliente_select_row_nombre
                                        (GtkEntry        *entry_nombre,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_entry_set_text(entry_nombre, "");
  gtk_entry_set_text(entry_nombre, modifica_cliente.nombre);
printf("en clist_modif_del_row_nombre nombre: %s\n",modifica_cliente.nombre);
}


void
on_clist_modificarcliente_select_row_domicilio
                                        (GtkEntry        *entry_domicilio,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_domicilio, "");
 gtk_entry_set_text(entry_domicilio, modifica_cliente.domicilio);
}


void
on_clist_modificarcliente_select_row_colonia
                                        (GtkEntry        *entry_colonia,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_colonia, "");
 gtk_entry_set_text(entry_colonia, modifica_cliente.colonia);
}


void
on_clist_modificarcliente_select_row_ciudad_estado
                                        (GtkEntry        *entry_ciudad_estado,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_ciudad_estado, "");
 gtk_entry_set_text(entry_ciudad_estado, modifica_cliente.ciudad);
}

void
on_clist_modificarcliente_select_row_cp
                                        (GtkEntry        *entry_cp,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_cp, "");
 gtk_entry_set_text(entry_cp, modifica_cliente.CPostal);
}


void
on_clist_modificarcliente_select_row_telefono
                                        (GtkEntry        *entry_telefono,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_telefono, "");
 gtk_entry_set_text(entry_telefono, modifica_cliente.telefono);
}


void
on_clist_modificarcliente_select_row_telefono2
                                        (GtkEntry        *entry_telefono2,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_telefono2, "");
 gtk_entry_set_text(entry_telefono2, modifica_cliente.telefono2);
}


void
on_clist_modificarcliente_select_row_telefono3
                                        (GtkEntry        *entry_telefono3,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_telefono3, "");
 gtk_entry_set_text(entry_telefono3, modifica_cliente.telefono3);
}


void
on_clist_modificarcliente_select_row_rfc
                                        (GtkEntry        *entry_rfc,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_rfc, "");
 gtk_entry_set_text(entry_rfc, modifica_cliente.RFC);
}


void
on_clist_modificarcliente_select_row_entrecalles
                                        (GtkEntry        *entry_calles,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_calles, "");
 gtk_entry_set_text(entry_calles, modifica_cliente.calles);
}


void
on_clist_modificarcliente_select_row_contacto
                                        (GtkEntry        *entry_contacto,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_contacto, "");
 gtk_entry_set_text(entry_contacto, modifica_cliente.contacto);
}


void
on_clist_modificarcliente_select_row_pago
                                        (GtkEntry        *entry_pago,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_pago, modifica_cliente.pago);
}


void
on_clist_modificarcliente_select_row_formato
                                        (GtkEntry        *entry_formato,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_formato, modifica_cliente.formato);
}


void
on_clist_modificarcliente_select_row_lista
                                        (GtkEntry        *entry_lista,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_lista, "");
 gtk_entry_set_text(entry_lista, lista2);
}
*/

/*void
on_clist_modificarcliente_select_row_vencimiento
                                        (GtkEntry        *entry_vencimiento,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 gtk_entry_set_text(entry_vencimiento, "");
 gtk_entry_set_text(entry_vencimiento, modifica_cliente.vencimiento);
 
}
*/

/*void
on_Modificar_cliente_show_actualizar   (GtkCombo       *combo_modificarclientelista,
                                        gpointer         user_data)
{

  GList *items = NULL;
  int error;

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
    	}
        else
	 {
		error = mysql_query(&mysql,"SELECT nombre FROM Lista");
		if(error == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			
			{
			 while((row = mysql_fetch_row(res)))
				{
				        printf("Entra al while \n");
					items = g_list_append (items, row[0]);
					gtk_combo_set_popdown_strings (GTK_COMBO (combo_modificarclientelista), items);	
				}
			 }
			
			printf("SELECT FROM Lista\n");
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	 }
		
	printf("desConecta en 2500\n");
  mysql_close(&mysql);
  
}*/


/*void
on_entry_modificarclientebuscar_activate
                                        (GtkCList        *lista_clientes,
                                        gpointer         user_data)
{
        char sqlclientes[400];
	gchar *lista[3];
	gchar *criterio;
	int er;

	GtkEntry *txtbuscarcliente = user_data;

	criterio = gtk_editable_get_chars(GTK_EDITABLE(txtbuscarcliente),0,-1);
	criterio_bak = criterio;
	sprintf(sqlclientes, "SELECT Cliente.id_cliente, Cliente.nombre, Lista.nombre FROM Cliente LEFT JOIN Lista ON Cliente.id_lista =  Lista.id_lista WHERE Cliente.nombre LIKE \"%%%s%%\" OR Cliente.contacto LIKE \"%%%s%%\" OR Cliente.id_cliente = \"%s\" ORDER BY Cliente.id_cliente",criterio,criterio,criterio);

	gtk_clist_clear(lista_clientes);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		printf("%s\n",sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					gtk_clist_append(lista_clientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	mysql_close(&mysql);

	gtk_clist_select_row(lista_clientes, 0, 0);
	gtk_entry_set_text(txtbuscarcliente,""); 
}*/


/*void
on_btn_modificarcliente_clicked_nombre (GtkEntry       *entry_nombre,
                                        gpointer         user_data)
{
 modifica_cliente.nombre = gtk_editable_get_chars(GTK_EDITABLE(entry_nombre),0,-1);
}


void
on_btn_modificarcliente_clicked_domicilio
                                        (GtkEntry       *entry_domicilio,
                                        gpointer         user_data)
{
 modifica_cliente.domicilio = gtk_editable_get_chars(GTK_EDITABLE(entry_domicilio),0,-1);
}


void
on_btn_modificarcliente_clicked_colonia
                                        (GtkEntry       *entry_colonia,
                                        gpointer         user_data)
{
 modifica_cliente.colonia = gtk_editable_get_chars(GTK_EDITABLE(entry_colonia),0,-1);
}


void
on_btn_modificarcliente_clicked_ciudad_estado
                                        (GtkEntry       *entry_ciudad_estado,
                                        gpointer         user_data)
{
 modifica_cliente.ciudad = gtk_editable_get_chars(GTK_EDITABLE(entry_ciudad_estado),0,-1);
}


void
on_btn_modificarcliente_clicked_telefono
                                        (GtkEntry       *entry_telefono,
                                        gpointer         user_data)
{
 modifica_cliente.telefono = gtk_editable_get_chars(GTK_EDITABLE(entry_telefono),0,-1);
}


void
on_btn_modificarcliente_clicked_telefono2
                                        (GtkEntry       *entry_telefono2,
                                        gpointer         user_data)
{
 modifica_cliente.telefono2 = gtk_editable_get_chars(GTK_EDITABLE(entry_telefono2),0,-1);
}


void
on_btn_modificarcliente_clicked_telefono3
                                        (GtkEntry       *entry_telefono3,
                                        gpointer         user_data)
{
 modifica_cliente.telefono3 = gtk_editable_get_chars(GTK_EDITABLE(entry_telefono3),0,-1);
}


void
on_btn_modificarcliente_clicked_cp     (GtkEntry       *entry_cp,
                                        gpointer         user_data)
{
 modifica_cliente.CPostal = gtk_editable_get_chars(GTK_EDITABLE(entry_cp),0,-1);
}


void
on_btn_modificarcliente_clicked_rfc    (GtkEntry       *entry_rfc,
                                        gpointer         user_data)
{
 modifica_cliente.RFC = gtk_editable_get_chars(GTK_EDITABLE(entry_rfc),0,-1);
}


void
on_btn_modificarcliente_clicked_contacto
                                        (GtkEntry       *entry_contacto,
                                        gpointer         user_data)
{
 modifica_cliente.contacto = gtk_editable_get_chars(GTK_EDITABLE(entry_contacto),0,-1);
}


void
on_btn_modificarcliente_clicked_lista  (GtkEntry       *entry_lista,
                                        gpointer         user_data)
{
 modifica_cliente.nombre_de_lista = gtk_editable_get_chars(GTK_EDITABLE(entry_lista),0,-1);
}


void
on_btn_modificarcliente_clicked_pago   (GtkButton       *entry_pago,
                                        gpointer         user_data)
{
 modifica_cliente.pago = gtk_editable_get_chars(GTK_EDITABLE(entry_pago),0,-1);
}


void
on_btn_modificarcliente_clicked_formato
                                        (GtkEntry       *entry_formato,
                                        gpointer         user_data)
{
 modifica_cliente.formato = gtk_editable_get_chars(GTK_EDITABLE(entry_formato),0,-1);
}


void
on_btn_modificarcliente_clicked_calles (GtkEntry       *entry_calles,
                                        gpointer         user_data)
{
 modifica_cliente.calles = gtk_editable_get_chars(GTK_EDITABLE(entry_calles),0,-1);
}


void
on_btn_modificarcliente_clicked_vencimiento
                                        (GtkEntry       *entry_vencimiento,
                                        gpointer         user_data)
{
 modifica_cliente.vencimiento = gtk_editable_get_chars(GTK_EDITABLE(entry_vencimiento), 0, -1);
}*/


/*void
on_btn_modificarcliente_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *entry_modificarclientenombre;
  GtkWidget *entry_modificarclientedomicilio;
  GtkWidget *entry_modificarclientecolonia;
  GtkWidget *entry_modificarclienteciudad_estado;
  GtkWidget *entry_modificarclientecp;
  GtkWidget *entry_modificarclientetelefono;
  GtkWidget *entry_modificarclientetelefono2;
  GtkWidget *entry_modificarclientetelefono3;
  GtkWidget *entry_modificarclienterfc;
  GtkWidget *entry_modificarclienteentre_calles;
  GtkWidget *entry_modificarclientecontacto;
  GtkWidget *combo_entry_modificarclientelista;
  GtkWidget *combo_entry_modificarclientepago;
  GtkWidget *combo_entry_modificarclienteformato;
  GtkWidget *entry_modificarclientevencimiento;
  gchar *id_cliente, *nombre, *domicilio, *colonia, *ciudad_estado, *cp, *telefono, *telefono2, *telefono3, *rfc, *entre_calles, *contacto, *lista, *pago, *formato, *vencimiento;
  
  MYSQL_RES *res_cta_contable;
  MYSQL_ROW row_cta_contable;
  char sql_checa_contable[200];
  char sql_contable[]="SELECT MAX(cuenta_contable)+1 FROM Cliente";
  char cta_contable[20]="0";
  int bandera_rfc;

  char cadenilla[10000];

  entry_modificarclientenombre = lookup_widget(GTK_WIDGET(button), "entry_modificarclientenombre");
  entry_modificarclientedomicilio = lookup_widget(GTK_WIDGET(button), "entry_modificarclientedomicilio");
  entry_modificarclientecolonia = lookup_widget(GTK_WIDGET(button), "entry_modificarclientecolonia");
  entry_modificarclienteciudad_estado = lookup_widget(GTK_WIDGET(button), "entry_modificarclienteciudad_estado");
  entry_modificarclientecp = lookup_widget(GTK_WIDGET(button), "entry_modificarclientecp");
  entry_modificarclientetelefono = lookup_widget(GTK_WIDGET(button), "entry_modificarclientetelefono");
  entry_modificarclientetelefono2 = lookup_widget(GTK_WIDGET(button), "entry_modificarclientetelefono2");
  entry_modificarclientetelefono3 = lookup_widget(GTK_WIDGET(button), "entry_modificarclientetelefono3");
  entry_modificarclienterfc = lookup_widget(GTK_WIDGET(button), "entry_modificarclienterfc");
  entry_modificarclienteentre_calles = lookup_widget(GTK_WIDGET(button), "entry_modificarclienteentre_calles");
  entry_modificarclientecontacto = lookup_widget(GTK_WIDGET(button), "entry_modificarclientecontacto");
  combo_entry_modificarclientelista = lookup_widget(GTK_WIDGET(button), "combo_entry_modificarclientelista");
  combo_entry_modificarclientepago = lookup_widget(GTK_WIDGET(button), "combo_entry_modificarclientepago");
  combo_entry_modificarclienteformato = lookup_widget(GTK_WIDGET(button), "combo_entry_modificarclienteformato");
  entry_modificarclientevencimiento = lookup_widget(GTK_WIDGET(button), "entry_modificarclientevencimiento");

  nombre        = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientenombre), 0,-1);
  domicilio     = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientedomicilio), 0,-1);
  colonia       = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientecolonia), 0,-1);
  ciudad_estado = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclienteciudad_estado), 0,-1);
  cp            = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientecp), 0,-1);
  telefono      = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientetelefono), 0,-1);
  telefono2     = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientetelefono2), 0,-1);
  telefono3     = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientetelefono3), 0,-1);
  rfc           = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclienterfc), 0,-1);
  entre_calles  = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclienteentre_calles), 0,-1);
  contacto      = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientecontacto), 0,-1);
  lista         = gtk_editable_get_chars(GTK_EDITABLE(combo_entry_modificarclientelista), 0,-1);
  pago          = gtk_editable_get_chars(GTK_EDITABLE(combo_entry_modificarclientepago), 0,-1);
  formato       = gtk_editable_get_chars(GTK_EDITABLE(combo_entry_modificarclienteformato), 0,-1);
  vencimiento   = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientevencimiento), 0,-1);
  
  sprintf (sql_checa_contable,"SELECT cuenta_contable FROM Cliente Where id_cliente = %s",id_cliente_modifica);
  
  if (strlen(rfc) > 0)
	bandera_rfc = arreglar_rfc(rfc);
  
  
  if((strlen(nombre) == 0) || (strlen(telefono) == 0) ||(strlen(domicilio) == 0))
  	Err_Info("Faltan algunos datos");
  else if(bandera_rfc == 1 && strlen(rfc) > 0)
  {
	Err_Info("El RFC es incorrecto");
	gtk_widget_grab_focus(entry_modificarclienterfc);
  }
  else
  {
  	res_cta_contable = conecta_bd_3(sql_checa_contable);
  	if (res_cta_contable != NULL)
	{
		row_cta_contable = mysql_fetch_row (res_cta_contable);
		if(strcmp(row_cta_contable[0],"0") == 0 && bandera_rfc != 1 && strlen(rfc) > 0)
  		{
  			//Si no tiene cuenta contable genera una nueva
			res_cta_contable = conecta_bd_3(sql_contable);
			if (res_cta_contable != NULL)
			{
				row_cta_contable = mysql_fetch_row (res_cta_contable);
				printf ("%s \n",row_cta_contable[0]);
				strcpy(cta_contable,row_cta_contable[0]);
			}
  		}
	  	else
  		{
  			//Si tiene cuenta contable solo la reafirma
			printf ("%s \n",row_cta_contable[0]);
			strcpy(cta_contable,row_cta_contable[0]);
  		}

	  	sprintf(cadenilla, "UPDATE Cliente SET nombre=\"%s\", domicilio=\"%s\", colonia=\"%s\", ciudad_estado=\"%s\", cp=\"%s\", telefono=\"%s\", telefono2=\"%s\", telefono3=\"%s\", rfc=\"%s\", entre_calles=\"%s\", contacto=\"%s\", id_lista=%s, tipo_pago=\"%s\", tipo_impresion=\"%s\", vencimiento=%s, cuenta_contable = %s WHERE id_cliente=%s", nombre, domicilio, colonia, ciudad_estado, cp, telefono, telefono2, telefono3, rfc, entre_calles, contacto, id_lista_modifica, pago, formato, vencimiento,cta_contable, id_cliente_modifica);

  		printf ("MODIFICA = %s\n",cadenilla);
	  	if(conecta_bd() == -1)
  		{
	      	printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
  		}
  		else
  		{
	      		printf("Cadena: %s\n",cadenilla);
      			er = mysql_query(&mysql,cadenilla);
      			if (er!=0)
			printf("Ocurrio un error al insertar los datos del cliente: %s\n", mysql_error(&mysql));
	    	}
	    	mysql_close(&mysql);
	}
	else
	{
		Err_Info ("Ocurrio un error al momento de obtener la cuenta contable.");
	}
    }
}
*/
//////////////////////aqui//////////////////////////////////////////////
/*void
on_btn_modificarcliente_lista          (GtkCList       *clist_clientes,
                                        gpointer         user_data)
{
        char sqlclientes[400] = "SELECT Cliente.id_cliente, Cliente.nombre, Lista.nombre FROM Cliente INNER JOIN Lista ON Cliente.id_lista =  Lista.id_lista WHERE Cliente.nombre LIKE \"%";
	gchar *lista[3];
	gchar *criterio;
	int er;

	criterio = criterio_bak;

	if(criterio)
	{
	strcat(sqlclientes, criterio);
	strcat(sqlclientes, "%\"");
	strcat(sqlclientes, " OR Cliente.id_cliente = \"");
	strcat(sqlclientes, criterio);
	}
	strcat(sqlclientes, "\" ORDER BY Cliente.id_cliente");

	gtk_clist_clear(clist_clientes);
	printf("Conecta en 2904\n");
	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		printf("%s\n",sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					gtk_clist_append(clist_clientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	printf("Conecta en 2904\n");
	mysql_close(&mysql);

	gtk_clist_select_row(clist_clientes, 0, 0);

}*/

////////////////////////////////aqui/////////////////////////////////////
/*void obtiene_nombre_de_lista(gchar *id_del_cliente)
{
	char sqllista2[200] = "SELECT Lista.nombre FROM Lista INNER JOIN Cliente ON Cliente.id_lista =  Lista.id_lista WHERE Cliente.id_cliente = ";
	int er;


        strcat(sqllista2,id_del_cliente);
	/*if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql2));
    	}
	else
	{
		er = mysql_query(&mysql,sqllista2);
		if(er == 0)
		{
			res2 = mysql_store_result(&mysql);
			if(res2)
			{
				
				while((row3=mysql_fetch_row(res2)))
				{
					lista2 = row3[0];
					printf("lista: %s\n",lista2);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
//	}
	printf("desConecta en 2946\n");
//	mysql_close(&mysql2);
}*/










/******************************************************************************************
					JORGE
*****************************************************************************************/







void
on_txttelefono_activate                (GtkCList        *listaclientes,
                                        gpointer         user_data)
{
	char sqlclientes[240];
	gchar *lista[2];
	gchar *criterio;
	int er;
	int num_clientes=0;
	GtkEntry *txttelefono = user_data;

	criterio = gtk_editable_get_chars(GTK_EDITABLE(txttelefono),0,-1);

	sprintf(sqlclientes, "SELECT id_cliente, nombre FROM Cliente WHERE Cliente.bloqueado='n' AND (telefono = '%s' OR telefono2 = '%s' OR telefono3 = '%s')", criterio, criterio, criterio);

	//strcat(sqlclientes, criterio);

	printf("Cadena de busqueda: %s\n", sqlclientes);
	gtk_clist_clear(listaclientes);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				num_clientes = mysql_num_rows(res);
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					gtk_clist_append(listaclientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	mysql_close(&mysql);

	if(num_clientes > 0)
	{
		gtk_clist_select_row(listaclientes, 0, 0);
		gtk_entry_set_text(txttelefono,"");
		gtk_widget_show(create_V_pedidos());
	}
}


/*void
on_btn_cancelaralta_clicked            (GtkButton       *btn_cancelaralta,
                                        gpointer         user_data)
{

}

void
on_closebutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

}*/


///////////////////////////////////////
//Esta las hizo Jorge
//////////////////////////////////////
void
on_nuevo_cliente_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *Alta_Cliente;
	Alta_Cliente = create_Alta_clientes_caja();
	gtk_widget_show(Alta_Cliente);
}


void
on_txtbarcode_activate_cliente         (GtkCList        *listaclientes,
                                        gpointer         user_data)
{
	char lista[10];
	gchar *dato;

	dato = lista;

	if(fila_clientes != -1)
	{
		gtk_clist_get_text(listaclientes, fila_clientes, 0, &dato);
		strcpy(codigo_cliente, dato);
	}
	else
		Info("Seleccione a un cliente por favor");
}


// 0 si hay suficientes articulos en inventario
// 1 si no hay
// 2 si ocurrio un error
int checa_inventario(char *id_art, double cantidad);

int checa_inventario(char *id_art, double cantidad)
{
	MYSQL_RES *res_inv;
	MYSQL_ROW row_inv;
	char sql_inv[100];
	
	sprintf (sql_inv,"SELECT cantidad FROM Inventario WHERE id_articulo = %s",id_art);
	
	res_inv = conecta_bd_3 (sql_inv);
	
	if (res_inv)
	{
		if ( (row_inv = mysql_fetch_row(res_inv)) )
		{
			if ( (atof(row_inv[0]) - cantidad) > 0)
			{
				return 0;
			}
			else
				return 1;
		}
	}
	
	return 2;
}

void
on_txtbarcode_activate                 (GtkCList        *listaarticulos,
                                        gpointer         user_data)
{
	GdkColor color,color2, amarillo, rojo, negro, azul;
	GtkWidget *lbltotal;
	GtkWidget *lblkilos;
	GtkWidget *lblpiezas;
	GtkWidget *lbldescuento;
	GtkWidget *lbl_informacion;
	GtkWidget *chkcajapedido;
	GtkWidget *clist_pedido;
	GtkWidget *scrollpedido;
	GtkWidget *lblpedido;
	GtkWidget *listaclientes;
	GtkWidget *txtbuscarcliente;
	char markup[200];

	GtkEntry *txtbarcode = user_data;
	gchar *tipo_entrada;
	gchar *barcode;
	gchar *dato,*dato2,*dato3, *dato4, *dato5;
	gchar *lista[8], *lista_pedido[2];
	char preciobien[10]="";
	char kilos[4], gramos[5], cmonto[12], descuento[21], id_articulo[11];
	char sqlarticulo[100];
	char sqlinventario[100];
	char sqlprecio[750] = "SELECT Articulo_Lista.precio,Tarjeta_Cliente.descuento,Articulo_Lista.precio_minimo FROM Articulo_Lista INNER JOIN Articulo ON Articulo_Lista.id_articulo = Articulo.id_articulo INNER JOIN Lista ON Articulo_Lista.id_lista = Lista.id_lista INNER JOIN Cliente ON Lista.id_lista = Cliente.id_lista LEFT JOIN Tarjeta_Cliente ON Tarjeta_Cliente.id_cliente = Cliente.id_cliente AND Tarjeta_Cliente.id_articulo = Articulo.id_articulo WHERE Articulo.codigo = ";
	char sqlpedido[500], sqlpedido_articulo[300], sqlpedido_articulo_precio[500], sqlclientearticulo[200];
	int er,i;
	float cantidad, precio, monto;
	char ctotal[20];
	char cdescuento[20];
	gboolean estabien=FALSE, bloqueado=FALSE, porpieza=FALSE;


	printf("Se ingresó un código de producto\n");
	strcpy(kilos,"   ");
	strcpy(gramos,"    ");
	strcpy(cmonto,"          ");

	//COLOR
	gdk_color_parse("#e5c5c5", &color);
	gdk_color_parse("WHITE", &color2);
	gdk_color_parse("#ebf4ba", &amarillo);
	gdk_color_parse("#aa0a0a", &rojo);
	gdk_color_parse("#000000", &negro);
	gdk_color_parse("#155ff2", &azul);

	lbl_informacion = lookup_widget(GTK_WIDGET(listaarticulos), "lbl_informacion");
	lbltotal = lookup_widget(GTK_WIDGET(listaarticulos), "lbltotal");
	lblkilos = lookup_widget(GTK_WIDGET(listaarticulos), "lblkilos");
	lblpiezas = lookup_widget(GTK_WIDGET(listaarticulos), "lblpiezas");
	lbldescuento = lookup_widget(GTK_WIDGET(listaarticulos), "lbldescuento");
	chkcajapedido = lookup_widget(GTK_WIDGET(listaarticulos), "chkcajapedido");
	clist_pedido = lookup_widget(GTK_WIDGET(listaarticulos), "clist_pedido");
	scrollpedido = lookup_widget(GTK_WIDGET(listaarticulos), "scrollpedido");
	lblpedido = lookup_widget(GTK_WIDGET(listaarticulos), "lblpedido");
	txtbuscarcliente = lookup_widget(GTK_WIDGET(listaarticulos), "txtbuscarcliente");
	listaclientes = lookup_widget(GTK_WIDGET(listaarticulos), "listaclientes");

	tipo_entrada = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 0, -1);

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkcajapedido)))
	{
 	gtk_clist_optimal_column_width(GTK_CLIST(clist_pedido),0);
	//gtk_clist_set_column_width(GTK_CLIST(clist_pedido),0,80);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkcajapedido), FALSE);
	sprintf(sqlpedido, "SELECT Pedido.id_pedido AS FOLIO, Usuario.nombre AS CAPTURO, DATE_FORMAT(Pedido.fecha, \"%%d/%%m/%%Y\")  AS FECHA, CONCAT(Pedido.HoraEntrega, '         ', DATE_FORMAT(Pedido.FechaEntrega,\"%%d/%%m/%%Y\")) AS ENTREGAR, Pedido.id_venta, Pedido.id_cliente FROM Pedido INNER JOIN Usuario ON Pedido.id_usuario=Usuario.id_usuario INNER JOIN Cliente ON Pedido.id_cliente=Cliente.id_cliente WHERE Pedido.id_pedido=%s", tipo_entrada);
	sprintf(sqlpedido_articulo, "SELECT Articulo.id_articulo, Articulo.nombre, Pedido_Articulo.cantidad, Articulo.tipo FROM Articulo INNER JOIN Pedido_Articulo ON Pedido_Articulo.id_articulo=Articulo.id_articulo INNER JOIN Pedido ON Pedido_Articulo.id_pedido=Pedido.id_pedido WHERE Pedido.id_pedido=%s",tipo_entrada);
	//sprintf(sqlpedido_articulo, "SELECT Pedido_Articulo.id_articulo, Articulo.nombre, Articulo_Lista.precio, Tarjeta_Cliente.descuento, Pedido_Articulo.cantidad, Articulo.tipo, Pedido.id_venta FROM Pedido_Articulo INNER JOIN Articulo ON Pedido_Articulo.id_articulo=Articulo.id_articulo INNER JOIN Articulo_Lista ON Articulo.id_articulo = Articulo_Lista.id_articulo INNER JOIN Pedido ON Pedido_Articulo.id_pedido=Pedido.id_pedido LEFT JOIN Tarjeta_Cliente ON Pedido.id_cliente=Tarjeta_Cliente.id_cliente AND Tarjeta_Cliente.id_articulo=Articulo.id_articulo WHERE Pedido.id_pedido=%s ORDER BY Pedido_Articulo.id_pedidos",tipo_entrada);
	id_pedido = atoi(tipo_entrada);
	//printf("ID de Pedido = %d\n", id_pedido);
	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlpedido);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				field = mysql_fetch_fields(res);
				if((row=mysql_fetch_row(res)))
				{
				  gtk_clist_clear(GTK_CLIST(clist_pedido));
				  num_articulos_venta = 0;
				  total_venta = 0;
				  total_kilos = 0;
				  total_piezas = 0;
				  total_descuento = 0;
				  if(row[4]==NULL)
				  {
					for(i=0;i<4;i++)
					{
						lista_pedido[0] = field[i].name;
						lista_pedido[1] = row[i];
						gtk_clist_append(GTK_CLIST(clist_pedido), lista_pedido);
						//gtk_clist_set_foreground(GTK_CLIST(clist_pedido), i, &color);
					}
					strcpy(codigo_cliente, row[5]);
					estabien = TRUE;
				  }
				}
			}
		}
	}
	mysql_close(&mysql);

	if(estabien)
	{
		gtk_entry_set_text(GTK_ENTRY(txtbuscarcliente), codigo_cliente);
		gtk_signal_emit_by_name(GTK_OBJECT(txtbuscarcliente), "activate");

		gtk_widget_show(clist_pedido);
		gtk_widget_show(lblpedido);
		es_pedido = TRUE;

		if(conecta_bd() == 1)
		{
			er = mysql_query(&mysql, sqlpedido_articulo);
			if(er == 0)
			{
				res = mysql_store_result(&mysql);
				if(res)
				{
					gtk_clist_clear(listaarticulos);
					while((row=mysql_fetch_row(res)))
					{
						lista[0]=row[0];
						lista[1]=row[1];
						//lista[3]=row[4];
						lista[7]="10";
						lista[5]=row[3];
						strcpy(tipo_art,row[3]);
						cantidad = atof(row[2]);
						sprintf(peso, "%.3f", cantidad);
						lista[3] = peso;
						sprintf(sqlpedido_articulo_precio, "SELECT Articulo_Lista.precio,Tarjeta_Cliente.descuento FROM Articulo_Lista INNER JOIN Articulo ON Articulo_Lista.id_articulo = Articulo.id_articulo INNER JOIN Lista ON Articulo_Lista.id_lista = Lista.id_lista INNER JOIN Cliente ON Lista.id_lista = Cliente.id_lista LEFT JOIN Tarjeta_Cliente ON Tarjeta_Cliente.id_cliente = Cliente.id_cliente AND Tarjeta_Cliente.id_articulo = Articulo.id_articulo WHERE Articulo.id_articulo = %s AND Cliente.id_cliente = %s",row[0], codigo_cliente);
						er = mysql_query(&mysql, sqlpedido_articulo_precio);
						if(er == 0)
						{
							res2 = mysql_store_result(&mysql);
							if(res2)
							{
								if((row2 = mysql_fetch_row(res2)))
								{
									if(row2[1])
									{
										///descuento = row[3]
										//precio = row[2]
										precio = atof(row2[0])+atof(row2[1]);
										total_descuento = total_descuento + (atof(row2[0])*cantidad)-(precio*cantidad);
										sprintf(descuento, "%.2f", (atof(row2[0])*cantidad)-(precio*cantidad));
										lista[6]=descuento;
									}
									else
									{
										precio = atof(row2[0]);
										sprintf(descuento, " ");
										lista[6]=descuento;
									}
									sprintf(preciobien,"%.2f", precio);
									lista[2] = preciobien;
									if(strcmp(tipo_art,"peso") == 0)
									{
										total_kilos = total_kilos + cantidad;
										total_piezas = total_piezas + 1;
										sprintf(ctotal, "%.3f", total_kilos);
										sprintf(markup, "<b>%s</b>", ctotal);
										gtk_label_set_markup (GTK_LABEL(lblkilos), markup);
										sprintf(ctotal, "%.1f", total_piezas);
										sprintf(markup, "<b>%s</b>", ctotal);
										gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
									}
									else
									{
										total_piezas = total_piezas + cantidad;
										sprintf(ctotal, "%.1f", total_piezas);
										sprintf(markup, "<b>%s</b>", ctotal);
										gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
									}
									monto = precio * cantidad;
									total_venta = total_venta + monto;
									sprintf(cmonto, "%.2f", monto);
									lista[4] = cmonto;
									num_articulos_venta = gtk_clist_append(listaarticulos, lista);
									gtk_clist_set_background(listaarticulos, num_articulos_venta, &color);
									gtk_clist_moveto(listaarticulos, num_articulos_venta, 0, 0.0, 0.0);
								}
							}
						}
						else
							printf("Error: %s\n", mysql_error(&mysql));
					}
				}
			}
			else
			{
				strcpy(Errores,mysql_error(&mysql));
				Err_Info(Errores);
			}
		}
		else
			Err_Info("No me puedo conectar a la base de datos");
		mysql_close(&mysql);
	}
	else
	{
			Err_Info("Ese pedido ya esta registrado");
		//printf("El row= %s\n", row[6]);
	}
	}
	else
	{
	
	// PARA CARNES BECERRA
	if (strlen(tipo_entrada) <= 13)
  if(strlen(tipo_entrada) < 12)
  {
	if(yacodigo == FALSE)
	{
		if(strlen(tipo_entrada)>0)
		{
			strcpy(codigo, tipo_entrada);
			if(conecta_bd() == 1)
			{
				sprintf(sqlarticulo,"SELECT nombre FROM Articulo WHERE codigo = %s", codigo);
				printf("Cadena SQL: %s\n", sqlarticulo);
				er = mysql_query(&mysql, sqlarticulo);
				if(er == 0)
				{
					res = mysql_store_result(&mysql);
					if(res)
					{
						if(mysql_num_rows(res)>0)
						{
							row = mysql_fetch_row(res);
							sprintf(markup, "<span color=\"#e59c3e\"><b>%s</b></span>", row[0]);
							yacodigo = TRUE;
							strcpy (bascula,"10");
							/*gtk_widget_set_sensitive(caja,FALSE);
							if (winbascula  == NULL)
							{
								winbascula = create_win_bascula();
							}
							gtk_widget_show (winbascula);
							gtk_window_present (GTK_WINDOW(winbascula));*/
						}
						else
						{
							sprintf(markup, "<span color=\"red\">...no existe el codigo</span>");
							yacodigo = FALSE;
						}
						strcpy(peso, "0.000");
					}
					else
						printf("No hay resultado\n");
				}
				else
				{
					printf("Error: %s\n",mysql_error(&mysql));
					sprintf(markup, " ");
				}
			}
			else
				Err_Info("No me puedo conectar a la base de datos");
			mysql_close(&mysql);
			printf("EL Markup: %s\n", markup);
			gtk_label_set_markup(GTK_LABEL(lbl_informacion), markup);
		}
	}
	else
	{
		strcpy(peso, tipo_entrada);
		//PARA CARNES BECERRA LOS  KILOS DEBEN SER MAS DE 10
		//if(atof(peso) < 10 && limite_piezas)
			//sprintf(peso, "%.2f", 0.00);
		yacodigo = FALSE;
		gtk_label_set_markup(GTK_LABEL(lbl_informacion), "");
	}
  }
  else
  {
	gtk_label_set_markup(GTK_LABEL(lbl_informacion), "");
	yacodigo = FALSE;

	barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 1, 5);
	strcpy(codigo, barcode);
	/*barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 3, 5);
	strcat(codigo, barcode);
	*/

	barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 5, 7);
	strcpy(kilos, barcode);

	//barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 7, 10);
	//strcpy(gramos, barcode);
	  
	barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 10, 11);
	strcpy(bascula, barcode);
	
	/*if (atoi(bascula) < 6)
	{
		barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 7, 9);
	        strcpy(gramos, barcode);
	}
	else
	{*/
		barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode), 7, 10);
                strcpy(gramos, barcode);
	//
	strcpy(num_vendedor,"");

        barcode = gtk_editable_get_chars(GTK_EDITABLE(txtbarcode),10,12);
        strcpy(num_vendedor,barcode);

	printf (" *** El vendedor es: %s\n",num_vendedor);

	printf("Kilos: %s\n", kilos);
	printf("Gramos: %s\n", gramos);
	printf("Bascula: %s\n", bascula);
	printf("El codigo de articulo: %s\n", codigo);

	/* Si es por peso en las basculas TORREY pone la cantidad en gramos */
	if(conecta_bd() == 1)
        {
                                sprintf(sqlarticulo,"SELECT tipo FROM Articulo WHERE codigo = %s", codigo);
                                printf("Cadena SQL: %s\n", sqlarticulo);
                                er = mysql_query(&mysql, sqlarticulo);
                                if(er == 0)
                                {
                                        res = mysql_store_result(&mysql);
                                        if(res)
                                        {
                                                if(mysql_num_rows(res)>0)
                                                {
                                                        row = mysql_fetch_row(res);
                                                        if(strcmp(row[0],"pieza") == 0)
                                                                porpieza = TRUE;
                                                }
                                                strcpy(peso, "0.000");
                                        }
                                        else
                                                printf("No hay resultado\n");
                                }
                                else
                                {
                                        printf("Error: %s\n",mysql_error(&mysql));
                                        sprintf(markup, " ");
                                }
        }else{
		Err_Info("No me puedo conectar a la base de datos");
	}
	mysql_close(&mysql);

	if(porpieza){
	 	strcpy(peso,gramos);
	}else{
  		strcpy(peso, kilos);
		strcat(peso,".");
		strcat(peso, gramos);
	}
	porpieza = FALSE;
  }

	printf("El codigo es: %s\n", codigo);
	printf("El peso: %s\n", peso);

  if((yacodigo == FALSE) && (atof(peso) > 0))
  {
	sprintf(sqlarticulo,"SELECT id_articulo, nombre, tipo FROM Articulo WHERE codigo = %s", codigo);

	strcat(sqlprecio, codigo);
	strcat(sqlprecio, " AND Cliente.id_cliente = ");
	strcat(sqlprecio, codigo_cliente);

	cantidad = atof(peso);
	printf("La cantidad es: %.3f\n", cantidad);
	peso[0] = '\0';
	sprintf(peso, "%.3f", cantidad);
	cantidad = atof(peso);

	if(fila_clientes != -1)
	{
		if(conecta_bd() == -1)
    		{
      			printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    		}
		else
		{
			er = mysql_query(&mysql,sqlarticulo);
			if(er == 0)
			{
				res = mysql_store_result(&mysql);
				if(mysql_num_rows(res) > 0)
				{
					if((row = mysql_fetch_row(res)))
					{
						sprintf(sqlclientearticulo,"SELECT Cliente_Articulo.bloqueado FROM Cliente_Articulo WHERE Cliente_Articulo.id_cliente = %s AND Cliente_Articulo.id_articulo = %s", codigo_cliente, row[0]);
						printf("La consulta para bloqueos: %s\n", sqlclientearticulo);
						er = mysql_query(&mysql,sqlclientearticulo);
						if(er == 0)
						{
							if((res2 = mysql_store_result(&mysql)))
							{
								if((row2 = mysql_fetch_row(res2)))
								{
									
									if(strcmp(row2[0],"s")==0)
									{
										//mysql_close(&mysql);
										Err_Info("Este producto esta bloqueado para este cliente.");
										bloqueado = TRUE;
									}
								}
							}
						}
						
						lista[0] = row[0];
						lista[1] = row[1];
						lista[3] = peso;
						lista[5] = row[2];
						lista[7] = bascula;
					
					
						if (checa_inventario( row[0], cantidad)  == 1)
						{
							Info ("No hay suficientes articulos en inventario.");
						}
					
						strcpy(tipo_art, row[2]);
						er = mysql_query(&mysql,sqlprecio);
						if(bloqueado == FALSE)
							printf("No bloqueado\n");
						else
							printf("Bloqueado\n");
						if((er == 0) && (bloqueado == FALSE))
						{
							res = mysql_store_result(&mysql);
							if(mysql_num_rows(res) > 0)
							{
								/*if((es_factura == TRUE) && (num_articulos_venta >= 11) && (es_pedido == FALSE))*/
								if((num_articulos_venta >= 10) && (es_pedido == FALSE))
								Info("Ya no se puede registrar otro producto\nporque no hay espacio en la factura.\nFinalice la venta y haga otra.");
							else
							{
							  if(row = mysql_fetch_row(res))
							  {
								
							    if(row[1])
							    {
								precio = atof(row[0])+atof(row[1]);
								total_descuento = total_descuento + (atof(row[0])*cantidad)-(precio*cantidad);
								sprintf(descuento, "%.2f", (atof(row[0])*cantidad)-(precio*cantidad));
								lista[6]=descuento;
							    }
							    else
							    {
								precio = atof(row[0]);
								sprintf(descuento, " ");
								lista[6]=descuento;
							    }
							    if(precio >= atof(row[2]))
							    {
								sprintf(preciobien,"%.2f", precio);
								lista[2] = preciobien;

								if(es_pedido==FALSE)
								{
									printf("El pedido es FALSE\n");
									if(strcmp(tipo_art,"peso") == 0)
									{
										total_kilos = total_kilos + cantidad;
										total_piezas = total_piezas + 1;
										sprintf(ctotal, "%.3f", total_kilos);
										sprintf(markup, "<b>%s</b>", ctotal);
										gtk_label_set_markup (GTK_LABEL(lblkilos), markup);
										sprintf(ctotal, "%.1f", total_piezas);
										sprintf(markup, "<b>%s</b>", ctotal);
										gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
									}
									else
									{
										total_piezas = total_piezas + cantidad;
										sprintf(ctotal, "%.1f", total_piezas);
										sprintf(markup, "<b>%s</b>", ctotal);
										gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
									}
									monto = precio * cantidad;
									sprintf(cmonto, "%.2f", monto);
									monto = atof(cmonto);
									total_venta = total_venta + monto;
									//sprintf(cmonto, "%.2f", monto);
									lista[4] = cmonto;
									num_articulos_venta = gtk_clist_append (listaarticulos, lista);
									gtk_clist_moveto(listaarticulos, num_articulos_venta, 0, 0.0, 0.0);
								}
								else
								{
									for(i=0;i<=num_articulos_venta; i++)
									{
										gtk_clist_get_text(listaarticulos,i, 0, &dato); //id_articulo
										gtk_clist_get_text(listaarticulos,i, 3, &dato2); //cantidad
										gtk_clist_get_text(listaarticulos,i, 4, &dato3); //total
										gtk_clist_get_text(listaarticulos,i, 6, &dato4); //descuento
										gtk_clist_get_text(listaarticulos,i, 7, &dato5); //bascula

										if(strcmp(lista[0], dato) == 0)
										{
											if(ya_se_marco[i] == 0)
											{
												ya_se_marco[i] = atof(dato2);
												if(strcmp(tipo_art, "peso") == 0)
												{
													total_kilos = total_kilos - atof(dato2);
													total_kilos = total_kilos + cantidad;
													//total_piezas = total_piezas + 1;
													sprintf(ctotal, "%.3f", total_kilos);
													sprintf(markup, "<b>%s</b>", ctotal);
													gtk_label_set_markup (GTK_LABEL(lblkilos), markup);
													sprintf(ctotal, "%.1f", total_piezas);
													sprintf(markup, "<b>%s</b>", ctotal);
													gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
												}
												else
												{
													total_piezas = total_piezas - atof(dato2);
													total_piezas = total_piezas + cantidad;
													sprintf(ctotal, "%.1f", total_piezas);
													sprintf(markup, "<b>%s</b>", ctotal);
													gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
												}
												total_descuento = total_descuento - atof(dato4);
											}
											else
											{
												if(strcmp(tipo_art, "peso") == 0)
												{
													//total_kilos = total_kilos - atof(dato2);
													total_kilos = total_kilos + cantidad;
													//total_piezas = total_piezas + 1;
													sprintf(ctotal, "%.3f", total_kilos);
													sprintf(markup, "<b>%s</b>", ctotal);
													gtk_label_set_markup (GTK_LABEL(lblkilos), markup);
													sprintf(ctotal, "%.1f", total_piezas);
													sprintf(markup, "<b>%s</b>", ctotal);
													gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
												}
												else
												{
													//total_piezas = total_piezas - atof(dato2);
													total_piezas = total_piezas + cantidad;
													sprintf(ctotal, "%.1f", total_piezas);
													sprintf(markup, "<b>%s</b>", ctotal);
													gtk_label_set_markup (GTK_LABEL(lblpiezas), markup);
												}
												cantidad = cantidad + atof(dato2);
												sprintf(peso, "%.3f", cantidad);
												lista[3] = peso;
											}
											monto = precio * cantidad;
											sprintf(cmonto, "%.2f", monto);
											//printf("Monto: %f\n", monto);   //Importante!!!! no quitar esta linea (quien sabe por que pero con esto jala)
											total_venta = total_venta - atof(dato3);
											total_venta = total_venta + monto;
											lista[4] = cmonto;
											lista[7]=bascula;
											gtk_clist_set_text (listaarticulos, i, 2, lista[2]);
											gtk_clist_set_text (listaarticulos, i, 3, lista[3]);
											gtk_clist_set_text (listaarticulos, i, 4, lista[4]);
											gtk_clist_set_text (listaarticulos, i, 7, lista[7]);

											if(atof(lista[3]) >= ya_se_marco[i])
											{
												gtk_clist_set_background (listaarticulos, i, &color2);
												gtk_clist_set_foreground (listaarticulos, i, &azul);
											}
											else
											{
												gtk_clist_set_background (listaarticulos, i, &color2);
												gtk_clist_set_foreground (listaarticulos, i, &rojo);
											}
											gtk_clist_moveto(listaarticulos, i, 0, 0.0, 0.0);
										}
									}
								}
							    }
							    else
								Err_Info("El precio es menor al precio minimo.\nEs necesario ajustarlo para este cliente.");
							  }
							}
							strcpy(peso, "0.000");
						}
					}
					}
				}
			}
		}
		mysql_close(&mysql);
	}
  }
}
	sprintf(ctotal, "%.2f", total_venta);
	sprintf(markup, "<span size=\"18000\" color=\"blue\"><b>$ %s</b></span>", ctotal);
	gtk_label_set_markup (GTK_LABEL(lbltotal), markup);

	sprintf(cdescuento, "%.2f", total_descuento);
	sprintf(markup, "<span size=\"12000\"><b>$ %s</b></span>", cdescuento);
	gtk_label_set_markup (GTK_LABEL(lbldescuento), markup);

	gtk_entry_set_text(txtbarcode, "");
	printf("Fila: %d\n", num_articulos_venta);
}


void
on_btncajacancelar_clicked_listaarticulos
                                        (GtkCList       *listaarticulos,
                                        gpointer         user_data)
{
	GtkWidget *clist_pedido;
	GtkWidget *lblpedido;
	int i;

	clist_pedido = lookup_widget(GTK_WIDGET(listaarticulos),"clist_pedido");
	lblpedido = lookup_widget(GTK_WIDGET(listaarticulos),"lblpedido");

	gtk_clist_clear(listaarticulos);
	num_articulos_venta = 0;
	total_venta = 0;
	total_kilos = 0;
	total_piezas = 0;
	total_descuento = 0;
	yacodigo = FALSE;
	strcpy(codigo,"");
	for(i=0;i<50;i++)
		ya_se_marco[i]=0;

	gtk_widget_hide(clist_pedido);
	gtk_widget_hide(lblpedido);
	es_pedido = FALSE;
	//id_pedido = 0;
}


void
on_listaarticulos_select_row           (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	fila_articulo = row;
	columna_articulo = column;
}


void
on_listaarticulos_unselect_row         (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	fila_articulo = -1;
	columna_articulo = -1;
}



void
on_radiobtncajafactura_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	es_factura = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
}


void
on_radiobtncajapcredito_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	char sqlcredito[200];
	int er;
	GtkWidget *radiobtncajap;

	radiobtncajap = lookup_widget(caja, "radiobtncajap");

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)) == TRUE)
	{
		sprintf(sqlcredito, "SELECT vencimiento FROM Cliente WHERE id_cliente = %s AND vencimiento > 0", codigo_cliente);
		if(conecta_bd() == 1)
		{
			er = mysql_query(&mysql, sqlcredito);
			if(er==0)
			{
				res = mysql_store_result(&mysql);
				if(mysql_num_rows(res) <= 0)
				{
					//cliente_credito = 0,
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtncajap),TRUE);
					Err_Info("Este cliente no tiene autorizado credito");
				}
				else
					cliente_credito = 1;
			}
			else
			{
				sprintf(Errores,mysql_error(&mysql));
				Err_Info(Errores);
			}
		}
		mysql_close(&mysql);
	}
}




void
on_btnquitar_clicked_lista             (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *listaarticulos;
	GtkWidget *lblkilos;
	GtkWidget *lblpiezas;
	GtkWidget *lbltotal;
	GtkWidget *lbldescuento;
	GtkWidget *entry_descuento;

	GList *lista;
	int i;
	gchar *dato, *dato2, *dato3;
	float menos;
	char markup[100];
	char ctotal[10] = "";

	listaarticulos = lookup_widget(caja, "listaarticulos");
	lblkilos = lookup_widget(caja, "lblkilos");
	lblpiezas = lookup_widget(caja, "lblpiezas");
	lbltotal = lookup_widget(caja, "lbltotal");
	lbldescuento = lookup_widget(caja, "lbldescuento");

	lista = GTK_CLIST(listaarticulos)->row_list;
	for (i=0; lista; lista = g_list_next (lista),i++)
	{
		if ((GTK_CLIST_ROW (lista)->state == GTK_STATE_SELECTED))
		{
			gtk_clist_get_text(GTK_CLIST(listaarticulos), i, 4, &dato);
			menos = atof(dato);
			total_venta = total_venta - menos;
			gtk_clist_get_text(GTK_CLIST(listaarticulos), i, 3, &dato);
			menos = atof(dato);
			gtk_clist_get_text(GTK_CLIST(listaarticulos), i, 5, &dato2);
			if(strcmp(dato2, "peso")==0)
			{
				total_kilos = total_kilos - menos;
				total_piezas = total_piezas - 1;
			}
			else
				total_piezas = total_piezas - menos;
			gtk_clist_get_text(GTK_CLIST(listaarticulos), i, 6, &dato3);
			menos = atof(dato3);
			printf("El descuento: %.2f\n", menos);
			total_descuento = total_descuento - menos;

			gtk_clist_remove(GTK_CLIST(listaarticulos), i);
			lista = GTK_CLIST(listaarticulos)->row_list;
			lista = g_list_first(lista);

			num_articulos_venta--;
			i=0;
		}

		printf("El tamao: %d\n",g_list_length(lista));

		printf("Estoy en el ciclo\n");

		while(g_main_iteration(FALSE));
	}

	sprintf(ctotal, "%.2f", total_venta);
	sprintf(markup, "<span size=\"18000\" color=\"blue\"><b>$ %s</b></span>", ctotal);
	gtk_label_set_markup(GTK_LABEL(lbltotal), markup);

	printf("Total venta: %.2f\n", total_venta);
	printf("Total venta caracteres: %s\n", ctotal);
}



void
on_txtbuscarcliente_activate           (GtkCList        *listaclientes,
                                        gpointer         user_data)
{
	char sqlclientes[250];
	gchar *lista[2];
	gchar *criterio;
	int er;
	GtkEntry *txtbuscarcliente = user_data;

	criterio = gtk_editable_get_chars(GTK_EDITABLE(txtbuscarcliente),0,-1);
	sprintf(sqlclientes, "SELECT id_cliente, nombre FROM Cliente WHERE Cliente.bloqueado='n' AND (nombre LIKE '%%%s%%' OR contacto LIKE '%%%s%%' OR id_cliente=\"%s\") ORDER BY id_cliente", criterio, criterio, criterio);

	gtk_clist_clear(listaclientes);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					gtk_clist_append(listaclientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	mysql_close(&mysql);

	gtk_clist_select_row(listaclientes, 0, 0);
	gtk_entry_set_text(txtbuscarcliente,"");
}


void
on_caja_show                           (GtkCList       *listaclientes,
                                        gpointer         user_data)
{
	char titulo[100];
	char sqldatos[100];
	char nusuario[100];
	char ncaja[100];
	char fecha[20];
	char hora[20];
	
	GtkWidget *menu_item_facturar_varios_folios;
	GtkWidget *label16, *label17;
	
	char sqlclientes[200] = "SELECT id_cliente, nombre FROM Cliente WHERE bloqueado = 'n' ORDER BY id_cliente";
	gchar *lista[2];
	int er,i;
	
	if (checar_puerto_serie() == 1)
		puerto_serie = 2;
	else if(checar_puerto_serie() == -1)
		puerto_serie = -1;
	else
		puerto_serie = 0;
	
	win_cajon_abierto = create_win_cajon_abierto();
	
	sacarfecha (fecha,hora);
	
	printf ("FECHA %s\n",fecha);
	printf ("HORA  %s\n",hora);
	//checar_puerto_serie();
	
	// MODULOS
	char nombre [] = "modulo de modificacion de clientes";
	char nombre_modulo [] = "modifica_cliente";
	
	if ( g_module_supported () )
	{
		printf ("### Soporte para librerias dinamicas. ###\n");
		
		/*modulo_clientes = g_module_open ( g_module_build_path("./modifica_cliente",nombre_modulo) , G_MODULE_BIND_LAZY);
		if ( ! modulo_clientes )
		{
			g_warning (G_STRLOC ": No se pudo cargar el modulo `%s': %s\n", nombre, g_module_error ());
        	}
		else
		{
			g_module_symbol(modulo_clientes,"create_Modificar_cliente", (gpointer *)&funcion);
		}*/
	}
	
	
	caja = user_data;
	
	// FIN DE MODULOS
	if(puerto_serie != -1)
		g_timeout_add (1000,checa_cajon, NULL);
	
	menu_item_facturar_varios_folios = lookup_widget (caja,"facturar_varios_folios");
	
	gtk_widget_hide (menu_item_facturar_varios_folios);
	
	for(i=0;i<50;i++)
		ya_se_marco[i]=0;

	gtk_clist_clear(listaclientes);
	gtk_clist_set_column_justification(listaclientes, 0, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_resizeable (listaclientes, 0, FALSE);
	gtk_clist_set_column_resizeable (listaclientes, 1, FALSE);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		/*** OBTIENE EL NOMBRE DEL USUARIO Y LA CAJA ***/
		sprintf(sqldatos, "SELECT nombre, apellido FROM Usuario WHERE id_usuario = %s", id_sesion_usuario);
		er = mysql_query(&mysql,sqldatos);
		if(er ==0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
				{
					strcpy(nusuario, row[0]);
					strcat(nusuario, " ");
					strcat(nusuario, row[1]);
				}
				else
					printf("Error en la consulta del usuario:  %s\n", mysql_error(&mysql));
			}
			else
				printf("Error en la consulta del usuario:  %s\n", mysql_error(&mysql));
		}
		else
			Err_Info("No se pudo obtener el nombre del usuario");
		sprintf(sqldatos, "SELECT nombre FROM Caja WHERE id_caja = %s", id_sesion_caja);
		er = mysql_query(&mysql,sqldatos);
		if(er ==0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
				{
					strcpy(ncaja, row[0]);
				}
				else
					printf("Error en la consulta de la caja:  %s\n", mysql_error(&mysql));
			}
			else
				printf("Error en la consulta de la caja:  %s\n", mysql_error(&mysql));
		}
		else
			Err_Info("No se pudo obtener el nombre del usuario");

		sprintf(titulo, "CARNICERIA EL TORREON - %s (%s)", nusuario, ncaja);
		gtk_window_set_title(GTK_WINDOW(caja), titulo);


		/*** LLENA LA LISTA DE CLIENTES ***/
		er = mysql_query(&mysql,sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					gtk_clist_append(listaclientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	mysql_close(&mysql);

	gtk_clist_select_row(listaclientes, 0, 0);

	label16 = lookup_widget(caja, "label16");
	label17 = lookup_widget(caja, "label17");
	gtk_widget_hide(label16);
	gtk_widget_hide(label17);
	
	//checa_cajon();
}


void
on_caja_show_la                        (GtkCList       *listaarticulos,
                                        gpointer         user_data)
{
	gtk_clist_set_column_justification(listaarticulos, 0, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(listaarticulos, 2, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(listaarticulos, 3, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(listaarticulos, 4, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_resizeable (listaarticulos, 0, FALSE);
	gtk_clist_set_column_resizeable (listaarticulos, 1, FALSE);
	gtk_clist_set_column_resizeable (listaarticulos, 2, FALSE);
	gtk_clist_set_column_resizeable (listaarticulos, 3, FALSE);
	gtk_clist_set_column_resizeable (listaarticulos, 4, FALSE);
	gtk_clist_set_column_visibility (listaarticulos, 0, FALSE);
	gtk_clist_set_column_width(listaarticulos, 1, 190);
	gtk_clist_set_column_width(listaarticulos, 2, 50);
	gtk_clist_set_column_width(listaarticulos, 3, 60);
	gtk_clist_set_column_width(listaarticulos, 4, 70);
	gtk_clist_set_column_visibility (listaarticulos, 5, FALSE);
	gtk_clist_set_column_visibility (listaarticulos, 6, FALSE);
	gtk_clist_set_column_visibility (listaarticulos, 7, FALSE);
}


void
on_listaclientes_select_row            (GtkCList        *listaclientes,
                                        gint             fila,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	GtkWidget *radiobtncaja;
	GtkWidget *radiobtncajap;
	GtkWidget *radiobtncajafactura;
	GtkWidget *radiobtncajapcredito;
	GtkWidget *btncajacancelar;
	char lista[10];
	char sqldatoscliente[100] = "SELECT tipo_impresion, tipo_pago FROM Cliente WHERE id_cliente = ";
	gchar *dato;
	int er;
	char tipo[20], formato[20];

	dato = lista;

	radiobtncaja = lookup_widget(caja, "radiobtncaja");
	radiobtncajap = lookup_widget(caja, "radiobtncajap");
	radiobtncajafactura = lookup_widget(caja, "radiobtncajafactura");
	radiobtncajapcredito = lookup_widget(caja, "radiobtncajapcredito");
	btncajacancelar = lookup_widget(caja, "btncajacancelar");

	fila_clientes = fila;
	if(fila_clientes != -1)
	{
		gtk_clist_get_text(listaclientes, fila_clientes, 0, &dato);
		strcpy(codigo_cliente, dato);
		strcat(sqldatoscliente, codigo_cliente);

		if (conecta_bd() == -1)
    		{
      			printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
	    	}
		else
		{
			er = mysql_query(&mysql,sqldatoscliente);
			if (er == 0) // Si no hay error
			{
				res = mysql_store_result (&mysql);
				if (res)
				{
					if((row = mysql_fetch_row(res)))
					{
						strcpy(formato, row[0]);
						strcpy(tipo, row[1]);
					}
				}
			}
			else
			{
				printf("Error: %s", mysql_error(&mysql));
			} // Fin condicion de error
		}
		mysql_close (&mysql);
		if(strcmp("ticket", formato) == 0)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtncaja), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtncajafactura), TRUE);
		if(strcmp("contado", tipo) == 0)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtncajap), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobtncajapcredito), TRUE);
		gtk_signal_emit_by_name(GTK_OBJECT(btncajacancelar),"clicked");
	}
	if(event)
	{
		if(event->type == GDK_2BUTTON_PRESS)
		{
			if(bDatos_cliente == FALSE)
			{
				Datos_cliente = create_Datos_cliente();
				gtk_widget_show(Datos_cliente);
				bDatos_cliente = TRUE;
			}
			else
			{
				gtk_window_present(GTK_WINDOW(Datos_cliente));
			}
		}
	}
}

void
on_listaclientes_unselect_row          (GtkCList        *listaclientes,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	fila_clientes = -1;
	codigo_cliente[0] = '\0';
}


gboolean
on_Datos_cliente_delete_event          (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_destroy(widget);
  bDatos_cliente = FALSE;
  return FALSE;
}


void
on_txtbuscarcliente_activate_focus_barcode
                                        (GtkEntry        *txtbarcode,
                                        gpointer         user_data)
{
	gtk_widget_grab_focus(GTK_WIDGET(txtbarcode));
}


void
on_Datos_cliente_show                  (GtkCList       *lista_datos_cliente,
                                        gpointer         user_data)
{
	gchar *lista[2];
	char sqldatoscliente[1000] = "SELECT Cliente.nombre AS Nombre, Cliente.domicilio as Domicilio, Cliente.colonia AS Colonia, Cliente.entre_calles AS Entre, Cliente.ciudad_estado AS Ciudad, Cliente.cp AS CP, Cliente.telefono AS 'Telefono 1', Cliente.telefono2 AS 'Telefono 2', Cliente.telefono3 AS 'Telefono 3', Cliente.rfc AS RFC, Cliente.contacto AS Contacto, Lista.nombre As Lista, Cliente.tipo_pago AS Pago, Cliente.tipo_impresion AS Impresion, Cliente.vencimiento AS Vencimiento FROM Cliente LEFT JOIN Lista ON Cliente.id_lista = Lista.id_lista WHERE id_cliente = ";
	int i,cuantos,er;

	strcat(sqldatoscliente, codigo_cliente);

	gtk_clist_clear(lista_datos_cliente);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqldatoscliente);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				cuantos = mysql_num_fields(res);
				field = mysql_fetch_field(res);
				row = mysql_fetch_row(res);
				for(i=0; i<cuantos; i++)
				{	
					lista[0] = field[i].name;
					if(row[i])
						lista[1] = row[i];
					else
						lista[1] = "";
					gtk_clist_append(lista_datos_cliente, lista);
				}
			}
		}
		else
			printf("Error: %s .... %s\n", mysql_error(&mysql), sqldatoscliente);
	}
	mysql_close(&mysql);

}


void
on_nuevo_empleado_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *seguridad;

	strcpy(ventana, "nuevo_empleado");

	seguridad = create_Seguridad();
	gtk_widget_show(seguridad);
}


void
on_entry_seguridadusuario_activate     (GtkEntry        *entry,
                                        gpointer         user_data)
{
	gtk_widget_grab_focus(GTK_WIDGET(entry));
}


void
on_btn_seguridadaceptar_clicked        (GtkWidget       *Seguridad,
                                        gpointer         user_data)
{
	GtkWidget *ventana_abrir;
	GtkWidget *entry_seguridadcontrasena;
	char sqlseguridad[150];
	gchar *contrasena;

	entry_seguridadcontrasena = lookup_widget(Seguridad, "entry_seguridadcontrasena");
	contrasena = gtk_editable_get_chars(GTK_EDITABLE(entry_seguridadcontrasena),0,-1);

	sprintf(sqlseguridad, "SELECT * FROM Usuario WHERE password = \"%s\" AND tipo = 'admin'", contrasena);

	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlseguridad);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if(mysql_num_rows(res) > 0)
				{
					if(strcmp(ventana, "nuevo_empleado") == 0)
					{
						ventana_abrir = create_Alta_empleado();
						gtk_widget_show(ventana_abrir);
					}
				}
				else
					Info("Permiso denegado");
			}
		}
		else
		{
			sprintf(Errores,mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
	{
		sprintf(Errores,mysql_error(&mysql));
		Err_Info(Errores);
	}
	mysql_close(&mysql);
	gtk_widget_destroy(GTK_WIDGET(Seguridad));
}



void
on_modificar_datos_cliente_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *Modificar_cliente;
	Modificar_cliente = create_Modificar_cliente();
	gtk_widget_show( GTK_WIDGET(Modificar_cliente) );
	/*if ( modulo_clientes )
	{
		if(funcion)
		{
			gtk_widget_show( GTK_WIDGET(funcion()) );
		}
	}*/
}


void
on_Alta_clientes_caja_show             (GtkCombo       *CB_lista,
                                        gpointer         user_data)
{
	char sqllistas[100] = "SELECT nombre FROM Lista";
	int er;
  	GList *items = NULL;

	Alta_clientes_caja = user_data;

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqllistas);
		if(er != 0)
			printf("Error en el query de listas: %s\n", mysql_error(&mysql));
		else
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					items = g_list_append (items, row[0]);
				}
			}
		}
	}
	mysql_close(&mysql);

  	gtk_combo_set_popdown_strings (GTK_COMBO (CB_lista), items);

}


void sacarfecha(char cfecha[11], char hora[10])
{
	char fechatmp[5];
	time_t td;
	struct tm *fecha;
	char sql_fecha_hora[] = "select time_format(curtime(),\"%H\"),  time_format(curtime(),\"%i\"),  time_format(curtime(),\"%s\"),  date_format(curdate(),\"%Y\"), date_format(curdate(),\"%m\"), date_format(curdate(),\"%d\");";
	MYSQL_RES *res_fecha;
	
	//printf ("Fecha Hora %s\n",sql_fecha_hora);
	
	res_fecha = conecta_bd_3(sql_fecha_hora);
	strcpy (cfecha,"");
	strcpy (hora,"");
	
	if (res_fecha)
	{
		if (mysql_num_rows (res_fecha) > 0)
		{
			if (row = mysql_fetch_row (res_fecha))
			{
				strcat (hora,row[0]);
				strcat (hora,":");
				strcat (hora,row[1]);
				strcat (hora,":");
				strcat (hora,row[2]);
				
				strcat (cfecha,row[3]);
				strcat (cfecha,row[4]);
				strcat (cfecha,row[5]);
			}
			else
			{
				Err_Info ("Error al obtener el resultado de la fecha y hora.");
			}
		}
		else
		{
			Err_Info ("Error al obtener la fecha y hora del servidor.");
		}
	}
	else
	{
		Err_Info ("Error al obtener la fecha y hora del servidor.");
	}
	
	mysql_free_result (res_fecha);

	/*cfecha[0] = '\0';
	time(&td);
denuez:
	if((fecha = localtime(&td)))
	{
//HORA
		if((fecha->tm_hour) < 10)
			sprintf(fechatmp, "0%d", fecha->tm_hour);
		else
			sprintf(fechatmp, "%d", fecha->tm_hour);
		strcpy(hora, fechatmp);					//horas
		strcat(hora, ":");
		if((fecha->tm_min) < 10)
			sprintf(fechatmp, "0%d", fecha->tm_min);
		else
			sprintf(fechatmp, "%d", fecha->tm_min);
		strcat(hora,fechatmp);					//minutos
		strcat(hora, ":");

		if((fecha->tm_sec) < 10)
			sprintf(fechatmp, "0%d", fecha->tm_sec);
		else
			sprintf(fechatmp, "%d", fecha->tm_sec);
		strcat(hora,fechatmp);					//segundos

//FECHA
		sprintf(fechatmp, "%d", fecha->tm_year+1900);		//aÃ±o
		strcpy(cfecha,fechatmp);

		if((fecha->tm_mon+1) < 10)
			sprintf(fechatmp, "0%d", fecha->tm_mon+1);
		else
			sprintf(fechatmp, "%d", fecha->tm_mon+1);
		strcat(cfecha, fechatmp);				//mes


		if((fecha->tm_mday) < 10)
			sprintf(fechatmp, "0%d", fecha->tm_mday);
		else
			sprintf(fechatmp, "%d", fecha->tm_mday);
		strcat(cfecha, fechatmp);				//day
	}
	else
		goto denuez;*/
}


void
on_btnloginok_activate_usuario         (GtkEntry       *entry_usuario,
                                        gpointer         user_data)
{
	vcaja.login = gtk_editable_get_chars(GTK_EDITABLE(entry_usuario), 0,-1);
}


void
on_btnloginok_activate_contrasena      (GtkEntry       *entry_contrasena,
                                        gpointer         user_data)
{
	vcaja.contrasena = gtk_editable_get_chars(GTK_EDITABLE(entry_contrasena), 0,-1);
}


void
on_btnloginok_activate_ok              (GtkWindow       *Inicio_Sesion,
                                        gpointer         user_data)
{
	GtkWidget *caja;
	char sqlcomprobar[200] = "SELECT id_usuario, nombre, tipo FROM Usuario WHERE username='";
	char sqlyainiciada[100] = "SELECT id_caja FROM Sesion WHERE id_usuario = ";
	char sqlsesion[100];
	char sqlcaja[100] = "SELECT id_caja FROM Caja WHERE nombre = '";
	int er;
	char fecha[11], hora[10];
	char id_caja[5];
	int hayconfig=0;
	int val = 0;
	int ok = 0;

		caja = create_caja();
		sacarfecha(fecha,hora);

		printf("Despues de fecha... bien!\n");

		strcat(sqlcomprobar, vcaja.login);
		strcat(sqlcomprobar, "' AND password='");
		strcat(sqlcomprobar, vcaja.contrasena);
		strcat(sqlcomprobar, "'");
	
		strcpy(usuario_login,vcaja.login);
		strcpy(password_login, vcaja.contrasena);
	
	/*	strcat(sqlcaja, vcaja.caja);
		strcat(sqlcaja, "'");*/
	

		hayconfig = Obtiene_caja();

		printf("Si hay configuracion...!\n");
		if(hayconfig == 1)
		{

			if(conecta_bd() == -1)
	    		{
	      			printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
	    		}
			else
			{
				printf("Si se conecta a la base de datos...!\n");
				strcat(sqlcaja, vcdb2.caja);
				strcat(sqlcaja, "'");
				er = mysql_query(&mysql, sqlcaja);
				printf("Buscando la caja...\n");
				if(er == 0)
				{
					res = mysql_store_result(&mysql);
					if(res)
					{
						printf("Si hubo resultado...\n");
						if((row=mysql_fetch_row(res)))
						{
							strcpy(id_caja, row[0]);
							strcpy(id_sesion_caja, row[0]);
							printf("La caja:%s\n", row[0]);
							er = mysql_query(&mysql,sqlcomprobar);
							if (er == 0) // Si no hay error
							{
								res = mysql_store_result (&mysql);
								if (res)
								{
									if((row = mysql_fetch_row(res)))
									{
										strcat(sqlyainiciada, row[0]);
										printf("Sesion......\n");
										/*er = mysql_query(&mysql,sqlyainiciada);
										if (er == 0) // Si no hay error
										{
											res2 = mysql_store_result(&mysql);
											if(!(row2 = mysql_fetch_row(res2)))
											{*/
												printf("No hay sesion iniciada....\n");
												sprintf(sqlsesion, "INSERT INTO Sesion VALUES(NULL, %s, %s, %s, '%s', '', '192.168.0.3')", row[0], id_caja, fecha, hora);
												er = mysql_query(&mysql,sqlsesion);
												if (er == 0) // Si no hay error
												{
													gtk_widget_destroy(GTK_WIDGET(Inicio_Sesion));
													printf("Bienvenido %s\n",row[1]);
													strcpy(id_sesion_usuario, row[0]);
													strcpy(tipo_sesion_usuario, row[2]);
													ok = 1;
												}
												else
													printf("Error: %s", mysql_error(&mysql));
											/*}
											else
												printf("El usuario ya iniciÃ³ sesiÃ³n en la caja %s\n", row2[0]);
										}
										else
											printf("Error: %s", mysql_error(&mysql));*/
									}
									else
										printf("Usuario o ContraseÃ±a incorrecta...\n");
								}
							}
							else
							{
								printf("Error: %s", mysql_error(&mysql));
							} // Fin condicion de error
						}
					}
					else
						printf("Hay un error: %s\n", mysql_error(&mysql));
				}
				else
					printf("Hay un error: %s\n", mysql_error(&mysql));
		
			}	
			mysql_close (&mysql);
			if(ok == 1)
			{
				val = checar_corte();
				if(val == 1)
					gtk_widget_show(caja);
				else
				{
					Info("Tu ya hiciste el corte en esta caja...");
					//gtk_widget_show(caja);
					fin_de_sesion();
				}
			}
			else
				Err_Info("Error al iniciar sesion...");
		}
		else if(hayconfig == -1)
		{
			GtkWidget *Configuracion_BD;
			Configuracion_BD = create_Configuracion_BD();
			gtk_widget_show(Configuracion_BD);	
		}
}


int fin_de_sesion()
{
	char sqlfinsesion[120];
	char sqlultima[100] = "SELECT id_sesion FROM Sesion ORDER BY id_sesion DESC LIMIT 1";
	int er;
	char fecha[11], hora[10];
	int val=0;

	sacarfecha(fecha, hora);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql, sqlultima);
		if(er ==0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
				{
					sprintf(sqlfinsesion, "UPDATE Sesion SET hora_salida = '%s' WHERE id_usuario = %s AND id_sesion = %s", hora, id_sesion_usuario, row[0]);
					er = mysql_query(&mysql,sqlfinsesion);
					if (er == 0) // Si no hay error
					{
						val = 1;
						printf("Se finalizÃ³ la sesion. Adios...\n");
						gtk_main_quit();
					}
					else
					{
						val = -1;
						sprintf(Errores,mysql_error(&mysql));
						Err_Info(Errores);
					}
				}
			}
			else
			{
				sprintf(Errores,mysql_error(&mysql));
				Err_Info(Errores);
			}
		}
		else
		{
			sprintf(Errores,mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	mysql_close (&mysql);
	return val;
}

gboolean
on_caja_delete_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	g_module_close(modulo_clientes);
	fin_de_sesion();
	return FALSE;
}


void
on_btncajacancelar_clicked_total       (GtkLabel       *lbltotal,
                                        gpointer         user_data)
{
	char markup[100];

	sprintf(markup, "<span size=\"18000\" color=\"blue\"><b>$ 0.00</b></span>");
	gtk_label_set_markup(lbltotal, markup);
}


void
on_btncajacancelar_clicked_kilos       (GtkLabel       *lblkilos,
                                        gpointer         user_data)
{
	GtkWidget *lblpiezas;
	GtkWidget *lbldescuento;
	GtkWidget *lbl_informacion;

	lblpiezas = lookup_widget(GTK_WIDGET(lblkilos), "lblpiezas");
	lbldescuento = lookup_widget(GTK_WIDGET(lblkilos), "lbldescuento");
	lbl_informacion = lookup_widget(GTK_WIDGET(lblkilos), "lbl_informacion");

	gtk_label_set_markup(GTK_LABEL(lbl_informacion), " ");
	gtk_label_set_markup(lblkilos, "<b>0.000</b>");
	gtk_label_set_markup(GTK_LABEL(lblpiezas), "<b>0.0</b>");
	gtk_label_set_markup(GTK_LABEL(lbldescuento), "<span size=\"12000\"><b>$ 0.00</b></span>");
}

void
on_btnquitar_clicked_total             (GtkLabel       *lbltotal,
                                        gpointer         user_data)
{

}


void
on_btnquitar_clicked_total_kilos       (GtkLabel       *lblkilos,
                                        gpointer         user_data)
{
	GtkWidget *lblpiezas;
	GtkWidget *lbldescuento;
	char markup[60];
	char ctotal[10] = "";

	lblpiezas = lookup_widget(GTK_WIDGET(lblkilos), "lblpiezas");
	lbldescuento = lookup_widget(GTK_WIDGET(lblkilos), "lbldescuento");

	sprintf(ctotal, "%.3f", total_kilos);
	sprintf(markup, "<b>%s</b>", ctotal);
	gtk_label_set_markup(lblkilos, markup);

	sprintf(ctotal, "%.1f", total_piezas);
	sprintf(markup, "<b>%s</b>", ctotal);
	gtk_label_set_markup(GTK_LABEL(lblpiezas), markup);

	sprintf(ctotal, "%.2f", total_descuento);
	sprintf(markup, "<span size=\"12000\"><b>$ %s</b></span>", ctotal);
	gtk_label_set_markup(GTK_LABEL(lbldescuento), markup);
}



void
on_caja_show_lbltotal                  (GtkLabel       *lbltotal,
                                        gpointer         user_data)
{
  char marca[100];

  sprintf(marca, "<span size=\"18000\" color=\"blue\"><b>$ 0.00</b></span>");

  gtk_label_set_markup (GTK_LABEL (lbltotal), marca);
}


void
on_base_de_datos_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *Configuracion_BD;
	Configuracion_BD = create_Configuracion_BD();
	gtk_widget_show(Configuracion_BD);
}


void
on_btnbdaceptar_clicked_ip             (GtkEntry       *entry_IP,
                                        gpointer         user_data)
{
	vcdb.ip = gtk_editable_get_chars(GTK_EDITABLE(entry_IP),0,-1);
}


void
on_btnbdaceptar_clicked_usuario        (GtkEntry       *entry_Usuario,
                                        gpointer         user_data)
{
	vcdb.usuario = gtk_editable_get_chars(GTK_EDITABLE(entry_Usuario),0,-1);
}


void
on_btnbdaceptar_clicked_contrasena     (GtkEntry       *entry_Contrasena,
                                        gpointer         user_data)
{
	vcdb.contrasena = gtk_editable_get_chars(GTK_EDITABLE(entry_Contrasena),0,-1);
}


void
on_btnbdaceptar_clicked_bd             (GtkEntry       *entry_BD,
                                        gpointer         user_data)
{
	vcdb.bd = gtk_editable_get_chars(GTK_EDITABLE(entry_BD),0,-1);
}


void
on_btnbdaceptar_clicked_caja           (GtkEntry       *combo_entry_Caja,
                                        gpointer         user_data)
{
	vcdb.caja = gtk_editable_get_chars(GTK_EDITABLE(combo_entry_Caja),0,-1);
}


void
on_btnbdaceptar_clicked                (GtkWindow       *Configuracion_BD,
                                        gpointer         user_data)
{
	FILE *fconfiguracionbd;
	char linea[100]="";
	int sepudo,er;
	char sqlcaja[100];

	if((fconfiguracionbd = fopen("configuracionbd.dat","w")))
	{
/*		if(chdir("~/") == -1)
			printf("Error al cambiar de directorio...%s\n", strerror(errno));*/

		strcpy(linea, "ip ");
		strcat(linea, vcdb.ip);
		fprintf(fconfiguracionbd, "%s\n", linea);

		strcpy(linea, "usuario ");
		strcat(linea, vcdb.usuario);
		fprintf(fconfiguracionbd, "%s\n", linea);

		strcpy(linea, "contrasena ");
		strcat(linea, vcdb.contrasena);
		fprintf(fconfiguracionbd, "%s\n", linea);
	
		strcpy(linea, "bd ");
		strcat(linea, vcdb.bd);
		fprintf(fconfiguracionbd, "%s\n", linea);

		strcpy(linea, "caja ");
		strcat(linea, vcdb.caja);
		fprintf(fconfiguracionbd, "%s\n", linea);

		fclose(fconfiguracionbd);
	}
	else
		printf("Hubo un error al guardar la configuracion...\n");

	sepudo = conecta_bd();
	if(sepudo == 1)
	{
		sprintf(sqlcaja,"SELECT id_caja FROM Caja WHERE nombre = '%s'", vcdb.caja);
		er = mysql_query(&mysql, sqlcaja);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if(mysql_num_rows(res)>0)
				{
					if((row = mysql_fetch_row(res)))
					{
						strcpy(id_sesion_caja,row[0]);
						gtk_widget_destroy(GTK_WIDGET(Configuracion_BD));
					}
				}
				else
					Err_Info("No existe ninguna caja con ese nombre");
			}
			else
				Err_Info("Hay un error en la consulta de la caja");

		}
		else
			Err_Info("Hay un error en la consulta de la caja");
		mysql_close(&mysql);
	}
	else
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
}

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
		if(!(mysql_real_connect(&mysql, ip, usuario, contrasena, bd, 0, NULL, 0))){
			printf("Host: %s\nUsuario: %s\nContraseña: %s\nBase de datos: %s\n",ip,usuario,contrasena, bd);
			return -1;
		}
		else
			return 1;
	}
	else
	{
		printf("No se pudo abrir el archivo....\n");
		return -1;
	}
}


void
on_Configuracion_BD_show_ip            (GtkEntry       *entry_IP,
                                        gpointer         user_data)
{
	FILE *fconfiguracionbd;
	char tmp;
	char tipo[30]="";
	char dato[50]="";
	int i=0;
	int j=0;

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
					strcpy(vcdb2.ip,dato);
				if(strcmp(tipo,"usuario") == 0)
					strcpy(vcdb2.usuario,dato);
				if(strcmp(tipo,"contrasena") == 0)
					strcpy(vcdb2.contrasena,dato);
				if(strcmp(tipo,"bd") == 0)
					strcpy(vcdb2.bd,dato);
				if(strcmp(tipo,"caja") == 0)
					strcpy(vcdb2.caja,dato);
				strcpy(tipo,"");
				for(i=0;i<30;i++)
					tipo[i]='\0';
				i=0;
			}
		}
		fclose(fconfiguracionbd);
	}
	gtk_entry_set_text(entry_IP, vcdb2.ip);
}


void
on_Configuracion_BD_show_usuario       (GtkEntry       *entry_Usuario,
                                        gpointer         user_data)
{
	gtk_entry_set_text(entry_Usuario, vcdb2.usuario);
}


void
on_Configuracion_BD_show_Contrasena    (GtkEntry       *entry_Contrasena,
                                        gpointer         user_data)
{
	gtk_entry_set_text(entry_Contrasena, vcdb2.contrasena);
}


void
on_Configuracion_BD_show_bd            (GtkEntry       *entry_BD,
                                        gpointer         user_data)
{
	gtk_entry_set_text(entry_BD, vcdb2.bd);
}


void
on_Configuracion_BD_show_caja          (GtkCombo       *combo_Caja,
                                        gpointer         user_data)
{
	char sqlcajas[50] = "SELECT nombre FROM Caja";
	GList *cajas=NULL;
	int er;

	if(conecta_bd() == -1)
	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql, sqlcajas);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					cajas = g_list_append (cajas, row[0]);
				}
			}
		}
	}
	mysql_close(&mysql);

  	gtk_combo_set_popdown_strings (GTK_COMBO (combo_Caja), cajas);
}

void
on_Configuracion_BD_show_caja_cual     (GtkEntry       *combo_entry_Caja,
                                        gpointer         user_data)
{
	gtk_entry_set_text(combo_entry_Caja, vcdb2.caja);
}


int Obtiene_caja()
{
	FILE *fconfiguracionbd;
	char tmp;
	char tipo[30]="";
	char dato[50]="";
	int i=0;
	int j=0;

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

				if(strcmp(tipo,"caja") == 0)
					strcpy(vcdb2.caja,dato);
				strcpy(tipo,"");
				for(i=0;i<30;i++)
					tipo[i]='\0';
				i=0;
			}
		}
		fclose(fconfiguracionbd);
		return 1;
	}
	else
		return -1;
}

void
on_btncajaok_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	if((total_venta>0) && (cliente_credito==1))
		gtk_widget_show(create_Confirmar_venta());
}



void
on_Confirmar_venta_show                (GtkLabel       *lblcambio,
                                        gpointer         user_data)
{
//  GdkColor azul = { 0, 50000, 30000, 49 };
	GtkWidget *radiobtncajapcredito;
	GtkWidget *btnconfirmaraceptar;
	GtkWidget *entryconfirma_vendedor;
	GtkWidget *entry_Efectivo;
	char efectivo[20];
	char marca[100];

	vendedorbien = FALSE;
	sprintf(marca,"<span size=\"23000\" color=\"#FFAB00\"><b>$ 0.00</b></span>");

	gtk_label_set_markup (GTK_LABEL (lblcambio), marca);
	btnconfirmaraceptar = lookup_widget(GTK_WIDGET(lblcambio), "btnconfirmaraceptar");
	radiobtncajapcredito = lookup_widget(caja, "radiobtncajapcredito");
	entryconfirma_vendedor = lookup_widget(GTK_WIDGET(lblcambio), "entryconfirma_vendedor");
	entry_Efectivo = lookup_widget(GTK_WIDGET(lblcambio), "entry_Efectivo");
	
	//
	gtk_entry_set_text(GTK_ENTRY(entryconfirma_vendedor),num_vendedor);

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobtncajapcredito)) == TRUE)
	{
		gtk_widget_grab_focus(btnconfirmaraceptar);
		gtk_widget_set_sensitive(entry_Efectivo, FALSE);
	}
	else
	{
		sprintf(efectivo, "%.2f", total_venta);
		gtk_entry_set_text(GTK_ENTRY(entry_Efectivo), efectivo);
		gtk_widget_grab_focus(entryconfirma_vendedor);
	}
}


void
on_entry_Efectivo_change               (GtkLabel     *lblcambio,
                                        gpointer         entry_Efectivo)
{
	float cambio=0;
	float efectivo=0;
	char label_cambio[100]="";

	efectivo_venta = gtk_editable_get_chars(entry_Efectivo, 0, -1);

	efectivo = atof(efectivo_venta);
	cambio = efectivo-total_venta;
	sprintf(label_cambio, "<span size=\"23000\" color=\"#FFAB00\"><b>$ %.2f</b></span>", cambio);

	gtk_label_set_markup(lblcambio, label_cambio);

}


void
on_btnconfirmaraceptar_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
	GTimer *timer;
	gulong microsegundos=1000000;
	gdouble tiempo=0;

	GtkWidget *radiobtncaja;
	GtkWidget *radiobtncajafactura;
	GtkWidget *radiobtncajap;
	GtkWidget *radiobtncajapcredito;
	GtkWidget *lista_articulos_venta;
	GtkWidget *label_total_venta;		//APUNTADORES PARA LA LISTA DE VENTA Y EL LABEL DE TOTAL DE LA VENTA
	GtkWidget *label_total_kilos;
	GtkWidget *label_total_piezas;
	GtkWidget *label_total_descuento;
	GtkWidget *clist_pedido;
	GtkWidget *lblpedido;
	GtkWidget *txtbuscarcliente;
	GtkWidget *lblconfirma_efectivo;
	GtkWidget *lblconfirma_cambio;
	GtkWidget *entry_Efectivo;
	GtkWidget *Confirmar_venta;
	GtkWidget *entryconfirma_vendedor;
	GtkWidget *btnconfirmarcancelar;

	//GtkWidget *listaclientes;

	char sqlregistraventa[500];
	char sqlregistraarticulo[350];
	//char sqlsacarventa[100] = "SELECT id_venta FROM Venta GROUP BY id_venta DESC LIMIT 1";
	//char sqlfactura[200] = "INSERT INTO Venta_Factura (id_factura, fecha, hora, observacion) VALUES(NULL,";
	char sqlprefcliente[100];
	char sqlcredito[100];
	char sqlcliente[100];
	char sqlcomision[200];
	char sqlporcentaje[150];
	char sqlpedido[200];
	char sqltmp[100];
	char sqlinventario[200];
	char fecha[11], hora[10];
	char monto[14]="";
	char subtotal[15]="";
	char iva[15]="";
	char precio[15]="";
	char id_articulo[10]="";
	char cantidad[15]="";
	char bascula[5]="";
	int er=0;
	int i;
	char fila[10]="";
	gchar *dato;
	long int id_de_la_venta;
	char id_de_venta[20];
	char forma_pago[10]="";
	char formato[10]="";
	char markup[100];

	dato = fila;

	sacarfecha(fecha, hora);

	radiobtncaja = lookup_widget(caja, "radiobtncaja");
	radiobtncajafactura = lookup_widget(caja, "radiobtncajafactura");
	radiobtncajap = lookup_widget(caja, "radiobtncajap");
	radiobtncajapcredito = lookup_widget(caja, "radiobtncajapcredito");
	lista_articulos_venta = lookup_widget(caja, "listaarticulos");
	label_total_venta = lookup_widget(caja, "lbltotal");
	label_total_kilos = lookup_widget(caja, "lblkilos");
	label_total_piezas = lookup_widget(caja, "lblpiezas");
	label_total_descuento = lookup_widget(caja, "lbldescuento");
	clist_pedido = lookup_widget(caja, "clist_pedido");
	lblpedido = lookup_widget(caja, "lblpedido");
	txtbuscarcliente = lookup_widget(caja, "txtbuscarcliente");
	lblconfirma_efectivo = lookup_widget(GTK_WIDGET(button), "lblconfirma_efectivo");
	lblconfirma_cambio = lookup_widget(GTK_WIDGET(button), "lblconfirma_cambio");
	entry_Efectivo = lookup_widget(GTK_WIDGET(button), "entry_Efectivo");
	Confirmar_venta = lookup_widget(GTK_WIDGET(button), "Confirmar_venta");
	entryconfirma_vendedor = lookup_widget(GTK_WIDGET(button), "entryconfirma_vendedor");
	btnconfirmarcancelar = lookup_widget(GTK_WIDGET(button), "btnconfirmarcancelar");
	//listaclientes = lookup_widget(caja, "listaclientes");

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobtncaja)) == TRUE)
		strcpy(formato, "ticket");
	else
		strcpy(formato, "factura");

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobtncajap)) == TRUE)
		strcpy(forma_pago, "contado");
	else
		strcpy(forma_pago, "credito");

	if(efectivo_venta)
	{
		if(atof(efectivo_venta) < total_venta)
			sprintf(efectivo_venta,"%.2f", total_venta);
	}

	if (guarda_iva)
	{
		sprintf(monto, "%.2f", total_venta);
		sprintf(subtotal, "%.2f", total_venta/1.15);
		sprintf(iva, "%.2f", (total_venta/1.15)*0.15);
		printf("Monto: %s\n", monto);
		printf("Subtotal: %s\n", subtotal);
		printf("IVA: %s\n", iva);
	}
	else
	{
		sprintf(monto, "%.2f", total_venta);
		sprintf(subtotal, "%.2f", total_venta);
		sprintf(iva, "0.00");
		printf("Monto: %s\n", monto);
		printf("Subtotal: %s\n", subtotal);
		printf("IVA: %s\n", iva);
	}

	sprintf(sqlregistraventa, "INSERT INTO Venta (id_venta, fecha, hora, monto, subtotal, iva, id_usuario, id_cliente, tipo, id_caja, descuento, id_empleado) VALUES(NULL, %s, '%s', %s, %s, %s, %s, %s, '%s', %s, %.2f, %s)", fecha, hora, monto, subtotal, iva, id_sesion_usuario, codigo_cliente, forma_pago, id_sesion_caja, total_descuento, id_empleado);
	printf("Registra Venta: %s\n", sqlregistraventa);

	/*strcat(sqlfactura, fecha);
	strcat(sqlfactura, ",'");
	strcat(sqlfactura, hora);
	strcat(sqlfactura, "',\"\")");*/

	printf("La fecha es: %s\nLa Hora: %s\n", fecha, hora);
	printf("La sentencia SQL: %s\n", sqlregistraventa);

	if((total_venta > 0)&&(cliente_credito==1)&&vendedorbien==TRUE)
	{
		if (conecta_bd() == -1)
    		{
      			Err_Info("No me puedo conectar a la base de datos.");
    		}
		else
		{
			er = mysql_query(&mysql,sqlregistraventa);
			if (er == 0) // Si no hay error
			{
				printf("Se registro la venta\n");
				id_de_la_venta = mysql_insert_id(&mysql);
				printf("Numero de articulos en la venta: %d\n", num_articulos_venta);
				printf("Numero de la venta: %ld\n", id_de_la_venta);
				for(i=0; i<= num_articulos_venta; i++)
				{
					sprintf(sqlregistraarticulo,"INSERT INTO Venta_Articulo (id_venta_articulo, id_venta, id_articulo, cantidad, id_bascula, precio, monto, subtotal, iva) VALUES(NULL, %ld, ", id_de_la_venta);
					gtk_clist_get_text(GTK_CLIST(lista_articulos_venta), i, 0, &dato);
					strcat(sqlregistraarticulo, dato);
					strcat(sqlregistraarticulo, ",");
					strcpy(id_articulo, dato);
					gtk_clist_get_text(GTK_CLIST(lista_articulos_venta), i, 3, &dato);
					strcat(sqlregistraarticulo, dato);
					strcat(sqlregistraarticulo, ",");
					strcpy(cantidad, dato);
					gtk_clist_get_text(GTK_CLIST(lista_articulos_venta), i, 7, &dato);
					strcat(sqlregistraarticulo, dato);
					strcat(sqlregistraarticulo, ",");
					strcpy(bascula, dato);
					gtk_clist_get_text(GTK_CLIST(lista_articulos_venta), i, 2, &dato);
					
					if (guarda_iva)
					{
						sprintf(precio, "%.2f", atof(dato)/1.15);
						strcat(sqlregistraarticulo, precio);
					}
					else
					{
						sprintf(precio, "%.2f", atof(dato));
						strcat(sqlregistraarticulo, precio);
					}
					strcat(sqlregistraarticulo, ",");
					gtk_clist_get_text(GTK_CLIST(lista_articulos_venta), i, 4, &dato);
					strcat(sqlregistraarticulo, dato);
					strcat(sqlregistraarticulo, ",");
					
					if (guarda_iva)
					{
						sprintf(subtotal, "%.2f", atof(dato)/1.15);
						strcat(sqlregistraarticulo, subtotal);
						strcat(sqlregistraarticulo, ",");
						sprintf(iva, "%.2f", (atof(dato)/1.15)*0.15);
						strcat(sqlregistraarticulo, iva);
					}
					else
					{
						sprintf(subtotal, "%.2f", atof(dato));
						strcat(sqlregistraarticulo, subtotal);
						strcat(sqlregistraarticulo, ",");
						sprintf(iva, "0.00");
						strcat(sqlregistraarticulo, iva);
					}
					
					strcat(sqlregistraarticulo, ")");
					er = mysql_query(&mysql,sqlregistraarticulo);
					printf("La sentencia: %s\n", sqlregistraarticulo);
					sprintf(sqlinventario, "UPDATE Inventario SET cantidad = cantidad - %s WHERE id_articulo = %s", cantidad, id_articulo);
					er = mysql_query(&mysql,sqlinventario);
					printf("La sentencia: %s\n", sqlinventario);
				}
				/*sprintf(sqltmp, "INSERT INTO Tabla_TMP VALUES(NULL, %d, 'venta', %s)", id_de_la_venta, id_sesion_caja);
				er = mysql_query(&mysql, sqltmp);
				if(er == 0)
					printf("Se registro la venta en la tabla TMP\n");
				else
					printf("Error al registrar la venta en la tabla temporal: %s\n", mysql_error(&mysql));*/
				
				sprintf(sqlprefcliente, "UPDATE Cliente SET tipo_pago='%s', tipo_impresion='%s' WHERE id_cliente=%s", forma_pago, formato, codigo_cliente);
				er = mysql_query(&mysql, sqlprefcliente);
				if(er != 0)
				{
					sprintf(Errores, "%s",mysql_error(&mysql));
					Err_Info(Errores);
				}
				num_articulos_venta = -1;
				total_descuento = 0;

				/*** SI LA VENTA ES DE CREDITO ***/
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobtncajapcredito)) == TRUE)
				{
					printf("\n\nVenta de Credito\n\n Id Venta: %ld\n",id_de_la_venta);
					sprintf(sqlcliente, "SELECT vencimiento FROM Cliente WHERE id_cliente = %s AND vencimiento > 0", codigo_cliente);
					er = mysql_query(&mysql, sqlcliente);
					if(er ==0)
					{
						res = mysql_store_result(&mysql);
						if(res)
						{
						  if (mysql_num_rows(res) > 0)
						  {
							if((row = mysql_fetch_row(res)))
							{
								printf("Vencimiento: %s\n", row[0]);
								sprintf(sqlcredito, "UPDATE Venta SET vencimiento = ADDDATE(CURDATE(), INTERVAL %s DAY) WHERE id_venta = %ld", row[0], id_de_la_venta);
								er = mysql_query(&mysql, sqlcredito);
								if(er == 0)
									printf("Se registrÃ³ el vencimiento del credito...\n");
								else
									printf("Error al registrar el vencimiento: %s\n", mysql_error(&mysql));
							}
							else
								printf("Error: %s\n", mysql_error(&mysql));
							mysql_free_result(res);
						  }
						  else
						  	Err_Info("El cliente no tiene\nautorizado credito.");
						}
						else
							printf("Error en el vencimiento: %s\n", mysql_error(&mysql));
					}
					else
						printf("Error al consultar el vencimiento del cliente: %s\n",mysql_error(&mysql));
				}

				/*** SE GUARDAN LAS COMISIONES **/
				sprintf(sqlporcentaje, "SELECT id_empleado, porcentaje FROM Comision WHERE id_cliente = %s", codigo_cliente);
				er = mysql_query(&mysql, sqlporcentaje);
				if(er == 0)
				{
					res = mysql_store_result(&mysql);
					if(res)
					{
						if(mysql_num_rows(res) > 0)
						{
							if((row = mysql_fetch_row(res)))
							{
								sprintf(sqlcomision, "INSERT INTO Comisiones VALUES(NULL, %s, %ld, %s, %.2f, 'n')", row[0], id_de_la_venta, fecha, atof(monto)*(atof(row[1])/100));
								er = mysql_query(&mysql, sqlcomision);
								if(er == 0)
									printf("Se registrÃ³ la comisiÃ³n...\n");
								else
									printf("Error al registrar la comisiÃ³n: %s\n", mysql_error(&mysql));
							}
						}
						else
							printf("No hay comisiones... \n");
						mysql_free_result(res);
					}
					else
						printf("Error: %s\n", mysql_error(&mysql));
				}
				else
					printf("Error: %s\n", mysql_error(&mysql));
				
				printf("El ID del pedido=%d\n",id_pedido);
				if(id_pedido > 0)
				{
					sprintf(sqlpedido, "UPDATE Pedido SET id_venta = %ld WHERE id_pedido = %d", id_de_la_venta, id_pedido);
					er = mysql_query(&mysql,sqlpedido);
					if(er != 0)
						Err_Info("Ocurrio un error al registrar el pedido");
					id_pedido = 0;
				}
				es_pedido = FALSE;
			}
			else
			{
				printf("Error: %s\n", mysql_error(&mysql));
			} // Fin condicion de error
		}

		mysql_close (&mysql);

		printf("Hasta aquÃ­ si...\n");
		sprintf(id_de_venta,"%ld",id_de_la_venta);
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobtncajap)) == TRUE)
			imprimirticket(id_de_venta, forma_pago, atof(efectivo_venta));
		else
			imprimirticket(id_de_venta, forma_pago, 0.00);

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobtncajafactura)) == TRUE)
			gtk_widget_show(create_dialog_facturar());

		gtk_clist_clear(GTK_CLIST(lista_articulos_venta));

		sprintf(markup, "<span size=\"18000\" color=\"blue\"><b>$ 0.00</b></span>");
		gtk_label_set_markup(GTK_LABEL(label_total_venta), markup);
		gtk_label_set_markup(GTK_LABEL(label_total_kilos), "<b>0.000</b>");
		gtk_label_set_markup(GTK_LABEL(label_total_piezas), "<b>0.0</b>");
		sprintf(markup, "<span size=\"12000\"><b>$ 0.00</b></span>");
		gtk_label_set_markup(GTK_LABEL(label_total_descuento), markup);

		gtk_widget_hide(lblpedido);
		gtk_widget_hide(clist_pedido);

		total_venta = 0;
		total_kilos = 0;
		total_piezas = 0;
		total_descuento = 0;
		for(i=0;i<50;i++)
			ya_se_marco[i]=0;

		timer = g_timer_new();
		printf("Hasta aqui si...\n");

		gtk_entry_set_text(GTK_ENTRY(txtbuscarcliente), "");
		gtk_signal_emit_by_name(GTK_OBJECT(txtbuscarcliente), "activate");
		/*gtk_widget_hide(lblconfirma_efectivo);
		gtk_widget_hide(lblconfirma_cambio);
		gtk_widget_hide(entry_Efectivo);
		gtk_widget_hide(btnconfirmarcancelar);*/
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobtncajap)) == TRUE)
		{
			while(tiempo < 9)
			{
				tiempo = g_timer_elapsed(timer, &microsegundos);
				while(g_main_iteration(FALSE));
			}
		}
		gtk_widget_destroy(Confirmar_venta);
	}
	else if (vendedorbien == FALSE)
	{
		gtk_widget_grab_focus(GTK_WIDGET(entryconfirma_vendedor));
		Err_Info("Es necesario el número de vendedor");
	}
}


void
on_okbtn_facturar_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *Facturar=create_Facturar();
	gtk_widget_show(Facturar);
}



void
on_facturar_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *Facturar=create_Facturar();
	gtk_widget_show(Facturar);
}


void
on_Facturar_show                       (GtkWidget       *widget,
                                        gpointer         user_data)
{
	GtkWidget *entry_Nofactura;
	char sqlfactura[200];
	char num_factura[20];
	int er;

	entry_Nofactura = lookup_widget(widget, "entry_Nofactura");

	if(conecta_bd() == 1)
	{
		sprintf(sqlfactura, "SELECT (num_factura+1) AS Factura FROM Venta ORDER BY num_factura DESC LIMIT 1");
		er = mysql_query(&mysql, sqlfactura);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if(mysql_num_rows(res) > 0)
				{
					if((row = mysql_fetch_row(res)))
					{
						strcpy(num_factura,row[0]);
						gtk_entry_set_text(GTK_ENTRY(entry_Nofactura), num_factura);
					}
				}
				else
					Info("Primera factura");
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
		Err_Info("No me puedo conectar a la base de datos");
	mysql_close(&mysql);
}


void
on_entry_Nofolio_activate              (GtkEntry        *entry,
                                        gpointer         user_data)
{
	GtkWidget *entry_facturarcliente;
	char sqlcliente[200];
	gchar *folio;
	char cliente[10];

	entry_facturarcliente = lookup_widget(GTK_WIDGET(entry),"entry_facturarcliente");

	folio = gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
	sprintf(sqlcliente, "SELECT Venta.id_cliente FROM Venta WHERE Venta.id_venta=%s", folio);
	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlcliente);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if(mysql_num_rows(res)>0)
				{
					if((row=mysql_fetch_row(res)))
					{
						strcpy(cliente, row[0]);
						gtk_entry_set_text(GTK_ENTRY(entry_facturarcliente),cliente);
					}
				}
			}
		}
		else
		{
			sprintf(Errores, "Error: %s", mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
	{
		sprintf(Errores, "No me puedo conectar a la base de datos:\n%s", mysql_error(&mysql));
		Err_Info(Errores);
	}
	mysql_close(&mysql);
}


void
on_btnfacturarok_clicked_ok            (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_Nofolio;
	GtkWidget *entry_Nofactura;
	GtkWidget *entry_facturarcliente;
	GtkWidget *textview_Observaciones;
	GtkTextBuffer *buff;
	GtkTextIter start, end;
	char sqlfactura[300];
	char sqlrelacion[150];
	//char sqlsacarfactura[100] = "SELECT id_factura FROM Venta_Factura GROUP BY id_factura DESC LIMIT 1";
	char sqlnumerofactura[200];
	char sqlarticulos[150];
	char sqlcliente[100];
	char sqlrfc[100];
	char fecha[11], hora[10];
	int er;
	int tmp_idfactura=0;
	int ya = 0;
	int rfcok = 0;
	gchar *observaciones, *folio, *factura, *cliente;

	entry_Nofolio = lookup_widget(GTK_WIDGET(button),"entry_Nofolio");
	entry_Nofactura = lookup_widget(GTK_WIDGET(button),"entry_Nofactura");
	entry_facturarcliente = lookup_widget(GTK_WIDGET(button),"entry_facturarcliente");
	textview_Observaciones = lookup_widget(GTK_WIDGET(button),"textview_Observaciones");

	buff = gtk_text_view_get_buffer (GTK_TEXT_VIEW(textview_Observaciones));

	gtk_text_buffer_get_iter_at_offset (buff, &start, 0);
	gtk_text_buffer_get_iter_at_offset (buff, &end, -1);

	observaciones = gtk_text_buffer_get_text(buff,&start,&end,TRUE);
	folio = gtk_editable_get_chars(GTK_EDITABLE(entry_Nofolio), 0, -1);
	factura = gtk_editable_get_chars(GTK_EDITABLE(entry_Nofactura), 0, -1);
	cliente = gtk_editable_get_chars(GTK_EDITABLE(entry_facturarcliente), 0, -1);

	if(!(conecta_bd() == -1))
	{
		sprintf(sqlnumerofactura, "SELECT * FROM Venta WHERE num_factura = %s OR (Venta.id_venta = %s AND Venta.num_factura > 0)", factura,folio);
		er = mysql_query(&mysql, sqlnumerofactura);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((mysql_num_rows(res) > 0) || (strlen(factura) < 1) || (strlen(folio) < 1))
					ya = 1;

			}
		}
	}
	mysql_close(&mysql);
 if(ya == 0)
 {
	sacarfecha(fecha, hora);

	sprintf(sqlrfc, "SELECT rfc FROM Cliente WHERE id_cliente = %s", cliente);

	if(conecta_bd_2(&mysql2) == -1)
	{
		printf("Error al conectar a la base de datos: %s\n", mysql_error(&mysql2));
	}
	else
	{
	  er = mysql_query(&mysql2, sqlrfc);
	  if(er == 0)
	  {
		res = mysql_store_result(&mysql2);
		if(res)
		{
			if(mysql_num_rows(res) > 0)
			{
				if((row = mysql_fetch_row(res)))
				{
					if(strlen(row[0]) > 0)
						rfcok = 1;
					else
						rfcok = 0;
				}
			}
		}
	  }
	  else
		printf("Ocurri un error: %s\n", mysql_error(&mysql2));
	  if(rfcok == 1)
	  {
		sprintf(sqlfactura, "UPDATE Venta SET num_factura=%s, fecha_factura='%s', hora_factura='%s', observacion='%s' WHERE id_venta=%s", factura, fecha, hora, observaciones,folio);
		er = mysql_query(&mysql2, sqlfactura);
		if(er == 0)
		{
			/*tmp_idfactura = mysql_insert_id(&mysql2);
			sprintf(sqlrelacion, "INSERT INTO Venta_Factura_Relacion VALUES(NULL, %s, %d, %s)", folio, tmp_idfactura, factura);
			er = mysql_query(&mysql2, sqlrelacion);
			if(er == 0)
			{*/
				sprintf(sqlcliente, "UPDATE Venta SET id_cliente=%s WHERE id_venta=%s", cliente, folio);
				er = mysql_query(&mysql2, sqlcliente);
				if(er == 0)
				//{	
					printf("Cliente registrado satisfactoriamente.....!\n");

					/*sprintf(sqlarticulos, "UPDATE Venta_Articulo SET id_factura=%d WHERE id_venta=%s", tmp_idfactura, folio);
					er = mysql_query(&mysql2, sqlarticulos);
					if(er == 0)
					{*/
						printf("Factura registrada satisfactoriamente.....!\n");
						imprimirticket(folio, "factura", 0);
						gtk_widget_destroy(GTK_WIDGET(button));
					/*}
					else
						printf("Ocurrio un error al relacionar los articulos de la factura: %s\n", mysql_error(&mysql2));
				}
				else
					printf("Ocurrio un error al relacionar el cliente con la factura: %s\n", mysql_error(&mysql2));
			}
			else
				printf("Hubo un error en el registro de la factura: %s\n", mysql_error(&mysql2));
		*/}
		else
			printf("Error al meter la factura: %s\n", sqlfactura);
	  }
	  else
	  	Err_Info("El cliente no tiene RFC registrado");
	}
	mysql_close(&mysql2);
  }
  else
	Info("La factura ya existe o falta algun dato");

  gtk_window_present(GTK_WINDOW(caja));
  entry_global=NULL;
}


void
on_btnfacturarbuscar_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_Nofolio;

	entry_Nofolio = lookup_widget(GTK_WIDGET(button), "entry_Nofolio");

	entry_global = entry_Nofolio;

	gtk_widget_show(create_Buscar_venta());
}


void
on_btnfacturarbuscarc_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_facturarcliente;
	
	entry_facturarcliente = lookup_widget(GTK_WIDGET(button), "entry_facturarcliente");
	
	entry_global = entry_facturarcliente;
	
	gtk_widget_show(create_Buscar_cliente());
}




//CORTE DE CAJA

void corte_caja()
{
	char sqlcorte[200];
	char fecha[10], hora[11];
	char sqlprimerav[200];
	char sqlultimav[250];
	char sqlprimerr[250];
	char sqlultimor[250];
	char sqlprimerp[250];
	char sqlultimop[250];
	char sqlcortes[100] = "SELECT id_corte_caja FROM Corte_Caja GROUP BY id_corte_caja DESC LIMIT 1";

	int id_p_venta=0;
	int id_u_venta=0;
	int id_p_retiro=0;
	int id_u_retiro=0;
	int id_p_pedido=0;
	int id_u_pedido=0;
	int er;

	sacarfecha(fecha,hora);

	sprintf(sqlprimerav, "SELECT id_venta FROM Venta WHERE id_usuario = %s AND id_caja = %s AND fecha = %s GROUP BY id_venta LIMIT 1", id_sesion_usuario, id_sesion_caja, fecha);
	sprintf(sqlultimav,  "SELECT id_venta FROM Venta WHERE id_usuario = %s AND id_caja = %s AND fecha = %s GROUP BY id_venta DESC LIMIT 1", id_sesion_usuario, id_sesion_caja, fecha);
	sprintf(sqlprimerr,  "SELECT id_retiro FROM Retiro WHERE id_usuario_cajero = %s AND id_caja = %s AND fecha = %s GROUP BY id_retiro LIMIT 1", id_sesion_usuario, id_sesion_caja, fecha);
	sprintf(sqlultimor,  "SELECT id_retiro FROM Retiro WHERE id_usuario_cajero = %s AND id_caja = %s AND fecha = %s GROUP BY id_retiro DESC LIMIT 1", id_sesion_usuario, id_sesion_caja, fecha);
	sprintf(sqlprimerp,  "SELECT id_pedido FROM Pedido WHERE id_usuario = %s AND id_caja = %s AND fecha = %s GROUP BY id_pedido LIMIT 1", id_sesion_usuario, id_sesion_caja, fecha);
	sprintf(sqlultimop,  "SELECT id_pedido FROM Pedido WHERE id_usuario = %s AND id_caja = %s AND fecha = %s GROUP BY id_pedido DESC LIMIT 1", id_sesion_usuario, id_sesion_caja, fecha);

/*	sprintf(sqlprimerav, "SELECT Venta.id_venta FROM Venta, Corte_Caja WHERE Venta.fecha = %s AND Venta.id_venta > Corte_Caja.id_venta_fin AND Venta.id_caja = %s AND Venta.id_usuario = %s GROUP BY id_venta LIMIT 1", fecha, id_sesion_caja, id_sesion_usuario);
	sprintf(sqlultimav, "SELECT Venta.id_venta FROM Venta, Corte_Caja WHERE Venta.fecha = %s AND Venta.id_venta > Corte_Caja.id_venta_fin AND Venta.id_caja = %s AND Venta.id_usuario = %s GROUP BY id_venta DESC LIMIT 1", fecha, id_sesion_caja, id_sesion_usuario);
	sprintf(sqlprimerr, "SELECT Retiro.id_retiro FROM Retiro, Corte_Caja WHERE Retiro.fecha = %s AND Retiro.id_retiro > Corte_Caja.id_retiro_fin AND Retiro.id_caja = %s AND Retiro.id_usuario_cajero = %s GROUP BY id_retiro LIMIT 1", fecha, id_sesion_caja, id_sesion_usuario);
	sprintf(sqlultimor, "SELECT Retiro.id_retiro FROM Retiro, Corte_Caja WHERE Retiro.fecha = %s AND Retiro.id_retiro > Corte_Caja.id_retiro_fin AND Retiro.id_caja = %s AND Retiro.id_usuario_cajero = %s GROUP BY id_retiro DESC LIMIT 1", fecha, id_sesion_caja, id_sesion_usuario);
	sprintf(sqlprimerp, "SELECT Pedido.id_pedido FROM Pedido, Corte_Caja WHERE Pedido.fecha = %s AND Pedido.id_pedido > Corte_Caja.id_pedido_fin AND Pedido.id_caja = %s AND Pedido.id_usuario = %s GROUP BY id_pedido LIMIT 1", fecha, id_sesion_caja, id_sesion_usuario);
	sprintf(sqlultimop, "SELECT Pedido.id_pedido FROM Pedido, Corte_Caja WHERE Pedido.fecha = %s AND Pedido.id_pedido > Corte_Caja.id_pedido_fin AND Pedido.id_caja = %s AND Pedido.id_usuario = %s GROUP BY id_pedido DESC LIMIT 1", fecha, id_sesion_caja, id_sesion_usuario);*/

	if(conecta_bd_2(&mysql2) == -1)
	{
		printf("No se pudo conectar a la base de datos...: %s\n", mysql_error(&mysql));
	}
	else
	{
		if(mysql_query(&mysql2, sqlprimerav) == 0)
		{
			res = mysql_store_result(&mysql2);
			if(res)
			{
				printf("Se registra la venta inicial...\n");
				if((row = mysql_fetch_row(res)))
				{
					id_p_venta = atoi(row[0]);

					printf("El id: %d\n", id_p_venta);
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}
		if(mysql_query(&mysql2, sqlultimav) == 0)
		{
			res = mysql_store_result(&mysql2);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
					id_u_venta = atoi(row[0]);
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}
		if(mysql_query(&mysql2, sqlprimerr) == 0)
		{
			res = mysql_store_result(&mysql2);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
					id_p_retiro = atoi(row[0]);
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}
		if(mysql_query(&mysql2, sqlultimor) == 0)
		{
			res = mysql_store_result(&mysql2);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
					id_u_retiro = atoi(row[0]);
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}
		if(mysql_query(&mysql2, sqlprimerp) == 0)
		{
			res = mysql_store_result(&mysql2);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
					id_p_pedido = atoi(row[0]);
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}
		if(mysql_query(&mysql2, sqlultimop) == 0)
		{
			res = mysql_store_result(&mysql2);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
					id_u_pedido = atoi(row[0]);
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}

		sprintf(sqlcorte, "INSERT INTO Corte_Caja VALUES(NULL, %s, %s, '%s', %d, %d, %d, %d, %d, %d, %s)", id_sesion_usuario, fecha, hora, id_p_venta, id_u_venta, id_p_retiro, id_u_retiro, id_p_pedido, id_u_pedido, id_sesion_caja);
		er = mysql_query(&mysql2, sqlcorte);
		if(er == 0)
		{
			printf("Se hizo el corte hasta el folio: %d\n", id_u_venta);
			er = mysql_query(&mysql2, sqlcortes);
			if(er == 0)
			{
				res = mysql_store_result(&mysql2);
				if(res)
				{
					row = mysql_fetch_row(res);
					printf("Si hizo el corte....%s\n", row[0]);
					if(imprimirticket(row[0], "corte_caja", 0)==1)
					{
						printf("ERROR\n");
					}
				}
			}
			else
				printf("No se pudieron sacar los cortes...: %s\n", mysql_error(&mysql2));
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}
	}
	mysql_close(&mysql2);
}

int checar_corte()
{
	char sqlchecar[200];
	char fecha[11];
	char hora[10];
	int er;
	int val=0;

	sacarfecha(fecha, hora);

	sprintf(sqlchecar, "SELECT id_corte_caja FROM Corte_Caja WHERE id_usuario = %s AND id_caja = %s AND fecha = %s", id_sesion_usuario, id_sesion_caja, fecha);

	if(conecta_bd() == -1)
		Err_Info("No me puedo conectar a la base de datos.\nLlame a maticaLX para que le resuelvan el problema");
	else
	{
		er = mysql_query(&mysql, sqlchecar);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if(mysql_num_rows(res) > 0)
					val = -1;
				else
					val = 1;
			}
			else
			{
				sprintf(Errores, "%s",mysql_error(&mysql));
				Err_Info(Errores);
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	mysql_close(&mysql);
	return val;
}

void
on_parcial_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	char sqlborrar[200];
	char fecha[11];
	char hora[10];
	int er;
	int val;

	val = checar_corte();
	if(val == 1)
	{
		corte_caja();
		sacarfecha(fecha, hora);
		sprintf(sqlborrar, "DELETE FROM Corte_Caja WHERE id_usuario = %s AND id_caja = %s AND fecha = %s", id_sesion_usuario, id_sesion_caja, fecha);
		if(conecta_bd() == -1)
			Err_Info("No me pude conectar a la base de datos.\nLlame a maticaLX para que le resulevan el problema");
		else
		{
			er = mysql_query(&mysql, sqlborrar);
			if(er == 0)
				Info("Se efectuo el corte parcial.\nNOTA: Este corte no es valido");
			else
			{
				sprintf(Errores, "OcurriÃ³ un error en el corte parcial:\n%s", mysql_error(&mysql));
				Info(Errores);
			}
		}
		mysql_close(&mysql);
	}
	else
		Err_Info("Ya se realizÃ³ el corte de caja");
}



void
on_final_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *txtbarcode;
	GtkWidget *venta;
	GtkWidget *btncajaok;
	GtkWidget *win_trabajando;
	gint result;
	int val;
	

	txtbarcode = lookup_widget(GTK_WIDGET(menuitem), "txtbarcode");
	venta = lookup_widget(GTK_WIDGET(menuitem), "venta");
	btncajaok = lookup_widget(GTK_WIDGET(menuitem), "btncajaok");

	val = checar_corte();
	if(val == 1)
	{
		result = Confirma("Estas seguro(a) que quieres\n hacer el corte de caja?");
		printf("El resultado: %d\n", result);
		switch (result)
		{
			case GTK_RESPONSE_ACCEPT:
				win_trabajando = create_win_trabajando ();
				gtk_widget_show (win_trabajando);
				corte_final = TRUE;
				while (g_main_iteration(FALSE));
			
				gtk_widget_set_sensitive(txtbarcode, FALSE);
				gtk_widget_set_sensitive(venta, FALSE);
				gtk_widget_set_sensitive(btncajaok, FALSE);
				corte_caja();
			
				corte_final = FALSE;
				gtk_widget_destroy(win_trabajando);
         			break;
      			default:
         			break;
    		}
	}
	else
		Err_Info("Ya se realizÃ³ el corte de caja");
	
}


void
on_global_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show(create_Corte_global());
}



void
on_reimprimir_ticket_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *Reimprimir_ticket = create_Reimprimir_ticket();
	gtk_widget_show(Reimprimir_ticket);
}


void
on_btn_reimpbuscar_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_reimpfolio;

	entry_reimpfolio = lookup_widget(GTK_WIDGET(button), "entry_reimpfolio");
	entry_global = entry_reimpfolio;

	gtk_widget_show(create_Buscar_venta());
}



void
on_btn_reimpok_clicked                 (GtkEntry       *entry_reimpfolio,
                                        gpointer         user_data)
{
	gchar *reimp_folio;
	char sqltipo[100];
	int er;
	char tipo[20], monto[20];

	reimp_folio = gtk_editable_get_chars(GTK_EDITABLE(entry_reimpfolio), 0, -1);
	sprintf(sqltipo, "SELECT tipo, monto FROM Venta WHERE id_venta = %s", reimp_folio);

	if(conecta_bd_2(&mysql2) == -1)
		Err_Info("No me puedo conectar a la base de datos");
	else
	{
		er = mysql_query(&mysql2, sqltipo);
		if(er == 0)
		{
			res = mysql_store_result(&mysql2);
			if(res)
			{
				if(mysql_num_rows(res) > 0)
				{
					if((row = mysql_fetch_row(res)))
					{
						strcpy(tipo,row[0]);
                                                strcpy(monto,row[1]);

						//##EDER
						imprimirticket(reimp_folio, tipo, atof(monto), 1);
					}
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql2));
			Err_Info(Errores);
		}
	}
	mysql_close(&mysql2);
	gtk_window_present(GTK_WINDOW(caja));
}


void
checar_permisos                        (GtkWidget       *widget,
                                        gpointer         user_data)
{
	GtkWidget *retiro_de_efectivo;
	GtkWidget *nuevo_cliente;
 	GtkWidget *modificar_datos_cliente;
	GtkWidget *configuracion;
	GtkWidget *btn_nuevo;
	GtkWidget *btn_borrar;
	GtkWidget *btn_cancelar;
	GtkWidget *btn_guardar;
	GtkWidget *global;

	retiro_de_efectivo = lookup_widget(widget, "retiro_de_efectivo");
	nuevo_cliente = lookup_widget(widget, "nuevo_cliente");
	modificar_datos_cliente = lookup_widget(widget, "modificar_datos_cliente");
	configuracion = lookup_widget(widget, "configuracion");
	btn_nuevo = lookup_widget(widget, "btn_nuevo");
	btn_borrar = lookup_widget(widget, "btn_borrar");
	btn_cancelar = lookup_widget(widget, "btn_cancelar");
	btn_guardar = lookup_widget(widget, "btn_guardar");
	global = lookup_widget(widget, "global");

	if(strcmp(tipo_sesion_usuario, "limit") == 0)
	{
		gtk_widget_set_sensitive(retiro_de_efectivo, FALSE);
		gtk_widget_set_sensitive(nuevo_cliente, FALSE);
		gtk_widget_set_sensitive(modificar_datos_cliente, FALSE);
		gtk_widget_set_sensitive(configuracion, FALSE);
		gtk_widget_set_sensitive(btn_nuevo, FALSE);
		gtk_widget_set_sensitive(btn_borrar, FALSE);
		gtk_widget_set_sensitive(btn_cancelar, FALSE);
		gtk_widget_set_sensitive(btn_guardar, FALSE);
//		gtk_widget_set_sensitive(global, FALSE);
	}
}


void
on_entry_dia_corte_global_changed      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *entry_mes_corte_global;
	int car;

	entry_mes_corte_global = lookup_widget(GTK_WIDGET(editable), "entry_mes_corte_global");

	car = gtk_editable_get_position(editable);
	if(car >= 1)
	{
		gtk_widget_grab_focus(entry_mes_corte_global);
	}
}


void
on_entry_ano_corte_blobal_changed      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *btn_corte_global;
	int car;

	btn_corte_global = lookup_widget(GTK_WIDGET(editable), "btn_corte_global");

	car = gtk_editable_get_position(editable);
	if(car >= 3)
	{
		gtk_widget_grab_focus(btn_corte_global);
	}
}


void
on_entry_mes_corte_global_changed      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *entry_ano_corte_global;
	int car;

	entry_ano_corte_global = lookup_widget(GTK_WIDGET(editable), "entry_ano_corte_global");

	car = gtk_editable_get_position(editable);
	if(car >= 1)
	{
		gtk_widget_grab_focus(entry_ano_corte_global);
	}
}


void
on_btn_corte_global_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_dia_corte_global;
	GtkWidget *entry_mes_corte_global;
	GtkWidget *entry_ano_corte_global;
	GtkWidget *Corte_global;
	char fecha[11];
	gchar *tmp_dia;
	gchar *tmp_mes;
	gchar *tmp_ano;

	entry_dia_corte_global = lookup_widget(GTK_WIDGET(button), "entry_dia_corte_global");
	entry_mes_corte_global = lookup_widget(GTK_WIDGET(button), "entry_mes_corte_global");
	entry_ano_corte_global = lookup_widget(GTK_WIDGET(button), "entry_ano_corte_global");
	Corte_global = lookup_widget(GTK_WIDGET(button), "Corte_global");

	tmp_dia = gtk_editable_get_chars(GTK_EDITABLE(entry_dia_corte_global), 0, -1);
	tmp_mes = gtk_editable_get_chars(GTK_EDITABLE(entry_mes_corte_global), 0, -1);
	tmp_ano = gtk_editable_get_chars(GTK_EDITABLE(entry_ano_corte_global), 0, -1);
	sprintf(fecha, "%s%s%s", tmp_ano, tmp_mes,tmp_dia);

	if(imprimirticket(fecha, "corte_all_caja", 0) == 1)
		Err_Info("OcurriÃ³ un error al efectuar el corte global");
	else
		Info("Corte global efectuado");
	gtk_widget_destroy(Corte_global);
}


void
on_llenar_pedido_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show(create_V_pedidos());
}


void
on_cierre_de_facturas_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show(create_Cierre_facturas());
}


void
on_entry_cierre_dedia_changed          (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkEditable *entry_cierre_dedia = user_data;
	if(gtk_editable_get_position(entry_cierre_dedia) > 0)
		gtk_widget_grab_focus(GTK_WIDGET(editable));
}


void
on_entry_cierre_demes_changed          (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkEditable *entry_cierre_demes = user_data;
	if(gtk_editable_get_position(entry_cierre_demes) > 0)
		gtk_widget_grab_focus(GTK_WIDGET(editable));
}


void
on_entry_cierre_deano_changed          (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkEditable *entry_cierre_deano = user_data;
	if(gtk_editable_get_position(entry_cierre_deano) > 2)
		gtk_widget_grab_focus(GTK_WIDGET(editable));
}


void
on_entry_cierre_hastadia_changed       (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkEditable *entry_cierre_hastadia = user_data;
	if(gtk_editable_get_position(entry_cierre_hastadia) > 0)
		gtk_widget_grab_focus(GTK_WIDGET(editable));
}


void
on_entry_cierre_hastames_changed       (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkEditable *entry_cierre_hastames = user_data;
	if(gtk_editable_get_position(entry_cierre_hastames) > 0)
		gtk_widget_grab_focus(GTK_WIDGET(editable));
}


void
on_entry_cierre_hastaano_changed       (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkEditable *entry_cierre_hastaano = user_data;
	if(gtk_editable_get_position(entry_cierre_hastaano) > 2)
		gtk_widget_grab_focus(GTK_WIDGET(editable));
}


void
on_btn_cierre_contado_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	char tipo[10]="contado";

	if(gtk_toggle_button_get_active(togglebutton) == TRUE)
		sacar_ventas_cierre(tipo);
}


void
on_btn_cierre_credito_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	char tipo[10]="credito";

	if(gtk_toggle_button_get_active(togglebutton) == TRUE)
		sacar_ventas_cierre(tipo);
}


void
on_btn_cierre_revisar_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *btn_cierre_contado;
	GtkWidget *btn_cierre_credito;
	char tipo[10];

	btn_cierre_contado = lookup_widget(GTK_WIDGET(button), "btn_cierre_contado");
	btn_cierre_credito = lookup_widget(GTK_WIDGET(button), "btn_cierre_credito");

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_cierre_contado)) == TRUE)
	{
		strcpy(tipo,"contado");
		sacar_ventas_cierre(tipo);
	}
	else
	{
		strcpy(tipo,"credito");
		sacar_ventas_cierre(tipo);
	}
}


void sacar_ventas_cierre(char *tipo)
{
	GtkWidget *clist_cierre_venta;
	GtkWidget *entry_dedia;
	GtkWidget *entry_demes;
	GtkWidget *entry_deano;
	GtkWidget *entry_hastadia;
	GtkWidget *entry_hastames;
	GtkWidget *entry_hastaano;
	GtkWidget *lbl_cierre_total;

	gchar *dedia, *demes, *deano, *hastadia, *hastames, *hastaano;
	gchar *lista[3];

	char sqlventas[350];
	char total[20];
	char total_tmp[50];
	int er;

	clist_cierre_venta = lookup_widget(GTK_WIDGET(Cierre_facturas), "clist_cierre_venta");
	entry_dedia = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_dedia");
	entry_demes = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_demes");
	entry_deano = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_deano");
	entry_hastadia = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastadia");
	entry_hastames = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastames");
	entry_hastaano = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastaano");
	lbl_cierre_total = lookup_widget(GTK_WIDGET(Cierre_facturas), "lbl_cierre_total");

	dedia = gtk_editable_get_chars(GTK_EDITABLE(entry_dedia),0,2);
	demes = gtk_editable_get_chars(GTK_EDITABLE(entry_demes),0,2);
	deano = gtk_editable_get_chars(GTK_EDITABLE(entry_deano),0,4);
	hastadia = gtk_editable_get_chars(GTK_EDITABLE(entry_hastadia),0,2);
	hastames = gtk_editable_get_chars(GTK_EDITABLE(entry_hastames),0,2);
	hastaano = gtk_editable_get_chars(GTK_EDITABLE(entry_hastaano),0,4);

	sprintf(sqlventas, "SELECT Venta.id_venta, Venta.fecha, ROUND(Venta.monto,2), Venta.num_factura FROM Venta WHERE Venta.tipo = '%s' AND Venta.cancelada = 'n' AND Venta.num_factura = 0 AND Venta.fecha BETWEEN '%s%s%s' AND '%s%s%s' ORDER BY Venta.id_venta", tipo, deano, demes, dedia, hastaano, hastames, hastadia);

	printf("Consulta: %s\n", sqlventas);

	gtk_clist_clear(GTK_CLIST(clist_cierre_venta));
	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlventas);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					/*if(row[3] == "0")
					{*/
						sprintf(total, "%.2f", atof(total)+atof(row[2]));
						lista[0]=row[0];
						lista[1]=row[1];
						lista[2]=row[2];
						gtk_clist_append(GTK_CLIST(clist_cierre_venta), lista);
					/*}
					else
						printf("Un NULL: %s\n", row[0]);*/
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
	{
		sprintf(Errores, "%s",mysql_error(&mysql));
		Err_Info(Errores);
	}
	mysql_close(&mysql);
	sprintf(total_tmp,"<span size=\"15000\"><b>$ %s</b></span>", total);
	printf("Cadena: %s\n", total);
	gtk_label_set_markup(GTK_LABEL(lbl_cierre_total), total_tmp);
}


void
on_Cierre_facturas_show                (GtkWidget       *widget,
                                        gpointer         user_data)
{
	GtkWidget *entry_dedia;
	GtkWidget *entry_demes;
	GtkWidget *entry_deano;
	GtkWidget *entry_hastadia;
	GtkWidget *entry_hastames;
	GtkWidget *entry_hastaano;
	GtkWidget *entry_cierre_factura;
	GtkWidget *btn_cierre_revisar;
	GtkWidget *clist_cierre_venta;

	char sqlfecha[200], sqlultimaf[200];
	int er;

	Cierre_facturas = widget;

	entry_dedia = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_dedia");
	entry_demes = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_demes");
	entry_deano = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_deano");
	entry_hastadia = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastadia");
	entry_hastames = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastames");
	entry_hastaano = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastaano");
	btn_cierre_revisar = lookup_widget(GTK_WIDGET(Cierre_facturas), "btn_cierre_revisar");
	clist_cierre_venta = lookup_widget(GTK_WIDGET(Cierre_facturas), "clist_cierre_venta");
	entry_cierre_factura = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_factura");

	gtk_clist_set_column_justification(GTK_CLIST(clist_cierre_venta),0,GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(GTK_CLIST(clist_cierre_venta),1,GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(GTK_CLIST(clist_cierre_venta),2,GTK_JUSTIFY_RIGHT);

	sprintf(sqlfecha, "SELECT DATE_FORMAT(CURDATE(), \"%%d\"), DATE_FORMAT(CURDATE(), \"%%m\"),DATE_FORMAT(CURDATE(), \"%%Y\")");
	sprintf(sqlultimaf, "SELECT num_factura+1 FROM Venta ORDER BY num_factura DESC LIMIT 1");

	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlfecha);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row=mysql_fetch_row(res)))
				{
					gtk_entry_set_text(GTK_ENTRY(entry_dedia), row[0]);
					gtk_entry_set_text(GTK_ENTRY(entry_demes), row[1]);
					gtk_entry_set_text(GTK_ENTRY(entry_deano), row[2]);
					gtk_entry_set_text(GTK_ENTRY(entry_hastadia), row[0]);
					gtk_entry_set_text(GTK_ENTRY(entry_hastames), row[1]);
					gtk_entry_set_text(GTK_ENTRY(entry_hastaano), row[2]);
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
		er = mysql_query(&mysql, sqlultimaf);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row=mysql_fetch_row(res)))
					gtk_entry_set_text(GTK_ENTRY(entry_cierre_factura), row[0]);
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
	{
		sprintf(Errores, "%s",mysql_error(&mysql));
		Err_Info(Errores);
	}
	mysql_close(&mysql);
	gtk_widget_grab_focus(btn_cierre_revisar);
}


void
on_btn_cierre_facturar_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *clist_cierre_venta;
	GtkWidget *btn_cierre_contado;
	GtkWidget *btn_cierre_credito;
	GtkWidget *entry_dedia;
	GtkWidget *entry_demes;
	GtkWidget *entry_deano;
	GtkWidget *entry_hastadia;
	GtkWidget *entry_hastames;
	GtkWidget *entry_hastaano;
	GtkWidget *entry_cierre_factura;
	gint sino;
	gchar *dedia, *demes, *deano, *hastadia, *hastames, *hastaano, *cierre_factura;
	char tipo[20];
	char sqlventas[350];
	char sqlregistro[350];
	char sqlfactura[200];
	char id_factura[20], num_factura[25];
	long int ultimo_id;

	clist_cierre_venta = lookup_widget(GTK_WIDGET(Cierre_facturas), "clist_cierre_venta");
	entry_dedia = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_dedia");
	entry_demes = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_demes");
	entry_deano = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_deano");
	entry_hastadia = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastadia");
	entry_hastames = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastames");
	entry_hastaano = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_hastaano");
	btn_cierre_contado = lookup_widget(GTK_WIDGET(Cierre_facturas), "btn_cierre_contado");
	btn_cierre_credito = lookup_widget(GTK_WIDGET(Cierre_facturas), "btn_cierre_credito");
	entry_cierre_factura = lookup_widget(GTK_WIDGET(Cierre_facturas), "entry_cierre_factura");

	dedia = gtk_editable_get_chars(GTK_EDITABLE(entry_dedia),0,2);
	demes = gtk_editable_get_chars(GTK_EDITABLE(entry_demes),0,2);
	deano = gtk_editable_get_chars(GTK_EDITABLE(entry_deano),0,4);
	hastadia = gtk_editable_get_chars(GTK_EDITABLE(entry_hastadia),0,2);
	hastames = gtk_editable_get_chars(GTK_EDITABLE(entry_hastames),0,2);
	hastaano = gtk_editable_get_chars(GTK_EDITABLE(entry_hastaano),0,4);
	cierre_factura = gtk_editable_get_chars(GTK_EDITABLE(entry_cierre_factura),0,-1);

 if(strlen(cierre_factura)>0)
 {
  sprintf(sqlfactura, "SELECT * FROM Venta WHERE num_factura=%s", cierre_factura);

  if(conecta_bd() == 1)
  {
   er = mysql_query(&mysql, sqlfactura);	//checa si ya existe la factura
   if(er == 0)
   {
    res = mysql_store_result(&mysql);
    if(res)
    {
      if(mysql_num_rows(res)<=0)
      {
	if(GTK_CLIST(clist_cierre_venta)->rows > 0)
	{
		sino = Confirma("Seguro que quieres hacer  \nel cierre de facturas?");
	}
	else
		sino = 0;
	if(sino == -3)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_cierre_contado)))
		{
			strcpy(tipo,"contado");
			//sprintf(sqlfactura, "INSERT INTO Venta_Factura VALUES(NULL, '%s%s%s', CURTIME(), 'Venta de CONTADO  (%s/%s/%s - %s/%s/%s)')", hastaano, hastames, hastadia, deano, demes, dedia, hastaano, hastames, hastadia);
		}
		else
		{
			strcpy(tipo,"credito");
		//	sprintf(sqlfactura, "INSERT INTO Venta_Factura VALUES(NULL, '%s%s%s', CURTIME(), 'Venta de CREDITO  (%s/%s/%s - %s/%s/%s)')", hastaano, hastames, hastadia, deano, demes, dedia, hastaano, hastames, hastadia);
		}
		sprintf(sqlventas, "SELECT Venta.id_venta FROM Venta WHERE Venta.tipo = '%s' AND Venta.cancelada = 'n' AND Venta.num_factura=0 AND Venta.fecha BETWEEN '%s%s%s' AND '%s%s%s' ORDER BY Venta.id_venta", tipo, deano, demes, dedia, hastaano, hastames, hastadia);

		printf("Consulta: %s\n", sqlventas);

		er = mysql_query(&mysql, sqlventas);
		if(er == 0)
			{
				/*sprintf(sqlfactura, "SELECT id_factura FROM Venta_Factura ORDER BY id_factura DESC LIMIT 1");
				ultimo_id = mysql_insert_id(&mysql);
				sprintf(id_factura,"%ld",ultimo_id);*/
				strcpy(num_factura, cierre_factura);
				/*cierre_num_factura = atoi(num_factura);
				er = mysql_query(&mysql, sqlventas);	//obtiene las ventas
				if(er == 0)
				{*/
					res = mysql_store_result(&mysql);
					if(res)
					{
						while((row=mysql_fetch_row(res)))
						{
							/*if(row[3] == NULL)
							{*/
								//sprintf(total, "%.2f", atof(total)+atof(row[2]));
								sprintf(sqlregistro, "UPDATE Venta SET num_factura  = %s, fecha_factura=CURDATE(), hora_factura=CURTIME(), observacion=\"Venta de %s (%s/%s/%s - %s/%s/%s)\" WHERE id_venta = %s", num_factura, tipo, deano, demes, dedia, hastaano, hastames, hastadia, row[0]);
								er = mysql_query(&mysql, sqlregistro);
								/*if(er == 0)
								{
									//printf("Se registrÃ³: %s\n", row[0]);
									sprintf(sqlregistro, "INSERT INTO Venta_Factura_Relacion VALUES(NULL, %s, %s, %s)", row[0], id_factura, num_factura);
									er = mysql_query(&mysql, sqlregistro);
									if(er != 0)
										printf("Error: %s\n", mysql_error(&mysql));
								}*/
								if(er!=0)
									printf("Error: %s\n", mysql_error(&mysql));
							//}
						}
						imprimirticket(num_factura, "cierre_factura", 0);
						imprimirticket(num_factura,"listado_cierre",0);
					}
				/*}
				else
				{
					sprintf(Errores, "%s",mysql_error(&mysql));
					Err_Info(Errores);
				}*/
			}
			else
			{
				sprintf(Errores, "%s",mysql_error(&mysql));
				Err_Info(Errores);
			}
	}
	gtk_widget_destroy(Cierre_facturas);
      }
      else
      	Err_Info("Ya existe la factura");
    }
   }
   else
   {
     sprintf(Errores, "%s",mysql_error(&mysql));
     Err_Info(Errores);
   }
  }
  else
  {
    sprintf(Errores, "%s",mysql_error(&mysql));
    Err_Info(Errores);
  }
  mysql_close(&mysql);
 }
 else
   Err_Info("Se necesita un numero de factura");
}


void
on_btnlistado_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *clist_cierre_venta;
	char num_fac[30];
	//FILE *impresora;

	clist_cierre_venta = lookup_widget(GTK_WIDGET(Cierre_facturas), "clist_cierre_venta");

	/*impresora = fopen("/dev/lp0", "w");
	for(i=0; i<GTK_CLIST(clist_cierre_venta)->rows; i++)
	{
		gtk_clist_get_text(clist_cierre_venta, i, 0, &folio);
		gtk_clist_get_text(clist_cierre_venta, i, 1, &fecha);
		gtk_clist_get_text(clist_cierre_venta, i, 2, &monto);
		strcpy(cfolio, folio);
		strcpy(cfecha, fecha);
		strcpy(cmonto, monto);
		fprintf(impresora,"%s %s %s\n", cfolio, cmonto, cfecha);
	}
	fclose(impresora);*/
	sprintf (num_fac,"%d",cierre_num_factura);
	imprimirticket(num_fac,"listado_cierre",0);
}



void
on_entry_buscarventa_cliente_changed   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	buscar_venta();
}


void
on_btn_buscarventafecha_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *calendario_buscarventa;

	calendario_buscarventa = lookup_widget(GTK_WIDGET(button), "calendario_buscarventa");

	gtk_widget_show(calendario_buscarventa);
}



void
on_calendario_buscarventa_day_selected (GtkCalendar     *calendar,
                                        gpointer         user_data)
{
	GtkWidget *btn_buscarventafecha;

	btn_buscarventafecha = lookup_widget(GTK_WIDGET(calendar),"btn_buscarventafecha");

	buscar_venta();

	gtk_widget_hide(GTK_WIDGET(calendar));
}


void buscar_venta()
{
	GtkWidget *entry_buscarventa_cliente;
	GtkWidget *calendario_buscarventa;
	GtkWidget *clist_buscarventa;
	char sqlventa[700];
	gchar *cliente;
	gchar *lista[6];
	guint ano, mes, dia;

	calendario_buscarventa = lookup_widget(GTK_WIDGET(Buscar_venta), "calendario_buscarventa");
	clist_buscarventa = lookup_widget(GTK_WIDGET(Buscar_venta), "clist_buscarventa");
	entry_buscarventa_cliente = lookup_widget(GTK_WIDGET(Buscar_venta), "entry_buscarventa_cliente");

	gtk_clist_set_column_justification(GTK_CLIST(clist_buscarventa),0,GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(GTK_CLIST(clist_buscarventa),1,GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(GTK_CLIST(clist_buscarventa),3,GTK_JUSTIFY_RIGHT);

	gtk_calendar_get_date(GTK_CALENDAR(calendario_buscarventa),&ano,&mes,&dia);
	mes++;

	cliente = gtk_editable_get_chars(GTK_EDITABLE(entry_buscarventa_cliente), 0, -1);

	sprintf(sqlventa, "SELECT Venta.id_venta, ROUND(Venta.monto,2) AS Monto, DATE_FORMAT(Venta.fecha,\"%%d/%%m/%%Y\") AS Fecha, Venta.num_factura, DATE_FORMAT(Venta.fecha_factura,\"%%d/%%m/%%Y\") AS Facturado, Cliente.nombre FROM Venta INNER JOIN Cliente ON Venta.id_cliente=Cliente.id_cliente WHERE Venta.fecha=%d%.2d%.2d AND Venta.cancelada='n' AND (Cliente.nombre LIKE \"%%%s%%\" OR Cliente.contacto LIKE \"%%%s%%\")", ano, mes, dia, cliente, cliente);

	gtk_clist_clear(GTK_CLIST(clist_buscarventa));
	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlventa);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0]=row[0];
					lista[1]=row[1];
					lista[2]=row[2];
					lista[3]=row[3];
					lista[4]=row[4];
					lista[5]=row[5];
					gtk_clist_append(GTK_CLIST(clist_buscarventa), lista);
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
	{
		sprintf(Errores, "%s",mysql_error(&mysql));
		Err_Info(Errores);
	}
	mysql_close(&mysql);
}


void
on_clist_buscarventa_select_row        (GtkCList        *clist,
                                        gint             fila,
                                        gint             columna,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gchar *dato;

	gtk_clist_get_text(clist, fila, 0, &dato);

	strcpy(folio_global, dato);
}


void
on_btn_buscarventaaceptar_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_entry_set_text(GTK_ENTRY(entry_global), folio_global);
	gtk_signal_emit_by_name(GTK_OBJECT(entry_global),"activate");
	gtk_widget_destroy(Buscar_venta);
	gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(entry_global))));
}


void
on_Buscar_venta_show                   (GtkWidget       *widget,
                                        gpointer         user_data)
{
	Buscar_venta = widget;
	buscar_venta();
}


void
on_btn_cajabuscararticulo_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *txtbarcode;

	txtbarcode = lookup_widget(caja, "txtbarcode");

	entry_global = txtbarcode;
	gtk_widget_grab_focus(txtbarcode);

	gtk_widget_show(create_Buscar_articulo());
}


void
on_Buscar_articulo_show                (GtkWidget       *widget,
                                        gpointer         user_data)
{
	GtkWidget *clist_articulos;
	
	Buscar_articulo = widget;
	clist_articulos = lookup_widget (widget,"clist_buscararticulo");
	
	buscar_articulo();
	
	gtk_clist_set_column_justification (GTK_CLIST (clist_articulos), 2, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_visibility    (GTK_CLIST (clist_articulos),2,FALSE);
}


void
on_entry_buscararticulonombre_changed  (GtkEditable     *editable,
                                        gpointer         user_data)
{
	buscar_articulo();
}


void
on_clist_buscararticulo_select_row     (GtkCList        *clist,
                                        gint             fila,
                                        gint             columna,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gchar *dato;

	gtk_clist_get_text(clist, fila, 0, &dato);

	strcpy(codigo_articulo_global, dato);
}


void
on_btn_buscararticuloaceptar_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_entry_set_text(GTK_ENTRY(entry_global), codigo_articulo_global);
	gtk_signal_emit_by_name(GTK_OBJECT(entry_global),"activate");
	gtk_widget_destroy(Buscar_articulo);
	gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(entry_global))));
}


void
on_btn_pedidobuscararticulo_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_codigo;

	entry_codigo = lookup_widget(GTK_WIDGET(button), "entry_codigo");

	entry_global = entry_codigo;
	gtk_widget_grab_focus(entry_codigo);

	gtk_widget_show(create_Buscar_articulo());
}



void buscar_articulo()
{
	GtkWidget *entry_buscararticulonombre;
	GtkWidget *clist_buscararticulo;
	GtkWidget *clist_clientes;
	GtkWidget *chk_precio;
	
	char sqlarticulo[600];
	gchar *articulo;
	gchar *lista[4];
	gchar *id_cliente;
	char precio[50];
	
	clist_buscararticulo       = lookup_widget(GTK_WIDGET(Buscar_articulo), "clist_buscararticulo"          );
	entry_buscararticulonombre = lookup_widget(GTK_WIDGET(Buscar_articulo), "entry_buscararticulonombre"    );
	clist_clientes             = lookup_widget(GTK_WIDGET(caja)           , "listaclientes"                 );
	chk_precio                 = lookup_widget(GTK_WIDGET(Buscar_articulo), "chk_ver_precio_buscar_articulo");
	
	gtk_clist_get_text (GTK_CLIST (clist_clientes),obtener_row_seleccionada (clist_clientes,0),0,&id_cliente);
	
	gtk_clist_set_column_justification(GTK_CLIST(clist_buscararticulo),0,GTK_JUSTIFY_RIGHT);
	
	articulo = gtk_editable_get_chars(GTK_EDITABLE(entry_buscararticulonombre), 0, -1);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(chk_precio)))
		sprintf(sqlarticulo, "SELECT Articulo.codigo, Articulo.nombre, Articulo_Lista.precio, Articulo.tipo, Tarjeta_Cliente.descuento FROM Articulo INNER JOIN Articulo_Lista ON Articulo_Lista.id_articulo = Articulo.id_articulo INNER JOIN Cliente ON Cliente.id_lista = Articulo_Lista.id_lista LEFT JOIN Tarjeta_Cliente ON Tarjeta_Cliente.id_cliente = Cliente.id_cliente AND Tarjeta_Cliente.id_articulo = Articulo_Lista.id_articulo  WHERE Articulo.nombre LIKE \"%%%s%%\" AND Cliente.id_cliente = %s", articulo,id_cliente);
	else
		sprintf(sqlarticulo, "SELECT Articulo.codigo, Articulo.nombre, \"-\", Articulo.tipo FROM Articulo WHERE Articulo.nombre LIKE \"%%%s%%\"", articulo);
	
	printf ("\n\n%s\n\n",sqlarticulo);
	
	gtk_clist_clear(GTK_CLIST(clist_buscararticulo));
	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlarticulo);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0]=row[0];
					lista[1]=row[1];
					if (row[4])
						sprintf (precio,"%.2f",atof (row[2]) + atof (row[4]));
					else
						sprintf (precio,"%.2f",atof (row[2]));
					formatea_cadena(precio);
					lista[2]=precio;
					lista[3]=row[3];
					gtk_clist_append(GTK_CLIST(clist_buscararticulo), lista);
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
		Err_Info("No me puedo conectar a la base de datos");
	mysql_close(&mysql);
}


void
on_Buscar_cliente_show                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
	Buscar_cliente = widget;
	buscar_cliente(0);
}


void
on_entry_buscarclientenombre_changed   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	buscar_cliente(0);
}


void
on_clist_buscarcliente_select_row      (GtkCList        *clist,
                                        gint             fila,
                                        gint             columna,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gchar *dato;

	gtk_clist_get_text(clist, fila, 0, &dato);

	strcpy(codigo_cliente_global, dato);
	printf("El codigo del cliente: %s\n", codigo_cliente_global);
}


void
on_btn_buscarcliente_aceptar_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
	printf("El codigo del cliente: %s\n", codigo_cliente_global);
	gtk_entry_set_text(GTK_ENTRY(entry_global), codigo_cliente_global);
	gtk_signal_emit_by_name(GTK_OBJECT(entry_global), "activate");
	gtk_signal_emit_by_name(GTK_OBJECT(ventana_oviedo), "show");
	gtk_widget_destroy(Buscar_cliente);
	//gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(entry_global)));
}


void buscar_cliente(int tipo)
{
	GtkWidget *entry_buscarclientenombre;
	GtkWidget *clist_buscarcliente;
	char sqlcliente[300];
	gchar *cliente;
	gchar *lista[3];

	clist_buscarcliente = lookup_widget(GTK_WIDGET(Buscar_cliente), "clist_buscarcliente");
	entry_buscarclientenombre = lookup_widget(GTK_WIDGET(Buscar_cliente), "entry_buscarclientenombre");

	gtk_clist_set_column_justification(GTK_CLIST(clist_buscarcliente),0,GTK_JUSTIFY_RIGHT);

	cliente = gtk_editable_get_chars(GTK_EDITABLE(entry_buscarclientenombre), 0, -1);

	if(tipo == 0)
		sprintf(sqlcliente, "SELECT id_cliente, nombre, telefono FROM Cliente WHERE Cliente.bloqueado='n' AND (nombre LIKE \"%%%s%%\" OR contacto LIKE \"%%%s%%\" OR telefono='%s' OR telefono2='%s' OR telefono3='%s')", cliente, cliente, cliente, cliente, cliente);
	else if(tipo == 1)
		sprintf(sqlcliente, "SELECT id_cliente, nombre, telefono FROM Cliente WHERE Cliente.id_cliente > 1 AND Cliente.bloqueado='n' AND (nombre LIKE \"%%%s%%\" OR contacto LIKE \"%%%s%%\" OR telefono='%s' OR telefono2='%s' OR telefono3='%s')", cliente, cliente, cliente, cliente, cliente);

	gtk_clist_clear(GTK_CLIST(clist_buscarcliente));
	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqlcliente);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0]=row[0];
					lista[1]=row[1];
					lista[2]=row[2];
					gtk_clist_append(GTK_CLIST(clist_buscarcliente), lista);
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	else
		Err_Info("No me puedo conectar a la base de datos");
	mysql_close(&mysql);
}


void
on_btn_buscarcliente_nuevo_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_show(create_Alta_clientes_caja());
}



void
on_reimprimir_pedido_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show(create_Reimprimir_pedido());
}


void
on_btn_reimppedidook_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_reimppedido;
	gchar *reimp_pedido;

	entry_reimppedido = lookup_widget(GTK_WIDGET(button),"entry_reimppedido");

	reimp_pedido = gtk_editable_get_chars(GTK_EDITABLE(entry_reimppedido), 0, -1);

	imprimirticket(reimp_pedido,"pedidos_paso1",0);

	gtk_window_present(GTK_WINDOW(caja));
}


///////DESCUENTOS

void
on_btncaja_descuento_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_show(create_Descuento());
}


void
on_btn_descuentoaceptar_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
	GdkColor color;

	GtkWidget *listaarticulos;
	GtkWidget *lblkilos;
	GtkWidget *lblpiezas;
	GtkWidget *lbltotal;
	GtkWidget *lbldescuento;
	GtkWidget *entry_descuento;

	gchar *descuento;
	GList *lista;
	int i;
	char precio_nuevo[20];
	char precio[20];
	char cantidad[20];
	char monto[20];
	gchar *pprecio;
	gchar *pcantidad;
	gsize algo=-1;
	char ctotal[20];
	char cdescuento[20];
	char markup[100];

	gdk_color_parse("#d65a31", &color);

	listaarticulos = lookup_widget(caja, "listaarticulos");
	lblkilos = lookup_widget(caja, "lblkilos");
	lblpiezas = lookup_widget(caja, "lblpiezas");
	lbltotal = lookup_widget(caja, "lbltotal");
	lbldescuento = lookup_widget(caja, "lbldescuento");
	entry_descuento = lookup_widget(GTK_WIDGET(button), "entry_descuento");

	descuento = g_locale_from_utf8(gtk_editable_get_chars(GTK_EDITABLE(entry_descuento), 0, -1),-1,&algo,&algo,NULL);

	if(atof(descuento)>0)
	{
		lista = GTK_CLIST(listaarticulos)->row_list;

		for (i=0; lista; lista = g_list_next (lista), i++)
			if ((GTK_CLIST_ROW (lista)->state == GTK_STATE_SELECTED))
			{
				//PRECIO
				gtk_clist_get_text(GTK_CLIST(listaarticulos), i, 2, &pprecio);
				strcpy(precio, pprecio);
				sprintf(precio_nuevo, "%.2f", atof(precio)-(atof(precio)*(atof(descuento)/100)));
				gtk_clist_set_text(GTK_CLIST(listaarticulos), i, 2, precio_nuevo);

				//MONTO
				gtk_clist_get_text(GTK_CLIST(listaarticulos), i, 3, &pcantidad);
				strcpy(cantidad, pcantidad);
				sprintf(monto, "%.2f", atof(precio_nuevo)*atof(cantidad));
				gtk_clist_set_text(GTK_CLIST(listaarticulos), i, 4, monto);

				//DESCUENTO
				total_descuento = total_descuento + ((atof(precio)*atof(cantidad)) - (atof(precio_nuevo)*atof(cantidad)));
				sprintf(cdescuento, "%.2f",((atof(precio)*atof(cantidad)) - (atof(precio_nuevo)*atof(cantidad))));
				gtk_clist_set_text(GTK_CLIST(listaarticulos), i, 6, cdescuento);

				//TOTAL
				total_venta = total_venta - ((atof(precio)*atof(cantidad)) - (atof(precio_nuevo)*atof(cantidad)));

				gtk_clist_set_foreground(GTK_CLIST(listaarticulos),i,&color);
				gtk_clist_unselect_row(GTK_CLIST(listaarticulos),i,0);
			}

		sprintf(markup, "<span size=\"18000\" color=\"blue\"><b>$ %.2f</b></span>", total_venta);
		gtk_label_set_markup (GTK_LABEL(lbltotal), markup);

		sprintf(markup, "<span size=\"12000\"><b>$ %.2f</b></span>", total_descuento);
		gtk_label_set_markup (GTK_LABEL(lbldescuento), markup);
	}

	gtk_widget_destroy(GTK_WIDGET(gtk_widget_get_toplevel(GTK_WIDGET(button))));
}


/////TERMIMNA DESCUENTOS


/************************************************************************************************
							EDER
*************************************************************************************************/




//Funcion para deshabilitar y limpiar las cajas de texto de Articulos
void disable_articulo()
{
	gtk_widget_set_sensitive (GTK_WIDGET (art_codigo),FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (art_nombre),FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (art_tipo),FALSE);
	gtk_entry_set_text (art_nombre,"");
//	gtk_entry_set_text (art_tipo,"");
	gtk_entry_set_text (art_codigo,"");
}


//funcion para llenar la lista de articulos con criterios en la busqueda
void busquedas(char *bus, GtkCList *listaArticulos)
{
	gchar *lista[4];
	gchar cad_SQL[200];
	int err;

	strcpy(cad_SQL,bus);
	gtk_clist_clear(listaArticulos);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		err = mysql_query(&mysql,cad_SQL);

		if (err == 0) // Si no hay error
		{

			//Se agregan los articulos a la lista
			res = mysql_store_result(&mysql);
			if(res)
			{
				while(( row=mysql_fetch_row(res)) )
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					lista[3] = row[3];
					gtk_clist_append(listaArticulos, lista);
				}
			}

		}
		else
		{
			printf("Error: %s", mysql_error(&mysql));
		} // Fin condicion de error
	}

	mysql_close (&mysql);
}




void
on_Ventana_Articulos_show_lineas       (GtkCList       *clist_linea,
                                        gpointer         user_data)
{
	char sqllineas[100] = "SELECT id_linea, nombre FROM Linea ORDER BY nombre";
	gchar *listalineas[2];
	int er;
	
	gtk_clist_set_column_justification(clist_linea, 0, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_resizeable (clist_linea, 0, FALSE);
	gtk_clist_set_column_resizeable (clist_linea, 1, FALSE);
	gtk_clist_set_column_visibility (clist_linea, 0, FALSE);
	gtk_clist_set_column_width(clist_linea, 0, 20);

	if(conecta_bd() == -1)
	{
		Err_Info("No me puedo conectar a la Base de Datos");
	}
	else
	{
		er = mysql_query(&mysql, sqllineas);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					listalineas[0] = row[0];
					listalineas[1] = row[1];
//					listalineas[2] = row[2];
					gtk_clist_append(clist_linea, listalineas);
				}
			}
		}
		else
		{
			sprintf(Errores, "%s",mysql_error(&mysql));
			Err_Info(Errores);
		}
	}
	mysql_close(&mysql);
}


void
on_clist_linea_select_row              (GtkCList        *clist_ListadoDeArticulos,
                                        gint             fila,
                                        gint             columna,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	char id_linea[5];
	gchar *pid_linea;
	GtkCList *clist_linea=user_data;
	char sqllarticulos[300];

	pid_linea = id_linea;

	fila_linea_articulo = fila;
	gtk_clist_get_text(clist_linea, fila_linea_articulo, 0, &pid_linea);
	
	printf("ID de la Linea: %s\n", pid_linea);
	strcpy(id_linea_articulo, pid_linea);

	

	sprintf(sqllarticulos, "SELECT id_articulo, codigo, nombre, tipo FROM Articulo WHERE id_linea = %s AND (nombre LIKE '%%%s%%' OR codigo LIKE '%s%%') ORDER BY codigo", id_linea_articulo, criterio_articulo, criterio_articulo);

	busquedas(sqllarticulos, clist_ListadoDeArticulos);
}


void
on_clist_linea_unselect_row            (GtkCList        *clist_ListadoDeArticulos,
                                        gint             fila,
                                        gint             columna,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	fila_linea_articulo = -1;
	strcpy(id_linea_articulo, "");
}



/////				 fin de las funciones						 ////////////////
void
on_Articulos_show                      (GtkWidget       *widget,
                                        gpointer         user_data)
{

}


void
on_Ventana_Articulos_show              (GtkCList       *listaArticulos,
                                        gpointer         user_data)
{
	gtk_clist_set_column_visibility (listaArticulos,0,FALSE);
	busquedas ("SELECT id_articulo, codigo, nombre, tipo FROM Articulo ORDER BY codigo",listaArticulos);
}


//Listado de Articulos

void
on_clist_ListadoDeArticulos_select_row (GtkCList        *lista_articulos,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gtk_clist_get_text(lista_articulos,row,0,&str_articulo.id_articulo);
	strcpy(id_actualiza_articulo, str_articulo.id_articulo);
	gtk_clist_get_text(lista_articulos,row,1,&str_articulo.codigo); // Se obtiene codigo de articulo
	gtk_clist_get_text(lista_articulos,row,2,&str_articulo.nombre);      // Se obtiene nombre de articulo
	gtk_clist_get_text(lista_articulos,row,3,&str_articulo.tipo);        // Se obtiene tipo de articulo
}

void
on_clist_ListadoDeArticulos_select_row_codigo
                                        (GtkEntry        *entry_codigo,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gtk_entry_set_text (entry_codigo,str_articulo.codigo);
	gtk_widget_set_sensitive (GTK_WIDGET(entry_codigo),TRUE);
}


void
on_clist_ListadoDeArticulos_select_row_nombre
                                        (GtkEntry        *entry_nombre,
                                        gint             fila,
                                        gint             columna,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gtk_entry_set_text (entry_nombre,str_articulo.nombre);
	gtk_widget_set_sensitive (GTK_WIDGET(entry_nombre),TRUE);
	gtk_widget_grab_focus (GTK_WIDGET(entry_nombre));
	indice_articulo = fila;
}


void
on_clist_ListadoDeArticulos_select_row_tipo
                                        (GtkEntry        *entry_tipo,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gtk_entry_set_text (entry_tipo,str_articulo.tipo);
	gtk_widget_set_sensitive (GTK_WIDGET(entry_tipo),TRUE);
}

//Fin de Listado de Articulos



// Codigo para actualizar o guardar articulo
void
on_btn_guardar_clicked_codigo          (GtkEntry       *codigo,
                                        gpointer         user_data)
{
	str_articulo.codigo = gtk_editable_get_chars (GTK_EDITABLE(codigo),0,-1);
	art_codigo = codigo;
}


void
on_btn_guardar_clicked_nombre          (GtkEntry       *nombre,
                                        gpointer         user_data)
{
	str_articulo.nombre = gtk_editable_get_chars (GTK_EDITABLE(nombre),0,-1);
	art_nombre = nombre;
}


void
on_btn_guardar_clicked_tipo            (GtkEntry       *tipo,
                                        gpointer         user_data)
{
	str_articulo.tipo = gtk_editable_get_chars (GTK_EDITABLE(tipo),0,-1);
	art_tipo = tipo;
}


void
on_btn_guardar_clicked_guarda_actualiza
                                        (GtkCList       *lista_articulo,
                                        gpointer         user_data)
{
	int  err;

	//Variables mysql

	long int ultimo_id=0;
	char sqllista[200];
	char sqlrepetido[250];
	char sqlenprecios[100];
	char sqlactualiza[300];
	char sqlinserta[300];
	char sqlnlistas[50] = "SELECT id_lista FROM Lista";

  if(fila_linea_articulo == -1)
	Info("Seleccione una linea de articulo");
  else
  {

	if(conecta_bd() == -1)
    	{
		Err_Info("No se pudo conectar a la base de datos.");
		g_print ("Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		if ( strcmp(str_articulo.codigo,"") == 0)
		{
			Info ("Por favor escriba el codigo del producto");
		}
		else if (strcmp(str_articulo.nombre,"") == 0)
		{
			Info ("Por favor escriba el nombre del producto");
		}
		else if (strcmp(str_articulo.tipo,"") == 0)
		{
			Info ("Por favor seleccione el tipo del producto");
		}
		else if(indice_articulo != -1)				//Se va a actualizar el producto...
		{
					disable_articulo();
					mysql_free_result (res);
					strcpy(sqlactualiza, "UPDATE Articulo SET  codigo =\"");

					if(str_articulo.codigo)
					   	strcat(sqlactualiza, str_articulo.codigo);
				   	strcat(sqlactualiza,"\" ");
     					strcat (sqlactualiza,", nombre=\"");
					if(str_articulo.nombre)
					   	strcat(sqlactualiza, str_articulo.nombre);
				   	strcat(sqlactualiza,"\" ");
					strcat (sqlactualiza,", tipo =\"");
					if(strcmp(str_articulo.tipo,"pieza")== 0)
						strcat(sqlactualiza, "pieza");
					if(strcmp(str_articulo.tipo,"peso") == 0)
					   	strcat(sqlactualiza, "peso");

					strcat(sqlactualiza,"\"");

					strcat(sqlactualiza, ", id_linea = ");
					strcat(sqlactualiza, id_linea_articulo);

					strcat(sqlactualiza," WHERE id_articulo = ");
					strcat(sqlactualiza,id_actualiza_articulo);
					printf("La cadena para actualizar: %s\n", sqlactualiza);

					err = mysql_query(&mysql,sqlactualiza);
					if (err != 0)
					{
						Err_Info("Ocurrio un error al actualizar el producto");
						printf("Error: %s\nLa cadena: %s\n", mysql_error(&mysql),sqlactualiza);
					}
					else
						Info("Articulo registrado");
		}
		else // Es nuevo....
		{
				sprintf(sqlrepetido, "SELECT * FROM Articulo WHERE (codigo = %s OR nombre = '%s')", str_articulo.codigo, str_articulo.nombre);
				err = mysql_query(&mysql, sqlrepetido);
				if(err != 0)
				{
					Err_Info("Hay un error en la consulta a la base de datos");
					printf("%s\n",mysql_error(&mysql));
				}
				else
				{
					res = mysql_store_result(&mysql);

					if (mysql_num_rows (res) > 0)  //condicion de datos repetidos
					{
						Err_Info ("Datos repetidos");
						mysql_free_result (res);
					}
					else //si no hay datos repetidos
					{
						disable_articulo();

						strcpy(sqlinserta, "INSERT INTO Articulo (id_articulo,codigo, nombre, tipo, imagen, id_linea) VALUES(");
						strcat(sqlinserta, "NULL,");
						strcat(sqlinserta, "\"");
     						if(str_articulo.codigo)
							strcat(sqlinserta, str_articulo.codigo);
						   	strcat(sqlinserta,"\",\"");
						if(str_articulo.nombre)
						   	strcat(sqlinserta, str_articulo.nombre);
						   	strcat(sqlinserta,"\",\"");
      						if(strcmp(str_articulo.tipo,"pieza")== 0)
						   	strcat(sqlinserta, "pieza");
						if(strcmp(str_articulo.tipo,"peso") == 0)
						   	strcat(sqlinserta, "peso");

						strcat(sqlinserta,"\",NULL,");
						strcat(sqlinserta, id_linea_articulo);
						strcat(sqlinserta,")");

						printf("La cadena para insertar: %s\n", sqlinserta);

						err = mysql_query(&mysql,sqlinserta);
						if (err != 0)
						{
							Err_Info("No se puede agregar el articulo");
							printf("%s\n", mysql_error(&mysql));
						}

//Se obtiene el id del nuevo articulo agregadp para ponerlo en el arreglo

						ultimo_id = mysql_insert_id(&mysql);
						if(ultimo_id != 0)
						{
							sprintf(id_nuevo_articulo,"%ld", ultimo_id);
							printf("El id del Articulo= %s\n", id_nuevo_articulo);
							err = mysql_query(&mysql,sqlnlistas);
							if(err == 0)
							{
								res = mysql_store_result(&mysql);
								if(res)
								{
									while((row = mysql_fetch_row(res)))
									{
										sprintf(sqlenprecios, "INSERT INTO Articulo_Lista VALUES(NULL, %s, %s, 0)", id_nuevo_articulo, row[0]);
										err = mysql_query(&mysql, sqlenprecios);
										if(err != 0)
										{
											sprintf(Errores, "%s",mysql_error(&mysql));
											Err_Info(Errores);
											Info("Consulte a su asesor en Matica LX");
										}
									}
								}
								else
									Err_Info("Un error al obtener el resultado de la consulta de las listas...");
							}
							else
								Err_Info("No se pudieron obtener las listas de precios");
						}
						else
							printf("\nNo se cambió el precio\n");
					} //fin de la condicion de datos repetidos
				}

		}
	}
	mysql_close (&mysql);

	//ACTUALIZA LA LISTA DE ARTICULOS

	sprintf(sqllista, "SELECT id_articulo, codigo, nombre, tipo FROM Articulo WHERE (nombre LIKE \"%%%s%%\" OR codigo LIKE '%s%%') AND id_linea = %s ORDER BY codigo", criterio_articulo, criterio_articulo, id_linea_articulo);
	busquedas(sqllista, lista_articulo);
  }
}

void
on_btn_nuevo_clicked                   (GtkEntry       *codigo,
                                        gpointer         user_data)
{
	int err;
	int n_codigo;
	char sqlcodigo[200];

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		//Se busca el ultimo codigo
		if(fila_linea_articulo == -1)
			Info("Seleccione una linea de articulo");
		else
		{
			sprintf(sqlcodigo, "SELECT MAX(codigo) FROM Articulo WHERE id_linea = %s", id_linea_articulo);
			printf("Cadena SQL: %s\n", sqlcodigo);
			err = mysql_query (&mysql,sqlcodigo);
			if (err == 0)
			{
				res = mysql_store_result (&mysql);
				if((row = mysql_fetch_row (res)))
				{
					if(row[0])
					{
						n_codigo = atoi (row[0]);
						n_codigo ++;
						sprintf (row[0],"%d",n_codigo);
						gtk_entry_set_text (codigo,row[0]); //Se asigna un codigo
						str_articulo.codigo = row[0];
					}
					else
					{
						Info("Nueva Linea de Productos");
						gtk_entry_set_text (codigo,"1"); //Se asigna 1 si no hay articulos
						str_articulo.codigo = "1";
					}
				}
				indice_articulo = -1;
			}
		}
		gtk_widget_set_sensitive (GTK_WIDGET(codigo),TRUE);
	}
	
	mysql_close (&mysql);
}


void
on_btn_nuevo_clicked_tipo              (GtkEntry       *tipo,
                                        gpointer         user_data)
{
//	gtk_entry_set_text (tipo,"");
	if(fila_linea_articulo != -1)
		gtk_widget_set_sensitive (GTK_WIDGET(tipo),TRUE);
}


void
on_btn_nuevo_clicked_nombre            (GtkEntry       *nombre,
                                        gpointer         user_data)
{
	gtk_entry_set_text (nombre,"");
	if(fila_linea_articulo != -1)
	{
		gtk_widget_set_sensitive (GTK_WIDGET(nombre),TRUE);
		gtk_widget_grab_focus (GTK_WIDGET(nombre));
	}
}


void
on_btn_cancelar_clicked_codigo         (GtkEntry       *codigo,
                                        gpointer         user_data)
{
	gtk_entry_set_text (codigo,"");
	if(fila_linea_articulo != -1)
		gtk_widget_set_sensitive (GTK_WIDGET (codigo),FALSE);
}


void
on_btn_cancelar_clicked_nombre         (GtkEntry       *nombre,
                                        gpointer         user_data)
{
	gtk_entry_set_text (nombre,"");
	gtk_widget_set_sensitive (GTK_WIDGET (nombre),FALSE);
}


void
on_btn_cancelar_clicked_tipo           (GtkEntry       *tipo,
                                        gpointer         user_data)
{
//	gtk_entry_set_text (tipo,"");
	gtk_widget_set_sensitive (GTK_WIDGET (tipo),FALSE);
}


void
on_entry_BusquedaArticulo_activate     (GtkEntry       *entry,
                                        gpointer         user_data)
{

}


void
on_entry_BusquedaArticulo_activate_busca (GtkCList        *listaArticulos,
                                        gpointer         user_data)
{
	gchar *param;
	char cad_SQL[300] = "SELECT id_articulo, codigo, nombre, tipo FROM Articulo WHERE ";
	
	param = gtk_editable_get_chars(GTK_EDITABLE(user_data), 0, -1);

	strcpy(criterio_articulo, param);

	strcat(cad_SQL,"(codigo like '");
	strcat(cad_SQL,param);
	strcat(cad_SQL,"%' or nombre like \"%");
	strcat(cad_SQL,param);
	strcat(cad_SQL,"%\")");

	if(fila_linea_articulo != -1)
	{
		printf("Si checa la linea...\n");
		strcat(cad_SQL, " AND id_linea = ");
		strcat(cad_SQL, id_linea_articulo);
	}
	strcat(cad_SQL, " ORDER BY codigo");
	
	printf("La cadena SQL: %s\n", cad_SQL);
	busquedas (cad_SQL,listaArticulos);
}

void
on_btn_borrar_clicked                  (GtkCList     *lista,
                                        gpointer         user_data)
{
	GtkWidget *msg_err;
	
	lista_borrada = lista;

	msg_err = create_dialog_Articulo_Eder();
	strcpy(mensaje, "Realmente desea borrar el Articulo.");
	gtk_widget_show (msg_err);
}


//Funcion para dialogo OK_CANCEL
void
on_dialog_Articulo_Eder_show           (GtkLabel 	*label,
                                        gpointer         user_data)
{
	gtk_label_set_text (label,mensaje);
}


void
on_btn_Cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy (GTK_WIDGET(button));
}


void
on_btn_OK_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
/*	int err;
	char cad_SQL[200] = "DELETE FROM Articulo where codigo = ";
	int indice;

if(str_articulo.codigo)
{
	strcat(cad_SQL,str_articulo.codigo);
//	strcat (cad_SQL,id_articulo_arreglo[indice_id_articulo]);

	printf("El Codigo del Articulo: %s\n", str_articulo.codigo);
	
	if(conecta_bd() == -1)
    	{
      		Err_Info("No se pudo conectar a la base de datos.");
		g_print ("Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		//Se busca el ultimo codigo
		err = mysql_query (&mysql,cad_SQL);
//		err = 0;
//		g_print ("%s \n",cad_SQL);
		if (err == 0)
		{
			Info ("Articulo borrado satisfactoriamente.");
		}
	}
	mysql_close(&mysql);

	gtk_widget_destroy (GTK_WIDGET(button));
	gtk_clist_remove (lista_borrada,indice_id_articulo);

	if (indice_id_articulo < nrows_articulos) //Si el articulo qyue se borro no es el ultimo se recorren los id
	{
		for (indice=indice_id_articulo; indice < nrows_articulos-1; indice++)
		{
			id_articulo_arreglo[indice] = id_articulo_arreglo[indice+1];
		}
	}
}
else
	Info("Seleccione una Articulo");
*/
}


void
on_btn_borrar_clicked_codigo           (GtkEntry       *codigo,
                                        gpointer         user_data)
{
	gtk_entry_set_text (codigo,"");
	gtk_widget_set_sensitive (GTK_WIDGET(codigo),FALSE);
}


void
on_btn_borrar_clicked_nombre           (GtkEntry       *nombre,
                                        gpointer         user_data)
{
	gtk_entry_set_text (nombre,"");
	gtk_widget_set_sensitive (GTK_WIDGET(nombre),FALSE);
}


void
on_btn_borrar_clicked_tipo             (GtkEntry       *tipo,
                                        gpointer         user_data)
{
	gtk_entry_set_text (tipo,"");
	gtk_widget_set_sensitive (GTK_WIDGET(tipo),FALSE);
}


void
on_entry_folio_activate_desc           (GtkScrolledWindow        *scroll,
                                        gpointer         user_data)
{
	scroll_desc = scroll;
}


void
on_entry_folio_activate_observacion    (GtkScrolledWindow        *scroll,
                                        gpointer         user_data)
{
	scroll_obs = scroll;
}

void
on_btn_cancela_venta_clicked_cancela   (GtkButton       *button,
                                        gpointer         user_data)
{
	Info ("En construccion");
}


void //Funcion para mostrar ventana de cancelacion de venta
on_cancelacion_venta_show              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show (create_Cancela_Venta_Eder());
}

void
on_entry_folio_activate_id             (GtkEntry        *entry,
                                        gpointer         user_data)
{
	id_venta = gtk_editable_get_chars (GTK_EDITABLE (entry),0,-1);
}

void
on_entry_folio_activate_lista          (GtkCList        *clist,
                                        gpointer         user_data)
{
	int err;
	char *lista[4];
	char cad_SQL[500]= "SELECT Articulo.nombre, FORMAT(Venta_Articulo.cantidad,2), FORMAT(Venta_Articulo.Precio,2), FORMAT(Venta_Articulo.monto,2), FORMAT(Venta.monto,2) FROM Venta, Venta_Articulo, Articulo WHERE Venta_Articulo.id_articulo = Articulo.id_articulo AND Venta.cancelada = 'n' AND Venta.id_venta = Venta_Articulo.id_venta AND Venta.id_venta = ";
   if(id_venta)
   {
	if (strlen(id_venta) > 0)
	{
		strcat (cad_SQL,id_venta);

		//Se realiza la consulta de la descripcion
		if(conecta_bd() == -1)
    		{
      			Err_Info("No se pudo conectar a la base de datos.");
			g_print ("Error : %s\n", mysql_error(&mysql));
    		}
		else
		{

			err = mysql_query (&mysql,cad_SQL);

			if (err == 0)
			{
				gtk_clist_clear (clist);
				gtk_clist_set_column_justification (clist,1,GTK_JUSTIFY_RIGHT);
				gtk_clist_set_column_justification (clist,2,GTK_JUSTIFY_RIGHT);
				gtk_clist_set_column_justification (clist,3,GTK_JUSTIFY_RIGHT);
				res = mysql_store_result (&mysql);
				if (mysql_num_rows(res) > 0)
				{
				while((row = mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					lista[3] = row[3];
					total_venta_cancelada = row[4];
					gtk_widget_set_sensitive(GTK_WIDGET (scroll_desc),TRUE);
					gtk_widget_set_sensitive(GTK_WIDGET (scroll_obs),TRUE);
					gtk_clist_append (clist,lista);
				}
				}
				else
				{
					Info ("La venta no existe o ya ha sido cancelada.");
					id_venta = NULL;
				}
			}
			else
			{
				Err_Info("Error en la consulta.");
				g_print ("Error : %s\n", mysql_error(&mysql));
			}

		}

		mysql_close (&mysql);
	}
   }
}


void
on_btn_cancela_venta_clicked_desc      (GtkScrolledWindow       *scroll,
                                        gpointer         user_data)
{
//	gtk_widget_set_sensitive (GTK_WIDGET (scroll),FALSE);
}


void
on_btn_cancela_venta_clicked_obs       (GtkScrolledWindow       *scroll,
                                        gpointer         user_data)
{
//	gtk_widget_set_sensitive (GTK_WIDGET (scroll),FALSE);
}


void
on_btn_cancela_venta_clicked_cancelar  (GtkTextView       *textview,
                                        gpointer         user_data)
{
  if(id_venta)
  {
	widget_scroll_obs = GTK_WIDGET (textview);
	gtk_widget_show (create_dialog_Cacela_Venta());
  }
}

void
on_entry_folio_activate_total          (GtkLabel        *total,
                                        gpointer         user_data)
{
	char markup[80];

	sprintf(markup, "<span color=\"red\" size=\"13000\">%s</span>", total_venta_cancelada);
	if (strcmp(total_venta_cancelada,"") != 0)
		gtk_label_set_markup (total,markup);
}


void
on_btn_cancelaventabuscar_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_folio;

	entry_folio = lookup_widget(GTK_WIDGET(button), "entry_folio");
	entry_global = entry_folio;

	gtk_widget_show(create_Buscar_venta());
}



void
on_cancelbutton_cancela_venta_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy(GTK_WIDGET (gtk_widget_get_toplevel(GTK_WIDGET(button))));
}


void
on_okbutton_cancela_venta_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  if(id_venta)
  {
	gtk_widget_destroy(GTK_WIDGET (gtk_widget_get_toplevel(GTK_WIDGET(button))));
	Cancela_Venta ();
  }
}

//Funcion para cancelar la venta

void Cancela_Venta()
{
	GtkWidget *button;
	GtkTextBuffer *buff;
	GtkWidget *window;
	char *data;
	GtkTextIter start, end;
	char cad_SQL[200];
	char cad_SQL2[200];
	char cad_SQL3[200];
	char sqlinventario[200];
	char sqlarticulos[200];
	char sqltmp[100];

	button = GTK_WIDGET (widget_scroll_obs);
	window = gtk_widget_get_toplevel(GTK_WIDGET(button));

	gtk_widget_set_sensitive (GTK_WIDGET (button),TRUE);

	buff = gtk_text_view_get_buffer (GTK_TEXT_VIEW(button));

	gtk_text_buffer_get_iter_at_offset (buff, &start, 0);
	gtk_text_buffer_get_iter_at_offset (buff, &end, -1);

	data = gtk_text_buffer_get_text(buff,&start,&end,TRUE);

	if (strcmp (data,"") != 0)
	{
		sprintf(cad_SQL3, "SELECT id_usuario FROM Venta WHERE id_venta = %s",id_venta);
		sprintf(cad_SQL, "INSERT INTO Venta_Cancelada values (NULL, %s, \"%s\")", id_venta, data);
		//g_print ("%s\n",cad_SQL);

		if (conecta_bd() == -1)
	    	{
	      		Err_Info("No se pudo conectar a la base de datos.");
			g_print ("Error : %s\n", mysql_error(&mysql));
	    	}
		else
		{
			er = mysql_query(&mysql,cad_SQL3);
			if(er == 0)
			{
				if((res = mysql_store_result(&mysql)))
				{
					if((row=mysql_fetch_row(res)))
					{
						if(strcmp(row[0],id_sesion_usuario) == 0)
						{
							er = mysql_query (&mysql,cad_SQL);
							if (er == 0)
							{
								sprintf(cad_SQL2, "UPDATE Venta SET cancelada = 's' WHERE id_venta = %s", id_venta);
								er = mysql_query (&mysql,cad_SQL2);
								if (er == 0)
								{
									sprintf(sqlarticulos,"SELECT id_articulo, cantidad FROM Venta_Articulo WHERE id_venta = %s", id_venta);
									er = mysql_query(&mysql, sqlarticulos);
									if(er == 0)
									{
										if((res = mysql_store_result(&mysql)))
										{
											while((row = mysql_fetch_row(res)))
											{
												sprintf(sqlinventario, "UPDATE Inventario SET cantidad = cantidad + %s WHERE id_articulo = %s", row[1], row[0]);
												er = mysql_query(&mysql, sqlinventario);
												if(er != 0)
												{
													sprintf(Errores, "No se pudo reintegrar el producto al inventario\n%s", mysql_error(&mysql));
													Err_Info (Errores);
												}
											}
										}
									}
									else
									{
										sprintf(Errores, "Error: %s", mysql_error(&mysql));
										Err_Info (Errores);
									}

									/*sprintf(sqltmp, "INSERT INTO Tabla_TMP VALUES(NULL, %s, 'cancelacion', %s)", id_venta, id_sesion_caja);
									er = mysql_query(&mysql, sqltmp);
									if(er == 0)	
									{*/
									Info ("Venta cancelada");
									gtk_widget_destroy(GTK_WIDGET (window));
									/*}
									else
										Err_Info ("Error en el registro para el corte de caja...");*/
								}
								else
								{
									Err_Info ("Error en la cancelacion en la tabla Venta_cancelada");
									g_print ("Error = %s", mysql_error(&mysql));
								}
							}
							else
							{
								Err_Info ("Error en la cancelacion en la tabla Venta");
								g_print ("Error = %s", mysql_error(&mysql));
							}
						}
						else
						{
							Err_Info ("Imposible cancelar la venta.\nSolo el usuario que la realizo puede hacerlo.");
							gtk_widget_destroy(GTK_WIDGET (window));
						}
					}
				}
			}
			else
				printf("Error: %s\n", mysql_error(&mysql));
		}
		mysql_close(&mysql);
		gtk_window_present(GTK_WINDOW(caja));
	}
	else
	{
		Err_Info ("Por favor escriba alguna observaciÃ²n por la cual se esta cancelando esta venta.");
	}
	
}



void
on_cancelar_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *Cancela_Venta_Eder;
	Cancela_Venta_Eder = create_Cancela_Venta_Eder();
	gtk_widget_show(Cancela_Venta_Eder);
}




/*********************************
OVIEDO
*********************************/




void
on_retiro_de_efectivo_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{




	if(bRetiro_Efectivo == FALSE)
		{
			Retiro_Efectivo = create_Retiro_Efectivo();
			gtk_widget_show(Retiro_Efectivo);

			bRetiro_Efectivo = TRUE;
		}
		else
		{
			gtk_window_present(GTK_WINDOW(Retiro_Efectivo));
		}

}


gboolean
on_Retiro_Efectivo_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_destroy(widget);
  bRetiro_Efectivo = FALSE;

  return FALSE;
}


void
on_txtRetiro_Password_activate         (GtkEntry        *txtRetiro_Usuario,
                                        gpointer         user_data)
{
 	char sql[300];
	int er;
	gboolean Verdadero = FALSE;

	GtkEntry * txtRetiro_Password = user_data;
	Retiro_Efectivo_Password = gtk_editable_get_chars(GTK_EDITABLE(txtRetiro_Password), 0, -1);

	Retiro_Efectivo_Usuario = gtk_editable_get_chars(GTK_EDITABLE(txtRetiro_Usuario), 0, -1);
	g_print(Retiro_Efectivo_Usuario);

	g_print(Retiro_Efectivo_Password);
	Retiro_Bandera = 0;
	if(conecta_bd() == -1){
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else{

		strcpy(sql,"SELECT  id_usuario  FROM  Usuario  WHERE 1  AND username  =  '");
		strcat(sql, Retiro_Efectivo_Usuario);
		strcat(sql,"' AND password = '");
		strcat(sql,Retiro_Efectivo_Password);
		strcat(sql,"' AND tipo = 'admin'");

		if((er = mysql_query(&mysql, sql)) != 0)
			printf("Error al consultar los tipos de documentos: %s\n", mysql_error(&mysql));
		else{
			if((res = mysql_store_result(&mysql))){
				if (mysql_num_rows(res) > 0){ //Todo CORRECTO!..

					if((row = mysql_fetch_row(res))){
						Retiro_Bandera = 1;
						gtk_widget_set_sensitive        (GTK_WIDGET(txtRetiro_Usuario),Verdadero);
						gtk_widget_set_sensitive        (GTK_WIDGET(txtRetiro_Password),Verdadero);
						Retiro_Autoriza = atoi(row[0]);
					}else{
						Err_Info("Error al obtener la clave del administrador\nIntentelo de nuevo");
					}
				}else{
					Retiro_Bandera = 0;
					Err_Info("No tienes permisos para retirar efectivo\nVerifica la clave del administrador");
					printf("No eres administrador\n");
				}
			}

		}

		mysql_close(&mysql); //Cierra conexion SQL
	}

}





void
on_txtRetiro_Password_activate_t       (GtkEntry        *txtRetiro_Cantidad,
                                        gpointer         user_data)
{
	if(Retiro_Bandera == 1){

		gtk_widget_show(GTK_WIDGET(txtRetiro_Cantidad)); //Abilita la ventana_oviedo
	}
}


void
on_txtRetiro_Password_activate_l       (GtkEntry        *lblCantidad,
                                        gpointer         user_data)
{
	//Para mostrar
	if(Retiro_Bandera == 1){
		gtk_widget_show(GTK_WIDGET(lblCantidad)); //Abilita la ventana_oviedo
	}
}


void
on_txtRetiro_Password_activate_b       (GtkEntry        *btnRetiro_Aceptar,
                                        gpointer         user_data)
{
	//Para mostrar
	if(Retiro_Bandera == 1){
		gtk_widget_show(GTK_WIDGET(btnRetiro_Aceptar)); //Abilita la ventana_oviedo
	}
}

void
on_btnRetiro_Aceptar_clicked           (GtkEntry       *txtRetiro_Cantidad,
                                        gpointer         user_data)
{
	int er;
	float Cantidad_Retiro = 0;
	char sql[300];
	char sqltmp[100];
	char sqlsacarid[100]="SELECT id_retiro FROM Retiro GROUP BY id_retiro DESC LIMIT 1";
	char msg[100];
	gchar *Retiro_Efectivo_Cantidad;


	Retiro_Efectivo_Cantidad = gtk_editable_get_chars(GTK_EDITABLE(txtRetiro_Cantidad), 0, -1);
	g_print(Retiro_Efectivo_Cantidad);

	Cantidad_Retiro = atof(Retiro_Efectivo_Cantidad);

	if(Cantidad_Retiro <= 0){
		Err_Info("Escribe una cantidad valida");
	}else{

		if(conecta_bd() == -1){
			printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
		}
		else{
			strcpy(sql,"");
			sprintf(sql, "INSERT INTO `Retiro` (`id_retiro`, `id_usuario_cajero`, `id_usuario_retira`, `fecha`, `hora`, `monto`, `id_caja`) VALUES ('', '%s', '%d', now(), now(), '%.2f', '%s')",id_sesion_usuario, Retiro_Autoriza, Cantidad_Retiro, id_sesion_caja);
			//printf("%s\n",sql);
			if((er = mysql_query(&mysql, sql)) != 0)
				printf("Error al consultar los tipos de documentos: %s\n", mysql_error(&mysql));
			else{
				if(mysql_affected_rows(&mysql) > 0){
					sprintf(msg,"Se retiro la cantidad de $%.2f",Cantidad_Retiro);

					gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(txtRetiro_Cantidad)));
  					bRetiro_Efectivo = FALSE;

					if((er = mysql_query(&mysql, sqlsacarid)) == 0)
					{
						res = mysql_store_result(&mysql);
						if(res)
						{
							if((row = mysql_fetch_row(res)))
							{
								/*sprintf(sqltmp, "INSERT INTO Tabla_TMP VALUES(NULL, %s, 'retiro', %s)", row[0],id_sesion_caja);
								if((er = mysql_query(&mysql, sqltmp)) == 0)*/
								Info(msg);
								/*else
									Err_Info("OcurriÃ³ un problema en el retiro.\nEs posible que no salga en el\ncorte de caja");*/
							}
						}

					}
					else
					{
						strcpy(msg,"");
						sprintf(msg, "No se pudo sacar el id: %s", mysql_error(&mysql));
						Err_Info(msg);
					}

				}else{
					Err_Info("No se puede agregar el retiro");
				}

			}

			mysql_close(&mysql); //Cierra conexion SQL
		}
	}
}

void
on_acerca_de_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show(create_Acerca_de());
}



void
on_entry_codigo_activate_actualiza_lista
                                        (GtkCList        *clist,
                                        gpointer         user_data)
{
	GtkEntry *entry = user_data;
	GtkWidget *widget;
	GtkWidget *entry_cantidad;

	char sql[650];
	int i,bandera;
	int row_total;//Numero total de row
	gchar *observacion_actual;
	gchar *lista[7];
	gchar *id_codigo_lista; //id en la lista
	float descuento = 0, precio_tmp = 0, precio_normal;
	char precio_char[20],cantidad_enlista[20],elerror[200];
	
	gboolean flag_o;
	
	MYSQL_RES *res_o;
	MYSQL_ROW row_o;
	char sql_o[600], cobservacion[300];


	gtk_clist_set_column_justification(clist,0,GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(clist,2,GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(clist,3,GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_justification(clist,4,GTK_JUSTIFY_RIGHT);

	printf("\nENTRA A PEDIDOS");

	articulo.codigo = gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
  	if (!g_strcasecmp(articulo.codigo,"")) { //Igual  Verdadero
			strcpy(articulo.codigo,"0");
	}
	printf("\n---->> %s",articulo.codigo);


	sprintf(sql,"SELECT Articulo.nombre, ROUND(Articulo_Lista.precio,2), ROUND(Tarjeta_Cliente.descuento,2), Articulo.tipo, Articulo.id_articulo, Articulo_Lista.precio_minimo FROM Articulo_Lista INNER JOIN Articulo ON Articulo_Lista.id_articulo = Articulo.id_articulo INNER JOIN Lista ON Articulo_Lista.id_lista = Lista.id_lista INNER JOIN Cliente ON Lista.id_lista = Cliente.id_lista LEFT JOIN Tarjeta_Cliente ON Tarjeta_Cliente.id_cliente = Cliente.id_cliente AND Tarjeta_Cliente.id_articulo = Articulo.id_articulo WHERE Articulo.codigo = %s AND Cliente.id_cliente = %s", articulo.codigo, codigo_cliente);

/******************************************************************************************************************************************************
*SELECT Articulo_Lista.precio,Tarjeta_Cliente.descuento FROM Articulo_Lista INNER JOIN Articulo ON Articulo_Lista.id_articulo = Articulo.id_articulo *INNER JOIN Lista ON Articulo_Lista.id_lista = Lista.id_lista INNER JOIN Cliente ON Lista.id_lista = Cliente.id_lista LEFT JOIN Tarjeta_Cliente ON *Tarjeta_Cliente.id_cliente = Cliente.id_cliente AND Tarjeta_Cliente.id_articulo = Articulo.id_articulo WHERE Articulo.codigo = xx AND Cliente.*id_cliente = xx;
******************************************************************************************************************************************************/

printf("\n%s",sql);

	widget = lookup_widget(GTK_WIDGET(clist),"entry_cantidad");

	lista[0] = "";
	lista[1] = "";
	lista[2] = "";
	lista[3] = "";
	lista[4] = "";
	lista[5] = "";
	lista[6] = "";
	lista[7] = "";
	//if(conecta_bd == -1)




	if(conecta_bd() == -1)
	{
		printf("No se pudo conectar a la base de datos para actualizar la lista de articulo: %s\n", mysql_error(&mysql));
		er = 1;
	}
	else
	{
		er = mysql_query(&mysql,sql);
		if(er != 0)
		{
			printf("No se puedo realizar el query: %s\n", mysql_error(&mysql));
			er = 1;
		}
		else
		{
			resultado = mysql_store_result(&mysql);
			if(resultado)
			{
				if((row = mysql_fetch_row(resultado)))
				{
					//Se cargan las observaciones anteriores del articulo para este cliente
					//sprintf (sql_o,"SELECT Pedido_Articulo.observaciones FROM Pedido_Articulo INNER JOIN Pedido ON Pedido.id_pedido = Pedido_Articulo.id_pedido WHERE Pedido_Articulo.id_articulo = %s AND Pedido.id_cliente = %s ORDER BY Pedido.fecha DESC LIMIT 1", row[4], codigo_cliente);
					
					if (aplicar_observaciones == TRUE)
					{
						sprintf (sql_o,"Select Pedido_Articulo.observaciones FROM Pedido_Articulo INNER JOIN Pedido ON Pedido.id_pedido = Pedido_Articulo.id_pedido  where Pedido.id_cliente = %s AND Pedido_Articulo.id_articulo= %s ORDER BY Pedido.fecha DESC LIMIT 1", codigo_cliente, row[4]);
						printf ("\n\nSQL_O = %s\n\n",sql_o);
						
						flag_o = FALSE;
						er = mysql_query(&mysql,sql_o);
						if(er != 0)
						{
							printf("No se puedo realizar el query: %s\n", mysql_error(&mysql));
							er = 1;
						}
						else
						{
							res_o = mysql_store_result(&mysql);
							if(res_o)
							{
								if((row_o = mysql_fetch_row(res_o)))
								{
									flag_o = TRUE;
									printf ("Observaciones = %s\n",row_o[0]);
									sprintf(cobservacion,"%s",row_o[0]);
								}
							}
						}
					}
					else
					{
						flag_o = TRUE;
						gtk_clist_get_text(clist,row_counter,5,&observacion_actual);
						sprintf(cobservacion,"%s",observacion_actual);
						//row_o[0] = observacion_actual;
					}
					
					articulo.nombre = row[0];
					printf("\nDESCUENTO: %s, NORMAL %s",row[2],row[1]);
					//printf("\OBSERVACIONES: %s, NORMAL %s",row[2],row[1]);
					if(row[2] != NULL){
						descuento = atof(row[2]);
						precio_tmp = atof(row[1]);
						precio_normal = precio_tmp + descuento;
						sprintf(precio_char,"%.2f",precio_normal);
						articulo.precio = precio_char;
					}else{
						articulo.precio = row[1];
					}
					if(atof(articulo.precio) >= atof(row[5]))
					{
						articulo.tipo = row[3];
						printf("\nCodigo = %s\nNombre = %s\nPrecio = %s\n",articulo.codigo,articulo.nombre,articulo.precio);
						bandera=0;
						row_total = clist->rows;
						if(row_total > 0){ //Es para verificar que no existan dos o mas ID repetidos
							for(i=0;i<row_total;i++){
					   			gtk_clist_get_text(clist,i,0,&id_codigo_lista);
						  		if (!g_strcasecmp(id_codigo_lista, articulo.codigo)) { //Igual  Verdadero
	   								row_counter = i;
		   							bandera= 1;
									break;
								}
							}
						}
						printf(" \nbandera = %d",bandera); //Esta es la bandera
						if(bandera==0) //Agrega un nuevo producto
						{
							row_counter = gtk_clist_prepend(clist,lista); //Agrega el producto arriba de la lista
							//articulo.observaciones = g_utf8_normalize("---",-1,G_NORMALIZE_ALL);
							articulo.observaciones = "";
							gtk_clist_set_text(clist,row_counter,5,articulo.observaciones);
						}else{
							entry_cantidad = lookup_widget(GTK_WIDGET(V_pedidos),"entry_cantidad");
							if(gtk_clist_get_text(clist,row_counter,3,&id_codigo_lista) == 1)
							{
								printf("Codigo lista: %s\n",id_codigo_lista);
								sprintf(cantidad_enlista,"%s",id_codigo_lista);
								gtk_entry_set_text(GTK_ENTRY(entry_cantidad),cantidad_enlista);
							}
						}


						printf("\n ROW COUNTER: %d",row_counter);

						gtk_clist_set_text(clist,row_counter,0,articulo.codigo);
						gtk_clist_set_text(clist,row_counter,1,articulo.nombre);
						gtk_clist_set_text(clist,row_counter,2,articulo.precio);
					
					
					
						if (flag_o == TRUE)
							gtk_clist_set_text(clist,row_counter,5,cobservacion);
						else
							gtk_clist_set_text(clist,row_counter,5,"");
					
						gtk_clist_set_text(clist,row_counter,6,articulo.tipo);
					
						gtk_clist_select_row(clist,row_counter,0);
						gtk_widget_grab_focus(widget);
					}
					else
					{
						sprintf(elerror,"El precio (%s) es menor al precio minimo (%s).\nEs necesario ajustarlo para este cliente.",articulo.precio,row[5]);
						Err_Info(elerror);
					}
				}
				else
				{
					printf("NO HAY ROW DE MYSQL\n");
					gtk_widget_grab_focus(GTK_WIDGET(entry));
					er = 1;
				}
			}else
			{
				printf("No se obtuvo el resultado del query: %s\n", mysql_error(&mysql));
				gtk_widget_grab_focus(GTK_WIDGET(entry));
				er = 1;
			}
		}
	}
	mysql_close(&mysql);


}


void
on_entry_cantidad_activate_actualiza_lista
                                        (GtkCList        *clist,
                                        gpointer         user_data)
{
	GtkWidget *entry_codigo,*label_kilos,*label_total;
	GtkEntry *entry = user_data;
	double cantidad = 0, precio = 0, monto = 0, total = 0, total2 = 0, kilos = 0, kilos2 = 0;
	char dato[10], dato2[10];
	gchar *gcantidad, *gprecio, *gmonto, *gkilos, *gtipo;
	char tipo[8];
	char prec[10], kil[20], mon[300], cant[10];
	int valida, i = 0, k = 0;
	int banderita = 0, bandera = 0;
	int row_total = 0;
	gchar *id_codigo_lista; //id en la lista


	entry_codigo = lookup_widget(GTK_WIDGET(clist),"entry_codigo");
	label_kilos = lookup_widget(GTK_WIDGET(clist),"label_kg");
	label_total = lookup_widget(GTK_WIDGET(clist),"label_total");

	gcantidad = gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
	strcpy(cant,gcantidad);

	//Entra a la funciond e arriba
	aplicar_observaciones=FALSE;
	on_entry_codigo_activate_actualiza_lista(clist,lookup_widget(GTK_WIDGET(clist),"entry_codigo"));
	aplicar_observaciones=TRUE;
	if(er != 1){

		for (i=0;i<=10;i++)
		{
			valida = cant[i];
			if ( valida>=48 || valida <=57 || valida == 46)
				banderita = 1;
			else
			{
				banderita = 0;
				break;
			}
		}

		if(banderita == 1)
		{
			bandera=0;
			row_total = clist->rows;
			if(row_total > 0){ //Es para verificar que no existan dos o mas ID repetidos
				for(i=0;i<row_total;i++){
					gtk_clist_get_text(clist,i,0,&id_codigo_lista);
					if (!g_strcasecmp(id_codigo_lista, articulo.codigo)) { //Igual  Verdadero, osea existe el contador

						row_counter = i;
						printf("\nMe meti al ciclo, y el row_counter es: %d\nY el articulo codigo es: %s",row_counter,articulo.codigo);
						bandera= 1;
						break;
					}
				}
			}


			if(bandera==1){
				printf("\nParte importante");
				gtk_clist_get_text(clist,row_counter,2,&gprecio);
				printf("\nEl precio inicial es: %s",gprecio);
				strcpy(prec,gprecio);

				cantidad = atof(cant);
				printf("\nCantidad = %s", cant);
				precio = atof(prec);

				monto = cantidad * precio;


				printf("\nel row_counter es: %d\n",row_counter);

				sprintf(dato,"%.3f",cantidad);
				sprintf(dato2,"%.2f", monto);
				printf("\nDato que se guarda:  %s\n",dato);
				gtk_clist_set_text(clist,row_counter,3,dato); //Aqui guarda la info en el Clist
				gtk_clist_set_text(clist,row_counter,4,dato2);
				row_total = clist->rows;
				printf("\nRow Total: %d",row_total);

				for(k = 0; k < row_total; k++)
				{
					gtk_clist_get_text(clist,k,3,&gkilos);
					gtk_clist_get_text(clist,k,4,&gmonto);
					gtk_clist_get_text(clist,k,6,&gtipo);
     					strcpy(tipo,gtipo);
					strcpy(mon,gmonto);
					printf("\nTIPO: %s",tipo);
					if(strcmp(tipo,"peso") == 0){ //Solo suma los pesos
						strcpy(kil,gkilos);
						kilos2 = atof(kil);
						kilos = kilos + kilos2;
						printf("\nKIL: %f",kilos);
						printf("\nJKIL2: %f",kilos2);
					}
					total2 = atof(mon);
					total = total + total2;
					printf("\nTotal: %f",total);
					printf("\nTotal2: %f",total2);

				}
				printf("\n");
				sprintf(kil,"%.3f",kilos);
				sprintf(mon,"<span size=\"15000\">%.2f</span>",total);
				gtk_label_set_text(GTK_LABEL(label_kilos),kil);
				gtk_label_set_markup(GTK_LABEL(label_total),mon);


				gtk_widget_grab_focus(entry_codigo);
	//			gtk_widget_show(create_V_observaciones());
			}
		}
		else
			gtk_widget_grab_focus(GTK_WIDGET(entry));


		gtk_entry_set_text(GTK_ENTRY(entry_codigo),"");
		gtk_entry_set_text(GTK_ENTRY(entry),"");
	}else{
		printf("\nSI ERROR");
		gtk_widget_grab_focus(GTK_WIDGET(entry_codigo));
	}
}

void
on_btn_pedidobuscar_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *txtbuscarcliente;

	txtbuscarcliente = lookup_widget(GTK_WIDGET(caja),"txtbuscarcliente");
	entry_global = txtbuscarcliente;

	gtk_widget_show(create_Buscar_cliente());
}




void
on_btn_aceptar_clicked_guarda_pedido   (GtkCList       *clist,
                                        gpointer         user_data)
{
	GtkWidget *txtbuscarcliente;
	GtkWidget *txtservicio;
	GtkCList *prueba = clist;
	GtkWidget *combo_entryHora, *combo_entryMes, *txtAno, *combo_entryDia, *combo_entryMinutos;
	char id_ultimopedido[15];
	gchar row_total;
	gchar *c_servicio;
	int i, contador_lista;
	int er = 0;
	char queryinsertapedido[1500];
	char querypedido[1500];
	char queryarticulo[1500];
	int id_ultimopedido_int = 0;
	char id_ultimopedido_char[30];
	//Variables para el time.h
	int var_ano, var_dia, var_mes, var_hora, var_min;
	gchar  *var_mes_char;
	char var_mes_char2[5], var_dia_char[3], var_ano_char[5], var_hora_char[3], var_min_char[3];
	char fecha_completa[12], hora_completa[9];
	//char s_observacion_pedido[500];
	
	GtkTextBuffer *buffer_observacion_pedido;
	GtkTextIter start,end;
	GtkWidget *tetxv_observacion_pedido;
	gchar *g_observaciones_pedido;
	
	gint result, error;
	error = 0;
	row_total = clist->rows;
	printf("\nNUM ROWS TOTAL: %d",row_total);
	contador_lista = 0;

	txtbuscarcliente = lookup_widget(caja, "txtbuscarcliente");
	
	tetxv_observacion_pedido = lookup_widget(GTK_WIDGET(clist),"txtv_observaciones_pedido");
	txtservicio              = lookup_widget(GTK_WIDGET(clist),"entry_servicio_extra");
	
	if(row_total == 0){ 
		Err_Info("No existen productos para realizar el pedido.");
		error = 1;
	}else{

		//Saca la informacion de los Combos;
		//Valida las fechas
		txtAno = lookup_widget(ventana_oviedo,"txtAno");
		var_ano = atoi(gtk_entry_get_text (GTK_ENTRY(txtAno)));
		strcpy(var_ano_char,"");
		sprintf(var_ano_char,"%d",var_ano);
		printf("\nANIO %s %d ",var_ano_char,var_ano);

		combo_entryMes = lookup_widget(ventana_oviedo,"cmbMes");
		var_mes_char = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(combo_entryMes)->entry),0,-1);
		var_mes = mes_numero(var_mes_char);

		sprintf(var_mes_char2,"%d",var_mes);

		printf("\nMES_ %s %d %s",var_mes_char,var_mes,var_mes_char2);

		combo_entryDia = lookup_widget(ventana_oviedo,"cmbDia");
		var_dia = atoi(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_entryDia)->entry)));
		sprintf(var_dia_char,"%d",var_dia);
		printf("\nDIA: %d\n",var_dia);

		combo_entryHora = lookup_widget(ventana_oviedo,"cmbHora");
		var_hora = atoi(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_entryHora)->entry)));
		sprintf(var_hora_char,"%d",var_hora);
		if(strlen(var_hora_char) == 1){
			var_hora_char[1] = var_hora_char[0];
			var_hora_char[0] = '0';
			var_hora_char[2] = '\0';
		}


		combo_entryMinutos = lookup_widget(ventana_oviedo,"cmbMinutos");
		var_min = atoi(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_entryMinutos)->entry)));
		sprintf(var_min_char,"%d",var_min);
		if(strlen(var_min_char) == 1){
			var_min_char[1] = var_hora_char[0];
			var_min_char[0] = '0';
			var_min_char[2] = '\0';
		}


		//Pasa las fechas a la variable fecha_completa

		//Pasa el aÃ±o
		if(strlen(var_ano_char) >= 4){
			fecha_completa[0] = var_ano_char[0];
			fecha_completa[1] = var_ano_char[1];
			fecha_completa[2] = var_ano_char[2];
			fecha_completa[3] = var_ano_char[3];
			//Pasa el Anio
		}else{
			fecha_completa[0] = ' ';
			fecha_completa[1] = ' ';
			fecha_completa[2] = ' ';
			fecha_completa[3] = ' ';
		}
		//Pone la diagonal de la fecha
		fecha_completa[4] = '-'; //DIAGONAL
		//Pasa el Mes
		if(strlen(var_mes_char2) == 2){
			fecha_completa[5] = var_mes_char2[0];
			fecha_completa[6] = var_mes_char2[1];
		}else{
			fecha_completa[5] = '0';
			fecha_completa[6] = var_mes_char2[0];
		}
		fecha_completa[7] = '-';
		//Pasa el Dia
		if(strlen(var_dia_char) == 2){
			fecha_completa[8] = var_dia_char[0];
			fecha_completa[9] = var_dia_char[1];
		}else{
			fecha_completa[8] = '0';
			fecha_completa[9] = var_dia_char[0];
		}
		fecha_completa[10] = '\0';
		printf("\n%s",fecha_completa);

		//Pasa la Hora
		sprintf(hora_completa,"%s:%s:00",var_hora_char, var_min_char);



		if (es_fecha(fecha_completa) == 0){
			Err_Info("La fecha no es correcta.");
			error = 1;
		}else{
			if(conecta_bd() == -1)
			{
				printf("No se pudo conectar a la base de datos: %s\n", mysql_error(&mysql));
			}
			else
			{
				sprintf(querypedido,"SELECT  '%d' < DATE_FORMAT( NOW(  ) ,  '%%Y'  )",var_ano);
				printf("\n%s\n",querypedido);
				er = mysql_query(&mysql,querypedido);
				if(er != 0){
					printf("Error al consultar la fecha actual\n");
					error=1;
				}else{
					resultado = mysql_store_result(&mysql);
					if(resultado)
					{
						row = mysql_fetch_row(resultado);
						if(row)
						{
							if(strcmp(row[0],"1")== 0){ //1 LA fecha es menjor a la actual
								Err_Info("El a\303\261o del pedido no puede ser menor al actual");
								error = 1;
							}
						}else{
							printf("\n Error al consultar la fecha actual");
							error = 1;
						}
					}else{
						printf("\n Error al consultar la fecha actual");
						error = 1;
					}
				}
			}
			mysql_close(&mysql);

			if(error == 0){ //Valida que la fecha no sea menor.
				if(conecta_bd() == -1)
				{
					printf("No se pudo conectar a la base de datos: %s\n", mysql_error(&mysql));
				}
				else
				{
					sprintf(querypedido,"SELECT  '%s %s' < NOW(  )",fecha_completa,hora_completa);
					printf("\n%s\n",querypedido);
					er = mysql_query(&mysql,querypedido);
					if(er != 0){
						printf("Error al consultar la fecha actual\n");
						error=1;
					}else{
						resultado = mysql_store_result(&mysql);
						if(resultado)
						{
							row = mysql_fetch_row(resultado);
							if(row)
							{
								if(strcmp(row[0],"1")== 0){ //1 LA fecha es menjor a la actual
									Err_Info("La fecha del pedido no puede ser menor al actual");
									error = 1;
								}
							}else{
								printf("\n Error al consultar la fecha actual");
								error = 1;
							}
						}else{
							printf("\n Error al consultar la fecha actual");
							error = 1;
						}
					}
				}
				mysql_close(&mysql);
			}
		}

printf("\nInicia Pedidos\n");


		if(error == 0){ //Valida los productos
			for(i=0;i<row_total;i++){
				gtk_clist_get_text(GTK_CLIST(prueba),i,3,&listaX[3]);
				if(strcmp(listaX[3],"")== 0 || strcmp(listaX[3],"0.000") == 0 ){
					contador_lista ++;
				}
			}
			printf("\nCONTAODR: %d\n",contador_lista);
			if(contador_lista == row_total){
				Err_Info("Ningun producto contiene una cantidad mayor a cero.");
				error = 1;
			}else if(contador_lista > 0){
				result = Confirma("Algunos productos no contienen una cantidad mayor a cero\nSi aceptas, estos productos no seran parte del pedido.");
				switch (result)
				{
					case GTK_RESPONSE_ACCEPT:
							printf("\nSe aceptan los productos ");
							error = 0;
						break;
					default:
							printf("\nNo se aceptan los productos");
							error = 1;
						break;
				}
				printf("\nNo se puede guardar la info - NUM ROW%d",row_total);
			}
		}
		else
			printf("Error = 0\n");

		printf("\n");

		if(error == 0){
			if(conecta_bd() == -1)
			{
				printf("No se pudo conectar a la base de datos para actualizar la lista de articulo: %s\n", mysql_error(&mysql));
			}
			else
			{
				/*buffer_observacion_pedido = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tetxv_observacion_pedido));
				
				gtk_text_buffer_get_iter_at_offset(buffer_observacion_pedido,&ini,0);
				gtk_text_buffer_get_iter_at_offset(buffer_observacion_pedido,&fin,-1);
				
				var_mes_char = gtk_text_buffer_get_text(buffer_observacion_pedido,&ini,&fin,TRUE);
				//sprintf (s_observacion_pedido,"%s",c_observaaciones_pedido);*/
				
				c_servicio = gtk_editable_get_chars(GTK_EDITABLE(txtservicio),0,-1);
				
				g_observaciones_pedido = "";
				
				buffer_observacion_pedido = gtk_text_view_get_buffer (GTK_TEXT_VIEW(tetxv_observacion_pedido));
				
				gtk_text_buffer_get_iter_at_offset (buffer_observacion_pedido, &start, 0);
				gtk_text_buffer_get_iter_at_offset (buffer_observacion_pedido, &end, -1);
				
				g_observaciones_pedido = gtk_text_buffer_get_text(buffer_observacion_pedido,&start,&end,TRUE);
				
				//g_observaciones_pedido[strlen(g_observaciones_pedido)] = '\0';
				
				sprintf(querypedido,"INSERT INTO Pedido  ( `id_pedido` , `id_venta` , `id_cliente` , `fecha` , `hora` , `id_usuario` , `id_carnicero` , `id_repartidor` , `HoraEntrega` , `FechaEntrega` , `HoraSalida` , `HoraLLegada` , `id_caja`,`observacion_pedido`, servicio ) VALUES (NULL, NULL, %s, CURDATE(), CURTIME(),%s, 0, 0,'%s','%s', 0, 0, %s, '%s', '%s')",codigo_cliente,id_sesion_usuario, hora_completa, fecha_completa, id_sesion_caja, g_observaciones_pedido, c_servicio);

				printf("\nPedido: %s",querypedido);
				er = mysql_query(&mysql,querypedido);
				if(er != 0)
					printf("Error al insertar en la tabla Pedido: %s\n", mysql_error(&mysql));
				else
				{
					id_ultimopedido_int = mysql_insert_id (&mysql); //Saca el ultimo ID insertado
					printf("\nNumero del ultimo Pedido: %d",id_ultimopedido_int);


					printf("\n NUM ROW%d",row_total);
					printf("\n NUM ROW2%d",row_counter);
					sprintf(id_ultimopedido,"%d",id_ultimopedido_int);
					printf("\nNumero del ultimo Pedido: %d",id_ultimopedido_int);
					for (i=0 ; i <= row_total ; i++)
					{

						gtk_clist_get_text(GTK_CLIST(prueba),i,0,&listaX[0]);
						gtk_clist_get_text(GTK_CLIST(prueba),i,3,&listaX[3]);
						gtk_clist_get_text(GTK_CLIST(prueba),i,5,&listaX[5]);

						printf("Articulo : %s\ni = %d\n",listaX[0],i);
						printf("Observaciones : %s\ni = %d\n",listaX[5],i);
						if(strcmp(listaX[3],"") != 0  && strcmp(listaX[3],"0.000") != 0 ){
							if (listaX)
							{
								sprintf(queryarticulo,"SELECT id_articulo FROM Articulo WHERE codigo = '%s'",listaX[0]);

								er = mysql_query(&mysql,queryarticulo);
								if (er == 0)
								{
									resultado = mysql_store_result(&mysql);
									if(resultado)
									{
										row = mysql_fetch_row(resultado);
										if(row)
										{
											sprintf(queryinsertapedido,"INSERT INTO Pedido_Articulo VALUES(NULL,%s,%s,%s,'%s')",id_ultimopedido,row[0],listaX[3],listaX[5]);
											printf("\n%s",queryinsertapedido);
											er = mysql_query(&mysql, queryinsertapedido);
											if(er != 0)
											{
												printf("Fallo el query para insertar en Pedido_Articulo: %s\n", mysql_error(&mysql));
											}
											else
											{
												printf("INSERT correcta\n");

												listaX[0] = "";
												listaX[1] = "";
												listaX[2] = "";
												listaX[3] = "";
												listaX[4] = "";
												listaX[5] = "";
											}
										}
										else
										{
											printf("No hubo row\n");
										}
									}
									else
									{
										printf("Sabequepaso\n");
									}
								}
								else
								{
									printf("hubo un error: %s\n", mysql_error(&mysql));
								}
							}
							else
							{
								printf("Paso algo\n");
							}
						}//Fin de No tiene Cantidad
					}//Fin del FOR
					//Cierra la ventana!..

					gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(clist)));

				}
			}
		}//Fin de no se aceptan los productos
		mysql_close(&mysql);
		if(er == 0)
		{
			sprintf (id_ultimopedido_char,"%d",id_ultimopedido_int);
			imprimirticket(id_ultimopedido_char,"pedidos_paso1",0);
		}
		gtk_signal_emit_by_name(GTK_OBJECT(txtbuscarcliente),"activate");
	}//Fin del Else de no existen productos
}


void
on_btn_cancelar_clicked_cierra_ventana (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}


void
on_V_pedidos_show_llena_lista          (GtkCList       *clist,
                                        gpointer         user_data)
{
	gchar *list[2];
	char querycliente[300];
	int er, i;
	/*gchar *list[2];
	char querycliente[300];
	int er, i;*/
	
	
	/*DATOS DE ENVIO*/
	
	V_pedidos = gtk_widget_get_toplevel(GTK_WIDGET(clist));
	GtkWidget *widget = lookup_widget(GTK_WIDGET(clist),"V_pedidos");
	
	GtkWidget *txtv_observaciones_articulo;
	GtkTextBuffer *buffer;
	
	GtkWidget *lbl_envionombre;
	GtkWidget *entry_enviodireccion;
	GtkWidget *entry_envioentre_calles;
	GtkWidget *entry_enviocolonia;
	GtkWidget *entry_enviociudad;
	GtkWidget *entry_enviocp;
	GtkWidget *entry_enviotelefono;
	GtkNotebook *notebook_datos;
	GtkWidget *lbl_enivar_a;
	char sqldatos[300];

	lbl_enivar_a = lookup_widget(widget, "lbl_enivar_a");
	notebook_datos = GTK_NOTEBOOK( lookup_widget(widget, "gtk_notebook_datos"));
	lbl_envionombre = lookup_widget(widget, "lbl_envionombre");
	entry_enviodireccion = lookup_widget(widget, "entry_enviodireccion");
	entry_enviocolonia = lookup_widget(widget, "entry_enviocolonia");
	entry_enviociudad = lookup_widget(widget, "entry_enviociudad");
	entry_enviocp = lookup_widget(widget, "entry_enviocp");
	entry_enviotelefono = lookup_widget(widget, "entry_enviotelefono");
	entry_envioentre_calles = lookup_widget(widget, "entry_envioentre_calles");
	txtv_observaciones_articulo = lookup_widget(widget, "txtv_observaciones_articulo_pedidos");
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtv_observaciones_articulo));
	
	g_signal_connect ((gpointer) buffer, "changed",
                    G_CALLBACK (on_txtbuffer_changed),
                    NULL);

	sprintf(sqldatos, "SELECT Cliente.nombre, Cliente_Envio.domicilio, Cliente_Envio.entre_calles, Cliente_Envio.colonia, Cliente_Envio.ciudad_estado, Cliente_Envio.cp, Cliente_Envio.telefono FROM Cliente LEFT JOIN Cliente_Envio ON Cliente.id_cliente = Cliente_Envio.id_cliente WHERE Cliente.id_cliente = %s", codigo_cliente);

	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqldatos);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
				{
					if ((row[1] != NULL) && (row[6] != NULL))
					{
						gtk_notebook_set_current_page(notebook_datos,1);
						gtk_label_set_markup(GTK_LABEL(lbl_enivar_a),"<span color='#0000FF'>Enviar a...</span>");
					}
					else
					{
						gtk_notebook_set_current_page(notebook_datos,0);
						gtk_label_set_markup(GTK_LABEL(lbl_enivar_a),"<span color='#000000'>Enviar a...</span>");
					}
					
					gtk_label_set_text(GTK_LABEL(lbl_envionombre), row[0]);
					
					if (row[1] != NULL)
						gtk_entry_set_text(GTK_ENTRY(entry_enviodireccion), row[1]);
					else
						gtk_entry_set_text(GTK_ENTRY(entry_enviodireccion), "");
					
					if (row[2] != NULL)
						gtk_entry_set_text(GTK_ENTRY(entry_envioentre_calles), row[2]);
					else
						gtk_entry_set_text(GTK_ENTRY(entry_envioentre_calles), "");
					
					if (row[3] != NULL)
						gtk_entry_set_text(GTK_ENTRY(entry_enviocolonia), row[3]);
					else
						gtk_entry_set_text(GTK_ENTRY(entry_enviocolonia), "");
					
					if(row[4] != NULL)
						if(strcmp(row[4],"") != 0)
							gtk_entry_set_text(GTK_ENTRY(entry_enviociudad), row[4]);
						
					if(row[5] != NULL)
						gtk_entry_set_text(GTK_ENTRY(entry_enviocp), row[5]);
					else
						gtk_entry_set_text(GTK_ENTRY(entry_enviocp), "");
					
					if(row[6] != NULL)
						gtk_entry_set_text(GTK_ENTRY(entry_enviotelefono), row[6]);
					else
						gtk_entry_set_text(GTK_ENTRY(entry_enviotelefono), "");
				}
			}
		}
		else
			printf("Error: %s\n", mysql_error(&mysql));
	}
	else
		Err_Info("No me puedo conectar a la base de datos");
	mysql_close(&mysql);
	/*FIN DATOS DE ENVIO*/

	list[0]="";
	list[1]="";
	ventana_oviedo = user_data;
	gtk_clist_clear(clist);

	sprintf(querycliente,"SELECT nombre AS NOMBRE, domicilio AS DOMICILIO, colonia AS COLONIA, ciudad_estado AS CIUDAD, telefono AS TELEFONO, telefono2 AS 'TELEFONO 2', telefono3 AS 'TELEFONO 3', entre_calles AS ENTRE FROM Cliente WHERE id_cliente = %s",codigo_cliente);

	if(conecta_bd() == -1)
	{
		printf("No se pudo conectar a la base de datos para llenar la lista cliente: %s\n", mysql_error(&mysql));
	}
	else
	{
		er = mysql_query(&mysql, querycliente);
		if(er != 0)
		{
			printf("Error en en el querycliente: %s\n", mysql_error(&mysql));
		}
		else
		{
			resultado = mysql_store_result(&mysql);
			if(resultado)
			{
				if((row = mysql_fetch_row(resultado))&&(field = mysql_fetch_field(resultado)))
				{
					for(i = 0; i < 8; i++)
					{	list[1] = row[i];
						list[0] = field[i].name;
						gtk_clist_append(clist, list);
					}
				}
			}
			else
			{
				printf("no hay resultado\n");
			}
		}
	}
	mysql_close(&mysql);

	list[0]="";
	list[1]="";
	ventana_oviedo = user_data;
	gtk_clist_clear(clist);

	sprintf(querycliente,"SELECT nombre AS NOMBRE, domicilio AS DOMICILIO, colonia AS COLONIA, ciudad_estado AS CIUDAD, telefono AS TELEFONO, telefono2 AS 'TELEFONO 2', telefono3 AS 'TELEFONO 3', entre_calles AS ENTRE FROM Cliente WHERE id_cliente = %s",codigo_cliente);

	if(conecta_bd() == -1)
	{
		printf("No se pudo conectar a la base de datos para llenar la lista cliente: %s\n", mysql_error(&mysql));
	}
	else
	{
		er = mysql_query(&mysql, querycliente);
		if(er != 0)
		{
			printf("Error en en el querycliente: %s\n", mysql_error(&mysql));
		}
		else
		{
			resultado = mysql_store_result(&mysql);
			if(resultado)
			{
				if((row = mysql_fetch_row(resultado))&&(field = mysql_fetch_field(resultado)))
				{
					for(i = 0; i < 8; i++)
					{	list[1] = row[i];
						list[0] = field[i].name;
						gtk_clist_append(clist, list);
					}
				}
			}
			else
			{
				printf("no hay resultado\n");
			}
		}
	}
	mysql_close(&mysql);

}


void
on_btn_pedidosenviar_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_show(create_Domicilio_envio());
}


void
on_Domicilio_envio_show                (GtkWidget       *widget,
                                        gpointer         user_data)
{
	GtkWidget *lbl_envionombre;
	GtkWidget *entry_enviodireccion;
	GtkWidget *entry_envioentre_calles;
	GtkWidget *entry_enviocolonia;
	GtkWidget *entry_enviociudad;
	GtkWidget *entry_enviocp;
	GtkWidget *entry_enviotelefono;
	char sqldatos[300];

	lbl_envionombre = lookup_widget(widget, "lbl_envionombre");
	entry_enviodireccion = lookup_widget(widget, "entry_enviodireccion");
	entry_enviocolonia = lookup_widget(widget, "entry_enviocolonia");
	entry_enviociudad = lookup_widget(widget, "entry_enviociudad");
	entry_enviocp = lookup_widget(widget, "entry_enviocp");
	entry_enviotelefono = lookup_widget(widget, "entry_enviotelefono");
	entry_envioentre_calles = lookup_widget(widget, "entry_envioentre_calles");

	sprintf(sqldatos, "SELECT Cliente.nombre, Cliente_Envio.domicilio, Cliente_Envio.entre_calles, Cliente_Envio.colonia, Cliente_Envio.ciudad_estado, Cliente_Envio.cp, Cliente_Envio.telefono FROM Cliente LEFT JOIN Cliente_Envio ON Cliente.id_cliente = Cliente_Envio.id_cliente WHERE Cliente.id_cliente = %s", codigo_cliente);

	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, sqldatos);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
				{
					gtk_label_set_text(GTK_LABEL(lbl_envionombre), row[0]);
					gtk_entry_set_text(GTK_ENTRY(entry_enviodireccion), row[1]);
					gtk_entry_set_text(GTK_ENTRY(entry_envioentre_calles), row[2]);
					gtk_entry_set_text(GTK_ENTRY(entry_enviocolonia), row[3]);
					if(row[4] != NULL)
						if(strcmp(row[4],"") != 0)
							gtk_entry_set_text(GTK_ENTRY(entry_enviociudad), row[4]);
					gtk_entry_set_text(GTK_ENTRY(entry_enviocp), row[5]);
					gtk_entry_set_text(GTK_ENTRY(entry_enviotelefono), row[6]);
				}
			}
		}
		else
			printf("Error: %s\n", mysql_error(&mysql));
	}
	else
		Err_Info("No me puedo conectar a la base de datos");
	mysql_close(&mysql);
}


void
on_btn_envioborrar_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_enviodireccion;
	GtkWidget *entry_envioentre_calles;
	GtkWidget *entry_enviocolonia;
	GtkWidget *entry_enviociudad;
	GtkWidget *entry_enviocp;
	GtkWidget *entry_enviotelefono;

	entry_enviodireccion = lookup_widget(GTK_WIDGET(button), "entry_enviodireccion");
	entry_enviocolonia = lookup_widget(GTK_WIDGET(button), "entry_enviocolonia");
	entry_enviociudad = lookup_widget(GTK_WIDGET(button), "entry_enviociudad");
	entry_enviocp = lookup_widget(GTK_WIDGET(button), "entry_enviocp");
	entry_enviotelefono = lookup_widget(GTK_WIDGET(button), "entry_enviotelefono");
	entry_envioentre_calles = lookup_widget(GTK_WIDGET(button), "entry_envioentre_calles");

	gtk_entry_set_text(GTK_ENTRY(entry_enviodireccion), "");
	gtk_entry_set_text(GTK_ENTRY(entry_envioentre_calles), "");
	gtk_entry_set_text(GTK_ENTRY(entry_enviocolonia), "");
	gtk_entry_set_text(GTK_ENTRY(entry_enviociudad), "");
	gtk_entry_set_text(GTK_ENTRY(entry_enviocp), "");
	gtk_entry_set_text(GTK_ENTRY(entry_enviotelefono), "");
}




void
on_btn_envioaceptar_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry_enviodireccion;
	GtkWidget *entry_envioentre_calles;
	GtkWidget *entry_enviocolonia;
	GtkWidget *entry_enviociudad;
	GtkWidget *entry_enviocp;
	GtkWidget *entry_enviotelefono;
	gchar *domicilio, *entre_calles, *colonia, *ciudad_estado, *cp, *telefono;
	char sqldatos[600];

	entry_enviodireccion = lookup_widget(GTK_WIDGET(button), "entry_enviodireccion");
	entry_enviocolonia = lookup_widget(GTK_WIDGET(button), "entry_enviocolonia");
	entry_enviociudad = lookup_widget(GTK_WIDGET(button), "entry_enviociudad");
	entry_enviocp = lookup_widget(GTK_WIDGET(button), "entry_enviocp");
	entry_enviotelefono = lookup_widget(GTK_WIDGET(button), "entry_enviotelefono");
	entry_envioentre_calles = lookup_widget(GTK_WIDGET(button), "entry_envioentre_calles");

	domicilio = gtk_editable_get_chars(GTK_EDITABLE(entry_enviodireccion), 0, -1);
	colonia = gtk_editable_get_chars(GTK_EDITABLE(entry_enviocolonia), 0, -1);
	ciudad_estado = gtk_editable_get_chars(GTK_EDITABLE(entry_enviociudad), 0, -1);
	cp = gtk_editable_get_chars(GTK_EDITABLE(entry_enviocp), 0, -1);
	telefono = gtk_editable_get_chars(GTK_EDITABLE(entry_enviotelefono), 0, -1);
	entre_calles = gtk_editable_get_chars(GTK_EDITABLE(entry_envioentre_calles), 0, -1);

	sprintf(sqldatos, "SELECT * FROM Cliente_Envio WHERE id_cliente = %s", codigo_cliente);

	printf("Cadena de actualizacion: %s\n", sqldatos);

	if(conecta_bd() == 1)
	{
	  if(strlen(domicilio)>0)
	  {
		er = mysql_query(&mysql, sqldatos);
		if(er != 0)
			Err_Info("Ocurri un error");
		else
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if(mysql_num_rows(res)>0)
				{
					sprintf(sqldatos, "UPDATE Cliente_Envio SET domicilio=\"%s\", entre_calles=\"%s\", colonia=\"%s\", ciudad_estado=\"%s\", cp=\"%s\", telefono=\"%s\" WHERE id_cliente=%s", domicilio, entre_calles, colonia, ciudad_estado, cp, telefono, codigo_cliente);
					er = mysql_query(&mysql, sqldatos);
					if(er != 0)
						Err_Info("Ocurri un error");
				}
				else
				{
					sprintf(sqldatos, "INSERT INTO Cliente_Envio (id_cliente_envio, id_cliente, domicilio, entre_calles, colonia, ciudad_estado, cp, telefono) VALUES(NULL, %s, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", codigo_cliente, domicilio, entre_calles, colonia, ciudad_estado, cp, telefono);
					er = mysql_query(&mysql, sqldatos);
					if(er != 0)
						Err_Info("Ocurri un error");
				}
			}
		}
	  }
	  else
	  {
	  	sprintf(sqldatos, "DELETE FROM Cliente_Envio WHERE id_cliente = %s", codigo_cliente);
		er = mysql_query(&mysql, sqldatos);
		if(er != 0)
			Err_Info("Ocurri un error");
	  }
	}
	else
		Err_Info("No me puedo conectar a la base de datos");
	mysql_close(&mysql);
}



void
on_btn_observaciones_aceptar_clicked   (GtkTextView       *text,
                                        gpointer         user_data)
{
	GtkTextBuffer *observacion;
	GtkTextIter ini,fin;
	GtkWidget *clist;
	observacion = gtk_text_view_get_buffer(text);
	printf("\nGUARDAR OBSERVACIONES");

	gtk_text_buffer_get_iter_at_offset(observacion,&ini,0);
	gtk_text_buffer_get_iter_at_offset(observacion,&fin,-1);
	listaX[5] = gtk_text_buffer_get_text(observacion,&ini,&fin,TRUE);
	clist = lookup_widget(ventana_oviedo,"clist_articulos");
	gtk_clist_set_text(GTK_CLIST(clist),row_counter,5,listaX[5]);
	listaX[5] = "";
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(text)));
	bV_observaciones = FALSE;
	gtk_widget_set_sensitive( lookup_widget(GTK_WIDGET(ventana_oviedo),"V_pedidos"), TRUE);

}


void
on_btn_observaciones_cancelar_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
	bV_observaciones = FALSE;
	gtk_widget_set_sensitive( lookup_widget(GTK_WIDGET(ventana_oviedo),"V_pedidos"), TRUE);
}

void
on_clist_articulos_select_row_guarda_row
                                        (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	GtkWidget *entry_codigo;
	GtkWidget *txtv_observaciones;
	GtkTextBuffer *buff;
	GtkTextIter start, end;
	selected_row = row;
	row_counter = row;
	gchar *observaciones;
	
	MYSQL_ROW rowo;
	
	char SQL[300];
	
	cambiar_observaciones = FALSE;
	
	gtk_clist_get_text(GTK_CLIST(clist),row,5,&observaciones);
	
	txtv_observaciones = lookup_widget(GTK_WIDGET(clist),"txtv_observaciones_articulo_pedidos");
	
	buff = gtk_text_view_get_buffer (GTK_TEXT_VIEW(txtv_observaciones));

	gtk_text_buffer_get_iter_at_offset (buff, &start, 0);
	gtk_text_buffer_get_iter_at_offset (buff, &end, -1);
	
	gtk_widget_grab_focus(txtv_observaciones);

	//observaciones = gtk_text_buffer_get_text(buff,&start,&end,TRUE);
	gtk_text_buffer_set_text (buff, observaciones, -1);
	
	gtk_widget_set_sensitive(txtv_observaciones, TRUE);
	/*if (event)
	if(event->type == GDK_2BUTTON_PRESS){ //Doble click
		printf("\nDoble Click");

		if(bV_observaciones == FALSE)
		{
			V_observaciones = create_V_observaciones();
			gtk_widget_show(V_observaciones);
			bV_observaciones  = TRUE;
			gtk_widget_set_sensitive( lookup_widget(GTK_WIDGET(clist),"V_pedidos"), FALSE);
		}
		else
		{
			gtk_window_present(GTK_WINDOW(V_observaciones));
		}
		entry_codigo = lookup_widget(GTK_WIDGET(clist),"entry_codigo");
		gtk_widget_grab_focus(entry_codigo);
	}*/
	
	
	/*sprintf(SQL, "SELECT Pedido_Articulo.observaciones FROM Pedido_Articulo INNER JOIN Pedido ON Pedido.id_pedido = Pedido_Articulo.id_pedido INNER JOIN Cliente_Envio ON Pedido.id_cliente = Cliente_Envio.id_cliente WHERE Pedido.id_cliente = %s ORDER BY Pedido.fecha DESC Limit 1", codigo_cliente);

	if(conecta_bd() == 1)
	{
		er = mysql_query(&mysql, SQL);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((rowo = mysql_fetch_row(res)))
				{
					printf ("Observaciones = %s\n", rowo[0]);
				}
			}
		}
		else
			printf("Error: %s\n", mysql_error(&mysql));
	}
	else
		Err_Info("No me puedo conectar a la base de datos");
	mysql_close(&mysql);
	
	
	
	printf("\n\n");*/
}


void
on_btn_quitar_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *clist, *entry_codigo, *label_kilos, *label_total;
	gchar *gkilos, *gmonto, *gtipo;
	char tipo[8];
	char mon[300],kil[20];
	int k, row_total;
	double kilos = 0, kilos2 =0, total=0, total2=0;

	entry_codigo = lookup_widget(GTK_WIDGET(button),"entry_codigo");
	if (selected_row >= 0){
		label_total = lookup_widget(GTK_WIDGET(button),"label_total");
		label_kilos = lookup_widget(GTK_WIDGET(button),"label_kg");
		clist = lookup_widget(GTK_WIDGET(button),"clist_articulos");

		gtk_clist_remove(GTK_CLIST(clist),selected_row);
		selected_row = -1;
		row_total = GTK_CLIST(clist)->rows;
		printf("\nROW: %d\n",row_total);
		kilos=0;
		total=0;

			if(row_total>0)
			{
				for(k = 0; k < row_total; k++)
				{
					gtk_clist_get_text(GTK_CLIST(clist),k,3,&gkilos);
					gtk_clist_get_text(GTK_CLIST(clist),k,4,&gmonto);
					gtk_clist_get_text(GTK_CLIST(clist),k,6,&gtipo);
     					strcpy(tipo,gtipo);
					strcpy(mon,gmonto);
					if(strcmp(tipo,"peso") == 0){ //Solo suma los pesos
						strcpy(kil,gkilos);
						kilos2 = atof(kil);
						kilos = kilos + kilos2;
						printf("\nKIL: %f",kilos);
						printf("\nJKIL2: %f",kilos2);
					}

					total2 = atof(mon);
					total = total + total2;
					printf("\nTotal: %f",total);
					printf("\nTotal2: %f",total2);

				}
			}
				printf("\n");
				sprintf(kil,"%.3f",kilos);
				sprintf(mon,"<span size=\"15000\">%.2f</span>",total);
				gtk_label_set_text(GTK_LABEL(label_kilos),kil);
				gtk_label_set_markup(GTK_LABEL(label_total),mon);

	}
	gtk_widget_grab_focus(entry_codigo);
}


void
gtk_widget_show_ventana                (GtkWidget       *widget,
                                        gpointer         user_data)
{
	GtkWidget *clist;
	GtkTextBuffer *buffer;
	GtkWidget *lblObservaciones_Codigo;
	char markup[20];

//	observacion = gtk_text_view_get_buffer(text);

	printf("\nCONTADOR ROW:%d",row_counter);
	clist = lookup_widget(ventana_oviedo,"clist_articulos");
	lblObservaciones_Codigo = lookup_widget(widget,"lblObservaciones_Codigo");

	gtk_clist_get_text(GTK_CLIST(clist),row_counter,5,&articulo.observaciones);
	printf("\nOBSERVACIONES : %s\n",articulo.observaciones);

	gtk_clist_get_text(GTK_CLIST(clist),row_counter,0,&articulo.codigo);
	gtk_clist_get_text(GTK_CLIST(clist),row_counter,1,&articulo.nombre);

	sprintf(markup, "<b>%s</b>",articulo.nombre);
	gtk_label_set_markup(GTK_LABEL(lblObservaciones_Codigo), markup);

	if (g_strcasecmp(articulo.observaciones,"")) {
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	 	gtk_text_buffer_set_text (buffer, articulo.observaciones, -1);
	}
}


gboolean
on_V_observaciones_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
printf("\nSE SALE DE OBSERVACIONES - DESTROY");
printf("\n");
  gtk_widget_destroy(widget);
  bV_observaciones = FALSE;
  gtk_widget_set_sensitive( lookup_widget(GTK_WIDGET(ventana_oviedo),"V_pedidos"), TRUE);

  return FALSE;
}


void
on_clist_cliente_select_row_guarda_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	selected_row_clientes = row;
	if(event->type == GDK_2BUTTON_PRESS){ //Doble click
		printf("\nDoble Click");
	}
}



void
gtk_widget_show_V_pedidos              (GtkWidget       *clist,
                                        gpointer         user_data)
{
GtkWidget *cmb_entryHora, *txt_entryAnio, *cmb_entryMinutos, *cmb_entryDia, *cmb_entryMes;
char querypedido[200], mes_char[15];
int er, error, mes_int, minutos_int;
char minutos[3], horas[3];


gtk_clist_set_column_resizeable(GTK_CLIST(clist),0,FALSE);
gtk_clist_set_column_resizeable(GTK_CLIST(clist),1,FALSE);
gtk_clist_set_column_resizeable(GTK_CLIST(clist),2,FALSE);
gtk_clist_set_column_resizeable(GTK_CLIST(clist),3,FALSE);
gtk_clist_set_column_resizeable(GTK_CLIST(clist),4,FALSE);


//Saca la fecha del sistema
	if(conecta_bd() == -1)
	{
		printf("No se pudo conectar a la base de datos: %s\n", mysql_error(&mysql));
	}
	else
	{
		sprintf(querypedido,"SELECT DATE_FORMAT( NOW(  ) ,  '%%d' ) , DATE_FORMAT( NOW(  ) ,  '%%m'  ) , DATE_FORMAT( NOW(  ) ,  '%%Y'  ) , DATE_FORMAT(DATE_ADD(NOW(),INTERVAL 30 MINUTE),\"%%H\"), MINUTE ( CURTIME(  )  )");
		printf("\n%s\n",querypedido);
		er = mysql_query(&mysql,querypedido);
		if(er != 0){
			Err_Info("Error al consultar la fecha actual\nModifica manualmente la fecha del pedido");
			error=1;
		}else{
			resultado = mysql_store_result(&mysql);
			if(resultado)
			{
				if((row = mysql_fetch_row(resultado)))
				{
					txt_entryAnio  = lookup_widget(clist,"txtAno");
					gtk_entry_set_text(GTK_ENTRY(txt_entryAnio),row[2]);

					cmb_entryDia  = lookup_widget(clist,"combo_entryDia");
					gtk_entry_set_text(GTK_ENTRY(cmb_entryDia),row[0]);

					//Agrega el Mes
					mes_int = atoi(row[1]);
					mes_caracter(mes_int,mes_char); //Funcion que regresa el mes en texto
					cmb_entryMes  = lookup_widget(clist,"combo_entryMes");
					gtk_entry_set_text (GTK_ENTRY(cmb_entryMes),mes_char);

					//Validar el rango de hora y minutos

					minutos_int = atoi(row[4]);
					strcpy(horas, row[3]);
					strcpy(minutos, row[4]);
					if(minutos_int>=0 && minutos_int<=15)
						strcpy(minutos,"30");
					else if(minutos_int>15 && minutos_int<=30)
						strcpy(minutos,"45");
					else if(minutos_int>30 && minutos_int<=45)
					{
						strcpy(minutos,"00");
					}
					else if((minutos_int>45 && minutos_int<=59))
					{
						strcpy(minutos,"15");
					}
					else
						strcpy(minutos,"--");

					cmb_entryHora  = lookup_widget(clist,"combo_entryHora");
					gtk_entry_set_text (GTK_ENTRY(cmb_entryHora),horas);

					cmb_entryMinutos  = lookup_widget(clist,"combo_entryMinutos");
					gtk_entry_set_text (GTK_ENTRY(cmb_entryMinutos),minutos);

     				}else{
					Err_Info("Error al consultar la fecha actual\nModifica manualmente la fecha del pedido");
					error = 1;
				}
			}else{
				Err_Info("Error al consultar la fecha actual\nModifica manualmente la fecha del pedido");
				error = 1;
			}
		}
	}
	mysql_close(&mysql);
}


int mes_numero(char *mes){

	if(strcmp(mes,"Enero")==0)
		return 1;
	else if(strcmp(mes,"Febrero")==0)
		return 2;
	else if(strcmp(mes,"Marzo")==0)
		return 3;
	else if(strcmp(mes,"Abril")==0)
		return 4;
	else if(strcmp(mes,"Mayo")==0)
		return 5;
	else if(strcmp(mes,"Junio")==0)
		return 6;
	else if(strcmp(mes,"Julio")==0)
		return 7;
	else if(strcmp(mes,"Agosto")==0)
		return 8;
	else if(strcmp(mes,"Septiembre")==0)
		return 9;
	else if(strcmp(mes,"Octubre")==0)
		return 10;
	else if(strcmp(mes,"Noviembre")==0)
		return 11;
	else if(strcmp(mes,"Diciembre")==0)
		return 12;
	else
		return 0;
}

int  es_fecha(char *cadena)
{
	//Nos dice si es una fecha valida con separacion / o -
	//Dia - Mes - Ano
   	int     status;
   	regex_t re;
	int dia, mes, anio;
	char dia_char[3], mes_char[3], anio_char[5];

	char *expresion_regular = "^([0-9]{4})(/|-){1})?([0-9]{2})(/|-){1})?([0-9]{2})$";
	printf("\nCadena Expresion Regular: %s\nCadena: %s",expresion_regular, cadena);

	if (regcomp(&re, expresion_regular, REG_EXTENDED|REG_NOSUB) != 0)  {
		return(0) ;              /* report error */
	}
	status = regexec(&re, cadena, (size_t) 0, NULL, 0);
	regfree(&re);
	//printf("\n%d",status[0]);
	if (status != 0)  {
		return(0) ;              /* report error */
	}

	//Convierte los caracteres!
	anio_char[0] = cadena[0];
	anio_char[1] = cadena[1];
	anio_char[2] = cadena[2];
	anio_char[3] = cadena[3];
	anio_char[4] ='\0';

	mes_char[0] = cadena[5];
	mes_char[1] = cadena[6];
	mes_char[2] ='\0';

	dia_char[0] = cadena[8];
	dia_char[1] = cadena[9];
	dia_char[2] ='\0';





	dia = atoi(dia_char);
	mes =atoi(mes_char);
	anio = atoi(anio_char);

	printf("\nDia: %d, Mes: %d, Anio: %d",dia,mes,anio);
	printf("\n");
   if (mes < 1 || mes > 12)
      return (0);
   if ((dia < 1 || dia > 31) || (mes == 4 && dia > 30) ||
       (mes == 6 && dia > 30) || (mes == 9 && dia > 30) ||
       (mes == 11 && dia > 30) || (mes == 2 && es_bisiesto(anio) && dia > 29) ||
       (mes == 2 && !es_bisiesto(anio) && dia > 28))
      return (0);
   return (1);
}

// Determina si un aÃ±o es bisiesto
   int es_bisiesto(int anio)
   {
   if (((anio % 4 == 0) && anio % 100 != 0) || anio % 400 == 0)
      return (1);
   return (0);
   }


   void mes_caracter(int mes, char mes_char[15]){
   	switch (mes){
		case 1:
			strcpy(mes_char,"Enero");
			break;
		case 2:
			strcpy(mes_char,"Febrero");
			break;
		case 3:
			strcpy(mes_char,"Marzo");
			break;
		case 4:
			strcpy(mes_char,"Abril");
			break;
		case 5:
			strcpy(mes_char,"Mayo");
			break;
		case 6:
			strcpy(mes_char,"Junio");
			break;
		case 7:
			strcpy(mes_char,"Julio");
			break;
		case 8:
			strcpy(mes_char,"Agosto");
			break;
		case 9:
			strcpy(mes_char,"Septiembre");
			break;
		case 10:
			strcpy(mes_char,"Octubre");
			break;
		case 11:
			strcpy(mes_char,"Noviembre");
			break;
		case 12:
			strcpy(mes_char,"Diciembre");
			break;
		default:
			strcpy(mes_char,"");
			break;
	}
}


void
on_clist_articulos_unselect_row_V_observaciones
                                        (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	GtkWidget *txtv_observaciones;
	
	txtv_observaciones = lookup_widget(GTK_WIDGET(clist),"txtv_observaciones_articulo_pedidos");
	
	gtk_widget_set_sensitive(txtv_observaciones, FALSE);
	selected_row = -1;
}

/*void
on_entry_buscarclientenombre_changed   (GtkEntry        *entry,
                                        gpointer         user_data)
{

}*/


void
on_facturar_folios_no_facturados_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_facturar_varios_folio_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	//gtk_widget_show (create_win_facturacion_no_facturado());
}


void
on_impresoras_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_widget_show (create_conf_impresion());
}


void
on_chk_ver_precio_buscar_articulo_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *clist_articulos;
	
	buscar_articulo();
	
	clist_articulos = lookup_widget (GTK_WIDGET(togglebutton),"clist_buscararticulo");
	
	if ( gtk_toggle_button_get_active (togglebutton) )
		gtk_clist_set_column_visibility(GTK_CLIST (clist_articulos),2,TRUE);
	else
		gtk_clist_set_column_visibility(GTK_CLIST (clist_articulos),2,FALSE);
}

void
on_win_bascula_show                    (GtkWidget       *widget,
                                        gpointer         user_data)
{
	char sql_basculas[]="SELECT id_bascula, bascula FROM Basculas";
	GtkWidget *clist_basculas;
	
	clist_basculas = lookup_widget(widget, "clist_basculas");
	
	gtk_window_present (GTK_WINDOW (winbascula));
	//Saca la fecha del sistema
	if(conecta_bd() == -1)
	{
		printf("No se pudo conectar a la base de datos: %s\n", mysql_error(&mysql));
	}
	else
	{
		er = mysql_query(&mysql,sql_basculas);
		if(er != 0){
			Err_Info("Error al consultar la fecha actual\nModifica manualmente la fecha del pedido");
		}else{
			resultado = mysql_store_result(&mysql);
			if(resultado)
			{
				while((row = mysql_fetch_row(resultado)))
				{
					gtk_clist_append(GTK_CLIST (clist_basculas),row);
				}
			}else{
				Err_Info("Error al consultar la fecha actual\nModifica manualmente la fecha del pedido");
			}
		}
	}
	gtk_clist_set_column_visibility(GTK_CLIST (clist_basculas),0,FALSE);
	mysql_close(&mysql);
	
	gtk_window_present (GTK_WINDOW (winbascula));
}

void
on_btn_aceptar_bascula_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *clist_basculas;
	GtkWidget *txtbarcode;
	//GtkWidget *clist_articulos;
	int r;
	gchar *id_bascula;
	
	txtbarcode      = lookup_widget(caja,"txtbarcode");
	//clist_articulos = lookup_widget(caja,"listaarticulos");
	clist_basculas  = lookup_widget(GTK_WIDGET (button), "clist_basculas");
	
	r = obtener_row_seleccionada (clist_basculas,0);
	if ( r != -1)
	{
		gtk_clist_get_text(GTK_CLIST(clist_basculas),r,0,&id_bascula);
		
		strcpy (bascula,id_bascula);
		
		gtk_widget_destroy (winbascula);
		winbascula = NULL;
		gtk_widget_set_sensitive(caja,TRUE);
		gtk_widget_grab_focus(txtbarcode);
	}
	else
	{
		Info ("Por favor selecciones una bascula de la lista.");
	}
}

gboolean
on_win_bascula_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

  return TRUE;
}

void
on_btn_aplicar_cambios_observaciones_pedidos_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTextBuffer *observacion;
	GtkTextIter ini,fin;
	GtkWidget *clist;
	GtkWidget *text;
	gchar *c_observaaciones;
	
	text  = lookup_widget(GTK_WIDGET(button),"txtv_observaciones_articulo_pedidos");
	clist = lookup_widget(GTK_WIDGET(button),"clist_articulos"); 
	
	observacion = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	printf("\nGUARDAR OBSERVACIONES");

	gtk_text_buffer_get_iter_at_offset(observacion,&ini,0);
	gtk_text_buffer_get_iter_at_offset(observacion,&fin,-1);
	c_observaaciones = gtk_text_buffer_get_text(observacion,&ini,&fin,TRUE);
	
	gtk_clist_set_text(GTK_CLIST(clist),row_counter,5,c_observaaciones);
}

void
on_btn_eliminar_observaciones_pedidos_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTextBuffer *observacion;
	GtkTextIter ini,fin;
	GtkWidget *clist;
	GtkWidget *text;
	gchar *c_observaaciones;
	
	text  = lookup_widget(GTK_WIDGET(button),"txtv_observaciones_articulo_pedidos");
	clist = lookup_widget(GTK_WIDGET(button),"clist_articulos"); 
	
	observacion = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	printf("\nGUARDAR OBSERVACIONES");

	gtk_text_buffer_get_iter_at_offset(observacion,&ini,0);
	gtk_text_buffer_get_iter_at_offset(observacion,&fin,-1);
	gtk_text_buffer_set_text (observacion, "", -1);
	//c_observaaciones = gtk_text_buffer_get_text(observacion,&ini,&fin,TRUE);
	
	gtk_clist_set_text(GTK_CLIST(clist),row_counter,5,"");
}

void
on_btn_guardar_observaciones_pedido_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}

void
on_txtbuffer_changed
                                        (GtkTextBuffer    *txtbuffer,
                                        gpointer         user_data)
{
	GtkTextBuffer *observacion;
	GtkTextIter ini,fin;
	GtkWidget *clist;
	GtkWidget *text;
	gchar *c_observaaciones;

	if (cambiar_observaciones)	
	{
		text  = lookup_widget(GTK_WIDGET(V_pedidos),"txtv_observaciones_articulo_pedidos");
		clist = lookup_widget(GTK_WIDGET(V_pedidos),"clist_articulos"); 
		
		observacion = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
		//printf("\nGUARDAR OBSERVACIONES");
		
		gtk_text_buffer_get_iter_at_offset(observacion,&ini,0);
		gtk_text_buffer_get_iter_at_offset(observacion,&fin,-1);
		c_observaaciones = gtk_text_buffer_get_text(observacion,&ini,&fin,TRUE);
		
		//printf ("%s\n",c_observaaciones);
		gtk_clist_set_text(GTK_CLIST(clist),row_counter,5,c_observaaciones);
	}
	else
	{
		cambiar_observaciones=TRUE;
	}
}

void
on_entryconfirma_vendedor_changed      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	char sql_vendedor[100];
	GtkWidget *lblconfirma_vendedor;
	gchar *vendedor;
	char nombre[70];
	
	lblconfirma_vendedor = lookup_widget(GTK_WIDGET(editable), "lblconfirma_vendedor");
	
	vendedor = gtk_editable_get_chars(GTK_EDITABLE(editable),0,-1);
	sprintf(sql_vendedor,"SELECT nombre, id_empleado FROM Empleado WHERE numero = '%s'",vendedor);
	vendedorbien = FALSE;
	if(conecta_bd() == -1)
	{
		printf("No se pudo conectar a la base de datos: %s\n", mysql_error(&mysql));
	}
	else
	{
		er = mysql_query(&mysql,sql_vendedor);
		if(er != 0){
			printf("Error: %s\n", mysql_error(&mysql));
			gtk_label_set_markup(GTK_LABEL(lblconfirma_vendedor),"");
		}else{
			resultado = mysql_store_result(&mysql);
			if(resultado)
			{
				if(mysql_num_rows(resultado) > 0)
				{
					if((row = mysql_fetch_row(resultado)))
					{
						sprintf(nombre,"<span color=\"blue\">%s</span>",row[0]);
						sprintf(id_empleado, "%s", row[1]);
						gtk_label_set_markup(GTK_LABEL(lblconfirma_vendedor),nombre);
						vendedorbien = TRUE;
					}
				}
				else
					gtk_label_set_markup(GTK_LABEL(lblconfirma_vendedor),"");
			}else{
				Err_Info("Error al consultar el nombre del vendedor");
				printf("Error: %s\n", mysql_error(&mysql));
			}
		}
	}
	mysql_close(&mysql);
}


/*
*	MODULO PARA MODIFICAR CLIENTES
*
*/

void obtiene_nombre_de_lista(gchar *id_del_cliente)
{
	char sqllista2[200] = "SELECT Lista.nombre FROM Lista INNER JOIN Cliente ON Cliente.id_lista =  Lista.id_lista WHERE Cliente.id_cliente = ";
	int er;


        strcat(sqllista2,id_del_cliente);
	/*if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql2));
    	}
	else
	{*/
		er = mysql_query(&mysql,sqllista2);
		if(er == 0)
		{
			res2 = mysql_store_result(&mysql);
			if(res2)
			{
				
				while((row3=mysql_fetch_row(res2)))
				{
					lista2 = row3[0];
					printf("lista: %s\n",lista2);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
//	}
	printf("desConecta en 2946\n");
//	mysql_close(&mysql2);
}


void
on_Modificar_cliente_show_actualizar   (GtkCombo       *combo_modificarclientelista,
                                        gpointer         user_data)
{

  GList *items = NULL;
  int error;

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
    	}
        else
	 {
		error = mysql_query(&mysql,"SELECT nombre FROM Lista");
		if(error == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			
			{
			 while((row = mysql_fetch_row(res)))
				{
				        printf("Entra al while \n");
					items = g_list_append (items, row[0]);
					gtk_combo_set_popdown_strings (GTK_COMBO (combo_modificarclientelista), items);	
				}
			 }
			
			printf("SELECT FROM Lista\n");
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	 }
		
	printf("desConecta en 2500\n");
  mysql_close(&mysql);
  
}


void
on_Modificar_cliente_show_lista        (GtkCList       *lista_clientes,
                                        gpointer         user_data)
{
	char sqlclientes[200] = "SELECT Cliente.id_cliente, Cliente.nombre, Lista.nombre FROM Cliente INNER JOIN Lista ON Cliente.id_lista =  Lista.id_lista";
	gchar *lista[3];
	int er;
	
	gtk_clist_clear(lista_clientes);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					gtk_clist_append(lista_clientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	printf("DesConecta en 2253 \n");
	mysql_close(&mysql);
	
	gtk_clist_select_row(lista_clientes, 0, 0);
}



void
on_entry_modificarclientebuscar_activate
                                        (GtkCList        *lista_clientes,
                                        gpointer         user_data)
{
        char sqlclientes[400];
	gchar *lista[3];
	gchar *criterio;
	int er;

	GtkEntry *txtbuscarcliente = user_data;

	criterio = gtk_editable_get_chars(GTK_EDITABLE(txtbuscarcliente),0,-1);
	criterio_bak = criterio;
	sprintf(sqlclientes, "SELECT Cliente.id_cliente, Cliente.nombre, Lista.nombre FROM Cliente LEFT JOIN Lista ON Cliente.id_lista =  Lista.id_lista WHERE Cliente.nombre LIKE \"%%%s%%\" OR Cliente.contacto LIKE \"%%%s%%\" OR Cliente.id_cliente = \"%s\" ORDER BY Cliente.id_cliente",criterio,criterio,criterio);

	gtk_clist_clear(lista_clientes);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		printf("%s\n",sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					gtk_clist_append(lista_clientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	mysql_close(&mysql);

	gtk_clist_select_row(lista_clientes, 0, 0);
	gtk_entry_set_text(txtbuscarcliente,""); 
}


void
on_btn_modificarcliente_clicked_nombre (GtkEntry       *entry_nombre,
                                        gpointer         user_data)
{
 modifica_cliente.nombre = gtk_editable_get_chars(GTK_EDITABLE(entry_nombre),0,-1);
}


void
on_btn_modificarcliente_clicked_domicilio
                                        (GtkEntry       *entry_domicilio,
                                        gpointer         user_data)
{
 modifica_cliente.domicilio = gtk_editable_get_chars(GTK_EDITABLE(entry_domicilio),0,-1);
}


void
on_btn_modificarcliente_clicked_colonia
                                        (GtkEntry       *entry_colonia,
                                        gpointer         user_data)
{
 modifica_cliente.colonia = gtk_editable_get_chars(GTK_EDITABLE(entry_colonia),0,-1);
}


void
on_btn_modificarcliente_clicked_ciudad_estado
                                        (GtkEntry       *entry_ciudad_estado,
                                        gpointer         user_data)
{
 modifica_cliente.ciudad = gtk_editable_get_chars(GTK_EDITABLE(entry_ciudad_estado),0,-1);
}


void
on_btn_modificarcliente_clicked_telefono
                                        (GtkEntry       *entry_telefono,
                                        gpointer         user_data)
{
 modifica_cliente.telefono = gtk_editable_get_chars(GTK_EDITABLE(entry_telefono),0,-1);
}


void
on_btn_modificarcliente_clicked_telefono2
                                        (GtkEntry       *entry_telefono2,
                                        gpointer         user_data)
{
 modifica_cliente.telefono2 = gtk_editable_get_chars(GTK_EDITABLE(entry_telefono2),0,-1);
}


void
on_btn_modificarcliente_clicked_telefono3
                                        (GtkEntry       *entry_telefono3,
                                        gpointer         user_data)
{
 modifica_cliente.telefono3 = gtk_editable_get_chars(GTK_EDITABLE(entry_telefono3),0,-1);
}


void
on_btn_modificarcliente_clicked_cp     (GtkEntry       *entry_cp,
                                        gpointer         user_data)
{
 modifica_cliente.CPostal = gtk_editable_get_chars(GTK_EDITABLE(entry_cp),0,-1);
}


void
on_btn_modificarcliente_clicked_rfc    (GtkEntry       *entry_rfc,
                                        gpointer         user_data)
{
 modifica_cliente.RFC = gtk_editable_get_chars(GTK_EDITABLE(entry_rfc),0,-1);
}


void
on_btn_modificarcliente_clicked_contacto
                                        (GtkEntry       *entry_contacto,
                                        gpointer         user_data)
{
 modifica_cliente.contacto = gtk_editable_get_chars(GTK_EDITABLE(entry_contacto),0,-1);
}


void
on_btn_modificarcliente_clicked_lista  (GtkEntry       *entry_lista,
                                        gpointer         user_data)
{
 modifica_cliente.nombre_de_lista = gtk_editable_get_chars(GTK_EDITABLE(entry_lista),0,-1);
}


void
on_btn_modificarcliente_clicked_pago   (GtkButton       *entry_pago,
                                        gpointer         user_data)
{
 modifica_cliente.pago = gtk_editable_get_chars(GTK_EDITABLE(entry_pago),0,-1);
}


void
on_btn_modificarcliente_clicked_formato
                                        (GtkEntry       *entry_formato,
                                        gpointer         user_data)
{
 modifica_cliente.formato = gtk_editable_get_chars(GTK_EDITABLE(entry_formato),0,-1);
}


void
on_btn_modificarcliente_clicked_calles (GtkEntry       *entry_calles,
                                        gpointer         user_data)
{
 modifica_cliente.calles = gtk_editable_get_chars(GTK_EDITABLE(entry_calles),0,-1);
}


void
on_btn_modificarcliente_clicked_vencimiento
                                        (GtkEntry       *entry_vencimiento,
                                        gpointer         user_data)
{
 modifica_cliente.vencimiento = gtk_editable_get_chars(GTK_EDITABLE(entry_vencimiento), 0, -1);
}

void
on_btn_modificarcliente_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *entry_modificarclientenombre;
  GtkWidget *entry_modificarclientedomicilio;
  GtkWidget *entry_modificarclientecolonia;
  GtkWidget *entry_modificarclienteciudad_estado;
  GtkWidget *entry_modificarclientecp;
  GtkWidget *entry_modificarclientetelefono;
  GtkWidget *entry_modificarclientetelefono2;
  GtkWidget *entry_modificarclientetelefono3;
  GtkWidget *entry_modificarclienterfc;
  GtkWidget *entry_modificarclienteentre_calles;
  GtkWidget *entry_modificarclientecontacto;
  GtkWidget *combo_entry_modificarclientelista;
  GtkWidget *combo_entry_modificarclientepago;
  GtkWidget *combo_entry_modificarclienteformato;
  GtkWidget *entry_modificarclientevencimiento;
  GtkWidget *clist_clientes;
  gchar *id_cliente, *nombre, *domicilio, *colonia, *ciudad_estado, *cp, *telefono, *telefono2, *telefono3, *rfc, *entre_calles, *contacto, *lista, *pago, *formato, *vencimiento;
  
  MYSQL_RES *res_cta_contable;
  MYSQL_ROW row_cta_contable;
  char sql_checa_contable[200];
  char sql_contable[]="SELECT MAX(cuenta_contable)+1 FROM Cliente";
  char cta_contable[20]="0";
  int bandera_rfc;

  char cadenilla[10000];

  entry_modificarclientenombre = lookup_widget(GTK_WIDGET(button), "entry_modificarclientenombre");
  entry_modificarclientedomicilio = lookup_widget(GTK_WIDGET(button), "entry_modificarclientedomicilio");
  entry_modificarclientecolonia = lookup_widget(GTK_WIDGET(button), "entry_modificarclientecolonia");
  entry_modificarclienteciudad_estado = lookup_widget(GTK_WIDGET(button), "entry_modificarclienteciudad_estado");
  entry_modificarclientecp = lookup_widget(GTK_WIDGET(button), "entry_modificarclientecp");
  entry_modificarclientetelefono = lookup_widget(GTK_WIDGET(button), "entry_modificarclientetelefono");
  entry_modificarclientetelefono2 = lookup_widget(GTK_WIDGET(button), "entry_modificarclientetelefono2");
  entry_modificarclientetelefono3 = lookup_widget(GTK_WIDGET(button), "entry_modificarclientetelefono3");
  entry_modificarclienterfc = lookup_widget(GTK_WIDGET(button), "entry_modificarclienterfc");
  entry_modificarclienteentre_calles = lookup_widget(GTK_WIDGET(button), "entry_modificarclienteentre_calles");
  entry_modificarclientecontacto = lookup_widget(GTK_WIDGET(button), "entry_modificarclientecontacto");
  combo_entry_modificarclientelista = lookup_widget(GTK_WIDGET(button), "combo_entry_modificarclientelista");
  combo_entry_modificarclientepago = lookup_widget(GTK_WIDGET(button), "combo_entry_modificarclientepago");
  combo_entry_modificarclienteformato = lookup_widget(GTK_WIDGET(button), "combo_entry_modificarclienteformato");
  entry_modificarclientevencimiento = lookup_widget(GTK_WIDGET(button), "entry_modificarclientevencimiento");
  clist_clientes                      = lookup_widget(GTK_WIDGET(button), "clist_modificarcliente");

  nombre        = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientenombre), 0,-1);
  domicilio     = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientedomicilio), 0,-1);
  colonia       = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientecolonia), 0,-1);
  ciudad_estado = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclienteciudad_estado), 0,-1);
  cp            = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientecp), 0,-1);
  telefono      = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientetelefono), 0,-1);
  telefono2     = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientetelefono2), 0,-1);
  telefono3     = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientetelefono3), 0,-1);
  rfc           = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclienterfc), 0,-1);
  entre_calles  = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclienteentre_calles), 0,-1);
  contacto      = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientecontacto), 0,-1);
  lista         = gtk_editable_get_chars(GTK_EDITABLE(combo_entry_modificarclientelista), 0,-1);
  pago          = gtk_editable_get_chars(GTK_EDITABLE(combo_entry_modificarclientepago), 0,-1);
  formato       = gtk_editable_get_chars(GTK_EDITABLE(combo_entry_modificarclienteformato), 0,-1);
  vencimiento   = gtk_editable_get_chars(GTK_EDITABLE(entry_modificarclientevencimiento), 0,-1);
  
  sprintf (sql_checa_contable,"SELECT cuenta_contable FROM Cliente Where id_cliente = %s",id_cliente_modifica);
  
  if (strlen(rfc) > 0)
	bandera_rfc = arreglar_rfc(rfc);
  
  
  if((strlen(nombre) == 0) || (strlen(telefono) == 0) ||(strlen(domicilio) == 0))
  	Err_Info("Faltan algunos datos");
  else if(bandera_rfc == 1 && strlen(rfc) > 0)
  {
	Err_Info("El RFC es incorrecto");
	gtk_widget_grab_focus(entry_modificarclienterfc);
  }
  else
  {
  	res_cta_contable = conecta_bd_3(sql_checa_contable);
  	if (res_cta_contable != NULL)
	{
		row_cta_contable = mysql_fetch_row (res_cta_contable);
		if(strcmp(row_cta_contable[0],"0") == 0 && bandera_rfc != 1 && strlen(rfc) > 0)
  		{
  			//Si no tiene cuenta contable genera una nueva
			res_cta_contable = conecta_bd_3(sql_contable);
			if (res_cta_contable != NULL)
			{
				row_cta_contable = mysql_fetch_row (res_cta_contable);
				printf ("%s \n",row_cta_contable[0]);
				strcpy(cta_contable,row_cta_contable[0]);
			}
  		}
	  	else
  		{
  			//Si tiene cuenta contable solo la reafirma
			printf ("%s \n",row_cta_contable[0]);
			strcpy(cta_contable,row_cta_contable[0]);
  		}

	  	sprintf(cadenilla, "UPDATE Cliente SET nombre=\"%s\", domicilio=\"%s\", colonia=\"%s\", ciudad_estado=\"%s\", cp=\"%s\", telefono=\"%s\", telefono2=\"%s\", telefono3=\"%s\", rfc=\"%s\", entre_calles=\"%s\", contacto=\"%s\", id_lista=%s, tipo_pago=\"%s\", tipo_impresion=\"%s\", vencimiento=%s, cuenta_contable = %s WHERE id_cliente=%s", nombre, domicilio, colonia, ciudad_estado, cp, telefono, telefono2, telefono3, rfc, entre_calles, contacto, id_lista_modifica, pago, formato, vencimiento,cta_contable, id_cliente_modifica);

  		printf ("MODIFICA = %s\n",cadenilla);
	  	if(conecta_bd() == -1)
  		{
	      	printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
  		}
  		else
  		{
	      		printf("Cadena: %s\n",cadenilla);
      			er = mysql_query(&mysql,cadenilla);
      			if (er!=0)
			{
				printf("Ocurrio un error al insertar los datos del cliente: %s\n", mysql_error(&mysql));
			}
			else
			{
				gtk_clist_set_text (GTK_CLIST (clist_clientes), obtener_row_seleccionada (clist_clientes,0), 1, nombre);
				gtk_clist_set_text (GTK_CLIST (clist_clientes), obtener_row_seleccionada (clist_clientes,0), 2, lista);
			}
	    	}
	    	mysql_close(&mysql);
	}
	else
	{
		Err_Info ("Ocurrio un error al momento de obtener la cuenta contable.");
	}
    }
}


void
on_btn_modificarcliente_lista          (GtkCList       *clist_clientes,
                                        gpointer         user_data)
{
        /*char sqlclientes[400] = "SELECT Cliente.id_cliente, Cliente.nombre, Lista.nombre FROM Cliente INNER JOIN Lista ON Cliente.id_lista =  Lista.id_lista WHERE Cliente.nombre LIKE \"%";
	gchar *lista[3];
	gchar *criterio;
	int er;

	criterio = criterio_bak;

	if(criterio)
	{
	strcat(sqlclientes, criterio);
	strcat(sqlclientes, "%\"");
	strcat(sqlclientes, " OR Cliente.id_cliente = \"");
	strcat(sqlclientes, criterio);
	}
	strcat(sqlclientes, "\" ORDER BY Cliente.id_cliente");

	gtk_clist_clear(clist_clientes);
	printf("Conecta en 2904\n");
	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqlclientes);
		printf("%s\n",sqlclientes);
		if(er == 0)
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista[0] = row[0];
					lista[1] = row[1];
					lista[2] = row[2];
					gtk_clist_append(clist_clientes, lista);
				}
			}
		}
		else
			printf("Error: %s", mysql_error(&mysql));
	}
	printf("Conecta en 2904\n");
	mysql_close(&mysql);*/

	//gtk_clist_select_row(clist_clientes, 0, 0);

}


void
on_clist_modificarcliente_select_row_codigo
                                        (GtkCList        *clist,
                                        gint             fila,
                                        gint             columna,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  GtkWidget *entry_modificarclientenombre;
  GtkWidget *entry_modificarclientedomicilio;
  GtkWidget *entry_modificarclientecolonia;
  GtkWidget *entry_modificarclienteciudad_estado;
  GtkWidget *entry_modificarclientecp;
  GtkWidget *entry_modificarclientetelefono;
  GtkWidget *entry_modificarclientetelefono2;
  GtkWidget *entry_modificarclientetelefono3;
  GtkWidget *entry_modificarclienterfc;
  GtkWidget *entry_modificarclienteentre_calles;
  GtkWidget *entry_modificarclientecontacto;
  GtkWidget *combo_entry_modificarclientelista;
  GtkWidget *combo_entry_modificarclientepago;
  GtkWidget *combo_entry_modificarclienteformato;
  GtkWidget *entry_modificarclientevencimiento;
  
  GtkWidget *combo_listas;
  GList *lista_listas = NULL;
  
  gchar *id_cliente, *nombre, *domicilio, *colonia, *ciudad_estado, *cp, *telefono, *telefono2, *telefono3, *rfc, *entre_calles, *contacto, *lista, *pago, *formato, *vencimiento;
  gchar *id_cliente_lista;

  char sqllistas[400];

  entry_modificarclientenombre = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientenombre");
  entry_modificarclientedomicilio = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientedomicilio");
  entry_modificarclientecolonia = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientecolonia");
  entry_modificarclienteciudad_estado = lookup_widget(GTK_WIDGET(clist), "entry_modificarclienteciudad_estado");
  entry_modificarclientecp = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientecp");
  entry_modificarclientetelefono = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientetelefono");
  entry_modificarclientetelefono2 = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientetelefono2");
  entry_modificarclientetelefono3 = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientetelefono3");
  entry_modificarclienterfc = lookup_widget(GTK_WIDGET(clist), "entry_modificarclienterfc");
  entry_modificarclienteentre_calles = lookup_widget(GTK_WIDGET(clist), "entry_modificarclienteentre_calles");
  entry_modificarclientecontacto = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientecontacto");
  combo_entry_modificarclientelista = lookup_widget(GTK_WIDGET(clist), "combo_entry_modificarclientelista");
  combo_entry_modificarclientepago = lookup_widget(GTK_WIDGET(clist), "combo_entry_modificarclientepago");
  combo_entry_modificarclienteformato = lookup_widget(GTK_WIDGET(clist), "combo_entry_modificarclienteformato");
  entry_modificarclientevencimiento = lookup_widget(GTK_WIDGET(clist), "entry_modificarclientevencimiento");
  combo_listas = lookup_widget(GTK_WIDGET(clist), "combo_modificarclientelista");

  gtk_clist_get_text(clist,fila,0,&id_cliente_lista);

  strcpy(id_cliente_modifica, id_cliente_lista);

  sprintf(sqllistas, "SELECT Cliente.id_cliente, Cliente.nombre, Cliente.domicilio, Cliente.colonia, Cliente.ciudad_estado, Cliente.cp, Cliente.telefono, Cliente.telefono2, Cliente.telefono3, Cliente.rfc, Cliente.entre_calles, Cliente.contacto, Lista.nombre, Cliente.tipo_pago, Cliente.tipo_impresion, Cliente.vencimiento, Cliente.id_lista FROM Cliente INNER JOIN Lista USING (id_lista) WHERE id_cliente = %s", id_cliente_lista);

	if(conecta_bd() == -1)
    	{
      		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
    	}
	else
	{
		er = mysql_query(&mysql,sqllistas);
		if(er != 0)
			printf("Error en el query de listas: %s\n", mysql_error(&mysql));
		else
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row=mysql_fetch_row(res)))
				{
					id_cliente = row[0];
					nombre = row[1];
					domicilio = row[2];
					colonia = row[3];
					ciudad_estado = row[4];
					cp = row[5];
					telefono = row[6];
					telefono2 = row[7];
					telefono3 = row[8];
					rfc = row[9];
					entre_calles = row[10];
					contacto = row[11];
					lista = row[12];
					pago = row[13];
					formato = row[14];
					vencimiento = row[15];
					strcpy(id_lista_modifica,row[16]);
				}
			}
		}
	}
 	mysql_close(&mysql);
	
	
	if(conecta_bd() == -1)
	{
		printf("No se pudo conectar a la base de datos. Error: %s\n", mysql_error(&mysql));
	}
	else
	{
		er = mysql_query(&mysql,"SELECT Lista.nombre From Lista");
		if(er != 0)
			printf("Error en el query de listas: %s\n", mysql_error(&mysql));
		else
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				while((row=mysql_fetch_row(res)))
				{
					lista_listas = g_list_append (lista_listas, row[0]);
				}
			}
		}
	}
	mysql_close(&mysql);

  	gtk_combo_set_popdown_strings (GTK_COMBO (combo_listas), lista_listas);

  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientenombre), nombre);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientedomicilio), domicilio);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientecolonia), colonia);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclienteciudad_estado), ciudad_estado);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientecp), cp);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientetelefono), telefono);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientetelefono2), telefono2);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientetelefono3), telefono3);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclienterfc), rfc);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclienteentre_calles), entre_calles);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientecontacto), contacto); 
  gtk_entry_set_text(GTK_ENTRY(combo_entry_modificarclientelista), lista);
  gtk_entry_set_text(GTK_ENTRY(combo_entry_modificarclientepago), pago);
  gtk_entry_set_text(GTK_ENTRY(combo_entry_modificarclienteformato), formato);
  gtk_entry_set_text(GTK_ENTRY(entry_modificarclientevencimiento), vencimiento);
}



void
on_clist_modificarcliente_select_row_nombre
                                        (GtkEntry        *entry_nombre,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
/*	gtk_entry_set_text(entry_nombre, "");
	gtk_entry_set_text(entry_nombre, modifica_cliente.nombre);
	printf("en clist_modif_del_row_nombre nombre: %s\n",modifica_cliente.nombre);*/
}


void
on_clist_modificarcliente_select_row_domicilio
                                        (GtkEntry        *entry_domicilio,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_domicilio, "");
 gtk_entry_set_text(entry_domicilio, modifica_cliente.domicilio);*/
}


void
on_clist_modificarcliente_select_row_colonia
                                        (GtkEntry        *entry_colonia,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_colonia, "");
 gtk_entry_set_text(entry_colonia, modifica_cliente.colonia);*/
}


void
on_clist_modificarcliente_select_row_ciudad_estado
                                        (GtkEntry        *entry_ciudad_estado,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_ciudad_estado, "");
 gtk_entry_set_text(entry_ciudad_estado, modifia_cliente.ciudad);*/
}


void
on_clist_modificarcliente_select_row_cp
                                        (GtkEntry        *entry_cp,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_cp, "");
 gtk_entry_set_text(entry_cp, modifica_cliente.CPostal);*/
}


void
on_clist_modificarcliente_select_row_telefono
                                        (GtkEntry        *entry_telefono,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_telefono, "");
 gtk_entry_set_text(entry_telefono, modifica_cliente.telefono);*/
}


void
on_clist_modificarcliente_select_row_telefono2
                                        (GtkEntry        *entry_telefono2,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_telefono2, "");
 gtk_entry_set_text(entry_telefono2, modifica_cliente.telefono2);*/
}


void
on_clist_modificarcliente_select_row_telefono3
                                        (GtkEntry        *entry_telefono3,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_telefono3, "");
 gtk_entry_set_text(entry_telefono3, modifica_cliente.telefono3);*/
}


void
on_clist_modificarcliente_select_row_rfc
                                        (GtkEntry        *entry_rfc,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_rfc, "");
 gtk_entry_set_text(entry_rfc, modifica_cliente.RFC);*/
}


void
on_clist_modificarcliente_select_row_entrecalles
                                        (GtkEntry        *entry_calles,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_calles, "");
 gtk_entry_set_text(entry_calles, modifica_cliente.calles);*/
}


void
on_clist_modificarcliente_select_row_contacto
                                        (GtkEntry        *entry_contacto,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_contacto, "");
 gtk_entry_set_text(entry_contacto, modifica_cliente.contacto);*/
}


void
on_clist_modificarcliente_select_row_pago
                                        (GtkEntry        *entry_pago,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_pago, modifica_cliente.pago);*/
}


void
on_clist_modificarcliente_select_row_formato
                                        (GtkEntry        *entry_formato,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_formato, modifica_cliente.formato);*/
}


void
on_clist_modificarcliente_select_row_lista
                                        (GtkEntry        *entry_lista,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_lista, "");
 gtk_entry_set_text(entry_lista, lista2);*/
}


void
on_clist_modificarcliente_select_row_vencimiento
                                        (GtkEntry        *entry_vencimiento,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 /*gtk_entry_set_text(entry_vencimiento, "");
 gtk_entry_set_text(entry_vencimiento, modifica_cliente.vencimiento);*/
 
}


void
on_combo_entry_modificarclientelista_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
	char sqllista[100];
	gchar *lista;

	lista = gtk_editable_get_chars(editable,0,-1);

	sprintf(sqllista, "SELECT id_lista FROM Lista WHERE nombre=\"%s\"", lista);

	if(conecta_bd() == -1)
  	{
      		printf("No se pudo conectar a la base de datos Error: %s\n", mysql_error(&mysql));
  	}
  	else
  	{
      		printf("Cadena: %s\n",sqllista);
      		er = mysql_query(&mysql,sqllista);
      		if (er!=0)
			printf("Ocurrió un error al insertar los datos del cliente: %s\n", mysql_error(&mysql));
		else
		{
			res = mysql_store_result(&mysql);
			if(res)
			{
				if((row = mysql_fetch_row(res)))
				{
					printf("ID Lista: %s\n", row[0]);
					strcpy(id_lista_modifica,row[0]);
				}
				else
					printf("Error: %s\n", mysql_error(&mysql));
			}
		}
    	}
	
    	mysql_close(&mysql);
}

