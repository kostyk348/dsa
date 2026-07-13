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

// hash map: dst ptr -> {ct_off, n, written}
#define HSIZE 65536
static unsigned long long h_dst[HSIZE];
static unsigned long long h_off[HSIZE];
static unsigned long h_n[HSIZE];
static unsigned char h_wr[HSIZE];
static unsigned h_count=0;

static unsigned long hashp(unsigned long long p){ return (p>>3)%HSIZE; }
static void map_put(unsigned long long dst, unsigned long long off, unsigned n){
    unsigned i=hashp(dst);
    for(int k=0;k<HSIZE;k++){
        unsigned j=(i+k)%HSIZE;
        if(h_dst[j]==0 || h_dst[j]==dst){
            if(h_dst[j]==0){ h_dst[j]=dst; h_count++; }
            h_off[j]=off; h_n[j]=n; h_wr[j]=0; return;
        }
    }
}
static int map_get(unsigned long long src, unsigned long long *off, unsigned *n){
    unsigned i=hashp(src);
    for(int k=0;k<HSIZE;k++){
        unsigned j=(i+k)%HSIZE;
        if(h_dst[j]==0) return 0;
        if(h_dst[j]==src && !h_wr[j]){ *off=h_off[j]; *n=h_n[j]; return 1; }
    }
    return 0;
}
static void map_mark(unsigned long long src){
    unsigned i=hashp(src);
    for(int k=0;k<HSIZE;k++){
        unsigned j=(i+k)%HSIZE;
        if(h_dst[j]==0) return;
        if(h_dst[j]==src){ h_wr[j]=1; return; }
    }
}

static int g_outfd=-1;

static void handle(const void *src, void *dst, size_t n){
    const unsigned char *s=(const unsigned char*)src;
    unsigned long long su=(unsigned long long)(uintptr_t)src;
    unsigned long long du=(unsigned long long)(uintptr_t)dst;

    if(g_mmap_base==0 && n>=16 && memcmp(s,g_ct,8)==0){
        g_mmap_base=su;
        logmsg("MMAP_BASE=%llx filesize=%llu\n", g_mmap_base, g_filesize);
    }
    if(g_mmap_base && n>=16){
        unsigned long long off = su - g_mmap_base;
        if(su>=g_mmap_base && off < g_filesize){
            map_put(du, off, n);
        }
    }
    if(g_outfd<0) g_outfd=open("/tmp/ojd_full.bin",O_WRONLY|O_CREAT|O_TRUNC,0644);
    unsigned long long off; unsigned rn;
    if(map_get(su,&off,&rn)){
        pwrite(g_outfd, src, rn, (off_t)off);
        map_mark(su);
        logmsg("PLAIN off=%llu n=%u\n", off, rn);
    }
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
