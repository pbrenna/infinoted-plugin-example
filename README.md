# infinoted-plugin-replacer
[![Build Status](https://api.travis-ci.org/pbrenna/infinoted-plugin-replacer.svg?branch=master)](https://travis-ci.org/pbrenna/infinoted-plugin-replacer)

This is a simple example plugin for
[infinoted](https://github.com/gobby/gobby/wiki/Dedicated-Server),
the dedicated Gobby server. This plugin replaces text with custom 
snippets defined in a configuration file.

This is originally based on the Linekeeper plugin provided with the 
[libinfinity](https://github.com/gobby/libinfinity/).

# Compiling 
This was tested on Arch Linux:
```
$ ./autogen.sh --prefix=/usr
$ sudo make install
```
This worked on Debian Jessie x86_64:
```
$ ./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu/
$ sudo make install
```

# Configuration
1. Create your ``replace-table.json``: 
   ```json
   {
   	"alpha" : "α",
	"beta" : "β",
	"\\latex_style_rule" : "Latex rules"
   }
   ```
   
   **Important**: a macro is not allowed to be a prefix of another macro;
   recursion in the macros is well-defined behaviour but can lead to unwanted
   results.
   

2. Add "replacer" to the plugin list in your ``infinoted.conf`` file
3. Append a ``[replacer]`` section to ``infinoted.conf``:

   ```
   [replacer]
   replace-table = /path/to/your/replace-table.json
   ```

# Usage
The plugin does nothing by default. It must be enabled (file by file) by 
having
```
#replacer on
```
followed by a newline, at the beginning of the text document.

## Licensing

Copyright (C) 2016 Pietro Brenna <pietrobrenna@zoho.com>

Copyright (C) 2016 Alessandro Bregoli <>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
MA 02110-1301, USA.
