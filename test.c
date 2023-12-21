#include <stdio.h>
#include "mem.h"
#include "common.h"
#include <assert.h>
#include <stdlib.h>

int main(){
	// tests de la fonction d'alignement sur 8 bits
	for(int taille=0; taille<9; taille++){
		assert(aligne_taille(taille, 8) == taille+8-taille%8);
	}

	void* p1;
        void* p2;
	void* p3;
	int taille;

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
	
	// test de l'allocation quand la mémoire est pleine
	p1 = mem_alloc(get_memory_size());
	for(int i=0; i<=10; i++){
		assert(mem_alloc(i) == NULL);
	}

	// test de la taille de la zone mémoire libérée
	memory_size = get_memory_size();
	mem_free(p1);
	assert(get_memory_size() == memory_size);
	
	// test de mem_free(NULL)
	memory_size = get_memory_size();
	mem_free(NULL);
	assert(memory_size == get_memory_size());

	// test du changement de la tête de la liste chaînée des zones libres
	struct zones_libres* zl = get_header()->liste_zone_libre;
	p1 = mem_alloc(10);
	p2 = mem_alloc(10);
	mem_free(p1);
	assert(zl == get_header()->liste_zone_libre);
	mem_free(p2);
	
	// test de libérer une zone occupée à côté d'une autre zone libre au niveau de la taille de la zone
	p1 = mem_alloc(10);
	p2 = mem_alloc(10);
	p3 = mem_alloc(10);
	mem_free(p1);
	zl = get_header()->liste_zone_libre;
	taille = zl->size;
	taille += *(&(zl->size)+zl->size);
	mem_free(p2);
	zl = get_header()->liste_zone_libre;
	assert(taille == zl->size);
	mem_free(p3);

	// même test que précédemment mais on vérifie le nombre de zones libres
	int nb_zones_libres = 1;
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

	// libérer une zone libre
	mem_free(get_header()->liste_zone_libre);
	
	return 0;
}
