# Trabajo MPI para Arquitectura de Computadores

La idea general es usar *work stealing*, y así no forzar a que los procesos más
rápidos tengan que esperar a los más lentos.

Esto implica overhead por comunicación, así que probablemente haya que decidir
sabiamente el tamaño por defecto de los mensajes.

## Instrucciones
> Descargar como .zip
> Descomprimir
> ```make```
> ```cd bin```
> ```mpirun -np X main HASH HASH ... HASH``` X == numero de procesos  HASH == uno mas de los hash de prueba
> Hash de prueba 
  >```aaTrgM6tVLhas``` 
  >```aagWNRh7V9kN6``` 
  >```aahpg4OwfHMXY``` 
  >```aaTwdzPnfU7XE``` 
  >```aaxF36GTHquV```
