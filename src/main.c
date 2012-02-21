#include <niurouting.h>
#include <trsp.h>

/**
 * <-----------------------------------
 * TRSP Structure
 */
typedef struct edge_columns {
    int id;
    int source;
    int target;
    int cost;
    int reverse_cost;
} edge_columns_t;

typedef struct restrict_columns {
    int target_id;
    int via_path;
    int to_cost;
} restrict_columns_t;
/**
 * ----------------------------------->
 * TRSP Structure
 */

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * <-----------------------------------
     * Module Structure
     */
    typedef struct NiuRoutingStruct {
        /* extends the sqlite3_vtab struct */
        const sqlite3_module *pModule; /* ptr to sqlite module: USED INTERNALLY BY SQLITE */
        int nRef; /* # references: USED INTERNALLY BY SQLITE */
        char *zErrMsg; /* error message: USE INTERNALLY BY SQLITE */

        // TRSP
        restrict_t *restricts;
        edge_t *edges;

        int total_edges;
        int total_restrictions;

        int source, target, directed, has_reverse_cost;

        const char *network_structure_table;
        const char *restriction_structure_table;
    } NiuRouting;

    typedef struct NiuRoutingCursorStruct {
        /* extends the sqlite3_vtab_cursor struct */
        NiuRouting *pVtab; /* Virtual table of this cursor */
        int eof; /* the EOF marker */

        path_element_t *path;
        int path_count;
        int index;

    } NiuRoutingCursor;

    static sqlite3_module niurouting_mod;

    int turn_restriction_shortest_path(NiuRouting *nr, NiuRoutingCursor *nrc) {
        int result = -1;
        int path_count = 0;
        char *err_msg;
        result = trsp_wrapper(
                nr->edges, nr->total_edges,
                nr->restricts, nr->total_restrictions,
                nr->source, nr->target,
                nr->directed, nr->has_reverse_cost,
                &nrc->path, &path_count, &err_msg);

        DBG("Result for path: %d \n", result);
        DBG("Error message: %s \n", err_msg);

        if (result < 0) {
            return SQLITE_ERROR;
        } else {

            // Set the values
            nrc->path_count = path_count;
            nrc->index = 0;

            DBG("Message received from inside: %s \n", err_msg);

            for (int z = 0; z < path_count; z++) {
                DBG("vetex: %d \n", nrc->path[z].vertex_id);
            }
        }

        return SQLITE_OK;
    }

    int remove_trsp_solution(NiuRoutingCursor *nrc) {
        if (nrc->path != NULL)
            free(nrc->path);

        return SQLITE_OK;
    }

    int remove_network(NiuRouting *nr) {
        if (nr->edges != NULL) {
            free(nr->edges);
        }
        nr->edges = NULL;
        nr->total_restrictions = 0;
        return SQLITE_OK;
    }

    int remove_restrictions(NiuRouting *nr) {
        if (nr->restricts != NULL) {
            free(nr->restricts);
        }
        nr->restricts = NULL;
        nr->total_restrictions = 0;
        return SQLITE_OK;
    }

    int load_restrictions(sqlite3 *db, NiuRouting *nr, char **pzErr) {

        // Free memory
        remove_restrictions(nr);

        sqlite3_stmt *stmt;
        const char *sql = sqlite3_mprintf("SELECT target_id, to_cost, via_path FROM \"%w\" order by target_id;", nr->restriction_structure_table);
        int result = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

        if (result == SQLITE_OK) {

            int size = 0, finish = 0;
            while (1) {

                result = sqlite3_step(stmt);
                if (result == SQLITE_DONE) {
                    finish = 1;
                    break;
                }
                if (result == SQLITE_ROW) {

                    size++;

                    int target = sqlite3_column_int(stmt, 0);
                    int cost = sqlite3_column_double(stmt, 1);
                    char *via_path = (char *) sqlite3_column_text(stmt, 2);

                    if (!nr->restricts) {
                        nr->restricts = (restrict_t*) malloc(size * sizeof (restrict_t));
                    } else {
                        nr->restricts = (restrict_t*) realloc(nr->restricts, size * sizeof (restrict_t));
                    }
                    if (nr->restricts == NULL) {
                        *pzErr = sqlite3_mprintf("Out of memory");
                        break;
                    }

                    restrict_t *rest = &nr->restricts[size - 1];
                    rest->target_id = target;
                    rest->to_cost = cost;
                    for (int t = 0; t < MAX_RULE_LENGTH; ++t)
                        rest->via[t] = -1;

                    if (via_path != NULL) {
                        char* pch = NULL;
                        int ci = 0;

                        pch = (char *) strtok(via_path, " ,");

                        while (pch != NULL && ci < MAX_RULE_LENGTH) {
                            rest->via[ci] = atoi(pch);
                            ci++;
                            pch = (char *) strtok(NULL, " ,");
                        }
                    }
                }
            }

            sqlite3_finalize(stmt);

            if (finish == 1) {
                nr->total_restrictions = size - 1;

                DBG("Loaded restrictions [%d] items. \n", nr->total_restrictions);

                return SQLITE_OK;
            } else {
                DBG("Loading restrictions error, Out of memory \n");

                return SQLITE_ERROR;
            }
        } else {
            DBG("Loading restriction error %s \n", sql);
            return SQLITE_ERROR;
        }
    }

    int load_network(sqlite3 *db, NiuRouting *nr, char **pzErr) {

        // Free memory
        remove_network(nr);
        remove_restrictions(nr);

        sqlite3_stmt *stmt;
        const char *sql = sqlite3_mprintf("SELECT segment, source, target, cost, reverse_cost FROM \"%w\" order by segment;", nr->network_structure_table);
        int result = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

        if (result == SQLITE_OK) {

            int size = 0, finish = 0;
            while (1) {

                result = sqlite3_step(stmt);
                if (result == SQLITE_DONE) {
                    finish = 1;
                    break;
                }
                if (result == SQLITE_ROW) {

                    size++;

                    int segment = sqlite3_column_int(stmt, 0);
                    int source = sqlite3_column_int(stmt, 1);
                    int target = sqlite3_column_int(stmt, 2);
                    double cost = sqlite3_column_double(stmt, 3);
                    double reverse_cost = sqlite3_column_double(stmt, 4);

                    if (!nr->edges) {
                        nr->edges = (edge_t*) malloc(size * sizeof (edge_t));
                    } else {
                        nr->edges = (edge_t*) realloc(nr->edges, size * sizeof (edge_t));
                    }
                    if (nr->edges == NULL) {
                        *pzErr = sqlite3_mprintf("Out of memory");
                        break;
                    }

                    edge_t *target_edge = (edge_t*) & nr->edges[size - 1];
                    target_edge->id = segment;
                    target_edge->source = source;
                    target_edge->target = target;
                    target_edge->cost = cost;
                    target_edge->reverse_cost = reverse_cost;
                }
            }

            sqlite3_finalize(stmt);

            if (finish == 1) {
                nr->total_edges = size - 1;

                DBG("Loaded network [%d] items. \n", nr->total_edges);

                return SQLITE_OK;
            } else {
                DBG("Loading network error, Out of memory \n");

                return SQLITE_ERROR;
            }
        } else {
            DBG("Loading network error %s \n", sql);
            return SQLITE_ERROR;
        }
    }

    static int xConnect(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVTab, char **pzErr) {

        DBG("NiuRouting xConnect \n");

        const char *virtual_table_name;
        const char *network_structure_table;
        const char *restriction_structure_table;

        NiuRouting *nr;

        // Get data table structure
        /**
         * 0 => Error retrieving table info data
         * 1 => Invalid table name, table doesn't exists
         * 2 => Wrong number of columns for table
         * 3 => Table doesn't match column names/types
         */
        int error;


        int n_rows;
        int n_columns;
        int response;
        char **results;
        char *err_msg = NULL;
        const char *col_name;
        const char *col_type;

        if (argc == 5) {

            virtual_table_name = argv[2];
            network_structure_table = argv[3];
            restriction_structure_table = argv[4];
            const char *table_structure = sqlite3_mprintf("CREATE TABLE \"%w\"(source hidden integer, target hidden integer, directed hidden integer, has_reverse_cost hidden integer, edge integer, node integer, cost double);", virtual_table_name);

            DBG("Network %s, Restrictions: %s \n", network_structure_table, restriction_structure_table);

            // Initialize for routing
            int ci = 1, has_segment = 0, has_source = 0, has_target = 0, has_cost = 0, has_reverse_cost = 0;
            int has_target_id = 0, has_via_path, has_to_cost = 0;

            // retrieving the base table columns for network
            const char *sql = sqlite3_mprintf("PRAGMA table_info(\"%w\");", network_structure_table);
            response = sqlite3_get_table(db, sql, &results, &n_rows, &n_columns, &err_msg);
            if (response != SQLITE_OK) {
                error = 0;
                goto illegal;
            }
            if (n_rows > 1) {

                DBG("Network table \n");

                for (; ci <= n_rows; ci++) {
                    col_name = results[(ci * n_columns) + 1];
                    col_type = results[(ci * n_columns) + 2];

                    DBG("Col: %s, Type: %s \n", col_name, col_type);

                    if (strcasecmp(col_name, "segment") == 0 && strcasecmp(col_type, "integer") == 0) {
                        has_segment = 1;
                    }
                    if (strcasecmp(col_name, "source") == 0 && strcasecmp(col_type, "integer") == 0) {
                        has_source = 1;
                    }
                    if (strcasecmp(col_name, "target") == 0 && strcasecmp(col_type, "integer") == 0) {
                        has_target = 1;
                    }
                    if (strcasecmp(col_name, "cost") == 0 && strcasecmp(col_type, "real") == 0) {
                        has_cost = 1;
                    }
                    if (strcasecmp(col_name, "reverse_cost") == 0 && strcasecmp(col_type, "real") == 0) {
                        has_reverse_cost = 1;
                    }
                }

                sqlite3_free_table(results);
            } else {
                error = 1;
                goto illegal;
            }

            // retrieve the colums for restrictions
            sql = sqlite3_mprintf("PRAGMA table_info(\"%w\");", restriction_structure_table);
            results = NULL;
            n_rows = 0;
            n_columns = 0;
            ci = 1;
            err_msg = NULL;
            response = sqlite3_get_table(db, sql, &results, &n_rows, &n_columns, &err_msg);
            if (response != SQLITE_OK) {
                error = 0;
                goto illegal;
            }
            if (n_rows > 1) {

                DBG("Restrictions table \n");

                for (; ci <= n_rows; ci++) {

                    col_name = results[(ci * n_columns) + 1];
                    col_type = results[(ci * n_columns) + 2];

                    DBG("Col: %s, Type: %s \n", col_name, col_type);

                    if (strcasecmp(col_name, "target_id") == 0 && strcasecmp(col_type, "integer") == 0) {
                        has_target_id = 1;
                    }
                    if (strcasecmp(col_name, "via_path") == 0 && strcasecmp(col_type, "text") == 0) {
                        has_via_path = 1;
                    }
                    if (strcasecmp(col_name, "to_cost") == 0 && strcasecmp(col_type, "real") == 0) {
                        has_to_cost = 1;
                    }
                }

                sqlite3_free_table(results);
            } else {
                error = 1;
                goto illegal;
            }

            // If table are OK
            if ((has_segment + has_source + has_target + has_cost + has_reverse_cost) == 5 && (has_target_id + has_via_path + has_to_cost) == 3) {

                DBG("Creating table %s \n", table_structure);

                if (sqlite3_declare_vtab(db, table_structure) == SQLITE_OK) {
                    nr = (NiuRouting*) sqlite3_malloc(sizeof (NiuRouting));

                    if (nr == NULL) {
                        *pzErr = sqlite3_mprintf("[NiuRouting module] CREATE VIRTUAL: No memory \n");
                        return SQLITE_NOMEM;
                    }

                    nr->network_structure_table = network_structure_table;
                    nr->restriction_structure_table = restriction_structure_table;

                    nr->edges = NULL;
                    nr->restricts = NULL;

                    nr->pModule = &niurouting_mod;
                    nr->nRef = 0;
                    nr->zErrMsg = NULL;
                    *ppVTab = (sqlite3_vtab*) nr;

                    load_network(db, nr, pzErr);
                    load_restrictions(db, nr, pzErr);

                    return SQLITE_OK;
                } else {
                    *pzErr = sqlite3_mprintf("[NiuRouting module] CREATE VIRTUAL: error \n");
                    return SQLITE_ERROR;
                }

            } else {
                error = 3;
                goto illegal;
            }
        } else {
            *pzErr = sqlite3_mprintf("[NiuRouting module] CREATE VIRTUAL: illegal arg list (network, restrictions); \n");
            return SQLITE_ERROR;
        }

illegal:
        // Some error
        *pzErr = sqlite3_mprintf("[NiuRouting module] can't build a valid NETWORK. Error [%d] \n", error);
        return SQLITE_ERROR;
    }

    static int xBestindex(sqlite3_vtab *pVTab, sqlite3_index_info * pIdxInfo) {

        DBG("NiuRouting xBestindex \n");

        int i;
        int source = 0, target = 0, directed = 0, has_reverse_cost = 0, errors = 0;

        for (i = 0; i < pIdxInfo->nConstraint; i++) {
            struct sqlite3_index_info::sqlite3_index_constraint *p = &(pIdxInfo->aConstraint[i]);
            if (p->usable) {

                DBG("U: %d, i: %d, O: %d \n", p->usable, p->iColumn, p->op);

                if (p->iColumn == 0 && p->op == SQLITE_INDEX_CONSTRAINT_EQ) {
                    source++;
                } else if (p->iColumn == 1 && p->op == SQLITE_INDEX_CONSTRAINT_EQ) {
                    target++;
                } else if (p->iColumn == 2 && p->op == SQLITE_INDEX_CONSTRAINT_EQ) {
                    directed++;
                } else if (p->iColumn == 3 && p->op == SQLITE_INDEX_CONSTRAINT_EQ) {
                    has_reverse_cost++;
                } else {
                    errors++;
                }
            }
        }

        if (source == 1 && target == 1 && directed == 1 && has_reverse_cost == 1 && errors == 0) {
            pIdxInfo->idxNum = 1;
            pIdxInfo->estimatedCost = 1.0;
            for (i = 0; i < pIdxInfo->nConstraint; i++) {
                if (pIdxInfo->aConstraint[i].usable) {
                    pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
                    pIdxInfo->aConstraintUsage[i].omit = 1;
                }
            }
        } else {
            pIdxInfo->idxNum = 0;
        }

        return SQLITE_OK;
    }

    static int xDisconnect(sqlite3_vtab * pVTab) {

        DBG("NiuRouting xDisconnect \n");

        NiuRouting *nr = (NiuRouting*) pVTab;

        remove_network(nr);
        remove_restrictions(nr);

        sqlite3_free(pVTab);
        return SQLITE_OK;
    }

    static int xOpen(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor) {

        DBG("NiuRouting xOpen \n");

        NiuRoutingCursor *pCursor = NULL;
        pCursor = (NiuRoutingCursor*) sqlite3_malloc(sizeof (NiuRoutingCursor));

        if (pCursor == NULL)
            return SQLITE_NOMEM;

        pCursor->pVtab = (NiuRouting*) pVTab;
        pCursor->path = NULL;
        pCursor->path_count = 0;
        pCursor->index = 0;
        pCursor->eof = 0;
        *ppCursor = (sqlite3_vtab_cursor*) pCursor;

        return SQLITE_OK;
    }

    static int xClose(sqlite3_vtab_cursor *pCursor) {

        DBG("NiuRouting xClose \n");

        remove_trsp_solution((NiuRoutingCursor*) pCursor);

        sqlite3_free(pCursor);

        return SQLITE_OK;
    }

    static int xFilter(sqlite3_vtab_cursor *pCursor, int idxNum, const char *idxStr, int argc, sqlite3_value **argv) {

        DBG("NiuRouting xFilter \n");

        NiuRoutingCursor *nrc = (NiuRoutingCursor*) pCursor;
        NiuRouting *nr = (NiuRouting*) nrc->pVtab;
        remove_trsp_solution(((NiuRoutingCursor*) pCursor));
        nrc->eof = 1;

        DBG("idxNum: %d, argc: %d \n", idxNum, argc);

        if (idxNum == 1 && argc == 4) {

            if (sqlite3_value_type(argv[0]) == SQLITE_INTEGER && sqlite3_value_type(argv[1]) == SQLITE_INTEGER && sqlite3_value_type(argv[2]) == SQLITE_INTEGER && sqlite3_value_type(argv[3]) == SQLITE_INTEGER) {
                int source = sqlite3_value_int(argv[0]);
                int target = sqlite3_value_int(argv[1]);
                int directed = sqlite3_value_int(argv[2]);
                int has_reverse_cost = sqlite3_value_int(argv[3]);

                nr->source = source;
                nr->target = target;
                nr->directed = directed;
                nr->has_reverse_cost = has_reverse_cost;

                nrc->eof = 0;

                DBG("Find route for s: %d, e: %d \n", source, target);

                // Calculate the routing
                return turn_restriction_shortest_path(nr, nrc);
            } else {
                DBG("Wrong type of arguments \n");

                return SQLITE_ERROR;
            }
        } else {

            DBG("Wrong number of arguments \n");

            return SQLITE_ERROR;
        }

        return SQLITE_OK;
    }

    static int xNext(sqlite3_vtab_cursor * pCursor) {

        DBG("NiuRouting xNext \n");

        NiuRoutingCursor *cursor = (NiuRoutingCursor*) pCursor;
        if (++cursor->index < cursor->path_count) {
            //cursor->index++;
        } else {
            cursor->eof = 1;
        }
        return SQLITE_OK;
    }

    static int xEof(sqlite3_vtab_cursor * pCursor) {

        DBG("NiuRouting xEof \n");

        NiuRoutingCursor *cursor = (NiuRoutingCursor*) pCursor;
        return cursor->eof;
    }

    static int xColumn(sqlite3_vtab_cursor *pCursor, sqlite3_context *pContext, int column) {

        DBG("NiuRouting xColumn \n");

        NiuRoutingCursor *nrc = (NiuRoutingCursor*) pCursor;
        NiuRouting *nr = (NiuRouting*) nrc->pVtab;

        path_element_t *path = &nrc->path[nrc->index];

        if (column == 4) {
            sqlite3_result_int(pContext, path->edge_id);
        } else if (column == 5) {
            sqlite3_result_int(pContext, path->vertex_id);
        } else if (column == 6) {
            sqlite3_result_double(pContext, path->cost);
        }

        return SQLITE_OK;
    }

    static int xRowid(sqlite3_vtab_cursor *pCursor, sqlite_int64 * pRowid) {

        DBG("NiuRouting xRowid \n");

        NiuRoutingCursor *nrc = (NiuRoutingCursor*) pCursor;
        *pRowid = nrc->index;

        return SQLITE_OK;
    }

    static int xRename(sqlite3_vtab *pVTab, const char *pNewName) {
        return SQLITE_READONLY;
    }

    int initialize_niurouting(sqlite3 * database) {

        DBG("Creating module... \n");

        niurouting_mod.iVersion = 1;
        niurouting_mod.xCreate = &xConnect;
        niurouting_mod.xConnect = &xConnect;
        niurouting_mod.xBestIndex = &xBestindex;
        niurouting_mod.xDisconnect = &xDisconnect;
        niurouting_mod.xDestroy = &xDisconnect;
        niurouting_mod.xOpen = &xOpen;
        niurouting_mod.xClose = &xClose;
        niurouting_mod.xFilter = &xFilter;
        niurouting_mod.xNext = &xNext;
        niurouting_mod.xEof = &xEof;
        niurouting_mod.xColumn = &xColumn;
        niurouting_mod.xRowid = &xRowid;
        niurouting_mod.xUpdate = NULL;
        niurouting_mod.xBegin = NULL;
        niurouting_mod.xSync = NULL;
        niurouting_mod.xCommit = NULL;
        niurouting_mod.xRollback = NULL;
        niurouting_mod.xFindFunction = NULL;
        niurouting_mod.xRename = xRename;

        int rc = sqlite3_create_module_v2(database, "NiuRouting", &niurouting_mod, NULL, 0);

        DBG("Create module done. \n", rc);

        if (rc != SQLITE_OK) {
            return SQLITE_ERROR;
        }

        // Return OK
        return SQLITE_OK;
    }
    /**
     * ----------------------------------->
     * Module Structure
     */

    /**
     * Routing version
     * @param context
     * @param argc
     * @param argv
     */
    static void niurouting_version(sqlite3_context *context, int argc, sqlite3_value **argv) {
        //std::string v(NIUROUTING_VERSION);
        //sqlite3_result_text(context, v.c_str(), v.size(), 0);
    }

    /**
     * SQLite Extension, Entry point
     * @param db
     * @param pzErrMsg
     * @param pApi
     * @return 
     */
    int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines * pApi) {
        SQLITE_EXTENSION_INIT2(pApi)

        sqlite3_create_function(db, "niurouting_version", 0, SQLITE_UTF8, 0, niurouting_version, 0, 0);

        // Initialize tables
        initialize_niurouting(db);

        return 0;
    }

#ifdef	__cplusplus
}
#endif