/* On inclut l'interface publique */
#include "mem.h"
/* ainsi que les détails d'implémentation locaux */
#include "common.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

/* Définition de l'alignement recherché
 * Avec gcc, on peut utiliser __BIGGEST_ALIGNMENT__
 * sinon, on utilise 16 qui conviendra aux plateformes qu'on cible
 */
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

struct zones_libres {
	// Taille, entête compris
	size_t size;
	struct zones_libres *next;
	/* ... */
};

struct zone_occupee{
	size_t size;
};

/* structure placée au début de la zone de l'allocateur

   Elle contient toutes les variables globales nécessaires au
   fonctionnement de l'allocateur

   Elle peut bien évidemment être complétée
   */
struct allocator_header {
	size_t memory_size;
	mem_fit_function_t *fit;
	struct zones_libres *liste_zone_libre;
	int taille_max_zone_libre;
};


/* La seule variable globale autorisée
 * On trouve à cette adresse le début de la zone à gérer
 * (et une structure 'struct allocator_header)
 */
static void *memory_addr;

static inline void *get_system_memory_addr() {
	return memory_addr;
}

static inline struct allocator_header *get_header() {
	struct allocator_header *h;
	h = get_system_memory_addr();
	return h;
}

static inline size_t get_system_memory_size() {
	return get_header()->memory_size;
}

//static inline size_t aligne_taille(size_t taille, int alignement){
//	return taille+alignement-(taille%alignement);
//}

static inline size_t aligne_taille(size_t taille, int alignement){
	return ((taille+(alignement-1))&~(alignement-1));
}

static inline void* aligne_adresse(void* adresse, int alignement){
	size_t decalage = alignement -1;
	uintptr_t adresse_alignee = ((uintptr_t)adresse + decalage) & ~decalage;
	return (void*)adresse_alignee;
}

void mem_init(void *mem, size_t taille) {
	memory_addr = mem;
	/* On vérifie qu'on a bien enregistré les infos et qu'on
	 * sera capable de les récupérer par la suite
	 */
	assert(mem == get_system_memory_addr());

	get_header()->memory_size = taille;
	assert(taille == get_system_memory_size());

	struct zones_libres* l;
	l = memory_addr + sizeof(struct allocator_header);
	l->size = taille - sizeof(struct allocator_header);
	l->next = NULL;

	get_header()->liste_zone_libre = l;
	get_header()->taille_max_zone_libre = l->size;

	/* On enregistre une fonction de recherche par défaut */
	mem_fit(&mem_fit_first);
}

void mem_show(void (*print)(void *, size_t, int)) {
	int taille_restante = get_header()->memory_size - 32;    // taille_restante = mem_size - sizeof(header)
	int taille_zone_actuelle = 0;
	char* zone_actuelle = memory_addr + sizeof(struct allocator_header);		// vérifier que le char* fonctionne
	int reste_zone_libre = 1;
	// vérifier si c'est NULL
	struct zones_libres* prochaine_zone_libre = get_header()->liste_zone_libre;
	while (taille_restante > 0) {
		taille_zone_actuelle =*( (size_t*) zone_actuelle);
		if((zone_actuelle) == (char*)(prochaine_zone_libre) && reste_zone_libre == 1){		
			print(zone_actuelle, taille_zone_actuelle, 1);
			prochaine_zone_libre = prochaine_zone_libre->next;
		}
		else{
			print(zone_actuelle, taille_zone_actuelle, 0);
		}	
		zone_actuelle = zone_actuelle + taille_zone_actuelle;
		taille_restante = taille_restante - taille_zone_actuelle;
		if(prochaine_zone_libre == NULL || prochaine_zone_libre->next == NULL){
			reste_zone_libre = 0;
		}
	}
}

void mem_fit(mem_fit_function_t *f) {
	get_header()->fit = f;
}

void *mem_alloc(size_t taille) {
	/* ... */
//	__attribute__((
//				unused)) // juste pour que gcc compile ce squelette avec -Werror 
//		struct zones_libres *zones_libres = get_header()->fit(...);
	/* ... */
//	return NULL;


	struct zones_libres *zones_libres = get_header()->fit(get_header()->liste_zone_libre, taille);
	if (zones_libres == NULL){
		return NULL;
	}
	struct zones_libres *zl = mem_fit_first(zones_libres, taille + sizeof(size_t));
	printf("adresse : %p\n",(void*)zl);
	return (void*)zl;

}


void mem_free(void *mem) {
	struct zones_libres* nouvelle_zone_libre = malloc(sizeof(struct zones_libres));
	//cas ou la zone mémoire est juste à côté du bloc de métadonnée donc au début de la mémoire
	if(mem == memory_addr + sizeof(struct allocator_header)){
		//modifier les liens de chainage
		nouvelle_zone_libre->next = get_header()->liste_zone_libre->next;
		get_header()->liste_zone_libre = nouvelle_zone_libre;
		// récupérer la taille t de la zone en question et la mettre à jour
		if(t > get_header()->taille_max_zone_libre){
			get_header()->taille_max_zone_libre = t;
		}
	}

	//cas ou la zone est entre 2 zones occupées


	//cas ou on est a cote d'une zone libre et du coup il faut fusionner les 2 zones libres en une.


	//cas ou la zone est à la fin de la mémoire ???
}

struct zones_libres *mem_fit_first(struct zones_libres *list, size_t size) {
	if(list == NULL){
		return NULL;
	}
	if(list->size >= size){
		struct zones_libres* zl = list;
		list = list->next;
		return zl;
	}
	struct zones_libres* parcours_zones_libres = list;

	while(parcours_zones_libres->next != NULL){
		if(parcours_zones_libres->next->size >= size){
			struct zones_libres* zl = parcours_zones_libres->next;
			if(parcours_zones_libres->next->next != NULL){
				parcours_zones_libres->next = parcours_zones_libres->next->next;
			}
			else{
				parcours_zones_libres->next = NULL;
				printf("adresse : %p\n", (void*)zl);
			return zl;
			}
		}
		parcours_zones_libres = parcours_zones_libres->next;
	}
	return NULL;
}

/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */
size_t mem_get_size(void *zone) {
	/* zone est une adresse qui a été retournée par mem_alloc() */

	/* la valeur retournée doit être la taille maximale que
	 * l'utilisateur peut utiliser dans cette zone */
	return 0;
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct zones_libres *mem_fit_best(struct zones_libres *list, size_t size) {
	return NULL;
}

struct zones_libres *mem_fit_worst(struct zones_libres *list, size_t size) {
	return NULL;
}
