//
//  echodemo.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "sophia"
#define PLUGNAME(NAME) sophia ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#include "sophia.h"
#define DEFINES_ONLY
#include "../plugin777.c"
#include "db777.c"
#undef DEFINES_ONLY

struct db777 *DB_msigs,*DB_NXTaccts;//,*DB_NXTassettx,*DB_nodestats;

STRUCTNAME SOPHIA;
char *PLUGNAME(_methods)[] = { "create", "close", "add", "find",
#ifdef BUNDLED
    "get", "set", "object", "env", "ctl","open", "destroy", "error", "delete", "async", "drop", "cursor", "begin", "commit", "type",
#endif
};

void sophia_idle(struct plugin_info *plugin) {} 

// env = sp_env(void);
// ctl = sp_ctl(env): get an environment control object.
// sp_error(env): check if there any error leads to the shutdown.

// sp_open(env): create environment, open or create pre-defined databases.
// sp_open(database): create or open database.

// adatabase = sp_async(database) Get a special asynchronous object, which can be used instead of a database object. All operations executed using this object should run in parallel. Callback function is called on completion (db.name.on_complete).


// transaction = sp_begin(env): create a transaction
// During transaction, all updates are not written to the database files until a sp_commit(3) is called. All updates that were made during transaction are available through sp_get(3) or by using cursor.
// The sp_destroy(3) function is used to discard changes of a multi-statement transaction. All modifications that were made during the transaction are not written to the log file.
// No nested transactions are supported.

// sp_commit(transaction): commit a transaction
// The sp_commit(3) function is used to apply changes of a multi-statement transaction. All modifications that were made during the transaction are written to the log file in a single batch.
// If committion failed, transaction modifications are discarded.


// sp_cursor(ctl) Create cursor over all configuration values (single order only).
// sp_cursor(database, object) Create a database cursor. Object might have position key and iteration order set.
// Supported orders: ">", ">=", "<, "<=". using sp_set(object, "order", "<=");


// object = sp_object(database): create new object for a transaction on selected database.
// The sp_object function returns an object which is intended to be used in set/get operations. Object might contain a key-value pair with any additional metadata.
// sp_object(cursor): get current positioned object.


// sp_delete(database, object) Do a single-statement transaction.
// sp_delete(transaction, object) Do a key deletion as a part of multi-statement transaction.
// sp_drop(snapshot) Delete snapshot.
// sp_drop(database) Close a database (v1.2.1 equal to sp_destroy(3)).
//	int sp_destroy(transaction);

// sp_set(sp_ctl(env), "snapshot", "today");
// snapshot = sp_get(ctl, "snapshot.today");

// database, snapshot, async_database

// sp_set(ctl, "key", "value") Set configuration variable value or call a system procedure.
// sp_get(ctl, "key") Get configuration variable value object.

// sp_set(database, object) Do a single-statement transaction.
//sp_get(database, object) Do a single-statement transaction.

// sp_set(transaction, object) Do a key update as a part of multi-statement transaction.
//sp_get(transaction, object) Do a key search as a part of multi-statement transaction visibility.

// sp_set(object, "field", ptr, size) Set or update an object field.
// uint32_t size; sp_get(object, "field", &size) Get an object field and its size. Size can be NULL.


// void *sp_type(void*, ...);

/*Snapshots represent Point-in-Time read-only database view.

It is possible to do sp_get(3) or sp_cursor(3) on snapshot object. To create a snapshot, new snapshot name should be set to snapshot control namespace.

void *ctl = sp_ctl(env);
sp_set(ctl, "snapshot", "today");
void *snapshot = sp_get(ctl, "snapshot.today");
Snapshots are not persistent, therefore snapshot object must be recreated after shutdown before opening environment with latest snapshot LSN number: snapshot.name.lsn.

void *ctl = sp_ctl(env);
sp_set(ctl, "snapshot", "today");
sp_set(ctl, "snapshot.today.lsn", "12345");
To delete a snapshot, sp_drop(3) or sp_destroy(3) should be called on snapshot object.*/


// sp_set(ctl, "backup.run");
// sp_set(ctl, "scheduler.checkpoint");
// Database monitoring is possible by getting current dynamic statistics via sp_ctl(3) object.

// sp_set(ctl, "db.database.lockdetect", a) == 1;

void *sophia_ptr(char *str)
{
    void *ptr = 0;
    if ( strlen(str) == (2 * sizeof(ptr)) )
        decode_hex((uint8_t *)&ptr,sizeof(ptr),str);
    return(ptr);
}

void *sophia_fieldptr(cJSON *json,char *fieldstr)
{
    void *ptr = 0;
    char str[MAX_JSON_FIELD];
    copy_cJSON(str,cJSON_GetObjectItem(json,fieldstr));
    if ( strlen(str) == (2 * sizeof(ptr)) )
        decode_hex((uint8_t *)&ptr,sizeof(ptr),str);
    return(ptr);
}

void sophia_retptrstr(char *retbuf,char *resultname,char *fieldname,void *ptr)
{
    char datastr[sizeof(ptr)*2 + 1];
    init_hexbytes_noT(datastr,(uint8_t *)&ptr,sizeof(ptr));
    sprintf(retbuf,"{\"result\":\"%s\",\"%s\":\"%s\"}",resultname,fieldname,datastr);
}

void sophia_retintstr(char *retbuf,char *resultname,int32_t retval)
{
    char datastr[sizeof(retval)*2 + 1];
    init_hexbytes_noT(datastr,(uint8_t *)&retval,sizeof(retval));
    sprintf(retbuf,"{\"method\":\"%s\",\"%s\":%d}",resultname,retval==0?"result":"error",retval);
}

void *sophia_funcptrcall(char *retbuf,int32_t max,cJSON *json,int32_t (*funcp)(void *,...),char *argfield)
{
    void *arg;
    char argptrstr[MAX_JSON_FIELD];
    copy_cJSON(argptrstr,cJSON_GetObjectItem(json,argfield));
    if ( (arg= sophia_ptr(argptrstr)) != 0 )
        (*funcp)(retbuf,(arg));
    return(arg);
}

void sophia_env(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"env","env",sp_env()); }

void sophia_ctl(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"ctl","ctl",sp_ctl(sophia_fieldptr(json,"env"))); }

void sophia_begin(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"begin","tx",sp_ctl(sophia_fieldptr(json,"env"))); }

void sophia_async(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"async","asyncdb",sp_async(sophia_fieldptr(json,"db"))); }

void sophia_object(char *retbuf,int32_t max,cJSON *json)
{
    char *retname = "object";
    void *ptr;
    if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
        ptr = sophia_fieldptr(json,"cursor"), retname = "current";
    sophia_retptrstr(retbuf,"object",retname,sp_object(ptr));
}

void sophia_destroy(char *retbuf,int32_t max,cJSON *json) { sophia_retintstr(retbuf,"destroy",sp_destroy(sophia_fieldptr(json,"ptr"))); }

void sophia_commit(char *retbuf,int32_t max,cJSON *json) { sophia_retintstr(retbuf,"commit",sp_error(sophia_fieldptr(json,"env"))); }

void sophia_error(char *retbuf,int32_t max,cJSON *json) { sophia_retintstr(retbuf,"error",sp_error(sophia_fieldptr(json,"env"))); }

void sophia_open(char *retbuf,int32_t max,cJSON *json)
{
    void *ptr;
    if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
        ptr = sophia_fieldptr(json,"env");
    sophia_retintstr(retbuf,"open",sp_open(ptr));
}

void sophia_delete(char *retbuf,int32_t max,cJSON *json)
{
    void *ptr,*obj;
    if ( (obj= sophia_fieldptr(json,"object")) == 0 )
    {
        if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
            ptr = sophia_fieldptr(json,"tx");
        sophia_retintstr(retbuf,"open",sp_delete(ptr,obj));
    } else sprintf(retbuf,"{\"error\":\"sophia_delete cant get object\"}");
}

void sophia_drop(char *retbuf,int32_t max,cJSON *json)
{
    void *ptr;
    if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
        ptr = sophia_fieldptr(json,"snapshot");
    sophia_retintstr(retbuf,"drop",sp_drop(ptr));
}

void sophia_cursor(char *retbuf,int32_t max,cJSON *json)
{
    void **obj;
    if ( (obj= sophia_fieldptr(json,"object")) == 0 )
        sophia_retptrstr(retbuf,"cursor","cursor",sp_cursor(sophia_fieldptr(json,"db"),obj));
    else sprintf(retbuf,"{\"error\":\"sophia_delete cant get object\"}");
}

void sophia_type(char *retbuf,int32_t max,cJSON *json)
{
    
}

void sophia_setget(char *retbuf,int32_t max,cJSON *json)
{
    char *keystr,*valstr,*ctlstr,*dbstr=0,*asyncstr=0; void *ctl,*db=0,*asyncdb=0,*obj,*value,*item;
    uint32_t size; int32_t err;
    keystr = cJSON_str(cJSON_GetObjectItem(json,"key"));
    valstr = cJSON_str(cJSON_GetObjectItem(json,"value"));
    if ( keystr != 0 && keystr[0] != 0 )
    {
        if ( (ctlstr= cJSON_str(cJSON_GetObjectItem(json,"ctl"))) != 0 )
        {
            if ( ctlstr[0] != 0 && (ctl= sophia_ptr(ctlstr)) != 0 )
            {
                if ( valstr != 0 )
                {
                    if ( (err= sp_set(ctl,keystr,valstr)) == 0 )
                        sprintf(retbuf,"{\"result\":\"setctl\",\"keystr\":\"%s\"}",valstr);
                    else strcpy(retbuf,"{\"error\":\"couldnt setctl\"}");
                }
                else
                {
                    if ( (value= sp_get(ctl,keystr,NULL)) != 0 )
                    {
                        if ( strlen(value) < max-64 )
                            sprintf(retbuf,"{\"result\":\"getctl\",\"keystr\":\"%s\"}",value);
                        else strcpy(retbuf,"{\"error\":\"value size too big\"}");
                        sp_destroy(value);
                    }
                }
            }
        }
        else if ( (dbstr= cJSON_str(cJSON_GetObjectItem(json,"db"))) != 0 || (asyncstr= cJSON_str(cJSON_GetObjectItem(json,"asyncdb"))) != 0 )
        {
            if ( dbstr[0] != 0 )
                db = sophia_ptr(dbstr);
            if ( asyncstr[0] != 0 )
                asyncdb = sophia_ptr(asyncstr);
            if ( db != 0 && (obj= sp_object(db)) != 0 )
            {
                if ( (err= sp_set(obj,"key",keystr,strlen(keystr))) == 0 )
                {
                    if ( (valstr= cJSON_str(cJSON_GetObjectItem(json,"value"))) != 0 )
                    {
                        if ( (err= sp_set(obj,"value",valstr,strlen(valstr))) != 0 )
                            strcpy(retbuf,"{\"error\":\"cant set object value\"}");
                        else if ( (err= sp_set(asyncdb!=0?asyncdb:db,obj)) != 0 )
                            strcpy(retbuf,"{\"error\":\"cant save object value\"}");
                        else strcpy(retbuf,"{\"result\":\"key/value set\"}");
                    }
                    else
                    {
                        if ( (item= sp_get(db,obj)) != 0 )
                        {
                            if ( (value= sp_get(item,"value",&size)) != 0 )
                            {
                                if ( size < max-64 )
                                    sprintf(retbuf,"{\"result\":\"getkey\",\"keystr\":\"%s\"}",value);
                                else strcpy(retbuf,"{\"error\":\"value size too big\"}");
                                sp_destroy(value);
                            } else strcpy(retbuf,"{\"error\":\"cant get value from item\"}");
                            sp_destroy(item);
                        } else strcpy(retbuf,"{\"error\":\"cant find item\"}");
                    }
                } else strcpy(retbuf,"{\"error\":\"cant set object key\"}");
                sp_destroy(obj);
            } else strcpy(retbuf,"{\"error\":\"invalid db or cant allocate object\"}");
        }
        else
        {
            //sp_get(transaction, object) Do a key search as a part of multi-statement transaction visibility.
            strcpy(retbuf,"{\"error\":\"unsupported transaction or search case\"}");
        }
    }
    else strcpy(retbuf,"{\"error\":\"missing key\"}");
}

int32_t db777_getind(struct db777 *DB)
{
    int32_t i;
    for (i=0; i<SOPHIA.numdbs; i++)
        if ( SOPHIA.DBS[i] == DB )
            return(i);
    return(-1);
}

struct db777 *db777_getDB(char *name)
{
    int32_t i;
    for (i=0; i<SOPHIA.numdbs; i++)
    {
        printf("(%s) ",SOPHIA.DBS[i]->name);
        if ( strcmp(SOPHIA.DBS[i]->name,name) == 0 )
            return(SOPHIA.DBS[i]);
    }
    return(0);
}

cJSON *db777_json(struct db777 *DB)
{
    void *cursor,*ptr,*ctl = sp_ctl(DB->env);
    char *key,*value;
    cJSON *json = cJSON_CreateObject();
    cursor = sp_cursor(ctl);
    while ( (ptr= sp_get(cursor)) != 0 )
    {
        key = sp_get(ptr,"key",NULL), value = sp_get(ptr,"value",NULL);
        cJSON_AddItemToObject(json,key,cJSON_CreateString(value!=0?value:""));
    }
    sp_destroy(cursor);
    return(json);
}

int32_t db777_free(struct db777 *DB)
{
    int32_t errs = 0;
    //if ( DB->asyncdb != 0 && sp_destroy(DB->asyncdb) != 0 )
    //    errs |= 2;
    if ( DB->db != 0 && sp_destroy(DB->db) != 0 )
        errs |= 1;
    if ( DB->env != 0 && sp_destroy(DB->env) != 0 )
        errs |= 8;
    free(DB);
    return(errs);
}

void *db777_abort(struct db777 *DB)
{
    db777_free(DB);
    return(0);
}

int32_t db777_close(struct db777 *DB)
{
    int32_t i,errs = 0;
    if ( DB != 0 && SOPHIA.numdbs > 0 )
    {
        for (i=0; i<SOPHIA.numdbs; i++)
        {
            if ( DB == SOPHIA.DBS[i] )
            {
                SOPHIA.DBS[i] = SOPHIA.DBS[--SOPHIA.numdbs], SOPHIA.DBS[SOPHIA.numdbs] = 0;
                if ( (errs= db777_free(DB)) != 0 )
                {
                    printf("sp_close DB.%p errs.%d\n",DB,errs);
                    return(-errs);
                }
                return(0);
            }
        }
    }
    errs |= 4;
    printf("couldnt find DB.%p errs.%d\n",DB,errs);
    return(-errs);
}

struct db777 *db777_create(char *specialpath,char *subdir,char *name,char *compression)
{
    struct db777 *DB = calloc(1,sizeof(*DB));
    char path[1024],dbname[1024],*str;
    int32_t err;
    cJSON *json;
    strcpy(dbname,name);
    if ( strlen(dbname) >= sizeof(DB->name)-1 )
        dbname[sizeof(DB->name)-1] = 0;
    strcpy(DB->name,dbname);
    DB->env = sp_env();
    DB->ctl = sp_ctl(DB->env);
    if ( SOPHIA.PATH[0] == '.' && SOPHIA.PATH[1] == '/' )
        strcpy(path,SOPHIA.PATH+2);
    else strcpy(path,SOPHIA.PATH);
    ensure_directory(path);
    if ( specialpath != 0 )
    {
        printf("path.(%s) special.(%s) subdir.(%s) name.(%s)\n",path,specialpath,subdir,name);
        sprintf(path + strlen(path),"/%s",specialpath), ensure_directory(path);
        if ( subdir != 0 && subdir[0] != 0 )
            sprintf(path + strlen(path),"/%s",subdir), ensure_directory(path);
        /*if ( strcmp(specialpath,"RAMCHAINS") == 0 )
        {
            strcat(path,"/"), strcat(path,subdir);
            ensure_directory(path);
            printf("path.(%s)\n",path);
        }
        else if ( strcmp(specialpath,"MGW") == 0 )
            strcat(path,"/"), strcmp(path,MGW.PATH);
        else if ( strcmp(specialpath,"DATADIR") == 0 )
            strcat(path,"/"), strcmp(path,SUPERNET.DATADIR);*/
    }
    strcat(path,"/"), strcat(path,name), os_compatible_path(path), ensure_directory(path);
    strcpy(DB->backupdir,path), strcat(DB->backupdir,"/backups");
    strcpy(dbname,compression != 0 && compression[0] != 0 ? compression : name);
    printf("SOPHIA.(%s) MGW.(%s) create path.(%s).(%s) -> [%s %s].%s\n",SOPHIA.PATH,MGW.PATH,name,compression,path,dbname,compression);
    if ( (err= sp_set(DB->ctl,"sophia.path",path)) != 0 )
    {
        printf("err.%d setting path\n",err);
        return(db777_abort(DB));
    }
    if ( (err= sp_set(DB->ctl,"scheduler.threads","1")) != 0 )
    {
        printf("err.%d setting scheduler.threads\n",err);
        return(db777_abort(DB));
    }
    sprintf(DB->dbname,"db.%s",dbname);
    if ( (err= sp_set(DB->ctl,"db",dbname)) != 0 )
    {
        printf("err.%d setting name\n",err);
        return(db777_abort(DB));
    }
    if ( (err= sp_set(DB->ctl,"backup.path",DB->backupdir)) != 0 )
        printf("error.%d setting backup.path (%s)\n",err,DB->backupdir);
    if ( compression != 0 )
    {
        sprintf(dbname,"db.%s.compression",name);
        if ( strcmp(compression,"lz4") == 0 )
        {
            if ( sp_set(DB->ctl,"db.lz4.compression","lz4") != 0 )
                printf("error setting lz4.(%s)\n",dbname);
        }
        else if ( strcmp(compression,"zstd") == 0 )
        {
            if ( sp_set(DB->ctl,"db.zstd.compression","zstd") != 0 )
                printf("error setting zstd.(%s)\n",dbname);
        }
        else printf("compression.(%s) not supported\n",compression), compression = "";
    } else compression = "";
    if ( (err= sp_open(DB->env)) != 0 )
    {
        printf("err.%d setting sp_open\n",err);
        return(db777_abort(DB));
    }
    if ( (DB->db= sp_get(DB->ctl,DB->dbname)) == 0 )
    {
        if ( (err= sp_open(DB->db)) != 0 )
        {
            printf("err.%d sp_open db.(%s)\n",err,DB->dbname);
            return(db777_abort(DB));
        }
        printf("created.(%s)\n",DB->dbname);
    }
    else
    {
        DB->asyncdb = sp_async(DB->db);
        printf("opened.(%s) env.%p ctl.%p db.%p asyncdb.%p\n",DB->dbname,DB->env,DB->ctl,DB->db,DB->asyncdb);
    }
    if ( 0 && (json= db777_json(DB)) != 0 )
    {
        str = cJSON_Print(json);
        printf("%s\n",str);
        free(str);
        free_json(json);
    }
    //SOPHIA.DBS = realloc(SOPHIA.DBS,(SOPHIA.numdbs+1) * sizeof(*DB));
    if ( SOPHIA.numdbs < sizeof(SOPHIA.DBS)/sizeof(*SOPHIA.DBS) )
        SOPHIA.DBS[SOPHIA.numdbs] = DB, SOPHIA.numdbs++;
    return(DB);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char *method,*name,*key,*value,*compression,*resultstr,*path,*subdir;
    struct db777 *DB;
    int32_t len,offset;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        strcpy(retbuf,"{\"result\":\"initflag > 0\"}");
    }
    else
    {
        name = cJSON_str(cJSON_GetObjectItem(json,"name"));
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        method = cJSON_str(cJSON_GetObjectItem(json,"method"));
        path = cJSON_str(cJSON_GetObjectItem(json,"path"));
        subdir = cJSON_str(cJSON_GetObjectItem(json,"subdir"));
        if ( method == 0 || name == 0 || name[0] == 0 )
        {
            printf("(%s) has not method or dbname\n",jsonstr);
            return(0);
        }
        printf("SOPHIA.(%s) for (%s)\n",method,name);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(method,"create") == 0 )
        {
            if ( strstr("../",name) != 0 || strstr("..\\",name) != 0 || name[0] == '/' || name[0] == '\\' || strcmp(name,"..") == 0  || strcmp(name,"*") == 0 )
                strcpy(retbuf,"{\"error\":\"no funny filenames\"}");
            else if ( path != 0 && (strcmp(path,"RAMCHAINS") != 0 || strcmp(path,"MGW") != 0 || strcmp(path,"DATADIR") != 0) )
                strcpy(retbuf,"{\"error\":\"only RAMCHAINS/MGW/DATADIR path allowed\"}");
            else
            {
                compression = cJSON_str(cJSON_GetObjectItem(json,"compression"));
                if ( (DB= db777_create(path,subdir,name,compression)) != 0 )
                {
                    strcpy(retbuf,"{\"result\":\"opened database\"}");
                }
                else strcpy(retbuf,"{\"error\":\"couldnt create database\"}");
            }
        }
        else if ( strcmp(method,"add") == 0 )
        {
            key = cJSON_str(cJSON_GetObjectItem(json,"key"));
            value = cJSON_str(cJSON_GetObjectItem(json,"value"));
            if ( key != 0 && key[0] != 0 && (DB= db777_getDB(name)) != 0 )
                sophia_retintstr(retbuf,"add",db777_addstr(DB,key,value));
            else strcpy(retbuf,"{\"error\":\"couldnt find database\"}");
        }
        else if ( strcmp(method,"find") == 0 )
        {
            key = cJSON_str(cJSON_GetObjectItem(json,"key"));
            if ( key != 0 && key[0] != 0 && (DB= db777_getDB(name)) != 0 )
            {
                strcpy(retbuf,"{\"method\":\"find\",\"result\":\"");
                offset = (int32_t)strlen(retbuf);
                if ( (len= db777_findstr(retbuf + offset,maxlen - offset,DB,key)) > 0 )
                    retbuf[offset + len] = 0, sprintf(retbuf + strlen(retbuf),"\",\"len\":%d}",len);
                else strcpy(retbuf,"{\"error\":\"couldnt find key\"}");
            }
        }
        else if ( strcmp(method,"close") == 0 )
        {
            if ( (DB= db777_getDB(name)) != 0 )
                sophia_retintstr(retbuf,"close",db777_close(DB));
            else strcpy(retbuf,"{\"error\":\"couldnt find dbname\"}");
        }
#ifdef BUNDLED
        else if ( strcmp(method,"object") == 0 )
            sophia_object(retbuf,maxlen,json);
        else if ( strcmp(method,"env") == 0 )
            sophia_env(retbuf,maxlen,json);
        else if ( strcmp(method,"ctl") == 0 )
            sophia_ctl(retbuf,maxlen,json);
        else if ( strcmp(method,"open") == 0 )
            sophia_open(retbuf,maxlen,json);
        else if ( strcmp(method,"destroy") == 0 )
            sophia_destroy(retbuf,maxlen,json);
        else if ( strcmp(method,"error") == 0 )
            sophia_error(retbuf,maxlen,json);
        else if ( strcmp(method,"delete") == 0 )
            sophia_delete(retbuf,maxlen,json);
        else if ( strcmp(method,"async") == 0 )
            sophia_async(retbuf,maxlen,json);
        else if ( strcmp(method,"drop") == 0 )
            sophia_drop(retbuf,maxlen,json);
        else if ( strcmp(method,"cursor") == 0 )
            sophia_cursor(retbuf,maxlen,json);
        else if ( strcmp(method,"begin") == 0 )
            sophia_begin(retbuf,maxlen,json);
        else if ( strcmp(method,"commit") == 0 )
            sophia_commit(retbuf,maxlen,json);
        else if ( strcmp(method,"type") == 0 )
            sophia_type(retbuf,maxlen,json);
#endif
        else sprintf(retbuf,"{\"error\":\"invalid sophia method\",\"method\":\"%s\",\"tag\":\"%llu\"}",method,(long long)tag);
        if ( retbuf[0] == 0 )
            sprintf(retbuf,"{\"error\":\"null return\",\"method\":\"%s\",\"tag\":\"%llu\"}",method,(long long)tag);
        else sprintf(retbuf + strlen(retbuf) - 1,",\"tag\":%llu}",(long long)tag);
    }
    return((int32_t)strlen(retbuf));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    while ( SOPHIA.numdbs > 0 )
        if ( db777_close(SOPHIA.DBS[0]) != 0 )
            break;
    return(retcode);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    // runtime specific state can be created and put into *data
    ensure_directory(SOPHIA.PATH);
    return(disableflags); // set bits corresponding to array position in _methods[]
}

#include "../plugin777.c"