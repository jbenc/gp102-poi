/*
 * Canmore GP-102+ POI Tool
 * Copyright (C) 2016  Jiri Benc <jbenc@upir.cz>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

struct poi_data {
	uint8_t unknown1;	/* seems to be always 0x01 */
	uint8_t icon;
	uint8_t unknown2[10];	/* seems to be always 0x00 0x00 0x01 followed by zeros */
	char name[10];
	uint8_t unknown3[54];	/* seems to be always zeros; perhaps still (part of) the name? */
	uint32_t lat;
	uint32_t lon;
	uint8_t unused[44];	/* 0xff */
};

static char icon_names[][11] = {
	"star", "home", "checkpoint",
	"car", "cafe", "train",
	"gas", "office", "airport",
};
#define ICONS	(sizeof(icon_names) / sizeof(icon_names[0]))

static void print_coord(double coord, int len, const char *mod)
{
	int deg = abs((int)coord);

	printf("%c %0*d° %06.3f", mod[!!(coord < 0)], len, deg, (fabs(coord) - deg) * 60);
}

static int read_poi(const char *fname)
{
	FILE *f;
	struct poi_data data;
	uint8_t buf;
	double lat, lon;

	f = fopen(fname, "rb");
	if (!f)
		goto err;
	if (fread(&data, sizeof(data), 1, f) < 1)
		goto err;
	if (fread(&buf, 1, 1, f)) {
		fprintf(stderr, "Error: unexpected size of `%s'.\n", fname);
		fclose(f);
		return 1;
	}
	fclose(f);

	if (data.icon >= ICONS) {
		fprintf(stderr, "Error: %s: invalid file type.\n", fname);
		return 1;
	}
	if (data.unknown1 != 0x01)
		fprintf(stderr, "Warning: %s: invalid signature, continuing anyway.\n", fname);
	lat = (double)(int32_t)le32toh(data.lat) / 100000;
	lon = (double)(int32_t)le32toh(data.lon) / 100000;
	printf("%s (%s) ", data.name, icon_names[data.icon]);
	print_coord(lat, 2, "NS");
	printf(" ");
	print_coord(lon, 3, "EW");
	printf("\n");

	return 0;

err:
	fprintf(stderr, "Error reading from `%s': %s\n", fname, strerror(errno));
	if (f)
		fclose(f);
	return 1;
}

static void skip_white(const char **pos)
{
	while (isspace(**pos))
		(*pos)++;
}

static void skip_string(const char **pos, const char *what)
{
	if (strncmp(*pos, what, strlen(what)))
		return;
	*pos += strlen(what);
}

static int get_letter(const char **pos, const char *letters)
{
	const char *cur_letter = letters;

	while (*cur_letter) {
		if (**pos == *cur_letter) {
			(*pos)++;
			return cur_letter - letters;
		}
		cur_letter++;
	}
	return -1;
}

static int get_number(const char **pos, double *dst)
{
	char buf[16];
	char *buf_end;
	int len = 0, was_dot = 0;

	while (isdigit(**pos) || **pos == '.') {
		if (len == 14)
			return 1;
		if (**pos == '.') {
			if (was_dot)
				return 1;
			was_dot = 1;
		}
		buf[len] = **pos;
		(*pos)++;
		len++;
	}
	buf[len] = '\0';
	*dst = strtod(buf, &buf_end);
	return !!(*buf_end);
}

static int parse_part_coord(const char **pos, const char *mod, const char *unit,
			    int *which_mod, double *part)
{
	int err;

	skip_white(pos);
	if (!isdigit(**pos))
		// end parsing of this coordinate
		return -1;
	err = get_number(pos, part);
	if (err)
		return 1;
	skip_white(pos);
	skip_string(pos, unit);
	skip_white(pos);
	if (*which_mod >= 0)
		return 0;
	*which_mod = get_letter(pos, mod);
	if (*which_mod >= 0)
		// end parsing of this coordinate
		return -1;
	return 0;
}

static int parse_one_coord(const char **pos, const char *mod, double *coord)
{
	int err;
	int which_mod;
	double deg, min = 0.0, sec = 0.0;

	skip_white(pos);
	which_mod = get_letter(pos, mod);
	skip_white(pos);
	if (!isdigit(**pos))
		return 1;

	err = parse_part_coord(pos, mod, "°", &which_mod, &deg);
	if (err > 0)
		return 1;
	if (!err) {
		err = parse_part_coord(pos, mod, "'", &which_mod, &min);
		if (err > 0)
			return 1;
		if (!err) {
			err = parse_part_coord(pos, mod, "\"", &which_mod, &sec);
			if (err > 0)
				return 1;
		}
	}
	if (which_mod < 0)
		return 1;

	*coord = deg + min / 60 + sec / 3600;
	if (which_mod)
		*coord = -*coord;
	return 0;
}

static int parse_middle(const char **pos)
{
	get_letter(pos, ",;");
	return 0;
}

static int parse_end(const char **pos)
{
	skip_white(pos);
	return !!(**pos);
}

static void parse_coords(const char *arg, double *lat, double *lon)
{
	const char *pos = arg;

	if (parse_one_coord(&pos, "NS", lat) ||
	    parse_middle(&pos) ||
	    parse_one_coord(&pos, "EW", lon) ||
	    parse_end(&pos)) {
		int i;

		fprintf(stderr, "Error: cannot parse coordinates:\n");
		fprintf(stderr, "       %s\n", arg);
		for (i = pos - arg; i >= 0; i--)
			putc(' ', stderr);
		fprintf(stderr, "       ^\n");
		exit(1);
	}
}

static const char *supported = "0123456789-.:/_";

static int check_name(const char *name)
{
	while (*name) {
		if (!index(supported, *name))
			return 1;
		name++;
	}
	return 0;
}

static int write_poi(const char *name, const char *arg)
{
	struct poi_data data = {
		.unknown1 = 0x01,
		.icon = 0,
		.unknown2 = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	};
	double lat, lon;

	if (check_name(name)) {
		fprintf(stderr, "Error: unsupported char in name. Supported chars are: %s\n", supported);
		return 1;
	}
	parse_coords(arg, &lat, &lon);

	memset(&data.unused, 0xff, sizeof(data.unused));
	strncpy(data.name, name, sizeof(data.name));
	data.lat = htole32((int32_t)(lat * 100000));
	data.lon = htole32((int32_t)(lon * 100000));
	fwrite(&data, sizeof(data), 1, stdout);
	return 0;
}

static void help(char *arg0)
{
	printf("Usage: %s poi_file [poi_file...]\n", arg0);
	printf("       %s -e name \"coords\"\n", arg0);
	exit(1);
}

int main(int argc, char **argv)
{
	int i, err = 0;

	if (argc <= 1)
		help(argv[0]);

	if (!strcmp(argv[1], "-e")) {
		if (argc <= 3)
			help(argv[0]);
		return write_poi(argv[2], argv[3]);
	}

	for (i = 1; i < argc; i++)
		err |= read_poi(argv[i]);
	return err;
}
