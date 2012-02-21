/* 
 * File:   niurouting.h
 * Author: jorgeosorio
 *
 * Created on 2 de febrero de 2012, 15:08
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