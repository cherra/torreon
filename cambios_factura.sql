alter table Venta add num_factura int unsigned not null AFTER pagada;
alter table Venta add fecha_factura date not null after num_factura;
alter table Venta add hora_factura time not null after fecha_factura;
alter table Venta add observacion text after hora_factura;
