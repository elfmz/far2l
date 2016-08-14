#include <stdio.h>
#include <string.h>

int main_generator(int argc, char **argv);
int main_convertor(int argc, char **argv);
int main_inserter(int argc, char **argv);

int main(int argc, char **argv)
{
	printf("FAR language umbrella tool\n");
	if (argc>1 && strcmp(argv[1], "generator")==0)
		return main_generator(argc - 1, argv +1);

	if (argc>1 && strcmp(argv[1], "convertor")==0)
		return main_convertor(argc - 1, argv +1);

	if (argc>1 && strcmp(argv[1], "inserter")==0)
		return main_inserter(argc - 1, argv +1);

	printf("Usage: %s [generator|convertor|inserter] [tool-specific-arguments]\n", argv[0]);
	return 1;
}
