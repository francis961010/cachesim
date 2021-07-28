#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define ERROR -1
#define MISS_PENALTY 100

typedef struct verboso{
    bool validez_previo;
    bool birtybit_previo;
    int operacion_anterior;
    int numero_linea;
    int tag_previo;
    int caso;
} Verboso;

typedef struct linea {
    bool validez;
    bool dirtybit;
    int tag;
    int bloque;
    int ultima_operacion;
} Linea;

typedef struct set {
    Linea **lineas; 
    int cantidad_lineas;
    int tamanio_bloque;
    int lineas_ocupadas;
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
    int numero_de_operacion;
    int instruction_pointer;
    char operacion;
    int direccion;
    int tamanio_operacion;
    int datos_leidos;
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

void seti_y_tag(u_int32_t *direccion, int tamanio_seti, int tamanio_block_offset, int *set_index, int *tag){

    int mask_s_b = (0x01 << (tamanio_block_offset + tamanio_seti)) - 1;
    int mask_b = (0x01 << tamanio_block_offset) - 1;
    int mask_s = mask_s_b ^ mask_b;

    *set_index = (mask_s & *(int*)direccion) >> tamanio_block_offset;
    
    int mask_tag = mask_s_b ^ 0xffffffff;

    int32_t aux = (*direccion & mask_tag);
    aux = aux >> (tamanio_block_offset + tamanio_seti);

    int tamanio_tag = 32 - ((tamanio_block_offset + tamanio_seti));
    int mask_tdos = (0x01 << tamanio_tag) - 1;

    *tag = mask_tdos & aux;
}

void crear_lineas(Set *set, int cant_lineas){ 

    set->lineas = calloc(cant_lineas, sizeof(Linea*));
    if(!set->lineas)
        return;
    for(int i = 0; i < cant_lineas; i++){
        set->lineas[i] = calloc(1, sizeof(Linea));
        set->lineas[i]->validez = false;
        set->lineas[i]->dirtybit = false;
    }
}

Cache *crear_cache(char const *parametros[]){
    Cache *cache = calloc(1, sizeof(Cache));
    if(!cache)
        return NULL;

    cache->cantidad_sets = atoi(parametros[4]);                            
    cache->sets = calloc(cache->cantidad_sets, sizeof(Set*));
    for(int i = 0; i < cache->cantidad_sets; i++) {
        cache->sets[i] = calloc(1, sizeof(Set));
        crear_lineas(cache->sets[i], atoi(parametros[3])); 
        cache->sets[i]->cantidad_lineas = atoi(parametros[3]);
        cache->sets[i]->lineas_ocupadas = 0;
    }
    cache->metrics = calloc(1, sizeof(Metrics));
    cache->mapeo_directo = false;
    if(atoi(parametros[3]) == 1)
        cache->mapeo_directo = true;
    return cache;
}

Linea *encontrar_linea(Set *set_actual, int32_t tag){

    for(int i = 0; i < set_actual->cantidad_lineas; i++){
        Linea *linea_actual = set_actual->lineas[i];

        if(linea_actual->tag == tag){
            return linea_actual;
        }
    }
    return NULL;
}

Linea *least_recently_used(Set *set_actual){
    
    int num_linea = 0;
    Linea *ultima_linea_usada = set_actual->lineas[num_linea];
    
    for(int i = 1; i < set_actual->cantidad_lineas; i++){
        if(ultima_linea_usada->ultima_operacion > set_actual->lineas[i]->ultima_operacion){
            ultima_linea_usada = set_actual->lineas[i];
            num_linea = i;
        }
    }
    return ultima_linea_usada;
}

void linea_fria(Set *set_actual, Metrics *metrics, int tag, Entrada *operacion_actual, int bytesxbloque){ 

   for(int i = 0; i < set_actual->cantidad_lineas; i++){
       if(set_actual->lineas[i]->validez == false){
            Linea *linea_actual = set_actual->lineas[i];
            linea_actual->tag = tag;
            linea_actual->validez = true;
            linea_actual->ultima_operacion = operacion_actual->numero_de_operacion;
            set_actual->lineas_ocupadas++;
            if(strcmp(&(operacion_actual->operacion), "W") == 0){
                linea_actual->dirtybit = true;
                metrics->bytes_read += bytesxbloque;
                metrics->wmiss++;
                metrics->stores++;
                metrics->write_time += 1+MISS_PENALTY;
                return;
            }
            else if(strcmp(&(operacion_actual->operacion), "R") == 0){ 
                metrics->read_time += 1+MISS_PENALTY;
                metrics->loads++;
                metrics->rmiss++;
                metrics->bytes_read += bytesxbloque;
                return;
            }
       }
   }
}

void linea_clean_miss(Metrics *metrics, int tag, Entrada *operacion_actual, Linea *linea_actual, int bytesxbloque){

    linea_actual->ultima_operacion = operacion_actual->numero_de_operacion;
    if(strcmp(&(operacion_actual->operacion), "W") == 0){
        linea_actual->dirtybit = true;
        linea_actual->tag = tag;
        metrics->bytes_read +=  bytesxbloque;
        metrics->stores++;
        metrics->wmiss++;
        metrics->write_time += 1 + MISS_PENALTY;
    }
    else if(strcmp(&(operacion_actual->operacion), "R") == 0){   
        linea_actual->tag = tag;
        metrics->bytes_read +=  bytesxbloque;
        metrics->rmiss++;
        metrics->loads++;
        metrics->read_time += 1 + MISS_PENALTY;
    }
}

void linea_dirty_miss(Metrics *metrics, int tag, Entrada *operacion_actual, Linea *linea_actual, int bytesxbloque){

    linea_actual->ultima_operacion = operacion_actual->numero_de_operacion;
    if(strcmp(&(operacion_actual->operacion), "W") == 0){
        linea_actual->dirtybit = true;
        linea_actual->tag = tag;
        metrics->bytes_written +=  bytesxbloque;
        metrics->bytes_read +=  bytesxbloque;
        metrics->stores++;
        metrics->wmiss++;
        metrics->dirty_wmiss++;
        metrics->write_time += 1 + (2 * MISS_PENALTY);
    }
    else if(strcmp(&(operacion_actual->operacion), "R") == 0){ 
        linea_actual->dirtybit = false;
        linea_actual->tag = tag;
        metrics->bytes_read +=  bytesxbloque;
        metrics->bytes_written +=  bytesxbloque;
        metrics->dirty_rmiss+=1;
        metrics->rmiss++;
        metrics->loads++;
        metrics->read_time += 1 + (MISS_PENALTY * 2);
    }
}

void linea_hit(Linea *linea_actual, Metrics *metrics, Entrada *operacion_actual){

    linea_actual->ultima_operacion = operacion_actual->numero_de_operacion;
    if(strcmp(&(operacion_actual->operacion), "W") == 0){ 
        linea_actual->dirtybit = true;
        metrics->write_time+=1;
        metrics->stores++;
        return;
    }
    else if(strcmp(&(operacion_actual->operacion), "R") == 0){
        metrics->read_time+=1;
        metrics->loads++;
        return;
    }
}

void modo_simple(Cache *cache, int tag, int set_index, Entrada *entrada, int bytesxbloque){

    Set *set_actual = cache->sets[set_index];
    Linea *linea = encontrar_linea(set_actual, tag);
    bool set_esta_lleno = false;
    if(set_actual->cantidad_lineas == set_actual->lineas_ocupadas)
        set_esta_lleno = true;

    if(!linea && !set_esta_lleno)
        linea_fria(set_actual, cache->metrics, tag, entrada, bytesxbloque);

    else if(!linea && set_esta_lleno){
        Linea *ultima_linea_usada = least_recently_used(set_actual);
        if(ultima_linea_usada->dirtybit == false)
            linea_clean_miss(cache->metrics, tag, entrada, ultima_linea_usada, bytesxbloque);
        else
            linea_dirty_miss(cache->metrics, tag, entrada, ultima_linea_usada, bytesxbloque);
    }
    else if(linea && linea->validez == true)
        linea_hit(linea, cache->metrics, entrada);
}

void encontrar_caso(Set *set_actual, Linea *linea, int encontro_linea, Verboso *datos){

    bool set_esta_lleno = false;
    if(set_actual->cantidad_lineas == set_actual->lineas_ocupadas)
        set_esta_lleno = true;

    if(encontro_linea == 1 && !set_esta_lleno){
        datos->caso = 1;
    }
    else if(encontro_linea == 1 && set_esta_lleno){
        if(linea->dirtybit == false){
           datos->caso = 1;
        }
        else{
            datos->caso = 2;
        }
    }
    else if(encontro_linea == 0 && linea->validez == true){
        datos->caso = 0;
    }
}

void linea_datos_verboso(Set *set_actual, int tag, Verboso *datos){
    Linea *linea = encontrar_linea(set_actual, tag);
    int es_valido = 0;
    if(!linea){
        es_valido = 1;
        linea = least_recently_used(set_actual);
    }
    encontrar_caso(set_actual, linea, es_valido, datos);
    datos->birtybit_previo = linea->dirtybit;
    if(!linea->tag)
        datos->tag_previo = -1;
    else
        datos->tag_previo = linea->tag; 
    datos->validez_previo = linea->validez;
    datos->operacion_anterior = linea->ultima_operacion;
    for(int i = 0; i < set_actual->cantidad_lineas; i++){
        if(linea->tag == set_actual->lineas[i]->tag)
            datos->numero_linea = i;
    }
}

void modo_verboso(Cache *cache, int tag, int set_index, Entrada *entrada){

    Set *set_actual = cache->sets[set_index];
    Verboso *datos_verboso = calloc(1, sizeof(Verboso));
    linea_datos_verboso(set_actual, tag, datos_verboso);
    printf("%i ", entrada->numero_de_operacion);
    switch (datos_verboso->caso){                           
        case 0:                                           
            printf("1 ");                                
            break;                                        
        case 1:                                          
            printf("2a ");                              
            break;                                      
        case 2:                                          
            printf("2b ");                              
            break;                                        
    }                                                   
    char hex_1[4];
    sprintf(hex_1, "%x", set_index);
    printf("%s ", hex_1);     
    char hex_2[10];
    sprintf(hex_2, "%x", tag);
    printf("%s ", hex_2);      
    printf("%i ", datos_verboso->numero_linea);     
    if(datos_verboso->tag_previo == -1)                       
        printf("%d ", datos_verboso->tag_previo);            
    else{                                                        
        char hex_3[10];
        sprintf(hex_3, "%x", datos_verboso->tag_previo);
        printf("%s ", hex_3);  
    }
    printf("%d ", datos_verboso->validez_previo);   
    printf("%d ", datos_verboso->birtybit_previo);  
    printf("%d\n", datos_verboso->operacion_anterior);  
    free(datos_verboso);
}

void devolver_resumen(Cache *cache, int E, int tamanio_cache){
    if(cache->mapeo_directo)
        printf("direct-mapped, ");
    else
        printf("%i-way, ", E);
    printf("%i sets, size = %iKB\n", cache->cantidad_sets, (int)(tamanio_cache/1000));
    printf("loads %i stores %i total %i\n", cache->metrics->loads, cache->metrics->stores, cache->metrics->stores+cache->metrics->loads);
    printf("rmiss %i wmiss %i total %i\n", cache->metrics->rmiss, cache->metrics->wmiss, cache->metrics->rmiss+cache->metrics->wmiss);
    printf("dirty rmiss %i dirty wmiss %i\n", cache->metrics->dirty_rmiss, cache->metrics->dirty_wmiss);
    printf("bytes read %i bytes written %i\n", cache->metrics->bytes_read, cache->metrics->bytes_written);
    printf("read time %i write time %i\n", cache->metrics->read_time, cache->metrics->write_time);
    printf("miss rate %f\n", (double)(cache->metrics->rmiss+cache->metrics->wmiss)/(cache->metrics->stores+cache->metrics->loads));
}

void destruir_cache(Cache *cache){
    
    for(int i = 0; i < cache->cantidad_sets; i++){
        Set *set_actual = cache->sets[i];
        for(int j = 0; j < set_actual->cantidad_lineas; j++)
            free(set_actual->lineas[j]);
        free(set_actual->lineas);
        free(set_actual);
    }
    free(cache->sets);
    free(cache->metrics);
    free(cache);
}

int main(int argc, char const *argv[]){//argv[4] -> nro de Sets; argv[3] -> asociatividad de la cache; argv[2] -> tamanio cache
    if(argc != 5 && argc != 8)
        return ERROR;

    bool es_verboso = false;
    if(argc == 8)
        es_verboso = true;

    if(!son_potencia_2(argv))
        return ERROR;

    Cache *cache = crear_cache(argv);
    if(!cache)
        return ERROR;
    size_t numero_de_operacion = 0;
    FILE *archivo = fopen(argv[1], "r");
    if(!archivo){
        printf("ERROR ARCHIVO\n");
        return ERROR;
    }
    while(!feof(archivo)){

        Entrada *entrada = calloc(1, sizeof(Entrada));
        fscanf(archivo, "%i: %c %i %i %i", &(entrada->instruction_pointer), &(entrada->operacion), &(entrada->direccion), &(entrada->tamanio_operacion), &(entrada->datos_leidos)); // guardo datosa de la liena;

        entrada->numero_de_operacion = numero_de_operacion;
        if(entrada->instruction_pointer == 0 && entrada->operacion == 0 && entrada->direccion == 0 && entrada->tamanio_operacion == 0 && entrada->datos_leidos == 0)
            break;
    
        int set_index;
        int tag;
        int bytesxbloque = (atoi(argv[2]) / (atoi(argv[3]) * atoi(argv[4])));
        seti_y_tag(&(entrada->direccion), log2(atoi(argv[4])), log2(bytesxbloque) ,&set_index, &tag);
        
        if(es_verboso && entrada->numero_de_operacion >= atoi(argv[6]) && entrada->numero_de_operacion <= atoi(argv[7]))
            modo_verboso(cache, tag, set_index, entrada);
        modo_simple(cache, tag, set_index, entrada, bytesxbloque);
        
        free(entrada);
        numero_de_operacion++;
        
        if(feof(archivo)){
            break;
        }
    }

    fclose(archivo);
    devolver_resumen(cache, atoi(argv[3]), atoi(argv[2]));
    destruir_cache(cache);
    return 0;
}
