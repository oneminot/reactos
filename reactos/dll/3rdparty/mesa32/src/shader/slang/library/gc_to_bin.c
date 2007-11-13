#include "../../grammar/grammar_crt.h"
#include "../../grammar/grammar_crt.c"
#include <stdlib.h>
#include <stdio.h>

static const char *slang_shader_syn =
#include "slang_shader_syn.h"
;

static int gc_to_bin (grammar id, const char *in, const char *out)
{
	FILE *f;
	byte *source, *prod;
	unsigned int size, i, line = 0;

	printf ("Precompiling %s\n", in);

	f = fopen (in, "r");
	if (f == NULL)
		return 1;
	fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	source = (byte *) grammar_alloc_malloc (size + 1);
	source[fread (source, 1, size, f)] = '\0';
	fclose (f);

	if (!grammar_fast_check (id, source, &prod, &size, 65536))
	{
		grammar_alloc_free (source);
		return 1;
	}

	f = fopen (out, "w");
	fprintf (f, "\n");
	fprintf (f, "/* DO NOT EDIT - THIS FILE IS AUTOMATICALLY GENERATED FROM THE FOLLOWING FILE: */\n");
	fprintf (f, "/* %s */\n", in);
	fprintf (f, "\n");
	for (i = 0; i < size; i++)
	{
		unsigned int a;
		if (prod[i] < 10)
			a = 1;
		else if (prod[i] < 100)
			a = 2;
		else
			a = 3;
		if (i < size - 1)
			a++;
		if (line + a >= 100)
		{
			fprintf (f, "\n");
			line = 0;
		}
		line += a;
		fprintf (f, "%d", prod[i]);
		if (i < size - 1)
			fprintf (f, ",");
	}
	fprintf (f, "\n");
	fclose (f);
	grammar_alloc_free (prod);
   return 0;
}

int main (int argc, char *argv[])
{
   grammar id;

   id = grammar_load_from_text ((const byte *) slang_shader_syn);
   if (id == 0) {
      fprintf(stderr, "Error loading grammar from text\n");
      return 1;
   }
   grammar_set_reg8 (id, (const byte *) "parsing_builtin", 1);
   grammar_set_reg8 (id, (const byte *) "shader_type", atoi (argv[1]));
   if (gc_to_bin (id, argv[2], argv[3])) {
      fprintf(stderr, "Error in gc_to_bin %s %s\n", argv[2], argv[3]);
      grammar_destroy (id);
      return 1;
   }
   grammar_destroy (id);
   return 0;
}

