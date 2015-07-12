//
//  relays777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//
// sync relays
// and then also to make sure adding relays on the fly syncs up to the current set of serviceproviders
// btc38
// join protocol + anti-sybil
// ipv6 got_newpeer.([2a03:b0c0:0:1010::e2:b001]:14631)

// "servicesecret" in SuperNET.conf
// register: ./BitcoinDarkd SuperNET '{"plugin":"relay","method":"busdata","destplugin":"relay","submethod":"serviceprovider","servicename":"echo","endpoint":""}'
// ./BitcoinDarkd SuperNET '{"method":"busdata","plugin":"relay","servicename":"echo","serviceNXT":"4273301882745002507","destplugin":"echodemo","submethod":"echo","echostr":"remote echo"}'

#define BUNDLED
#define PLUGINSTR "relay"
#define PLUGNAME(NAME) relay ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "utils/system777.c"
#include "utils/NXT777.c"
#include "plugin777.c"
#include "utils/SaM.c"
#undef DEFINES_ONLY

int32_t issue_generateToken(char encoded[NXT_TOKEN_LEN],char *key,char *origsecret)
{
    char cmd[16384],secret[8192],token[MAX_JSON_FIELD+2*NXT_TOKEN_LEN+1],*jsontxt; cJSON *tokenobj,*json;
    encoded[0] = 0;
    escape_code(secret,origsecret);
    sprintf(cmd,"requestType=generateToken&website=%s&secretPhrase=%s",key,secret);
    if ( (jsontxt= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsontxt);
        if ( (json= cJSON_Parse(jsontxt)) != 0 )
        {
            //printf("(%s) -> token.(%s)\n",cmd,cJSON_Print(json));
            tokenobj = cJSON_GetObjectItem(json,"token");
            copy_cJSON(token,tokenobj);
            if ( encoded != 0 )
                strcpy(encoded,token);
            free_json(json);
        }
        free(jsontxt);
        return(0);
    }
    return(-1);
}

uint32_t calc_nonce(char *str,int32_t leverage,int32_t maxmillis,uint32_t nonce)
{
    uint64_t hit,threshold; bits384 sig; double endmilli; int32_t len;
    len = (int32_t)strlen(str);
    if ( leverage != 0 )
    {
        threshold = calc_SaMthreshold(leverage);
        if ( maxmillis == 0 )
        {
            if ( (hit= calc_SaM(&sig,(void *)str,len,(void *)&nonce,sizeof(nonce))) >= threshold )
            {
                printf("nonce failure hit.%llu >= threshold.%llu\n",(long long)hit,(long long)threshold);
                if ( (threshold - hit) > ((uint64_t)1L << 32) )
                    return(0xffffffff);
                else return((uint32_t)(threshold - hit));
            }
        }
        else
        {
            endmilli = (milliseconds() + maxmillis);
            while ( milliseconds() < endmilli )
            {
                randombytes((void *)&nonce,sizeof(nonce));
                if ( (hit= calc_SaM(&sig,(void *)str,len,(void *)&nonce,sizeof(nonce))) < threshold )
                    return(nonce);
            }
        }
    }
    return(0);
}

int32_t nonce_leverage(char *broadcaststr)
{
    int32_t leverage = 4;
    if ( broadcaststr != 0 && broadcaststr[0] != 0 )
    {
        if ( strcmp(broadcaststr,"allnodes") == 0 )
            leverage = 6;
        else if ( strcmp(broadcaststr,"join") == 0 )
            leverage = 9;
        else if ( strcmp(broadcaststr,"servicerequest") == 0 )
            leverage = 6;
        else if ( strcmp(broadcaststr,"allrelays") == 0 )
            leverage = 5;
        else if ( atoi(broadcaststr) != 0 )
            leverage = atoi(broadcaststr);
    }
    return(leverage);
}

char *get_broadcastmode(cJSON *json,char *broadcastmode)
{
    char servicename[MAX_JSON_FIELD],*bstr;
    copy_cJSON(servicename,cJSON_GetObjectItem(json,"servicename"));
    if ( servicename[0] != 0 )
        broadcastmode = "servicerequest";
    else if ( (bstr= cJSON_str(cJSON_GetObjectItem(json,"broadcast"))) != 0 )
        return(bstr);
    //printf("(%s) get_broadcastmode.(%s) servicename.[%s]\n",cJSON_Print(json),broadcastmode!=0?broadcastmode:"",servicename);
    return(broadcastmode);
}

uint32_t nonce_func(int32_t *leveragep,char *str,char *broadcaststr,int32_t maxmillis,uint32_t nonce)
{
    int32_t leverage = nonce_leverage(broadcaststr);
    if ( maxmillis == 0 && *leveragep != leverage )
        return(0xffffffff);
    *leveragep = leverage;
    return(calc_nonce(str,leverage,maxmillis,nonce));
}

int32_t construct_tokenized_req(uint32_t *noncep,char *tokenized,char *cmdjson,char *NXTACCTSECRET,char *broadcastmode)
{
    char encoded[2*NXT_TOKEN_LEN+1],ftoken[2*NXT_TOKEN_LEN+1],ftokenstr[2*NXT_TOKEN_LEN+128],broadcaststr[512]; uint32_t nonce,nonceerr; int32_t i,leverage,n = 100;
    *noncep = 0;
    if ( broadcastmode == 0 )
        broadcastmode = "";
    _stripwhite(cmdjson,' ');
    for (i=0; i<n; i++)
    {
        if ( (nonce= nonce_func(&leverage,cmdjson,broadcastmode,5000,0)) != 0 )
            break;
        printf("iter.%d of %d couldnt find nonce, try again\n",i,n);
    }
    if ( (nonceerr= nonce_func(&leverage,cmdjson,broadcastmode,0,nonce)) != 0 )
    {
        printf("error validating nonce.%u -> %u\n",nonce,nonceerr);
        tokenized[0] = 0;
        return(0);
    }
    *noncep = nonce;
    sprintf(broadcaststr,",\"broadcast\":\"%s\",\"usedest\":\"yes\",\"nonce\":\"%u\",\"leverage\":\"%u\"",broadcastmode,nonce,leverage);
    //sprintf(broadcaststr,",\"broadcast\":\"%s\",\"usedest\":\"yes\"",broadcastmode);
    //printf("GEN.(%s).(%s) -> (%s) len.%d crc.%u\n",broadcastmode,cmdjson,broadcaststr,(int32_t)strlen(cmdjson),_crc32(0,(void *)cmdjson,(int32_t)strlen(cmdjson)));
    issue_generateToken(encoded,cmdjson,NXTACCTSECRET);
    if ( strcmp(NXTACCTSECRET,SUPERNET.NXTACCTSECRET) != 0 )
    {
        issue_generateToken(ftoken,cmdjson,SUPERNET.NXTACCTSECRET);
        sprintf(ftokenstr,",\"ftoken\":\"%s\"",ftoken);
    } else ftokenstr[0] = 0;
    encoded[NXT_TOKEN_LEN] = ftoken[NXT_TOKEN_LEN] = 0;
    if ( SUPERNET.iamrelay == 0 )
        sprintf(tokenized,"[%s, {\"token\":\"%s\"%s}]",cmdjson,encoded,broadcaststr);
    else if ( NXTACCTSECRET == GENESIS_SECRET )
        sprintf(tokenized,"[%s, {\"token\":\"%s\"\"forwarder\":\"%s\"%s}]",cmdjson,encoded,GENESISACCT,broadcaststr);
    else sprintf(tokenized,"[%s, {\"token\":\"%s\",\"forwarder\":\"%s\"%s%s}]",cmdjson,encoded,SUPERNET.NXTADDR,ftokenstr,broadcaststr);
    return((int32_t)strlen(tokenized)+1);
}

int32_t issue_decodeToken(char *sender,int32_t *validp,char *key,uint8_t encoded[NXT_TOKEN_LEN])
{
    char cmd[4096],token[MAX_JSON_FIELD+2*NXT_TOKEN_LEN+1],*retstr;
    cJSON *nxtobj,*validobj,*json;
    *validp = -1;
    sender[0] = 0;
    memcpy(token,encoded,NXT_TOKEN_LEN);
    token[NXT_TOKEN_LEN] = 0;
    sprintf(cmd,"requestType=decodeToken&website=%s&token=%s",key,token);
    if ( (retstr = issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            validobj = cJSON_GetObjectItem(json,"valid");
            if ( validobj != 0 )
                *validp = ((validobj->type&0xff) == cJSON_True) ? 1 : 0;
            nxtobj = cJSON_GetObjectItem(json,"account");
            copy_cJSON(sender,nxtobj);
            free_json(json), free(retstr);
            //printf("decoded valid.%d NXT.%s len.%d\n",*validp,sender,(int32_t)strlen(sender));
            if ( sender[0] != 0 )
                return((int32_t)strlen(sender));
            else return(0);
        }
        free(retstr);
    }
    return(-1);
}

int32_t validate_token(char *forwarder,char *pubkey,char *NXTaddr,char *tokenizedtxt,int32_t strictflag)
{
    cJSON *array=0,*firstitem=0,*tokenobj,*obj; uint32_t nonce; int64_t timeval,diff = 0; int32_t valid,leverage,retcode = -13;
    char buf[MAX_JSON_FIELD],serviceNXT[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],broadcaststr[MAX_JSON_FIELD],*broadcastmode,*firstjsontxt = 0;
    uint8_t encoded[4096];
    array = cJSON_Parse(tokenizedtxt);
    NXTaddr[0] = pubkey[0] = forwarder[0] = 0;
    if ( array == 0 )
    {
        printf("couldnt validate.(%s)\n",tokenizedtxt);
        return(-2);
    }
    if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
    {
        firstitem = cJSON_GetArrayItem(array,0);
        if ( pubkey != 0 )
        {
            obj = cJSON_GetObjectItem(firstitem,"pubkey");
            copy_cJSON(pubkey,obj);
        }
        obj = cJSON_GetObjectItem(firstitem,"NXT"), copy_cJSON(buf,obj);
        obj = cJSON_GetObjectItem(firstitem,"serviceNXT"), copy_cJSON(serviceNXT,obj);
        if ( NXTaddr[0] != 0 && strcmp(buf,NXTaddr) != 0 )
            retcode = -3;
        else
        {
            strcpy(NXTaddr,buf);
//printf("decoded.(%s)\n",NXTaddr);
            if ( strictflag != 0 )
            {
                timeval = get_cJSON_int(firstitem,"time");
                diff = (timeval - time(NULL));
                if ( diff < -60 )
                    retcode = -6;
                else if ( diff > strictflag )
                {
                    printf("time diff %lld too big %lld vs %ld\n",(long long)diff,(long long)timeval,time(NULL));
                    retcode = -5;
                }
            }
            if ( retcode != -5 && retcode != -6 )
            {
                firstjsontxt = cJSON_Print(firstitem), _stripwhite(firstjsontxt,' ');
//printf("(%s)\n",firstjsontxt);
                tokenobj = cJSON_GetArrayItem(array,1);
                obj = cJSON_GetObjectItem(tokenobj,"token");
                copy_cJSON((char *)encoded,obj);
                copy_cJSON(forwarder,cJSON_GetObjectItem(tokenobj,"forwarder"));
                memset(sender,0,sizeof(sender));
                valid = -1;
                if ( issue_decodeToken(sender,&valid,firstjsontxt,encoded) > 0 )
                {
                    if ( NXTaddr[0] == 0 )
                        strcpy(NXTaddr,sender);
                    nonce = (uint32_t)get_API_int(cJSON_GetObjectItem(tokenobj,"nonce"),0);
                    leverage = (uint32_t)get_API_int(cJSON_GetObjectItem(tokenobj,"leverage"),0);
                    copy_cJSON(broadcaststr,cJSON_GetObjectItem(tokenobj,"broadcast"));
                    broadcastmode = get_broadcastmode(firstitem,broadcaststr);
                    retcode = valid;
                    if ( nonce_func(&leverage,firstjsontxt,broadcastmode,0,nonce) != 0 )
                    {
                        //printf("(%s) -> (%s) leverage.%d len.%d crc.%u\n",broadcaststr,firstjsontxt,leverage,len,_crc32(0,(void *)firstjsontxt,len));
                        retcode = -4;
                    }
                    if ( Debuglevel > 2 )
                        printf("signed by valid NXT.%s valid.%d diff.%lld forwarder.(%s)\n",sender,valid,(long long)diff,forwarder);
                    if ( strcmp(sender,NXTaddr) != 0 && strcmp(sender,serviceNXT) != 0 )
                    {
                        printf("valid.%d diff sender.(%s) vs NXTaddr.(%s) serviceNXT.(%s)\n",valid,sender,NXTaddr,serviceNXT);
                        //if ( strcmp(NXTaddr,buf) == 0 )
                        //    retcode = valid;
                        retcode = -7;
                    }
                } else printf("decode error\n");
                if ( retcode < 0 )
                    printf("err.%d: signed by invalid sender.(%s) NXT.%s valid.%d or timediff too big diff.%lld, buf.(%s)\n",retcode,sender,NXTaddr,valid,(long long)diff,tokenizedtxt);
                free(firstjsontxt);
            }
        }
    } else printf("decode arraysize.%d\n",cJSON_GetArraySize(array));
    if ( array != 0 )
        free_json(array);
    if ( retcode < 0 )
        printf("ret.%d signed by valid NXT.%s valid.%d diff.%lld forwarder.(%s) nonce.%u\n",retcode,sender,valid,(long long)diff,forwarder,nonce);
    return(retcode);
}

void nn_syncbus(cJSON *json)
{
    cJSON *argjson,*second; char forwarder[MAX_JSON_FIELD],*jsonstr; uint64_t forwardbits,nxt64bits;
    //printf("pubsock.%d iamrelay.%d arraysize.%d\n",RELAYS.pubsock,SUPERNET.iamrelay,cJSON_GetArraySize(json));
    if ( RELAYS.pubrelays >= 0 && SUPERNET.iamrelay != 0 && is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
    {
        argjson = cJSON_GetArrayItem(json,0);
        second = cJSON_GetArrayItem(json,1);
        copy_cJSON(forwarder,cJSON_GetObjectItem(second,"forwarder"));
        ensure_jsonitem(second,"forwarder",SUPERNET.NXTADDR);
        jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
        forwardbits = conv_acctstr(forwarder), nxt64bits = conv_acctstr(SUPERNET.NXTADDR);
        if ( forwardbits == 0 )//|| forwardbits == nxt64bits )
        {
            if ( Debuglevel > 2 )
                printf("BUS-SEND.(%s) forwarder.%llu vs %llu\n",jsonstr,(long long)forwardbits,(long long)nxt64bits);
            nn_send(RELAYS.pubrelays,jsonstr,(int32_t)strlen(jsonstr)+1,0);
        }
        free(jsonstr);
    }
}

queue_t busdataQ[2];
struct busdata_item { struct queueitem DL; bits256 hash; cJSON *json; char *retstr,*key; uint64_t dest64bits,senderbits; uint32_t queuetime,donetime; };
struct service_provider { UT_hash_handle hh; int32_t sock; } *Service_providers;
struct serviceprovider { uint64_t servicebits; char name[32],endpoint[64]; };

void free_busdata_item(struct busdata_item *ptr)
{
    if ( ptr->json != 0 )
        free_json(ptr->json);
    if ( ptr->retstr != 0 )
        free(ptr->retstr);
    if ( ptr->key != 0 )
        free(ptr->key);
    free(ptr);
}

char *lb_serviceprovider(struct service_provider *sp,uint8_t *data,int32_t datalen)
{
    int32_t i,sendlen,recvlen; char *msg,*jsonstr = 0;
    for (i=0; i<10; i++)
        if ( (nn_socket_status(sp->sock,1) & NN_POLLOUT) != 0 )
            break;
    if ( Debuglevel > 2 )
        printf("lb_serviceprovider.(%s)\n",data);
    if ( (sendlen= nn_send(sp->sock,data,datalen,0)) == datalen )
    {
        for (i=0; i<10; i++)
            if ( (nn_socket_status(sp->sock,1) & NN_POLLIN) != 0 )
                break;
        if ( (recvlen= nn_recv(sp->sock,&msg,NN_MSG,0)) > 0 )
        {
            printf("servicerecv.(%s)\n",msg);
            jsonstr = clonestr((char *)msg);
            nn_freemsg(msg);
        } else printf("lb_serviceprovider timeout\n");
    } else printf("sendlen.%d != datalen.%d\n",sendlen,datalen);
    return(jsonstr);
}

void *serviceprovider_iterator(struct kv777 *kv,void *_ptr,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    char numstr[64]; struct serviceprovider *S = key; cJSON *item,*array = _ptr;
    if ( keysize == sizeof(*S) )
    {
        item = cJSON_CreateObject();
        cJSON_AddItemToObject(item,S->name,cJSON_CreateString(S->endpoint));
        sprintf(numstr,"%llu",(long long)S->servicebits), cJSON_AddItemToObject(item,"serviceNXT",cJSON_CreateString(numstr));
        cJSON_AddItemToArray(array,item);
        return(0);
    }
    printf("unexpected services entry size.%d/%d vs %ld? abort serviceprovider_iterator\n",keysize,valuesize,sizeof(*S));
    return(KV777_ABORTITERATOR);
}

struct connectargs { char *servicename,*endpoint; int32_t sock; };
void *serviceconnect_iterator(struct kv777 *kv,void *_ptr,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    struct serviceprovider *S = key; struct connectargs *ptr = _ptr;
    if ( keysize == sizeof(*S) && strcmp(ptr->servicename,S->name) == 0 && S->servicebits != 0 )
    {
        nn_connect(ptr->sock,S->endpoint), printf("SERVICEPROVIDER CONNECT ");
        if ( ptr->endpoint != 0 && strcmp(S->endpoint,ptr->endpoint) == 0 )
            ptr->endpoint = 0;
    }
    printf("%24llu %16s %s\n",(long long)S->servicebits,S->name,S->endpoint);
    return(0);
}

cJSON *serviceprovider_json()
{
    cJSON *json,*array;
    json = cJSON_CreateObject(), array = cJSON_CreateArray();
    kv777_iterate(SUPERNET.services,array,0,serviceprovider_iterator);
    cJSON_AddItemToObject(json,"services",array);
    return(json);
}

uint32_t find_serviceprovider(struct serviceprovider *S)
{
    uint32_t *timestampp; int32_t len = sizeof(*timestampp);
    if ( (timestampp= kv777_read(SUPERNET.services,S,sizeof(*S),0,&len)) != 0 && len == sizeof(uint32_t) )
        return(*timestampp);
    return(0);
}

void set_serviceprovider(struct serviceprovider *S,char *serviceNXT,char *servicename,char *endpoint)
{
    memset(S,0,sizeof(*S));
    S->servicebits = conv_acctstr(serviceNXT);
    strncpy(S->name,servicename,sizeof(S->name)-1);
    strncpy(S->endpoint,endpoint,sizeof(S->endpoint)-1);
    S->endpoint[sizeof(S->endpoint)-1] = 0;
}

int32_t remove_service_provider(char *serviceNXT,char *servicename,char *endpoint)
{
    struct serviceprovider S;
    set_serviceprovider(&S,serviceNXT,servicename,endpoint);
    return(kv777_delete(SUPERNET.services,&S,sizeof(S)));
}

int32_t add_serviceprovider(struct serviceprovider *S,uint32_t timestamp)
{
    if ( kv777_write(SUPERNET.services,S,sizeof(*S),&timestamp,sizeof(timestamp)) != 0 )
        return(0);
    return(-1);
}

int32_t add_service_provider(char *serviceNXT,char *servicename,char *endpoint)
{
    struct serviceprovider S;
    set_serviceprovider(&S,serviceNXT,servicename,endpoint);
    if ( find_serviceprovider(&S) == 0 )
        add_serviceprovider(&S,(uint32_t)time(NULL));
    return(0);
}

struct service_provider *find_servicesock(char *servicename,char *endpoint)
{
    struct service_provider *sp,*checksp; int32_t sendtimeout,recvtimeout,retrymillis,maxmillis; struct connectargs args;
    HASH_FIND(hh,Service_providers,servicename,strlen(servicename),sp);
    if ( sp == 0 )
    {
        printf("Couldnt find service.(%s)\n",servicename);
        sp = calloc(1,sizeof(*sp));
        HASH_ADD_KEYPTR(hh,Service_providers,servicename,strlen(servicename),sp);
        sp->hh.key = clonestr(servicename);
        HASH_FIND(hh,Service_providers,servicename,strlen(servicename),checksp);
        if ( checksp != sp )
        {
            printf("checksp.%p != %p\n",checksp,sp);
        }
        if ( (sp->sock= nn_socket(AF_SP,NN_REQ)) >= 0 )
        {
            sendtimeout = 1000, recvtimeout = 10000, maxmillis = 3000, retrymillis = 100;
            if ( sendtimeout > 0 && nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            else if ( recvtimeout > 0 && nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            else if ( nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RECONNECT_IVL,&retrymillis,sizeof(retrymillis)) < 0 )
                fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
            else if ( nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&maxmillis,sizeof(maxmillis)) < 0 )
                fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
            args.servicename = servicename, args.endpoint = endpoint, args.sock = sp->sock;
            kv777_iterate(SUPERNET.services,&args,0,serviceconnect_iterator); // scan DB and nn_connect
        }
    } // else printf("sp.%p found servicename.(%s) sock.%d\n",sp,servicename,sp->sock);
    if ( endpoint != 0 )
    {
        fprintf(stderr,"create servicename.(%s) sock.%d <-> (%s)\n",servicename,sp->sock,endpoint);
        nn_connect(sp->sock,endpoint);
    }
    return(sp);
}

char *busdata_addpending(char *destNXT,char *sender,char *key,uint32_t timestamp,cJSON *json,char *forwarder,cJSON *origjson)
{
    cJSON *argjson; struct busdata_item *ptr; bits256 hash; struct service_provider *sp; int32_t valid;
    char submethod[512],servicecmd[512],endpoint[512],destplugin[512],retbuf[128],serviceNXT[128],servicename[512],servicetoken[512],*hashstr,*str,*retstr;
    if ( key == 0 || key[0] == 0 )
        key = "0";
     if ( (hashstr= cJSON_str(cJSON_GetObjectItem(json,"H"))) != 0 )
        decode_hex(hash.bytes,sizeof(hash),hashstr);
    else memset(hash.bytes,0,sizeof(hash));
    copy_cJSON(submethod,cJSON_GetObjectItem(json,"submethod"));
    copy_cJSON(destplugin,cJSON_GetObjectItem(json,"destplugin"));
    copy_cJSON(servicename,cJSON_GetObjectItem(json,"servicename"));
    copy_cJSON(servicecmd,cJSON_GetObjectItem(json,"servicecmd"));
    //printf("addpending.(%s %s).%s\n",destplugin,servicename,submethod);
    if ( strcmp(submethod,"serviceprovider") == 0 )
    {
        copy_cJSON(endpoint,cJSON_GetObjectItem(json,"endpoint"));
        copy_cJSON(servicetoken,cJSON_GetObjectItem(json,"servicetoken"));
        if ( issue_decodeToken(serviceNXT,&valid,endpoint,(void *)servicetoken) > 0 )
            printf("valid.(%s) from serviceNXT.%s\n",endpoint,serviceNXT);
        if ( strcmp(servicecmd,"remove") == 0 )
        {
            remove_service_provider(serviceNXT,servicename,endpoint);
            sprintf(retbuf,"{\"result\":\"serviceprovider endpoint removed\",\"endpoint\":\"%s\",\"serviceNXT\":\"%s\"}",endpoint,serviceNXT);
        }
        else if ( serviceNXT[0] == 0 || is_decimalstr(serviceNXT) == 0 || calc_nxt64bits(serviceNXT) == 0 )
            return(clonestr("{\"error\":\"no serviceNXT\"}"));
        else
        {
            if ( add_service_provider(serviceNXT,servicename,endpoint) == 0 )
                find_servicesock(servicename,endpoint);
            else find_servicesock(servicename,0);
            find_servicesock(servicename,0);
            sprintf(retbuf,"{\"result\":\"serviceprovider added\",\"endpoint\":\"%s\",\"serviceNXT\":\"%s\"}",endpoint,serviceNXT);
        }
        nn_syncbus(origjson);
        return(clonestr(retbuf));
    }
    else if ( servicename[0] != 0 )
    {
        copy_cJSON(serviceNXT,cJSON_GetObjectItem(json,"serviceNXT"));
        printf("service.%s (%s) serviceNXT.%s\n",servicename,submethod,serviceNXT);
        if ( (sp= find_servicesock(servicename,0)) == 0 )
            return(clonestr("{\"result\":\"serviceprovider not found\"}"));
        else
        {
            //HASH_FIND(hh,Service_providers,servicename,strlen(servicename),sp);
            argjson = cJSON_Duplicate(origjson,1);
            ensure_jsonitem(cJSON_GetArrayItem(argjson,1),"usedest","yes");
            str = cJSON_Print(argjson), _stripwhite(str,' ');
            free_json(argjson);
            if ( (retstr= lb_serviceprovider(sp,(uint8_t *)str,(int32_t)strlen(str)+1)) != 0 )
            {
                free(str);
                if ( Debuglevel > 2 )
                    printf("LBS.(%s)\n",retstr);
                return(retstr);
            }
            free(str);
            return(clonestr("{\"result\":\"no response from provider\"}"));
        }
    } else return(0);
    ptr = calloc(1,sizeof(*ptr));
    ptr->json = cJSON_Duplicate(json,1), ptr->queuetime = (uint32_t)time(NULL), ptr->key = clonestr(key);
    ptr->dest64bits = conv_acctstr(destNXT), ptr->senderbits = conv_acctstr(sender);
    if ( (hashstr= cJSON_str(cJSON_GetObjectItem(json,"H"))) != 0 )
        decode_hex(ptr->hash.bytes,sizeof(ptr->hash),hashstr);
    else memset(ptr->hash.bytes,0,sizeof(ptr->hash));
    printf("%s -> %s add pending %llx\n",sender,destNXT,(long long)ptr->hash.txid);
    queue_enqueue("busdata",&busdataQ[0],&ptr->DL);
    return(0);
}

uint8_t *encode_str(int32_t *cipherlenp,void *str,int32_t len,bits256 destpubkey,bits256 myprivkey,bits256 mypubkey)
{
    uint8_t *buf,*nonce,*cipher,*ptr;
    buf = calloc(1,len + crypto_box_NONCEBYTES + crypto_box_ZEROBYTES + sizeof(mypubkey));
    ptr = cipher = calloc(1,len + crypto_box_NONCEBYTES + crypto_box_ZEROBYTES + sizeof(mypubkey));
    memcpy(cipher,mypubkey.bytes,sizeof(mypubkey));
    nonce = &cipher[sizeof(mypubkey)];
    randombytes(nonce,crypto_box_NONCEBYTES);
    cipher = &nonce[crypto_box_NONCEBYTES];
   //printf("len.%ld -> %ld %ld\n",len,len+crypto_box_ZEROBYTES,len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES);
    memset(cipher,0,len+crypto_box_ZEROBYTES);
    memset(buf,0,crypto_box_ZEROBYTES);
    memcpy(buf+crypto_box_ZEROBYTES,str,len);
    crypto_box(cipher,buf,len+crypto_box_ZEROBYTES,nonce,destpubkey.bytes,myprivkey.bytes);
    free(buf);
    *cipherlenp = ((int32_t)len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES + sizeof(mypubkey));
    return(ptr);
}

int32_t decode_cipher(uint8_t *str,uint8_t *cipher,int32_t *lenp,uint8_t *myprivkey)
{
    bits256 srcpubkey; uint8_t *nonce; int i,err,len = *lenp;
    memcpy(srcpubkey.bytes,cipher,sizeof(srcpubkey)), cipher += sizeof(srcpubkey), len -= sizeof(srcpubkey);
    nonce = cipher;
    cipher += crypto_box_NONCEBYTES, len -= crypto_box_NONCEBYTES;
    err = crypto_box_open((uint8_t *)str,cipher,len,nonce,srcpubkey.bytes,myprivkey);
    for (i=0; i<len-crypto_box_ZEROBYTES; i++)
        str[i] = str[i+crypto_box_ZEROBYTES];
    *lenp = len - crypto_box_ZEROBYTES;
    return(err);
}

cJSON *privatemessage_encrypt(uint64_t destbits,char *pmstr)
{
    uint8_t *cipher; bits256 destpubkey,onetime_pubkey,onetime_privkey; cJSON *strjson;
    char *hexstr,destNXT[64]; int32_t len,haspubkey,cipherlen; uint32_t crc;
    expand_nxt64bits(destNXT,destbits);
    destpubkey = issue_getpubkey(&haspubkey,destNXT);
    crypto_box_keypair(onetime_pubkey.bytes,onetime_privkey.bytes);
    len = (int32_t)strlen(pmstr);
    cipher = encode_str(&cipherlen,pmstr,len,destpubkey,onetime_privkey,onetime_pubkey);
    if ( haspubkey == 0 || cipher == 0 )
    {
        printf("destNXT.%s has no pubkey\n",destNXT);
        return(cJSON_CreateString(""));
    }
    //printf("[%s].%d ",pmstr,len);
    crc = _crc32(0,cipher,cipherlen);
    hexstr = malloc((cipherlen + sizeof(uint32_t) + 1)*2 + 1);
    init_hexbytes_noT(hexstr,(void *)&crc,sizeof(crc));
    init_hexbytes_noT(&hexstr[sizeof(crc) << 1],(void *)cipher,cipherlen + 1);
    //printf("len.%d crc.%x encrypt.(%s) -> (%s) dest.%llu\n",len,crc,pmstr,hexstr,(long long)destbits);
    strjson = cJSON_CreateString(hexstr);
    free(hexstr), free(cipher);
    return(strjson);
}

int32_t privatemessage_decrypt(uint8_t *databuf,int32_t len,char *datastr)
{
    char *pmstr,*decoded; cJSON *json; int32_t len2,n,len3; uint32_t crc,checkcrc;
    //printf("decoded.(%s) -> (%s)\n",datastr,databuf);
    if ( (json= cJSON_Parse((char *)databuf)) != 0 )
    {
        if ( (pmstr= cJSON_str(cJSON_GetObjectItem(json,"PM"))) != 0 )
        {
            sprintf((void *)databuf,"{\"method\":\"PM\",\"PM\":\"");
            len2 = (int32_t)strlen(pmstr) >> 1;
            n = (int32_t)strlen((char *)databuf);
            decode_hex(&databuf[n],len2,pmstr);
            memcpy(&crc,&databuf[n],sizeof(uint32_t));
            len3 = (int32_t)(len2 - 1 - (int32_t)sizeof(crc));
            checkcrc = _crc32(0,&databuf[n + sizeof(crc)],len3);
            if ( crc != checkcrc )
            {
                databuf[0] = 0;
                //printf("(%s) crc.%x != checkcrc.%x len.%d\n",databuf,crc,checkcrc,len3);
            }
            else
            {
                //printf("crc matched\n");
                decoded = calloc(1,len3);
                if ( decode_cipher((void *)decoded,&databuf[n + sizeof(crc)],&len3,SUPERNET.myprivkey) == 0 )
                    sprintf((char *)databuf,"{\"method\":\"PM\",\"PM\":\"%s\"}",decoded);
                else databuf[0] = 0;//, printf("decrypt error.(%s)\n",decoded);
                free(decoded);
            }
        } //else printf("no PM str\n");
    }
    return(len);
}

char *privatemessage_recv(char *jsonstr)
{
    uint32_t ind; cJSON *argjson; struct kv777_item *ptr; char *pmstr = 0;
    if ( (argjson= cJSON_Parse(jsonstr)) != 0 )
    {
        pmstr = cJSON_str(cJSON_GetObjectItem(argjson,"PM"));
        if ( SUPERNET.PM != 0 && pmstr != 0 )
        {
            printf("ind.%d ",SUPERNET.PM->numkeys);
            ind = SUPERNET.PM->numkeys;
            ptr = kv777_write(SUPERNET.PM,&ind,sizeof(ind),pmstr,(int32_t)strlen(pmstr)+1);
            kv777_flush();
        }
        printf("privatemessage_recv.(%s)\n",pmstr!=0?pmstr:"<no message>");
    }
    return(clonestr("{\"result\":\"success\",\"action\":\"privatemessage received\"}"));
}

char *busdata(char *tokenstr,char *forwarder,char *sender,int32_t valid,char *key,uint32_t timestamp,uint8_t *msg,int32_t datalen,cJSON *origjson)
{
    cJSON *json; char destNXT[64],*retstr = 0;
    if ( SUPERNET.iamrelay != 0 && valid > 0 )
    {
        if ( (json= cJSON_Parse((void *)msg)) != 0 )
        {
            if ( (retstr= busdata_addpending(destNXT,sender,key,timestamp,json,forwarder,origjson)) == 0 )
                nn_syncbus(origjson);
            free_json(json);
        } else printf("couldnt decode.(%s)\n",msg);
    }
    if ( Debuglevel > 2 )
        printf("busdata.(%s) valid.%d -> (%s)\n",msg,valid,retstr!=0?retstr:"");
    return(retstr);
}

int32_t busdata_validate(char *forwarder,char *sender,uint32_t *timestamp,uint8_t *databuf,int32_t *datalenp,void *msg,cJSON *json)
{
    char pubkey[256],hexstr[65],sha[65],datastr[8192],fforwarder[512],fsender[512]; int32_t valid,fvalid; cJSON *argjson; bits256 hash;
    *timestamp = *datalenp = 0; forwarder[0] = sender[0] = 0;
    //printf("busdata_validate.(%s)\n",msg);
    if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
    {
        argjson = cJSON_GetArrayItem(json,0);
        *timestamp = (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,"time"),0);
        if ( (valid= validate_token(forwarder,pubkey,sender,msg,(*timestamp != 0) * MAXTIMEDIFF)) <= 0 )
        {
            fprintf(stderr,"error valid.%d sender.(%s) forwarder.(%s)\n",valid,sender,forwarder);
            return(valid);
        }
        if ( strcmp(forwarder,sender) != 0 && (fvalid= validate_token(fforwarder,pubkey,fsender,msg,(*timestamp != 0) * MAXTIMEDIFF)) <= 0 )
        {
            fprintf(stderr,"error fvalid.%d fsender.(%s) fforwarder.(%s)\n",fvalid,fsender,fforwarder);
            return(fvalid);
        }
        copy_cJSON(datastr,cJSON_GetObjectItem(argjson,"data"));
        if ( strcmp(sender,SUPERNET.NXTADDR) != 0 || datastr[0] != 0 )
        {
            copy_cJSON(sha,cJSON_GetObjectItem(argjson,"H"));
            if ( datastr[0] != 0 )
                decode_hex(databuf,(int32_t)(strlen(datastr)+1)>>1,datastr);
            else databuf[0] = 0;
            *datalenp = (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,"n"),0);
            calc_sha256(hexstr,hash.bytes,databuf,*datalenp);
            if ( strcmp(hexstr,sha) == 0 )
            {
                *datalenp = privatemessage_decrypt(databuf,*datalenp,datastr);
                return(1);
            }
            else printf("hash mismatch %s vs %s\n",hexstr,sha);
        }
        else
        {
            strcpy((char *)databuf,msg);
            *datalenp = (int32_t)strlen((char *)databuf) + 1;
            return(1);
        }
    } else printf("busdata_validate not array (%s)\n",msg);
    return(-1);
}

char *busdata_duppacket(cJSON *json)
{
    cJSON *argjson,*second; char method[MAX_JSON_FIELD],*str;
    argjson = cJSON_GetArrayItem(json,0);
    second = cJSON_GetArrayItem(json,1);
    copy_cJSON(method,cJSON_GetObjectItem(argjson,"method"));
    if ( strcmp(method,"PM") == 0 )
        ensure_jsonitem(second,"forwarder",GENESISACCT);
    else ensure_jsonitem(second,"forwarder",SUPERNET.NXTADDR);
    ensure_jsonitem(second,"usedest","yes");
    ensure_jsonitem(second,"stop","yes");
    cJSON_DeleteItemFromObject(second,"broadcast");
    str = cJSON_Print(json), _stripwhite(str,' ');
    return(str);
}

char *busdata_deref(char *tokenstr,char *forwarder,char *sender,int32_t valid,char *databuf,cJSON *json)
{
    char plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],buf[MAX_JSON_FIELD],servicename[MAX_JSON_FIELD],*broadcaststr,*str=0,*retstr = 0;
    cJSON *dupjson,*second,*argjson,*origjson; uint64_t forwardbits; uint32_t ind;
    if ( SUPERNET.iamrelay != 0 && (broadcaststr= cJSON_str(cJSON_GetObjectItem(cJSON_GetArrayItem(json,1),"broadcast"))) != 0 )
    {
        dupjson = cJSON_Duplicate(json,1);
        argjson = cJSON_GetArrayItem(dupjson,0);
        copy_cJSON(method,cJSON_GetObjectItem(argjson,"method"));
        second = cJSON_GetArrayItem(dupjson,1);
        if ( cJSON_GetObjectItem(second,"forwarder") == 0 )
        {
            if ( (forwardbits= conv_acctstr(forwarder)) == 0 && cJSON_GetObjectItem(second,"stop") == 0 )
            {
                str = busdata_duppacket(dupjson);
                if ( RELAYS.pubrelays >= 0 && (strcmp(broadcaststr,"allrelays") == 0 || strcmp(broadcaststr,"join") == 0) )
                {
                    printf("[%s] broadcast.(%s) forwarder.%llu vs %s\n",broadcaststr,str,(long long)forwardbits,SUPERNET.NXTADDR);
                    nn_send(RELAYS.pubrelays,str,(int32_t)strlen(str)+1,0);
                }
                else if ( RELAYS.pubglobal >= 0 && strcmp(broadcaststr,"allnodes") == 0 )
                {
                    printf("ALL [%s] broadcast.(%s) forwarder.%llu vs %s\n",broadcaststr,str,(long long)forwardbits,SUPERNET.NXTADDR);
                    nn_send(RELAYS.pubglobal,str,(int32_t)strlen(str)+1,0);
                    if ( strcmp(method,"PM") == 0 )
                    {
                        if ( SUPERNET.rawPM != 0 )
                        {
                            ind = SUPERNET.rawPM->numkeys;
                            printf("RELAYSAVE.(%s)\n",str);
                            dKV777_write(SUPERNET.relays,SUPERNET.rawPM,calc_nxt64bits(sender),str,(int32_t)strlen(str)+1), kv777_flush();
                        }
                        free(str);
                        free_json(dupjson);
                        return(clonestr("{\"result\":\"success\",\"action\":\"privatemessage broadcast\"}"));
                    }
                }
                //free(str);
            } // else printf("forwardbits.%llu stop.%p\n",(long long)forwardbits,cJSON_GetObjectItem(second,"stop"));
        }
        free_json(dupjson);
    }
    argjson = cJSON_GetArrayItem(json,0);
    copy_cJSON(method,cJSON_GetObjectItem(argjson,"method"));
    if ( strcmp(method,"PM") == 0 )
    {
        printf("got PM.(%s)\n",databuf);
        if ( SUPERNET.iamrelay != 0 )
        {
            if ( str == 0 )
            {
                dupjson = cJSON_Duplicate(json,1);
                str = busdata_duppacket(dupjson);
                free_json(dupjson);
            }
            if ( SUPERNET.rawPM != 0 )
            {
                ind = SUPERNET.rawPM->numkeys;
                printf("RELAYSAVE2.(%s)\n",str);
                dKV777_write(SUPERNET.relays,SUPERNET.rawPM,calc_nxt64bits(sender),str,(int32_t)strlen(str)+1), kv777_flush();
            }
            free(str);
            return(clonestr("{\"result\":\"success\",\"action\":\"privatemessage ignored\"}"));
        }
        else return(privatemessage_recv(databuf));
    }
    if ( str != 0 )
        free(str);
    if ( (origjson= cJSON_Parse(databuf)) != 0 )
    {
        if ( is_cJSON_Array(origjson) != 0 && cJSON_GetArraySize(origjson) == 2 )
        {
            argjson = cJSON_GetArrayItem(origjson,0);
            copy_cJSON(buf,cJSON_GetObjectItem(argjson,"NXT"));
            if ( strcmp(buf,SUPERNET.NXTADDR) != 0 )
            {
                printf("tokenized json not local.(%s)\n",databuf);
                free_json(origjson);
                return(clonestr("{\"error\":\"tokenized json not local\"}"));
            }
        }
        else argjson = origjson;
        copy_cJSON(plugin,cJSON_GetObjectItem(argjson,"destplugin"));
        if ( plugin[0] == 0 )
            copy_cJSON(plugin,cJSON_GetObjectItem(argjson,"destagent"));
        copy_cJSON(method,cJSON_GetObjectItem(argjson,"submethod"));
        copy_cJSON(buf,cJSON_GetObjectItem(argjson,"method"));
        copy_cJSON(servicename,cJSON_GetObjectItem(argjson,"servicename"));
        if ( Debuglevel > 2 )
            printf("relay.%d buf.(%s) method.(%s) servicename.(%s) token.(%s)\n",SUPERNET.iamrelay,buf,method,servicename,tokenstr!=0?tokenstr:"");
        if ( SUPERNET.iamrelay != 0 && ((strcmp(buf,"busdata") == 0 && strcmp(method,"serviceprovider") == 0) || servicename[0] != 0) ) //
        {
printf("bypass deref (%s) (%s) (%s)\n",buf,method,servicename);
            free_json(origjson);
            return(0);
        }
        cJSON_ReplaceItemInObject(argjson,"method",cJSON_CreateString(method));
        cJSON_ReplaceItemInObject(argjson,"plugin",cJSON_CreateString(plugin));
        cJSON_DeleteItemFromObject(argjson,"submethod");
        cJSON_DeleteItemFromObject(argjson,"destplugin");
        str = cJSON_Print(argjson), _stripwhite(str,' ');
        if ( Debuglevel > 2 )
            printf("call (%s %s) (%s)\n",plugin,method,str);
        retstr = plugin_method(-1,0,0,plugin,method,0,0,str,(int32_t)strlen(str)+1,SUPERNET.PLUGINTIMEOUT/2,tokenstr);
        free_json(origjson);
        free(str);
    }
    return(retstr);
}

char *nn_busdata_processor(uint8_t *msg,int32_t len)
{
    cJSON *json,*argjson,*dupjson,*tokenobj = 0; uint32_t timestamp; int32_t datalen,valid = -2; uint8_t databuf[8192]; uint64_t destbits;
    char usedest[128],key[MAX_JSON_FIELD],src[MAX_JSON_FIELD],destNXT[MAX_JSON_FIELD],forwarder[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],*str,*tokenstr=0,*broadcaststr,*retstr = 0;
    if ( Debuglevel > 2 )
        fprintf(stderr,"nn_busdata_processor.(%s)\n",msg);
    if ( (json= cJSON_Parse((char *)msg)) != 0 && is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
    {
        argjson = cJSON_GetArrayItem(json,0);
        tokenobj = cJSON_GetArrayItem(json,1);
        if ( (valid= busdata_validate(forwarder,sender,&timestamp,databuf,&datalen,msg,json)) > 0 )
        {
            if ( datalen <= 0 )
            {
                free_json(json);
                return(clonestr("{\"result\":\"no data decrypted\"}"));
            }
            copy_cJSON(destNXT,cJSON_GetObjectItem(argjson,"destNXT"));
            destbits = conv_acctstr(destNXT), expand_nxt64bits(destNXT,destbits);
            if ( destNXT[0] == 0 || strcmp(destNXT,SUPERNET.NXTADDR) == 0 || strcmp(destNXT,SUPERNET.SERVICENXT) == 0 )
            {
                if ( cJSON_GetObjectItem(tokenobj,"valid") != 0 )
                    cJSON_DeleteItemFromObject(tokenobj,"valid");
                if ( cJSON_GetObjectItem(tokenobj,"sender") != 0 )
                    cJSON_DeleteItemFromObject(tokenobj,"sender");
                cJSON_AddItemToObject(tokenobj,"valid",cJSON_CreateNumber(valid));
                cJSON_AddItemToObject(tokenobj,"sender",cJSON_CreateString(sender));
                tokenstr = cJSON_Print(tokenobj), _stripwhite(tokenstr,' ');
                copy_cJSON(src,cJSON_GetObjectItem(argjson,"NXT"));
                copy_cJSON(key,cJSON_GetObjectItem(argjson,"key"));
                copy_cJSON(usedest,cJSON_GetObjectItem(cJSON_GetArrayItem(json,1),"usedest"));
                if ( usedest[0] != 0 )
                    retstr = busdata_deref(tokenstr,forwarder,sender,valid,(char *)databuf,json);
                if ( retstr == 0 )
                    retstr = busdata(tokenstr,forwarder,sender,valid,key,timestamp,databuf,datalen,json);
            }
 //printf("valid.%d forwarder.(%s) sender.(%s) src.%-24s key.(%s) datalen.%d\n",valid,forwarder,sender,src,key,datalen);
        }
        else if ( RELAYS.pubglobal >= 0 && SUPERNET.iamrelay != 0 && argjson != 0 && tokenobj != 0 && (broadcaststr= cJSON_str(cJSON_GetObjectItem(tokenobj,"broadcast"))) != 0 && strcmp(broadcaststr,"allnodes") == 0 && cJSON_GetObjectItem(argjson,"stop") == 0 )
        {
            dupjson = cJSON_Duplicate(json,1);
            if ( cJSON_GetObjectItem(tokenobj,"stop") == 0 )
            {
                tokenobj = cJSON_GetArrayItem(dupjson,1);
                cJSON_DeleteItemFromObject(tokenobj,"broadcast");
                ensure_jsonitem(tokenobj,"stop","yes");
                str = cJSON_Print(dupjson), _stripwhite(str,' ');
                printf("[%s] blind broadcast.(%s) by %s\n",broadcaststr,str,SUPERNET.NXTADDR);
                nn_send(RELAYS.pubglobal,str,(int32_t)strlen((char *)str)+1,0);
                free(str);
                retstr = clonestr("{\"result\":\"success\",\"broadcast\":\"allnodes\"}");
            } else retstr = clonestr("{\"error\":\"already stop\",\"broadcast\":\"nowhere\"}");
            free_json(dupjson);
        }
        else retstr = clonestr("{\"error\":\"busdata doesnt validate\"}");
        if ( tokenstr != 0 )
            free(tokenstr);
        free_json(json);
    } else retstr = clonestr("{\"error\":\"couldnt parse busdata\"}");
    if ( Debuglevel > 2 )
        fprintf(stderr,"BUSDATA.(%s) -> %p.(%s)\n",msg,retstr,retstr);
    return(retstr);
}

int32_t is_duplicate_tag(uint64_t tag)
{
    static uint64_t Tags[8192]; static int32_t nextj; int32_t j;
    for (j=0; j<sizeof(Tags)/sizeof(*Tags); j++)
    {
        if ( Tags[j] == 0 )
        {
            nextj = j;
            break;
        }
        else if ( Tags[j] == tag )
        {
            fprintf(stderr,"skip duplicate tag.%llu\n",(long long)tag);
            return(1);
        }
    }
    if ( j == sizeof(Tags)/sizeof(*Tags) || Tags[j] == 0 )
    {
        //fprintf(stderr,"Tag[%d] <-- %llu\n",nextj,(long long)tag);
        Tags[nextj++ % (sizeof(Tags)/sizeof(*Tags))] = tag;
    }
    return(0);
}

char *create_busdata(int32_t *sentflagp,uint32_t *noncep,int32_t *datalenp,char *_jsonstr,char *broadcastmode,char *destNXTaddr)
{
    char key[MAX_JSON_FIELD],method[MAX_JSON_FIELD],plugin[MAX_JSON_FIELD],destNXT[MAX_JSON_FIELD],servicetoken[NXT_TOKEN_LEN+1],endpoint[128],hexstr[65],numstr[65],*newmethod;
    char *str,*str2,*jsonstr,*tokbuf = 0,*tmp,*secret,*pmstr;
    bits256 hash; uint64_t destbits,nxt64bits,tag; uint16_t port; uint32_t timestamp; cJSON *datajson,*json,*second,*dupjson=0; int32_t tlen,diff,datalen = 0;
    *sentflagp = *datalenp = *noncep = 0;
    if ( Debuglevel > 2 )
        printf("create_busdata.(%s).%s -> %s\n",_jsonstr,broadcastmode!=0?broadcastmode:"",destNXTaddr!=0?destNXTaddr:"");
    if ( (json= cJSON_Parse(_jsonstr)) != 0 )
    {
        if ( broadcastmode != 0 && strcmp(broadcastmode,"publicaccess") != 0 )
        {
            if ( cJSON_GetObjectItem(json,"tag") != 0 )
            {
                dupjson = cJSON_Duplicate(json,1);
                cJSON_DeleteItemFromObject(dupjson,"tag");
                jsonstr = cJSON_Print(dupjson);
            } else jsonstr = _jsonstr;
            _stripwhite(jsonstr,' ');
            calc_sha256(0,hash.bytes,(void *)jsonstr,(int32_t)strlen(jsonstr));
            if ( is_duplicate_tag(hash.txid) != 0 )
            {
                if ( jsonstr != _jsonstr )
                    free(jsonstr);
                return(0);
            }
            if ( jsonstr != _jsonstr )
                free(jsonstr);
            if ( dupjson != 0 )
                free_json(dupjson);
        }
        jsonstr = _jsonstr;
        if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
        {
            *datalenp = (int32_t)strlen(jsonstr) + 1;
            second = cJSON_GetArrayItem(json,1);
            *sentflagp = (cJSON_GetObjectItem(cJSON_GetArrayItem(json,0),"stop") != 0 || cJSON_GetObjectItem(second,"stop") != 0);
            ensure_jsonitem(second,"stop","yes");
            ensure_jsonitem(second,"usedest","yes");
            free_json(json);
            return(jsonstr);
        }
        broadcastmode = get_broadcastmode(json,broadcastmode);
        if ( broadcastmode != 0 && strcmp(broadcastmode,"join") == 0 )
            diff = 60, port = SUPERNET.port + LB_OFFSET;
        else diff = 0, port = SUPERNET.serviceport;
        *sentflagp = (cJSON_GetObjectItem(json,"stop") != 0);
        copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
        copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
        if ( plugin[0] == 0 )
            copy_cJSON(plugin,cJSON_GetObjectItem(json,"agent"));
        if ( destNXTaddr != 0 )
            strcpy(destNXT,destNXTaddr);
        else destNXT[0] = 0;
        if ( (destbits= conv_acctstr(destNXTaddr)) != 0 && (pmstr= cJSON_str(cJSON_GetObjectItem(json,"PM"))) != 0 )
        {
            //printf("destbits.%llu (%s)\n",(long long)destbits,destNXT);
            cJSON_ReplaceItemInObject(json,"PM",privatemessage_encrypt(destbits,pmstr));
            newmethod = "PM";
            cJSON_ReplaceItemInObject(json,"method",cJSON_CreateString(newmethod));
            secret = GENESIS_SECRET;
            cJSON_DeleteItemFromObject(json,"destNXT");
        } else secret = SUPERNET.NXTACCTSECRET, newmethod = "busdata";
        if ( cJSON_GetObjectItem(json,"endpoint") != 0 )
        {
            if ( broadcastmode != 0 && strcmp(broadcastmode,"join") == 0 )
            {
                ensure_jsonitem(json,"lbendpoint",SUPERNET.lbendpoint);
                ensure_jsonitem(json,"relaypoint",SUPERNET.relayendpoint);
                ensure_jsonitem(json,"globalpoint",SUPERNET.globalendpoint);
                strcpy(endpoint,SUPERNET.lbendpoint);
            }
            sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,port);
            cJSON_ReplaceItemInObject(json,"endpoint",cJSON_CreateString(endpoint));
            if ( secret != GENESIS_SECRET && SUPERNET.SERVICESECRET[0] != 0 && issue_generateToken(servicetoken,endpoint,SUPERNET.SERVICESECRET) == 0 )
            {
                cJSON_AddItemToObject(json,"servicetoken",cJSON_CreateString(servicetoken));
                secret = SUPERNET.SERVICESECRET;
            }
        }
        if ( broadcastmode != 0 && broadcastmode[0] != 0 )
        {
            cJSON_ReplaceItemInObject(json,"method",cJSON_CreateString(newmethod));
            cJSON_ReplaceItemInObject(json,"plugin",cJSON_CreateString("relay"));
            cJSON_AddItemToObject(json,"submethod",cJSON_CreateString(method));
            //if ( strcmp(plugin,"relay") != 0 )
                cJSON_AddItemToObject(json,"destplugin",cJSON_CreateString(plugin));
        }
        if ( (tag= get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"))) == 0 )
        {
            randombytes((uint8_t *)&tag,sizeof(tag));
            sprintf(numstr,"%llu",(long long)tag);
            cJSON_AddItemToObject(json,"tag",cJSON_CreateString(numstr));
        } else sprintf(numstr,"%llu",(long long)tag);
        timestamp = (uint32_t)time(NULL);
        copy_cJSON(key,cJSON_GetObjectItem(json,"key"));
        datajson = cJSON_CreateObject();
        cJSON_AddItemToObject(datajson,"tag",cJSON_CreateString(numstr));
        if ( broadcastmode != 0 && broadcastmode[0] != 0 )
            cJSON_AddItemToObject(datajson,"broadcast",cJSON_CreateString(broadcastmode));
        cJSON_AddItemToObject(datajson,"plugin",cJSON_CreateString("relay"));
        cJSON_AddItemToObject(datajson,"key",cJSON_CreateString(key));
        cJSON_AddItemToObject(datajson,"time",cJSON_CreateNumber(timestamp + diff));
        if ( secret != GENESIS_SECRET )
        {
            cJSON_AddItemToObject(datajson,"method",cJSON_CreateString("busdata"));
            if ( SUPERNET.SERVICESECRET[0] != 0 )
                cJSON_AddItemToObject(datajson,"serviceNXT",cJSON_CreateString(SUPERNET.SERVICENXT));
            nxt64bits = conv_acctstr(SUPERNET.NXTADDR);
            sprintf(numstr,"%llu",(long long)nxt64bits), cJSON_AddItemToObject(datajson,"NXT",cJSON_CreateString(numstr));
        }
        else cJSON_AddItemToObject(datajson,"method",cJSON_CreateString("PM"));
        //ensure_jsonitem(datajson,"stop","yes");
        str = cJSON_Print(json), _stripwhite(str,' ');
        datalen = (int32_t)(strlen(str) + 1);
        tmp = malloc((datalen << 1) + 1);
        init_hexbytes_noT(tmp,(void *)str,datalen);
        cJSON_AddItemToObject(datajson,"data",cJSON_CreateString(tmp));
        calc_sha256(hexstr,hash.bytes,(uint8_t *)str,datalen);
        cJSON_AddItemToObject(datajson,"n",cJSON_CreateNumber(datalen));
        cJSON_AddItemToObject(datajson,"H",cJSON_CreateString(hexstr));
        str2 = cJSON_Print(datajson), _stripwhite(str2,' ');
        tokbuf = calloc(1,strlen(str2) + 4096);
        tlen = construct_tokenized_req(noncep,tokbuf,str2,secret,broadcastmode);
        if ( Debuglevel > 2 )
            printf("method.(%s) created busdata.(%s) -> (%s) tlen.%d\n",method,str,tokbuf,tlen);
        free(tmp), free(str), free(str2), str = str2 = 0;
        *datalenp = tlen;
        if ( jsonstr != _jsonstr )
            free(jsonstr);
        free_json(json);
    } else printf("couldnt parse busdata json.(%s)\n",_jsonstr);
    return(tokbuf);
}

char *busdata_sync(uint32_t *noncep,char *jsonstr,char *broadcastmode,char *destNXTaddr)
{
    struct applicant_info apply,*ptr;
    int32_t sentflag,datalen,sendlen = 0; char plugin[512],destplugin[512],*data,*retstr,*submethod; cJSON *json;
    json = cJSON_Parse(jsonstr);
    copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
    copy_cJSON(destplugin,cJSON_GetObjectItem(json,"destplugin"));
    if ( destplugin[0] == 0 )
        strcpy(destplugin,plugin);
    if ( strcmp(plugin,"relay") == 0 && strcmp(destplugin,"relay") == 0 && broadcastmode == 0 )
        broadcastmode = "4";
    sentflag = 0;
    if ( Debuglevel > 2 )
        printf("relay.%d busdata_sync.(%s) (%s)\n",SUPERNET.iamrelay,jsonstr,broadcastmode==0?"":broadcastmode);
    if ( (data= create_busdata(&sentflag,noncep,&datalen,jsonstr,broadcastmode,destNXTaddr)) != 0 )
    {
        if ( SUPERNET.iamrelay != 0 )
        {
            if ( json != 0 )
            {
                if ( strcmp(broadcastmode,"publicaccess") == 0 )
                {
                    retstr = nn_busdata_processor((uint8_t *)data,datalen);
                    if ( data != jsonstr )
                        free(data);
                    free_json(json);
                    if ( Debuglevel > 2 )
                        printf("relay returns publicaccess.(%s)\n",retstr);
                    return(retstr);
                } else free_json(json), json = 0;
                if ( sentflag == 0 && RELAYS.pubglobal >= 0 && (strcmp(broadcastmode,"allnodes") == 0 || strcmp(broadcastmode,"8") == 0) )
                {
                    if( (sendlen= nn_send(RELAYS.pubglobal,data,datalen,0)) != datalen )
                    {
                        if ( Debuglevel > 1 )
                            printf("globl sendlen.%d vs datalen.%d (%s) %s\n",sendlen,datalen,(char *)data,nn_errstr());
                        if ( data != jsonstr )
                            free(data);
                        free_json(json);
                        return(clonestr("{\"error\":\"couldnt send to allnodes\"}"));
                    }
                    sentflag = 1;
                }
            }
            if ( sentflag == 0 && RELAYS.pubrelays >= 0 )
            {
                if( (sendlen= nn_send(RELAYS.pubrelays,data,datalen,0)) != datalen )
                {
                    if ( Debuglevel > 1 )
                        printf("sendlen.%d vs datalen.%d (%s) %s\n",sendlen,datalen,(char *)data,nn_errstr());
                    if ( data != jsonstr )
                        free(data);
                    if ( json != 0 )
                        free_json(json);
                    return(clonestr("{\"error\":\"couldnt send to allrelays\"}"));
                } // else printf("PUB.(%s) sendlen.%d datalen.%d\n",data,sendlen,datalen);
                sentflag = 1;
            }
            if ( data != jsonstr )
                free(data);
            if ( json != 0 )
                free_json(json);
            return(clonestr("{\"result\":\"sent to bus\"}"));
        }
        else
        {
            if ( json != 0 )
            {
                if ( broadcastmode == 0 && cJSON_str(cJSON_GetObjectItem(json,"servicename")) == 0 )
                {
                    if ( Debuglevel > 2 )
                        printf("call busdata proc.(%s)\n",data);
                    retstr = nn_busdata_processor((uint8_t *)data,datalen);
                }
                else
                {
                    if ( Debuglevel > 2 )
                        printf("LBsend.(%s)\n",data);
                    retstr = nn_loadbalanced((uint8_t *)data,datalen);
                    submethod = cJSON_str(cJSON_GetObjectItem(json,"submethod"));
                    if ( submethod != 0 && strcmp(destplugin,"relay") == 0 && strcmp(submethod,"join") == 0 && SUPERNET.noncing == 0 )
                    {
                        void recv_nonces(void *_ptr);
                        SUPERNET.noncing = 1;
                        if ( SUPERNET.responses != 0 )
                            free(SUPERNET.responses), SUPERNET.responses = 0;
                        apply.startflag = 1;
                        apply.senderbits = SUPERNET.my64bits;
                        ptr = calloc(1,sizeof(*ptr));
                        *ptr = apply;
                        portable_thread_create((void *)recv_nonces,ptr);
                        printf("START receiving nonces\n");
                    }
                }
                if ( Debuglevel > 2 && retstr != 0 )
                    printf("busdata nn_loadbalanced retstr.(%s) %p\n",retstr,retstr);
                if ( data != jsonstr )
                    free(data);
                if ( json != 0 )
                    free_json(json);
                return(retstr);
            } else printf("Cant parse busdata_sync.(%s)\n",jsonstr);
        }
    }
    if ( json != 0 )
        free_json(json);
    return(clonestr("{\"error\":\"error creating busdata\"}"));
}

int32_t complete_relay(struct relayargs *args,char *retstr)
{
    int32_t len,sendlen;
    _stripwhite(retstr,' ');
    len = (int32_t)strlen(retstr)+1;
    if ( args->type != NN_BUS && args->type != NN_SUB && (sendlen= nn_send(args->sock,retstr,len,0)) != len )
    {
        printf("complete_relay.%s warning: send.%d vs %d for (%s) sock.%d %s\n",args->name,sendlen,len,retstr,args->sock,nn_errstr());
        return(-1);
    }
    //printf("SUCCESS complete_relay.(%s) -> sock.%d %s\n",retstr,args->sock,args->name);
    return(0);
}

int32_t busdata_poll()
{
    char tokenized[65536],*msg,*retstr; cJSON *json,*retjson,*obj; uint64_t tag; int32_t len,noneed,sock,i,n = 0; uint32_t nonce;
    if ( RELAYS.numservers > 0 )
    {
        for (i=0; i<RELAYS.numservers; i++)
        {
            sock = RELAYS.pfd[i].fd;
            if ( (len= nn_recv(sock,&msg,NN_MSG,0)) > 0 )
            {
                if ( Debuglevel > 2 )
                    printf("RECV.%d (%s)\n",sock,msg);
                n++;
                if ( (json= cJSON_Parse(msg)) != 0 )
                {
                    if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
                        obj = cJSON_GetArrayItem(json,0);
                    else obj = json;
                    tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"tag"));
                    //fprintf(stderr,"got tag.%llu\n",(long long)tag);
                    if ( is_duplicate_tag(tag) == 0 )
                    {
                        if ( (retstr= nn_busdata_processor((uint8_t *)msg,len)) != 0 )
                        {
                            noneed = 0;
                            if ( (retjson= cJSON_Parse(retstr)) != 0 )
                            {
                                if ( is_cJSON_Array(retjson) != 0 && cJSON_GetArraySize(retjson) == 2 )
                                {
                                    noneed = 1;
                                    //fprintf(stderr,"return.(%s)\n",retstr);
                                    nn_send(sock,retstr,(int32_t)strlen(retstr)+1,0);
                                }
                                free_json(retjson);
                            }
                            if ( noneed == 0 )
                            {
                                len = construct_tokenized_req(&nonce,tokenized,retstr,(sock == RELAYS.servicesock) ? SUPERNET.SERVICESECRET : SUPERNET.NXTACCTSECRET,0);
                                //fprintf(stderr,"tokenized return.(%s)\n",tokenized);
                                nn_send(sock,tokenized,len,0);
                            }
                            free(retstr);
                        } else nn_send(sock,"{\"error\":\"null return\"}",(int32_t)strlen("{\"error\":\"null return\"}")+1,0);
                    } else nn_send(sock,"{\"error\":\"duplicate command\"}",(int32_t)strlen("{\"error\":\"duplicate command\"}")+1,0);
                    free_json(json);
                }
                nn_freemsg(msg);
            }
        }
    }
    if ( SUPERNET.iamrelay != 0 )
    {
        int32_t dKV777_ping(struct dKV777 *dKV);
        static double lastping;
        if ( milliseconds() > (lastping + SUPERNET.relays->pinggap) )
        {
            dKV777_ping(SUPERNET.relays);
            lastping = milliseconds();
        }
    }
    return(n);
}

void busdata_init(int32_t sendtimeout,int32_t recvtimeout,int32_t firstiter)
{
    char endpoint[512]; int32_t i;
    RELAYS.servicesock = RELAYS.pubglobal = RELAYS.pubrelays = RELAYS.lbserver = -1;
    endpoint[0] = 0;
    if ( (RELAYS.subclient= nn_createsocket(endpoint,0,"NN_SUB",NN_SUB,0,sendtimeout,recvtimeout)) >= 0 )
    {
        RELAYS.pfd[RELAYS.numservers++].fd = RELAYS.subclient, printf("numservers.%d\n",RELAYS.numservers);
        nn_setsockopt(RELAYS.subclient,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
    } else printf("error creating subclient\n");
    RELAYS.lbclient = nn_lbsocket(SUPERNET.PLUGINTIMEOUT,SUPERNET_PORT + LB_OFFSET,SUPERNET.port + PUBGLOBALS_OFFSET,SUPERNET.port + PUBRELAYS_OFFSET);
    printf("LBclient.%d port.%d\n",RELAYS.lbclient,SUPERNET_PORT + LB_OFFSET);
    sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.serviceport);
    if ( (RELAYS.servicesock= nn_createsocket(endpoint,1,"NN_REP",NN_REP,SUPERNET.serviceport,sendtimeout,recvtimeout)) >= 0 )
        RELAYS.pfd[RELAYS.numservers++].fd = RELAYS.servicesock, printf("numservers.%d\n",RELAYS.numservers);
    else printf("error createing servicesock\n");
    if ( SUPERNET.iamrelay != 0 )
    {
        sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + LB_OFFSET);
        if ( (RELAYS.lbserver= nn_createsocket(endpoint,1,"NN_REP",NN_REP,SUPERNET.port + LB_OFFSET,sendtimeout,recvtimeout)) >= 0 )
            RELAYS.pfd[RELAYS.numservers++].fd = RELAYS.lbserver, printf("numservers.%d\n",RELAYS.numservers);
        else printf("error creating lbserver\n");
        sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBGLOBALS_OFFSET);
        RELAYS.pubglobal = nn_createsocket(endpoint,1,"NN_PUB",NN_PUB,SUPERNET.port + PUBGLOBALS_OFFSET,sendtimeout,recvtimeout);
        sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBRELAYS_OFFSET);
        RELAYS.pubrelays = nn_createsocket(endpoint,1,"NN_PUB",NN_PUB,SUPERNET.port + PUBRELAYS_OFFSET,sendtimeout,recvtimeout);
    }
    for (i=0; i<RELAYS.numservers; i++)
        RELAYS.pfd[i].events = NN_POLLIN | NN_POLLOUT;
    printf("SUPERNET.iamrelay %d, numservers.%d ipaddr.(%s://%s) port.%d serviceport.%d\n",SUPERNET.iamrelay,RELAYS.numservers,SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port,SUPERNET.serviceport);
    if ( SUPERNET.iamrelay != 0 )
        SUPERNET.relays = dKV777_init("relays",&SUPERNET.rawPM,1,8,RELAYS.pubrelays,RELAYS.subclient,RELAYS.active.connections,RELAYS.active.num,1 << CONNECTION_NUMBITS,SUPERNET.port + PUBRELAYS_OFFSET,0.);
}

int32_t init_SUPERNET_pullsock(int32_t sendtimeout,int32_t recvtimeout)
{
    char bindaddr[64],*transportstr; int32_t iter;
    if ( (SUPERNET.pullsock= nn_socket(AF_SP,NN_PULL)) < 0 )
    {
        printf("error creating pullsock %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_settimeouts(SUPERNET.pullsock,sendtimeout,recvtimeout) < 0 )
    {
        printf("error settime pullsock timeouts %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    printf("SUPERNET.pullsock.%d\n",SUPERNET.pullsock);
    for (iter=0; iter<2; iter++)
    {
        transportstr = (iter == 0) ? "ipc" : "inproc";
        sprintf(bindaddr,"%s://SuperNET",transportstr);
        if ( nn_bind(SUPERNET.pullsock,bindaddr) < 0 )
        {
            printf("error binding pullsock to (%s) %s\n",bindaddr,nn_strerror(nn_errno()));
            return(-1);
        }
    }
    return(0);
}

