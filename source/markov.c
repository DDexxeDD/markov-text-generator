#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "cvector.h"
#include "stringbuilder.h"
#include "pcg_variants.h"
#include "entropy.h"

#include "markov.h"

void ngram_key_map_init (ngram_key_map_t* map)
{
	ok_map_init (&map->key_map);
	ok_map_init (&map->sentence_begin);
	ok_map_init (&map->sentence_end);
	ok_map_init (&map->quote_begin);
	ok_map_init (&map->quote_end);
}

void ngram_key_map_deinit (ngram_key_map_t* map)
{
	ok_map_deinit (&map->key_map);
	ok_map_deinit (&map->sentence_begin);
	ok_map_deinit (&map->sentence_end);
	ok_map_deinit (&map->quote_begin);
	ok_map_deinit (&map->quote_end);
}

void markov_initialize (markov_table_t* table)
{
	table->ngram2_keys = NULL;
	cvector_init (table->ngram2_keys, 10, NULL);
	table->ngram1_keys = NULL;
	cvector_init (table->ngram1_keys, 10, NULL);

	ok_map_init (&table->ngram1);
	ok_map_init (&table->ngram2);

	ngram_key_map_init (&table->ngram1_key_map);
	ngram_key_map_init (&table->ngram2_key_map);
}

void free_keys (cvector (char*)* keys)
{
	for (int iter = 0; iter < cvector_size (*keys); iter++)
	{
		free ((*keys)[iter]);
	}
	cvector_free (*keys);
}

void markov_free (markov_table_t* table)
{
	free_keys (&table->ngram1_keys);
	free_keys (&table->ngram2_keys);

	ok_map_foreach (&table->ngram1, const char* key, word_map_t map)
	{
		// keep the compiler quiet
		(void)key;
		ok_map_deinit (&map);
	}

	ok_map_foreach (&table->ngram2, const char* key, word_map_t map)
	{
		// keep the compiler quiet
		(void)key;
		ok_map_deinit (&map);
	}

	ok_map_deinit (&table->ngram1);
	ok_map_deinit (&table->ngram2);

	ngram_key_map_deinit (&table->ngram1_key_map);
	ngram_key_map_deinit (&table->ngram2_key_map);
}

/*
 * only basic ascii text is supported
 * words are composed of
 * 	letters:
 * 		a-z A-Z
 * 	and punctuation:
 * 		"',;:-.?!_
 */
char* next_token (string_builder_t* string, char* text)
{
	char* start = text;
	char* end = text;

	while (*start != '\"'
		&& *start != '\''
		&& *start != ','
		&& *start != ';'
		&& *start != ':'
		&& *start != '-'
		&& *start != '.'
		&& *start != '?'
		&& *start != '!'
		&& *start != '_'
		&& !(*start >= 65 && *start <= 90)
		&& !(*start >= 97 && *start <= 122)
		&& *start != '\0')
	{
		start++;
	}

	end = start;

	while (*end == '\"'
		|| *end == '\''
		|| *end == ','
		|| *end == ';'
		|| *end == ':'
		|| *end == '-'
		|| *end == '.'
		|| *end == '?'
		|| *end == '!'
		|| *end == '_'
		|| (*end >= 65 && *end <= 90)
		|| (*end >= 97 && *end <= 122))
	{
		end++;
	}

	sb_append_ex (string, start, end - start);

	return end;
}

// convenience to save a couple keystrokes :D
void map_put_key (key_map_t* map, char** key)
{
	ok_map_put (map, *key, *key);
}

/*
 * function: markov_build_table
 *
 * parameters:
 * 	table - the markov table we want to populate
 * 	text - the tex we are using to populate the markov table
 */
void markov_build_table (markov_table_t* table, char* text)
{
	char* cursor = text;
	string_builder_t word1 = sb_create ();
	string_builder_t word2 = sb_create ();
	string_builder_t word3 = sb_create ();
	// previous_word is used to tell if an ngram starts a sentence
	string_builder_t previous_word = sb_create ();
	// use string to build ngram2 of word1 + word2
	string_builder_t string = sb_create ();

	// this loop populates all of the key maps
	while (*cursor != '\0')
	{
		cursor = next_token (&word1, cursor);

		if (!ok_map_contains (&table->ngram1_key_map.key_map, word1.value))
		{
			cvector_push_back (table->ngram1_keys, NULL);
			char** key = cvector_end (table->ngram1_keys) - 1;
			*key = malloc (word1.length + 1);
			strcpy (*key, word1.value);

			map_put_key (&table->ngram1_key_map.key_map, key);

			if (word1.value[0] == '\"')
			{
				map_put_key (&table->ngram1_key_map.quote_begin, key);
			}
			if (word1.value[word1.length - 1] == '\"')
			{
				map_put_key (&table->ngram1_key_map.quote_end, key);

				if (word1.length > 2)
				{
					if (word1.value[word1.length - 2] == '!'
						|| word1.value[word1.length - 2] == '?'
						|| word1.value[word1.length - 2] == '.')
					{
						map_put_key (&table->ngram1_key_map.sentence_end, key);
					}
				}
			}
			if (word1.length > 1)
			{
				if (word1.value[word1.length - 1] == '!'
					|| word1.value[word1.length - 1] == '?'
					|| word1.value[word1.length - 1] == '.')
				{
					map_put_key (&table->ngram1_key_map.sentence_end, key);
				}
			}

			if (previous_word.length > 0)
			{
				if (ok_map_contains (&table->ngram1_key_map.sentence_end, previous_word.value))
				{
					map_put_key (&table->ngram1_key_map.sentence_begin, key);
				}  	
			}
			// this should only apply to the first word in the entire text
			else
			{
				map_put_key (&table->ngram1_key_map.sentence_begin, key);
			}
		}

		// dont increment cursor
		// 	we will use the word2 position to populate word1 next loop
		next_token (&word2, cursor);

		// if we reach the end of the input text
		// 	word2 should have length 0
		if (word2.length == 0)
		{
			break;
		}

		sb_append (&string, word1.value);
		sb_append_char (&string, ' ');
		sb_append (&string, word2.value);

		if (!ok_map_contains (&table->ngram2_key_map.key_map, string.value))
		{
			cvector_push_back (table->ngram2_keys, NULL);
			char** key = cvector_end (table->ngram2_keys) - 1;
			*key = malloc (string.length + 1);
			strcpy (*key, string.value);

			ok_map_put (&table->ngram2_key_map.key_map, *key, *key);
		}

		sb_clear (&previous_word);
		sb_append (&previous_word, word1.value);
		sb_clear (&word1);
		sb_clear (&word2);
		sb_clear (&string);
	}

	sb_clear (&word1);
	sb_clear (&word2);
	sb_clear (&string);

	/*
	 * now that ngram1 and ngram2 keys have been populated
	 * 	we can build the actual markov table
	 */
	cursor = text;

	while (*cursor != '\0')
	{
		cursor = next_token (&word1, cursor);
		// dont increment cursor again
		// 	we will use its current location to populate word1 next loop
		//
		// populate word2 and then word3 without moving cursor forward
		next_token (&word3, next_token (&word2, cursor));

		// this should happen at the end of the file
		if (word2.length == 0)
		{
			break;
		}

		if (!ok_map_contains (&table->ngram1, word1.value))
		{
			word_map_t* map = ok_map_put_and_get_ptr (&table->ngram1, ok_map_get (&table->ngram1_key_map.key_map, word1.value));
			ok_map_init (map);
		}

		word_map_t* map = ok_map_get_ptr (&table->ngram1, word1.value);

		if (!ok_map_contains (map, word2.value))
		{
			ok_map_put (map, ok_map_get (&table->ngram1_key_map.key_map, word2.value), 1);
		}
		else
		{
			int count = ok_map_get (map, word2.value);
			ok_map_put (map, word2.value, count + 1);
		}

		sb_append (&string, word1.value);
		sb_append_char (&string, ' ');
		sb_append (&string, word2.value);

		if (!ok_map_contains (&table->ngram2, string.value))
		{
			word_map_t* map = ok_map_put_and_get_ptr (&table->ngram2, ok_map_get (&table->ngram2_key_map.key_map, string.value));
			ok_map_init (map);
		}

		if (word3.length > 0)
		{
			map = ok_map_get_ptr (&table->ngram2, string.value);

			if (!ok_map_contains (map, word3.value))
			{
				ok_map_put (map, ok_map_get (&table->ngram1_key_map.key_map, word3.value), 1);
			}
			else
			{
				int count = ok_map_get (map, word3.value);
				ok_map_put (map, word3.value, count + 1);
			}
		}

		sb_clear (&word1);
		sb_clear (&word2);
		sb_clear (&word3);
		sb_clear (&string);
	}

	sb_destroy (&word1);
	sb_destroy (&word2);
	sb_destroy (&word3);
	sb_destroy (&previous_word);
	sb_destroy (&string);
}

/*
 * choose a word from a map based on the probability of its occurence
 *
 * returns NULL if there is some kind of failure
 * or if the input map is empty
 */
const char* choose_ngram (word_map_t* words)
{
	if (ok_map_count (words) <= 0)
	{
		return NULL;
	}

	int total_count = 0;
	ok_map_foreach (words, const char* key, int count)
	{
		(void) key;
		total_count += count;
	}

	const char* word = NULL;
	int random_index = pcg32_boundedrand (total_count);

	ok_map_foreach (words, const char* key, int count)
	{
		random_index -= count;

		if (random_index <= 0)
		{
			word = key;
			break;
		}
	}

	return word;
}

/*
 * function: markov_generate_text
 *
 * parameters:
 * 	markov_table - the markov table to use with generating lines
 * 	lines - the number of lines to generate
 *
 * the generate lines are generated as continuous text
 * 	the end of one line leads to the beginning of the next
 * lines are printed on their own lines for readability
 * 	but could be printed continuously
 */
void markov_generate_text (markov_table_t* markov_table, int lines)
{
	// how many lines we have generated so far
	int line_count = 0;

	// all of the generated lines will be stored continuously in string
	// 	string will be printed out at the end
	string_builder_t string = sb_create ();
	// ngram2 is the concatenation of word1 + ' ' + word2
	string_builder_t ngram2 = sb_create ();
	string_builder_t word1 = sb_create ();
	string_builder_t word2 = sb_create ();

	// start everything by picking a word that begins a sentence
	uint32_t random_index = pcg32_boundedrand (ok_map_count (&markov_table->ngram1_key_map.sentence_begin));
	uint32_t match = 0;
	ok_map_foreach (&markov_table->ngram1_key_map.sentence_begin, const char* key, char* word)
	{
		(void)key;
		if (match == random_index)
		{
			sb_append (&word1, word);
		}

		match++;
	}

	// in case we have a 1 word sentence
	if (ok_map_contains (&markov_table->ngram1_key_map.sentence_end, word1.value))
	{
		line_count++;
	}

	sb_append (&string, word1.value);

	word_map_t* map = ok_map_get_ptr (&markov_table->ngram1, word1.value);

	sb_append (&word2, choose_ngram (map));

	// to make generated sentences easier to read
	// 	put them on their own line
	bool newline = false;
	while (line_count < lines)
	{
		sb_clear (&ngram2);
		sb_append (&ngram2, word1.value);
		sb_append_char (&ngram2, ' ');
		sb_append (&ngram2, word2.value);

		if (newline)
		{
			sb_append_char (&string, '\n');
			newline = false;
		}
		else
		{
			sb_append_char (&string, ' ');
		}
		sb_append (&string, word2.value);

		if (ok_map_contains (&markov_table->ngram1_key_map.sentence_end, word2.value))
		{
			line_count++;
			newline = true;
		}

		sb_clear (&word1);
		sb_append (&word1, word2.value);
		sb_clear (&word2);

		map = ok_map_get_ptr (&markov_table->ngram2, ngram2.value);

		sb_append (&word2, choose_ngram (map));
	}

	printf ("\n");
	printf ("generated:\n\n");
	printf ("%s\n", string.value);
	printf ("\n");

	sb_destroy (&string);
	sb_destroy (&ngram2);
	sb_destroy (&word1);
	sb_destroy (&word2);
}
