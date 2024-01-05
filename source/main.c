#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cvector.h"
#define STRING_BUILDER_IMPLEMENTATION
#include "stringbuilder.h"
#include "ok_lib.h"
#include "pcg_variants.h"
#include "entropy.h"

#include "markov.h"

void load_file (markov_table_t* table, const char* filename)
{
	char buffer[128] = {0};
	FILE* file = fopen (filename, "r");
	string_builder_t file_text = sb_create ();

	if (!file)
	{
		printf ("load file failed: %s\n", filename);
		return;
	}

	while (fgets (buffer, sizeof (buffer), file) != NULL)
	{
		sb_append (&file_text, buffer);
	}

	markov_build_table (table, file_text.value);

	fclose (file);
	sb_destroy (&file_text);
}

int main (void)
{
	// seed random number generation
	uint64_t pcg_seed;
	entropy_getbytes (&pcg_seed, sizeof (pcg_seed));
	pcg32_srandom (pcg_seed, 54u);

	markov_table_t markov_table;
	markov_initialize (&markov_table);

	load_file (&markov_table, "books/defiant agents - andre norton.txt");

	markov_generate_text (&markov_table, 5);
	markov_free (&markov_table);

	return 0;
}
