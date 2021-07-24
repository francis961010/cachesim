#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define ERROR -1

typedef struct linea {
    bool validez;
    bool dirtybit;
    int tag;
    int bloque;
} Linea;

typedef struct set {
    Linea **lineas; 
    int cantidad_lineas;
    int tamanio_bloque;
} Set;

typedef struct metrics {
    int loads;
    int stores;
    int rmiss;
    int wmiss;
    int dirty_rmiss;
    int dirty_wmiss;
    int bytes_read;
    int bytes_written;
    int read_time;
    int write_time;
} Metrics;

typedef struct cache {
    Set **sets;
    Metrics *metrics; 
    int cantidad_sets;
    bool mapeo_directo;
} Cache;

typedef struct entrada {
    u_int32_t instruction_pointer;
    char operacion;
    u_int32_t direccion;
    int tamanio_operacion;
    u_int32_t datos_leidos;
} Entrada;

bool son_potencia_2(char const *argv[]){
    if(ceil(log2(atoi(argv[2]))) != floor(log2(atoi(argv[2]))))
        return false;
    if(ceil(log2(atoi(argv[3]))) != floor(log2(atoi(argv[3]))))
        return false;
    if(ceil(log2(atoi(argv[4]))) != floor(log2(atoi(argv[4]))))
        return false;
    return true;
}

void seti_y_tag(u_int32_t direccion, int *set_index, int *tag){ //aplic amas para conseguir el tag, set index y block offset

}

void modo_simple(){ // modo simple

}

Cache *crear_cache(char const *parametros[]){ // guarda memoria para el cache
    Cache *cache = calloc(1, sizeof(Cache));
    if(!cache)
        return NULL;

    cache->cantidad_sets = parametros[4];                               //S
    cache->sets = calloc(cache->cantidad_sets, sizeof(Set));
    for(int i = 0; i < cache->cantidad_sets; i++) {
        cache->sets[i]->lineas = crear_lineas(atoi(parametros[3])); //Lineas
        cache->sets[i]->cantidad_lineas = atoi(parametros[3]);
    }
    cache->metrics = calloc(1, sizeof(Metrics));
    cache->mapeo_directo = false;
    if(atoi(parametros[3]) == 1)
        cache->mapeo_directo = true;

    return cache;
}

Linea **crear_lineas(int cant_lineas){ //crea lineas del cache
    Linea **lineas = calloc(cant_lineas, sizeof(Linea));
    if(!lineas)
        return NULL;
    for(int i = 0; i < cant_lineas; i++){
        lineas[i]->validez = false;
        lineas[i]->dirtybit = false;
    }
    return lineas
}

int main(int argc, char const *argv[]){

    if(argc != 5 && argc != 8) //console command no es tamanio correcto
        return ERROR;

    if(!son_potencia_2(argv)) //argumentos no son pares [2] ; [3] ; [4]
        return ERROR;

    Cache *cache = crear_cache(argv);   //crea cache
    if(!cache)
        return ERROR;

    FILE *archivo = fopen(argv[1], "r");
    if(!archivo)
        return ERROR;
    while(!feof){
        Entrada *entrada = NULL;
        fscanf(archivo, entrada->instruction_pointer, entrada->operacion, entrada->direccion, entrada->tamanio_operacion, entrada->datos_leidos); // guardo datosa de la liena;
        ///Datos de la direccion
        int set_index;
        int tag;
        int bloque;
        seti_y_tag(entrada->direccion ,&set_index, &tag /*, ALGO MAS*/);
        ///

        if(argc == 5)
            modo_simple();


    }
    return 0;
}


//funcion que encuentra la linea dentro del set dado en el parametro

//funcion para write

//funcion para read

