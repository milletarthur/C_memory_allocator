#include "common.h"
#include "mem.h"
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define TAILLE_BUFFER 128

void* p1;
void* p2;
void* p3;
size_t taille;
size_t memory_size;


// tests de la fonction d'alignement sur 8 bits
void test0(){
	for(int taille=0; taille<9; taille++){
		assert(aligne_taille(taille, 8) == taille+8-taille%8);
	}
}

void test1(){
	// tests de la mise à jour de la taille de la mémoire restante
	void* memory_addr = get_memory_adr();
	size_t memory_size = get_memory_size();
	mem_init(memory_addr, memory_size);
	p1 = mem_alloc(12);
	memory_size -= 24;
	assert(get_memory_size() == memory_size);
	p2 = mem_alloc(30);
	memory_size -= 40;
	assert(get_memory_size() == memory_size);
	// ....
	mem_free(p1);
	mem_free(p2);
}

void test2(){
	// test de l'allocation quand la mémoire est pleine
	p1 = mem_alloc(get_memory_size());
	for(int i=0; i<=10; i++){
		assert(mem_alloc(i) == NULL);
	}
}

void test3(){
	// test de la taille de la zone mémoire libérée
	memory_size = get_memory_size();
	mem_free(p1);
	assert(get_memory_size() == memory_size);
}

void test4(){
	// test de mem_free(NULL)
	memory_size = get_memory_size();
	mem_free(NULL);
	assert(memory_size == get_memory_size());
}

void test5(){
	// test du changement de la tête de la liste chaînée des zones libres
	struct zones_libres* zl;
	zl = get_header()->liste_zone_libre;
	p1 = mem_alloc(10);
	p2 = mem_alloc(10);
	mem_free(p1);
	assert(zl == get_header()->liste_zone_libre);
	mem_free(p2);
}

void test6(){
	// test de libérer une zone occupée à côté d'une autre zone libre au niveau de la taille de la zone
	p1 = mem_alloc(10);
	p2 = mem_alloc(10);
	p3 = mem_alloc(10);
	mem_free(p1);
	struct zones_libres* zl;
	zl = get_header()->liste_zone_libre;
	taille = zl->size;
	taille += *(&(zl->size)+zl->size);
	mem_free(p2);
	zl = get_header()->liste_zone_libre;
	assert(taille == zl->size);
	mem_free(p3);
}

void test7(){
	// même test que précédemment mais on vérifie le nombre de zones libres
	int nb_zones_libres = 1;
	struct zones_libres* zl;
	zl = get_header()->liste_zone_libre;
	p1 = mem_alloc(10);
	p2 = mem_alloc(10);
	p3 = mem_alloc(10);
	int c = 0;
	// on vérifie qu'il y a bien 1 seule zone libre dans la mémoire
	while(zl != NULL){
		c++;
		zl = zl->next;
	}
	assert(nb_zones_libres == c);
	// on a libéré la première zone occupée, donc on a maintenant 2 zones libres
	mem_free(p1);
	nb_zones_libres = 2;
	zl = get_header()->liste_zone_libre;
	c=0;
	while(zl != NULL){
		c++;
		zl = zl->next;
	}
	assert(nb_zones_libres == c);
	// on a libéré la deuxième zone occupée, donc avec la fusion, on a toujours 2 zones libres
	mem_free(p2);
	zl = get_header()->liste_zone_libre;
	c=0;
	while(zl != NULL){
		c++;
		zl = zl->next;
	}
	assert(nb_zones_libres == c);
	// on a libéré la dernière zone occupée, donc comme il y avait une zone libre avant et après, on fusionne tout, on a alors juste 1 zone libre
	mem_free(p3);
	nb_zones_libres = 1;
	c=0;
	while(zl != NULL){
		c++;
		zl = zl->next;
	}
	assert(nb_zones_libres == c);
}

//void test8(){}

//void test9(){}


void aide() {
    fprintf(stderr, "Aide :\n");
    fprintf(stderr, "Saisir l'une des commandes suivantes\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "test i  :  effectuer le ième test\n");
    fprintf(stderr, "\n");
}

void afficher_zone(void *adresse, size_t taille, int free) {
    printf("Zone %s, Adresse : %lu, Taille : %lu\n", free ? "libre" : "occupee",
           adresse - get_memory_adr(), (unsigned long)taille);
}

void afficher_zone_libre(void *adresse, size_t taille, int free) {
    if (free)
        afficher_zone(adresse, taille, 1);
}

void afficher_zone_occupee(void *adresse, size_t taille, int free) {
    if (!free)
        afficher_zone(adresse, taille, 0);
}

int main() {
    char buffer[TAILLE_BUFFER];
    char commande;

    aide();
    mem_init(get_memory_adr(), get_memory_size());

    while (1) {
        printf("? ");
        fflush(stdout);
        commande = getchar();
        switch (commande) {
        case '0':
            test0();
	    break;
        case '1':
            test1();
            break;
		case '2':
            test2();
            break;
		case '3':
            test3();
            break;
		case '4':
            test4();
            break;
		case '5':
            test5();
            break;
		case '6':
            test6();
            break;
		case '7':
            test7();
            break;
        default:
            fprintf(stderr, "Commande inconnue !\n");
        }
        /* vide ce qu'il reste de la ligne dans le buffer d'entree */
        fgets(buffer, TAILLE_BUFFER, stdin);
    }
    return 0;
}
