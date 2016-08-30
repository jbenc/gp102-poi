gp102-poi: gp102-poi.c
	gcc -W -Wall -o $@ $<

clean:
	rm -f gp102-poi
