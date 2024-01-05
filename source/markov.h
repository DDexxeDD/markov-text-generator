#ifndef _markov_h_
#define _markov_h_

#include <stddef.h>

#include "cvector.h"
#include "ok_lib.h"

// word_map_t is used to keep track of words following an ngram
// 	the key is the word
// 	the value is an integer
// 		which counts the number of times the word occurs after an ngram in the text
typedef struct ok_map_of (const char*, int) word_map_t;

// the key in an ngram_map_t is the ngram
// 	the value is a map
// 		which contains all of the words which follow the ngram in the input text
typedef struct ok_map_of (const char*, word_map_t) ngram_map_t;

//map to locations of the strings used as keys in other maps
//	ok_map does not allocate and copy strings used as keys
//		you have to store them somewhere on your own
typedef struct ok_map_of (const char*, char*) key_map_t;

typedef struct
{
	key_map_t key_map;
	key_map_t sentence_begin;
	key_map_t sentence_end;
	key_map_t quote_begin;
	key_map_t quote_end;
} ngram_key_map_t;

typedef struct
{
	// the the strings which will be used as keys in the hashmaps
	cvector (char*) ngram1_keys;
	cvector (char*) ngram2_keys;

	// maps to the keys stored in the ngram*_keys vectors
	// 	these are for easier lookup than iterating the _keys vectors
	ngram_key_map_t ngram1_key_map;
	ngram_key_map_t ngram2_key_map;

	ngram_map_t ngram1;  // 1 word ngrams (1gram)
	ngram_map_t ngram2;  // 2 word ngrams (2gram)
} markov_table_t;

void markov_initialize (markov_table_t* table);
void markov_free (markov_table_t* table);
void markov_build_table (markov_table_t* table, char* text);
void markov_generate_text (markov_table_t* markov_table, int length);

#endif
