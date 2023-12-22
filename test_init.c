#include "common.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define TAILLE_BUFFER 128

void aide() {
    fprintf(stderr, "Aide :\n");
    fprintf(stderr, "Saisir l'une des commandes suivantes\n");
	fprintf(stderr, "0...9     :   effectuer le test\n");
	fprintf(stderr, "h         :   afficher cette aide\n");
    fprintf(stderr, "q         :   quitter ce programme\n");
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
        case '8':
            test8();
            break;  
        case '9':
            test9();
            break;  
		case 'h':
            aide();
            break;
        case 'q':
            exit(0);
        default:
            fprintf(stderr, "Commande inconnue !\n");
        }
        /* vide ce qu'il reste de la ligne dans le buffer d'entree */
        fgets(buffer, TAILLE_BUFFER, stdin);
    }
    return 0;
}
