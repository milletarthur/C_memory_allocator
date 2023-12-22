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
#define ALIGNMENT 8
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

struct zone{
	struct zones_libres* zl;
	struct zone_occupee* zo;
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

// Renvoie 0 si la zone mémoire est occupée et 1 si elle est libre
static inline int type_zone(void* zone){
	struct zones_libres* zl = get_header()->liste_zone_libre;
	while((void*)zl != zone && zl->next != NULL){
		zl = zl->next;
	}
	if((void*)zl == zone){
		return 1;
	}
	return 0;
}

// Renvoie la zone libre ou la zone occupée correspondant
struct zone type_de_zone(void* zone){
	struct zones_libres* zl = get_header()->liste_zone_libre;
	struct zone z;
	z.zo = NULL;
	z.zl = NULL;

	if((void*)zl > zone){
		z.zo = memory_addr + sizeof(struct allocator_header);
		return z;	
	}

	while(zl != NULL){
		//if((void*)zl == zone){
		if(zl == (struct zones_libres*)zone){
			z.zl = zl;
			return z;
		}
		zl = zl->next;
	}

	struct zone_occupee* zo = memory_addr + sizeof(struct allocator_header);
	while((void*)zo != NULL){
		if((void*)zo == zone){
			z.zo = zo;
			return z;
		}
		zo = zo+zo->size;
	}
	return z;	
}

// Renvoie la zone mémoire libre précédente
struct zones_libres* zone_precedente(struct zones_libres* zl){
	struct zones_libres* libre = get_header()->liste_zone_libre;
	if(libre == NULL){
		return NULL;
	}
	if(libre == zl){ return libre;}

	while(libre->next != NULL && libre->next != zl){
		libre = libre->next;
	}
	if(libre->next == zl){
		return libre;
	}
	printf("libre : %p\nlibre->next: %p\n",libre, libre->next);
	return NULL;
}

// Renvoie la zone mémoire précédente
void* zone_prec(void* zone){
	struct zone_occupee* zo = memory_addr + sizeof(struct allocator_header);
	if((void*)zo == zone){		// la zone mémoire précédente est le bloc de métadonnées
		return NULL;
	}
	while((void*)zo+zo->size != zone && (void*)zo+zo->size != NULL){
		zo = zo+zo->size;
	}
	if((void*)zo+zo->size == zone){
		return (void*)zo;
	}
	return NULL;
}
	/*
	// Renvoie l'adresse du début de la zone mémoire suivante
	void* zone_suivante(void* zone){
	struct zone_occupee* zo = zone;
	void* zone_suivante = (void*)zo + zo->size;
	return zone_suivante;
	}

	void mem_init(void *mem, size_t taille) {
	memory_addr = mem;
	// On vérifie qu'on a bien enregistré les infos et qu'on
	// sera capable de les récupérer par la suite
	//
	assert(mem == get_system_memory_addr());

	get_header()->memory_size = taille;
	assert(taille == get_system_memory_size());

	struct zones_libres* l;
	l = memory_addr + sizeof(struct allocator_header);
	l->size = taille - sizeof(struct allocator_header);
	l->next = NULL;

	get_header()->liste_zone_libre = l;
	get_header()->taille_max_zone_libre = l->size;

	//On enregistre une fonction de recherche par défaut 
	mem_fit(&mem_fit_first);
	}
	*/
	// Renvoie l'adresse du début de la zone mémoire suivante
void* zone_suivante(void* zone){
	struct zone_occupee* zo = zone;
	void* zone_suivante = (void*)zo + zo->size;
	return zone_suivante;
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
			print(zone_actuelle + sizeof(size_t), taille_zone_actuelle - sizeof(size_t), 0);
		}	
		zone_actuelle = zone_actuelle + taille_zone_actuelle;
		taille_restante = taille_restante - taille_zone_actuelle;
		if(prochaine_zone_libre == NULL /*|| prochaine_zone_libre->next == NULL*/){
			reste_zone_libre = 0;
		}
	}
}

void mem_fit(mem_fit_function_t *f) {
	get_header()->fit = f;
}

void *mem_alloc(size_t taille) {

	size_t taille_pour_fct = taille + sizeof(size_t);	 // allignement
	taille_pour_fct = aligne_taille(taille_pour_fct, ALIGNMENT);
	struct zones_libres* case_a_remplir = get_header()->fit(get_header()->liste_zone_libre, taille_pour_fct);
	struct zones_libres* suiv_case_a_remplir = case_a_remplir->next;
	if(case_a_remplir == NULL){ return NULL;}

	struct zones_libres* pred_case_a_remplir = zone_precedente(case_a_remplir);
	if (pred_case_a_remplir == NULL){ printf("oups\n");}
	//struct zones_libres* pred_case_a_remplir = get_header()->liste_zone_libre;
	if(taille_pour_fct + sizeof(struct zones_libres) <= case_a_remplir->size){
		
		if (pred_case_a_remplir == NULL){ printf("oups\n");}
		char* debut_zl_a_initialiser = (char*)case_a_remplir + taille_pour_fct;
		pred_case_a_remplir->next = (struct zones_libres*)debut_zl_a_initialiser;
		pred_case_a_remplir->next->size = case_a_remplir->size - taille_pour_fct;
		if (pred_case_a_remplir == case_a_remplir){
			get_header()->liste_zone_libre = (struct zones_libres*)debut_zl_a_initialiser;
			//get_header()->liste_zone_libre->next = case_a_remplir->next->next;
			//pred_case_a_remplir->next = case_a_remplir->next;
		} /*else {
			pred_case_a_remplir->next->next = case_a_remplir->next; //a voir si c'est ca 
		}*/
		if(case_a_remplir->next != NULL){
			((struct zones_libres*)debut_zl_a_initialiser)->next = suiv_case_a_remplir;
		}
	} else {
		taille_pour_fct += case_a_remplir->size - taille_pour_fct;
		if(pred_case_a_remplir == get_header()->liste_zone_libre){
			get_header()->liste_zone_libre = get_header()->liste_zone_libre->next;
		} else {
			pred_case_a_remplir->next = case_a_remplir->next;
		}
		
	}

	case_a_remplir->size = taille_pour_fct;

	void* rv = (char*)case_a_remplir + sizeof(size_t);
	return rv;
}

	/*
	   void mem_free(void *mem) {

	   struct zone z = type_de_zone(mem);
	   struct zone_occupee* zo = z.zo;
	   struct zones_libres* liste_zl = get_header()->liste_zone_libre;
	   struct zones_libres* tete = liste_zl;					// pas utile ??
	   struct zone var_zone_suivante = type_de_zone(zone_suivante(mem));
	   struct zone var_zone_precedente = type_de_zone(zone_prec(mem));
	   struct zones_libres* nouvelle_zl = (struct zones_libres*)&zo;
	   nouvelle_zl->size = z.zo->size;

	   printf("size : %ld\n", nouvelle_zl->size);	

	//cas ou la zone mémoire est juste à côté du bloc de métadonnée donc au début de la mémoire

	printf("&zo = %p\n",zo);
	printf("memory_addr + sizeof(header) = %p\n", memory_addr + sizeof(struct allocator_header));

	if(memory_addr + sizeof(struct allocator_header) == zo){
	get_header()->liste_zone_libre = nouvelle_zl;
	get_header()->liste_zone_libre->next = liste_zl;
	get_header()->memory_size += nouvelle_zl->size;
	return;
	}

	//cas ou la zone est entre 2 zones occupées

	if(var_zone_suivante.zo != NULL && var_zone_precedente.zo != NULL){
	while(liste_zl->next != NULL){
	if((void*)liste_zl < mem && mem < (void*)liste_zl->next){
	nouvelle_zl->next = liste_zl->next;
	liste_zl->next = nouvelle_zl;
	get_header()->liste_zone_libre = tete; 		// ???
	return;				
	}
	liste_zl = liste_zl->next;
	}
	}

	//cas ou on est a cote d'une zone libre et du coup il faut fusionner les 2 zones libres en une.
	//cas où la zone précédente est libre et la suivante est occupée	
	if(var_zone_precedente.zl != NULL && var_zone_suivante.zo != NULL){
	//nouvelle_zl->next = var_zone_precedente.zl->next;
	//var_zone_precedente.zl->next = nouvelle_zl;
	//var_zone_precedente.zl->size += nouvelle_zl->size;

	while(liste_zl != var_zone_precedente.zl){
	liste_zl = liste_zl->next;
	}
	nouvelle_zl->next = liste_zl->next;
	liste_zl->next = nouvelle_zl;
	liste_zl->size += nouvelle_zl->size;

	get_header()->liste_zone_libre = tete;				// ???
	return;
	}

	//cas où la zone précédente est occupée et la suivante est libre
	if(var_zone_precedente.zo != NULL && var_zone_suivante.zl != NULL){

	//struct zones_libres* zl_prec = zone_precedente(var_zone_suivante.zl);
	//nouvelle_zl->next = zl_prec->next;
	//nouvelle_zl->size += var_zone_suivante.zl->size;
	//zl_prec->next = nouvelle_zl;

	while(liste_zl->next != var_zone_suivante.zl){
	liste_zl = liste_zl->next;
	}
	nouvelle_zl->next = liste_zl->next;
	liste_zl->next = nouvelle_zl;
	liste_zl->size += var_zone_suivante.zl->size;

	get_header()->liste_zone_libre = tete;				// ???
	return;
}

//cas où la zone précédente et la zone suivante sont libres --> fusionner les 3 zones en une
if(var_zone_precedente.zl != NULL && var_zone_suivante.zl != NULL){
	var_zone_precedente.zl->size = var_zone_precedente.zl->size + nouvelle_zl->size + var_zone_suivante.zl->size;
	nouvelle_zl->next = var_zone_precedente.zl->next;
	var_zone_precedente.zl->next = nouvelle_zl;

	//get_header()->liste_zone_libre = ????
	return;
}

}

*/

void fusionner_zl(){
	struct zones_libres* a_fusionner = get_header()->liste_zone_libre;
	void* next_zone = (void*) a_fusionner;
	while ((a_fusionner != NULL) &&  (a_fusionner->next != NULL)){
		next_zone = (void *)((char*) next_zone + a_fusionner->size);
		while((a_fusionner->next != NULL) && ((void*) a_fusionner->next) == (next_zone)){
			next_zone = (void *)((char*) next_zone + a_fusionner->next->size);
			a_fusionner->size += a_fusionner->next->size;
			a_fusionner->next = a_fusionner->next->next;
		}
		a_fusionner = a_fusionner->next;
	}
}

struct zones_libres* retrouve_prec (void* mem){
	struct zones_libres* rv = get_header()->liste_zone_libre;
	if(rv == NULL){ return NULL;}
	void* addr_fin_zone = (char*)rv + rv->size;
	while((rv != NULL) && (addr_fin_zone <= mem)){
		if(addr_fin_zone == mem){ return rv;}
		addr_fin_zone = (char*)rv - rv->size;
		rv = rv->next;
	}
	return zone_precedente(rv);
}


void mem_free(void *mem) {
	if(mem == NULL){return;}
	if(get_header()->liste_zone_libre == NULL){
		get_header()->liste_zone_libre = (struct zones_libres*)((char*)mem - sizeof(size_t));
		get_header()->liste_zone_libre->size = ((struct zone_occupee*)((char*)mem - sizeof(size_t)))->size;
		get_header()->liste_zone_libre->next = NULL;
	}
	struct zones_libres* zone_av = retrouve_prec(mem - sizeof(size_t));
	if((void*)zone_av > mem){
		get_header()->liste_zone_libre = (struct zones_libres*)((char*)mem - sizeof(size_t)) ;
		//get_header()->liste_zone_libre->size = *((int*)mem);
		get_header()->liste_zone_libre->next = zone_av;
		fusionner_zl();
		return;
	}
	struct zone_occupee* taille_mem = (struct zone_occupee*)((char*)mem - sizeof(size_t));
	struct zones_libres* zone_ap = zone_av->next;
	zone_av->next = (struct zones_libres*)((char*)mem - sizeof(size_t));
	zone_av->next->size = taille_mem->size;//*((int*)mem) + sizeof (size_t);
	zone_av->next->next = zone_ap;
	fusionner_zl();
}

/*void mem_free(void *mem){

  size_t taille = *((char*)mem - sizeof(size_t));
  get_header()->memory_size += taille;
  struct zones_libres* liste_zl = get_header()->liste_zone_libre;

  struct zone_occupee* zo = memory_addr + sizeof(struct allocator_header) + sizeof(size_t);
  zo->size = *((char*)zo - sizeof(size_t));

// cas où la liste est NULL
if(liste_zl == NULL){
get_header()->liste_zone_libre = (struct zones_libres*)mem;
get_header()->liste_zone_libre->size = taille;
get_header()->liste_zone_libre->next = NULL;
return;
}

// cas où la zone mémoire est à coté du header
if(zo == mem){
struct zones_libres* l_zl = get_header()->liste_zone_libre;
get_header()->liste_zone_libre = (struct zones_libres*)mem;
get_header()->liste_zone_libre->size = taille;
get_header()->liste_zone_libre->next = l_zl;
return;
}

// parcours des zones mémoires grâce à leur taille jusqu'à la zone précédente
while(zo + zo->size != mem){
zo += zo->size;
zo->size = *((char*)zo - sizeof(size_t));
}

// on cherche la zone libre précédente
while(liste_zl != NULL){
if(liste_zl == (struct zones_libres*)zo){
struct zones_libres* zl_suiv = liste_zl->next;
liste_zl->next = (struct zones_libres*)mem;
liste_zl->next->next = zl_suiv;

}
liste_zl = liste_zl->next;
}

//fusionner_zl();
return;
}
}*/

struct zones_libres *mem_fit_first(struct zones_libres *list, size_t size) {
	if(list == NULL){
		return NULL;
	}
	if(list->size >= size){
		struct zones_libres* zl = list;
		//list = list->next;
		return zl;
	}
	struct zones_libres* parcours_zones_libres = list;

	while(parcours_zones_libres->next != NULL){
		if(parcours_zones_libres->next->size >= size){
			return parcours_zones_libres->next;
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


/*---------------------Partie TEST----------------------*/

void* p1;
void* p2;
void* p3;
size_t taille;

// tests de la fonction d'alignement sur ALIGNMENT bits
void test0(){
	printf("Test de la fonction d'alignement\n\n");
	for(int taille=1; taille<100; taille+=5){
		assert(aligne_taille(taille, ALIGNMENT) == taille + (ALIGNMENT - (taille % ALIGNMENT)) % ALIGNMENT);
		printf("taille : %d\ntaille alignée sur %d : %ld\n",ALIGNMENT,taille,aligne_taille(taille, ALIGNMENT));
	}
	printf("\nTest OK\n");
}

void test1(){
	// tests de la mise à jour de la taille de la mémoire restante
	printf("Test mise à jour de la taille de la mémoire restante après allocation\n\n");
	get_header()->memory_size -= sizeof(struct allocator_header);
	//void* memory_addr = get_memory_adr();
	size_t memory_size = get_header()->memory_size;
	size_t prec_mem_size = get_header()->memory_size;
	//mem_init(memory_addr, memory_size);
	printf("taille restante de la mémoire : %ld\n\n", get_header()->liste_zone_libre->size);
	p1 = mem_alloc(12);
	printf("Mémoire allouée en %p de taille 12\n",p1);
	memory_size -= aligne_taille(12+sizeof(size_t), ALIGNMENT);
	assert(get_header()->liste_zone_libre->size == memory_size);
	printf("taille restante de la mémoire : %ld = %ld - %ld\n\n", get_header()->liste_zone_libre->size, prec_mem_size, aligne_taille(12+sizeof(size_t), ALIGNMENT));
	p2 = mem_alloc(30);
	prec_mem_size = memory_size;
	printf("Mémoire allouée en %p\n",p2);
	memory_size -= aligne_taille(30+sizeof(size_t), ALIGNMENT);
	assert(get_header()->liste_zone_libre->size == memory_size);
	printf("taille restante de la mémoire : %ld = %ld - %ld\n", get_header()->liste_zone_libre->size, prec_mem_size, aligne_taille(30+sizeof(size_t), ALIGNMENT));
	// ....
	mem_free(p1);
	mem_free(p2);
	printf("\nTest OK\n");
}

void test2(){
	// test de l'allocation quand la mémoire est pleine
	printf("Test lorsqu'on essaie d'allouer une case mémoire lorsque la mémoire est pleine.\n\n");
	p1 = mem_alloc(get_memory_size()-sizeof(struct allocator_header)-sizeof(size_t));
	for(int i=10; i<=100; i+=10){
		p2 = mem_alloc(i);
		printf("Tentative d'allocation d'une case mémoire de taille %d.\n",i);
		assert(p2 == NULL);
		printf("Echec de l'allocation.\n");
	}
	mem_free(p1);
	printf("\nTest OK\n");
}

void test3(){
	// test de la taille de la zone mémoire libérée
	printf("Test de vérification de la taille de la zone libre après la libération de la seule zone occupée.\n");
	p1 = mem_alloc(get_memory_size());
	size_t memory_size = get_memory_size();
	mem_free(p1);
	assert(get_memory_size() == memory_size);
	printf("\nTest OK\n");
}

void test4(){
	// test de mem_free(NULL)
	printf("Test de mem_free(NULL).\n");
	size_t memory_size = get_header()->liste_zone_libre->size;
	mem_free(NULL);
	assert(memory_size == get_header()->liste_zone_libre->size);
	printf("\nTest OK\n");
}

void test5(){
	// test du changement de la tête de la liste chaînée des zones libres
	printf("Test changement de la tête de la liste chaînée des zones libres.\n\n");
	struct zones_libres* zl;
	zl = get_header()->liste_zone_libre;
	printf("adresse de la tête : %p\n",zl);
	p1 = mem_alloc(10);
	p2 = mem_alloc(10);
	mem_free(p1);
	printf("adresse de la nouvelle tête : %p\n", get_header()->liste_zone_libre);
	assert(zl == get_header()->liste_zone_libre);
	mem_free(p2);
	printf("\nTest OK\n");
}

void test6(){
	// test de libérer une zone occupée à côté d'une autre zone libre au niveau de la taille de la zone
	printf("Test de la mise à jour de la taille d'une zone libre lorsqu'on libère une zone occupée à côté.\n\n");
	struct zones_libres* zl;
	zl = get_header()->liste_zone_libre;
	p1 = mem_alloc(10);
	printf("Mémoire allouée en %p\n",p1);
	p2 = mem_alloc(100);
	printf("Mémoire allouée en %p\n",p2);
	p3 = mem_alloc(50);
	printf("Mémoire allouée en %p\n\n",p3);
	printf("taille de la première zone occupée: %ld\n",aligne_taille(10,ALIGNMENT));
	printf("taille de la seconde zone occupée: %ld\n\n", aligne_taille(100,ALIGNMENT));
	printf("On libère en %p\n",p1);
	mem_free(p1);
	taille = aligne_taille(10,ALIGNMENT)+aligne_taille(100,ALIGNMENT)+2*sizeof(size_t);
	printf("On libère en %p\n\n",p2);
	mem_free(p2);
	zl = get_header()->liste_zone_libre;
	printf("taille de la première zone : %ld\n",zl->size);
	printf("taille de la seconde zone : %ld\n\n", *(&(zl->size)+zl->size));
	printf("taille de la nouvelle zone fusionnée : %ld = %ld + %ld + 2*sizeof(size_t)\n", taille, aligne_taille(10,ALIGNMENT), aligne_taille(100,ALIGNMENT));
	assert(taille == zl->size);
	mem_free(p3);
	printf("\nTest OK\n");
}

void test7(){
	// même test que précédemment mais on vérifie le nombre de zones libres
	printf("Test de la fusion de zones libres après 3 allocations successives.\n\n");
	int nb_zones_libres = 1;
	struct zones_libres* zl;
	p1 = mem_alloc(10);
	p2 = mem_alloc(10);
	p3 = mem_alloc(10);
	int c = 0;
	zl = get_header()->liste_zone_libre;
	// on vérifie qu'il y a bien 1 seule zone libre dans la mémoire
	while(zl != NULL){
		c++;
		zl = zl->next;
	}
	printf("Pour le moment on a alloué 3 zones occupées à la suite, on a donc juste %d zone libre\n",c);
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
	printf("On a libéré la première zone occupée, on a maintenant %d zones libres\n", c);
	assert(nb_zones_libres == c);
	// on a libéré la deuxième zone occupée, donc avec la fusion, on a toujours 2 zones libres
	mem_free(p2);
	zl = get_header()->liste_zone_libre;
	c=0;
	while(zl != NULL){
		c++;
		zl = zl->next;
	}
	printf("On a libéré la seconde zone occupée qui a fusionnée avec la première, donc a %d zones libres\n",c);
	assert(nb_zones_libres == c);
	// on a libéré la dernière zone occupée, donc comme il y avait une zone libre avant et après, on fusionne tout, on a alors juste 1 zone libre
	mem_free(p3);
	nb_zones_libres = 1;
	c=0;
	while(zl != NULL){
		c++;
		zl = zl->next;
	}
	printf("On a libéré la dernière zone occupée, donc on a tout fusionée, on a plus que %d zone libre\n", c);
	assert(nb_zones_libres == c);
	printf("\nTest OK\n");
}
