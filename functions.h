static inline struct allocator_header* get_header();

static inline size_t get_system_memory_size();

static inline size_t aligne_taille(size_t taille, int alignement);

void fusionner_zl();

struct allocator_header {
	size_t memory_size;
	mem_fit_function_t *fit;
	struct zones_libres *liste_zone_libre;
	int taille_max_zone_libre;
};

struct zones_libres {
	size_t size;
	struct zones_libres *next;
};