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
	uint8_t unknown2[10];	/* seems to be always 0x00 0x00 0x10 followed by zeros */
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

int main(int argc, char **argv)
{
	int i, err = 0;

	if (argc <= 1) {
		printf("Usage: %s poi_file [poi_file...]\n", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; i++)
		err |= read_poi(argv[i]);
	return err;
}
