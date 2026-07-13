#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

static int logfd=-1;
static void open_log(void){ if(logfd<0) logfd=open("/tmp/ojd_dump.log",O_WRONLY|O_CREAT|O_TRUNC,0644); }
static void logmsg(const char*fmt,...){ char b[256]; va_list a; va_start(a,fmt); int n=vsnprintf(b,sizeof b,fmt,a); va_end(a); if(logfd>=0) write(logfd,b,n); }

static unsigned char g_ct[]={0xed,0xce,0x8a,0x22,0x70,0x78,0xea,0x1d};
static volatile unsigned long long g_mmap_base=0;
static unsigned long long g_filesize=1339392;

#define HSIZE 131072
static unsigned long long h_ptr[HSIZE];
static unsigned long long h_off[HSIZE];
static unsigned char h_wr[HSIZE];
static unsigned long h_count=0, h_coll=0;

static unsigned long hashp(unsigned long long p){ return (p>>3)%HSIZE; }
static void map_set(unsigned long long ptr, unsigned long long off){
    unsigned i=hashp(ptr);
    for(unsigned k=0;k<HSIZE;k++){
        unsigned j=(i+k)%HSIZE;
        if(h_ptr[j]==0){ h_ptr[j]=ptr; h_off[j]=off; h_wr[j]=0; h_count++; return; }
        if(h_ptr[j]==ptr){ return; } // already tracked, keep
    }
    h_coll++;
}
static int map_get(unsigned long long ptr, unsigned long long *off){
    unsigned i=hashp(ptr);
    for(unsigned k=0;k<HSIZE;k++){
        unsigned j=(i+k)%HSIZE;
        if(h_ptr[j]==0) return 0;
        if(h_ptr[j]==ptr){ *off=h_off[j]; return 1; }
    }
    return 0;
}
static void map_mark(unsigned long long ptr){
    unsigned i=hashp(ptr);
    for(unsigned k=0;k<HSIZE;k++){
        unsigned j=(i+k)%HSIZE;
        if(h_ptr[j]==0) return;
        if(h_ptr[j]==ptr){ h_wr[j]=1; return; }
    }
}
static int h_wr_at(unsigned long long ptr);
static int g_outfd=-1;

static void handle(const void *src, void *dst, size_t n){
    const unsigned char *s=(const unsigned char*)src;
    unsigned long long su=(unsigned long long)(uintptr_t)src;
    unsigned long long du=(unsigned long long)(uintptr_t)dst;

    if(g_mmap_base==0 && n>=16 && memcmp(s,g_ct,8)==0){
        g_mmap_base=su; logmsg("MMAP_BASE=%llx\n", g_mmap_base);
    }
    if(g_mmap_base && n>=16){
        unsigned long long off = su - g_mmap_base;
        if(su>=g_mmap_base && off < g_filesize){
            map_set(du, off);
        }
    }
    if(g_outfd<0) g_outfd=open("/tmp/ojd_full.bin",O_WRONLY|O_CREAT|O_TRUNC,0644);
    unsigned long long off;
    if(map_get(su,&off)){
        if(!h_wr_at(su)){
            pwrite(g_outfd, src, n, (off_t)off);
            map_mark(su);
            logmsg("PLAIN off=%llu n=%lu\n", off, (unsigned long)n);
        }
        // chain dst as same content at same offset (handles decrypt output buffer)
        if(n>=16) map_set(du, off);
    }
}
static int h_wr_at(unsigned long long ptr){
    unsigned i=hashp(ptr);
    for(unsigned k=0;k<HSIZE;k++){ unsigned j=(i+k)%HSIZE; if(h_ptr[j]==0) return 0; if(h_ptr[j]==ptr) return h_wr[j]; }
    return 0;
}
typedef void*(*memcpy_t)(void*,const void*,size_t);
void *memcpy(void *dst, const void *src, size_t n){
    static memcpy_t real=NULL; if(!real) real=(memcpy_t)dlsym(RTLD_NEXT,"memcpy");
    handle(src,dst,n); return real(dst,src,n);
}
typedef void*(*memmove_t)(void*,const void*,size_t);
void *memmove(void *dst, const void *src, size_t n){
    static memmove_t real=NULL; if(!real) real=(memmove_t)dlsym(RTLD_NEXT,"memmove");
    handle(src,dst,n); return real(dst,src,n);
}
void __attribute__((constructor)) init(void){ open_log(); }
