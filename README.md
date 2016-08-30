Canmore GP-102+ POI Tool
========================

A tool to read and create POI files for Canmore GP-102+.

The data are stored in GP-102/POIs/ directory at the device. Files need to
be named '000', '001', etc.

Usage
-----

To read and decode all POI files from the device:

```
gp102-poi /path/to/canmore/GP-102/POIs/*
```

To create a POI file:

```
gp102-poi -e "name" "coords" > /path/to/canmore/GP-102/POIs/000
```

The coordinates are accepted in almost any format. Note that only numbers
and a few special characters are available for the name.

License
-------

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

Credits
-------

Written by Jiri Benc.

Inspired by Canmore G-Porter POI Reader by Archil Imnadze,
https://github.com/aimnadze/canmore-gporter-poi-reader
