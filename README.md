# Trabajo MPI para Arquitectura de Computadores

La idea general es usar *work stealing*, y así no forzar a que los procesos más
rápidos tengan que esperar a los más lentos.

Esto implica overhead por comunicación, así que probablemente haya que decidir
sabiamente el tamaño por defecto de los mensajes.

## Instrucciones

1. Descargar como .zip y descomprimir o clonar con `git`.

```
$ make bench
```

Acabaréis con un monton de datos en `bench/data/`, podéis usarlo para montar
estadísticas. El nombre del fichero indica el modo de compilación, de ejecución,
el número de procesos "trabajadores" y de contraseñas.

### Hashes de prueba

 * `aaTrgM6tVLhas`
 * `aagWNRh7V9kN6`
 * `aahpg4OwfHMXY`
 * `aaTwdzPnfU7XE`
