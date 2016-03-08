# Trabajo MPI para Arquitectura de Computadores

La idea general es usar *work stealing*, y así no forzar a que los procesos más
rápidos tengan que esperar a los más lentos.

Esto implica overhead por comunicación, así que probablemente haya que decidir
sabiamente el tamaño por defecto de los mensajes.
