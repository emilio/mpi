---
title: Práctica MPI
subtitle: Arquitectura de Computadores - 2016
numbersections: true
theme: white
lang: es
babel-lang: spanish
css:
  - reveal-custom.css
author:
  - Emilio Cobos Álvarez
  - Konstantin Danielov Kostandev
  - Alberto González San Juan
  - Héctor Gonzalo Andrés
---

# Problema a resolver

Conseguir las cadenas originales a partir de los hash obtenidos mediante la
utilización de la función `crypt()`.

Esto se realizará mediante fuerza bruta, aprovechando la tecnología MPI para
poder dividir el trabajo a realizar entre varios procesos y nodos de forma
simple.

---

# Primera idea: Work Stealing

Inicialmente tratamos de usar **Work Stealing**, es decir, **paralelismo
especulativo**. Es un algoritmo que se basa en que uno o varios procesos tienen
una cola de trabajo por hacer, pero otro proceso con la cola vacía puede
efectivamente *robarle* el trabajo cuando su propia cola esté vacía.

Los requerimientos del enunciado de la práctica hacían que la única forma viable
fuera que el proceso 0 tuviera la única cola de trabajo.

Por desgracia, **no funcionó la implementación inicial** (dos hilos
sincronizados dentro del proceso 0), ya que las llamadas síncronas de MPI
bloquean **todo el proceso**, no sólo el hilo.

Hacerlo funcionar en nuestra implementación inicial **sería viable**, pero
habría que hacer balance entre repartir trabajo y realizarlo.

---

# El algoritmo

El algoritmo es una **versión simplificada del Work Stealing**, en la que el
proceso 0 mantiene una cola de trabajos a realizar, y **el resto de procesos
piden al proceso cero** según van pudiendo realizar un trabajo.

De esta manera **no todos los procesos tienen porqué realizar el mismo
trabajo**, y se evitan esperas de los procesos más rápidos.

---

# Modos de ejecución

Aparte, tenemos dos modos de ejecución: **síncrono y asíncrono**.

 * En el **modo síncrono** el proceso 0 se asegura de que todos han acabado de
     realizar los trabajos que tenían asignados antes de pasar a la siguiente
     contraseña.
 * En el **modo asíncrono** nada más se encuentra la contraseña se pasa a la
     siguiente, lo que hace que **el total de trabajo reportado para
     desencriptar una contraseña no sea exacto**.

---

# Funcionamiento del programa

Lo primero que realiza el programa es enterarse de cuántos procesos hay.

 * Si **sólo hay un proceso**, al no haber otros procesos a los que asignar el
   trabajo, el proceso cero realiza un **fallback secuencial**,
   `sequential_fallback()`, en el que realiza todo el trabajo.

 * Si hay procesos a los que distribuir el trabajo, se utiliza la estrategia
   comentada anteriormente.

---

## Un sólo proceso

Será el proceso el que realice todos los trabajos. Para reutilizar lógica,
genera un `job_t` que abarca todas las posibles combinaciones de cada
contraseña, y lo procesa él mismo.


## $N$ procesos ($N - 1$ *workers*)

El proceso cero ejecutará la función `dispatcher_thread()`, que se encargará de
leer las contraseñas y distribuir el trabajo.

Esa función fue inicialmente planteada para correr en un hilo diferente, por eso
su signatura:

```c
void* dispatcher_thread(void* arg);
```

---

## Flujo de información

El flujo de información es el siguiente:

 1. El proceso 0 manda un objeto del tipo `epoch_info_t` al resto de procesos.
    Éste objeto contiene un contador monótono (que se usará para controlar la
    concurrencia en el modo asíncrono) y la contraseña a descifrar.

 2. El proceso cero escucha al resto de procesos continuamente, tanto por
    solicitudes de trabajo (`request_t`), como para respuestas de cómo ha ido el
    trabajo (`reply_t`).

    1. Si le llega una solicitud, envía el siguiente trabajo a realizar, o si no
       hay más un trabajo marcado como inválido (`is_valid = 0`).

    2. Si le llega una respuesta, actualiza los datos del trabajo realizado por
       el *worker*, y si es correcta bien espera al resto (síncrono), o pasa
       a la siguiente fase (asíncrono).

 3. El *worker* lee el mensaje de información (`epoch_info_t`), y si es válido
    realiza una petición de trabajo y aguarda el trabajo. Si el trabajo es
    válido, lo realiza y reporta al proceso cero las iteraciones que ha
    realizado y si ha encontrado la contraseña. Si no, vuelve a esperar un
    mensaje para la siguiente contraseña.

---

# Tipos de datos

Aparte del tipo de información `job_t`, que representa un trabajo, y que ya
hemos visto, tenemos toda una serie de tipos de datos que permiten la
comunicación correcta entre los procesos.

---

## Información de la "época" (`epoch_info_t`)

Ya hemos discutido este tipo de dato anteriormente, aquí está la definición:

```c
// src/epoch_info.h
// This is the struct sent to signify the start of a new epoch.
// It contains the password to decrypt and the epoch itself.
typedef struct epoch_info {
    uint32_t epoch;
    char password[CRYPT_PASSWORD_LEN + 1];
} epoch_info_t;
```

Para representar el fin del trabajo se enviará una cadena vacía en el campo de
la contraseña (y adicionalmente `-1` en el campo `epoch`, es decir, el máximo
valor posible).

---

## Solicitud de trabajo (`request_t`)

Los *workers* enviarán este objeto para informar al proceso cero de que están
listos para realizar un trabajo. Contiene sólo el `epoch` en el que están los
hijos, para que el proceso padre pueda saber si la contraseña que están
descifrando sigue activa y, si no es el caso, pueda enviarles un trabajo
inválido para que se muevan a la siguiente[^async-only].

[^async-only]: Nótese que esto sólo puede pasar en modo asíncrono, si pasara en
modo síncrono el programa acabaría abruptamente porque sería un error lógico.

```c
// src/request.h
typedef struct request { uint32_t epoch; } request_t;
```

---

## Trabajo (`job_t`)

Es una de las estructuras centrales, que contiene si es válido (`is_valid`), en
qué contraseña tiene que empezar (`start`), cuántas contraseñas tiene que
comprobar (`length`), y el `epoch` al que corresponde el trabajo.

La longitud por defecto del trabajo (`JOB_SIZE`, por defecto 5000), es un
parámetro interesante que puede tener impacto tanto en el rendimiento como en la
responsividad del programa.

```c
// src/job.h
typedef struct job {
    // A job can only be processed if it's valid.
    // An invalid job is returned if there is no more
    // remaining work to do.
    uint8_t is_valid;
    // The first password you'd want to check.
    uint32_t start;
    // How many passwords you have to check.
    uint32_t length;
    // A monothonic counter used to identify a batch of
    // job/request pairs.
    uint32_t epoch;
} job_t;
```

---

## Respuesta (`reply_t`)

Cuando un proceso acaba un trabajo que le había sido asignado, responde al
proceso cero con éste objeto, que indica la `epoch`, cuántas iteraciones ha
realizado (`try_count`), si ha conseguido encontrar la contraseña
(`is_success`), y si es así cuál es (`decrypted`).

```c
// src/reply.h
typedef struct reply {
    uint8_t is_success;
    uint32_t try_count;
    uint32_t epoch;
    char decrypted[MPI_DECRYPTED_PASSWORD_LEN + 1];
} reply_t;
```

---

# Informe en CSV

Cuando corres el proceso con una configuración determinada se genera un archivo
csv en la carpeta `bench/data` cuyo nombre depende exclusivamente de esa
configuración.

Cada línea del archivo tendrá los resultados de descifrar cada una de las
contraseñas.

El nombre del archivo **depende del modo de compilación y ejecución, del número
de procesos, del número de contraseñas, y de un hash de esas contraseñas**
(usando la función `crypt()`).

Se puede ver el código relevante en los archivos `src/csv.h` y `src/csv.c`.

---

# Testing

Se ha llegado a probar **hasta con 250 procesos en 25 ordenadores distintos**.
En la carpeta `bench/` se encuentran los scripts utilizados para automatizar la
tarea de subir el programa compilado a los ordenadores (`bench/add-ssh`), y de
probar diferentes configuraciones (`bench/bench-local`).

---

# Generación de gráficas

Con la cantidad enorme de datos generados en CSV, está claro que no íbamos
a hacer todas las gráficas manualmente. Por eso también existe un script
(`bench/gen-graphics.py`), que utiliza el módulo `matplotlib` de Python para
generarlas.

---

# Datos

A continuación mostraremos gráficas en las podremos comparar tiempos de
ejecución, dependiendo del número de procesos, del tamaño de trabajo y del modo
de ejecución y compilación.

---

![Modo asíncrono,
1 contraseña, 15000](../bench/by-password/release-async-job-size-15000-aal9_sIHZQyhA.svg)


![Modo asíncrono,
1 contraseña, 5000](../bench/by-password/release-async-job-size-5000-aal9_sIHZQyhA.svg)

---

![Modo síncrono,
1 contraseña, 15000](../bench/by-password/release-sync-job-size-15000-aal9_sIHZQyhA.svg)


![Modo síncrono,
1 contraseña, 5000](../bench/by-password/release-sync-job-size-5000-aal9_sIHZQyhA.svg)

---

![Modo asíncrono, 4 contraseñas,
15000](../bench/by-password/release-async-job-size-15000-aaTrgM6tVLhasaagWNRh7V9kN6aahpg4OwfHMXYaaTwdzPnfU7XE.svg)

![Modo asíncrono, 4 contraseñas,
5000](../bench/by-password/release-async-job-size-5000-aaTrgM6tVLhasaagWNRh7V9kN6aahpg4OwfHMXYaaTwdzPnfU7XE.svg)

---

![Modo síncrono, 4 contraseñas,
15000](../bench/by-password/release-sync-job-size-15000-aaTrgM6tVLhasaagWNRh7V9kN6aahpg4OwfHMXYaaTwdzPnfU7XE.svg)

![Modo síncrono, 4 contraseñas,
5000](../bench/by-password/release-sync-job-size-5000-aaTrgM6tVLhasaagWNRh7V9kN6aahpg4OwfHMXYaaTwdzPnfU7XE.svg)

---

![Modo asíncrono, 127 workers, 4 contraseñas,
5000](../bench/127-workers/release-async-127-workers-4-passwords-job-size-15000-74.HC7QMV0cr..csv.svg)

![Modo síncrono, 127 workers, 4 contraseñas,
5000](../bench/127-workers/release-sync-127-workers-4-passwords-job-size-15000-74.HC7QMV0cr..csv.svg)

---

# ¿Preguntas?

---

# Fin

![Ningún trabajador fue herido durante esta práctica](marx-bio.jpg)
