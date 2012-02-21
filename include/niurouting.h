/*
Copyright (c) 2012 jorgeliecer.osorio@gmail.com

Permission is hereby granted, free of charge, to any 
person obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the 
Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the 
Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice 
shall be included in all copies or substantial portions of 
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY 
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS 
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef NIUROUTING_H
#define	NIUROUTING_H

typedef unsigned char BOOL;

#define true 1
#define false 0

#undef DEBUG
//#define DEBUG 1

#ifdef DEBUG
#define DBG(format, arg...) fprintf(stderr, format , ## arg)
#else
#define DBG(format, arg...) do { ; } while (0)
#endif

#define NIUROUTING_VERSION "Niu Routing V1.0 <jorgeliecer.osorio@gmail.com>"

#include <sqlite3ext.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static SQLITE_EXTENSION_INIT1;

#endif