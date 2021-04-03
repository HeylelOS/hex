#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <err.h>

struct bin2src_args {
	const char *symbol;
	const char *output;
};

static void
bin2src_usage(const char *bin2srcname) {
	fprintf(stderr, "usage: %s -S <symbol> -o <output> files...\n", bin2srcname);
	exit(EXIT_FAILURE);
}

static int
bin2src_valid_symbol(const char *symbol) {

	if (!isalpha(*symbol) && *symbol != '_') {
		return 1;
	}

	do {
		symbol++;
	} while (isalnum(*symbol) || *symbol == '_');

	return *symbol != '\0';
}

static const struct bin2src_args
bin2src_parse_args(int argc, char **argv) {
	struct bin2src_args args = {
		.symbol = NULL,
		.output = NULL,
	};
	int c;

	while (c = getopt(argc, argv, ":S:o:"), c != -1) {
		switch (c) {
		case 'S':
			args.symbol = optarg;
			break;
		case 'o':
			args.output = optarg;
			break;
		case ':':
			fprintf(stderr, "%s: -%c: Missing argument\n", *argv, optopt);
			bin2src_usage(*argv);
		default:
			fprintf(stderr, "%s: Unknown argument -%c\n", *argv, optopt);
			bin2src_usage(*argv);
		}
	}

	if (args.symbol == NULL) {
		fprintf(stderr, "%s: Missing symbol name\n", *argv);
		bin2src_usage(*argv);
	}

	if (bin2src_valid_symbol(args.symbol) != 0) {
		fprintf(stderr, "%s: Invalid symbol '%s'\n", *argv, args.symbol);
		bin2src_usage(*argv);
	}

	if (args.output == NULL) {
		fprintf(stderr, "%s: Missing output file\n", *argv);
		bin2src_usage(*argv);
	}

	if (argc == optind) {
		fprintf(stderr, "%s: Missing input file(s)\n", *argv);
		bin2src_usage(*argv);
	}

	return args;
}

static inline char
nib2hex(unsigned char nibble) {
	return nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
}

static int
bin2src_dump_file(const char *input, unsigned long pagesize, unsigned long *sizep, FILE *outp) {
	int fd = open(input, O_RDONLY);
	int retval = -1;

	if (fd >= 0) {
		/* To increase performances, we batch writes,
		writing "0x.2X, " sizeof (unsigned long) times at once.
		We have a single format buffer for the whole file, filled up first,
		and then modified for each byte in a batch. */
		static const char format[] = "0x00, ";
		char wordformat[(sizeof (format) - 1) * sizeof (unsigned long)];
		const char * const wordformatend = wordformat + sizeof (wordformat);
		char buffer[pagesize];
		ssize_t readval;

		for (char *current = wordformat; current != wordformatend; current = stpncpy(current, format, sizeof (format) - 1));

		while (readval = read(fd, buffer, sizeof (buffer)), readval > 0) {
			const char *current = buffer, * const end = buffer + readval;
			const char * const aligned = (const char *)((uintptr_t)end & ~(sizeof (unsigned long) - 1));

			while (current != aligned) {
				char *byteformat = wordformat;

				while (byteformat != wordformatend) {
					const unsigned char byte = *current;

					byteformat[2] = nib2hex(byte >> 4);
					byteformat[3] = nib2hex(byte & 0xF);

					byteformat += sizeof (format) - 1;
					current++;
				}

				fwrite(wordformat, sizeof (wordformat), 1, outp);
			}

			while (current != end) {
				fprintf(outp, "0x%.2X, ", (unsigned char)*current);
				current++;
			}

			*sizep += readval;
		}

		if (readval != 0) {
			warn("read %s", input);
		} else {
			retval = 0;
		}

		close(fd);
	} else {
		warn("open %s", input);
	}

	return retval;
}

int
main(int argc, char **argv) {
	const struct bin2src_args args = bin2src_parse_args(argc, argv);
	FILE * const outp = fopen(args.output, "w");
	int retval = EXIT_FAILURE;

	if (outp != NULL) {
		char **argpos = argv + optind, ** const argend = argv + argc;
		const unsigned long pagesize = getpagesize();
		unsigned long size = 0;

		fprintf(outp, "const unsigned char %s[] = { ", args.symbol);

		while (argpos != argend) {
			const char * const input = *argpos;

			if (bin2src_dump_file(input, pagesize, &size, outp) != 0) {
				break;
			}

			argpos++;
		}

		fprintf(outp, "}; const unsigned long %s_size = %lu;", args.symbol, size);

		fclose(outp);

		if (argpos == argend) {
			retval = EXIT_SUCCESS;
		} else {
			remove(args.output);
		}
	} else {
		warn("fopen %s", args.output);
	}

	return retval;
}

