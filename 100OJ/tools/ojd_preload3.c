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
#define MAXPAGES 4096
static volatile unsigned long long g_page_dst[MAXPAGES];
static volatile int g_page_written[MAXPAGES];
static int g_outfd=-1;
static int g_chunks=0;

static void handle(const void *src, void *dst, size_t n){
    const unsigned char *s=(const unsigned char*)src;
    unsigned long long su=(unsigned long long)(uintptr_t)src;
    unsigned long long du=(unsigned long long)(uintptr_t)dst;

    if(g_mmap_base==0 && n>=16 && memcmp(s,g_ct,8)==0){
        g_mmap_base=su; logmsg("MMAP_BASE=%llx\n", g_mmap_base);
    }
    if(g_mmap_base && n==4096){
        unsigned long long off = su - g_mmap_base;
        if(su>=g_mmap_base && off < (unsigned long long)MAXPAGES*4096 && (off%4096)==0){
            int idx=(int)(off/4096);
            if(g_page_dst[idx]==0){ g_page_dst[idx]=du; g_chunks++; }
        }
    }
    if(g_outfd<0) g_outfd=open("/tmp/ojd_full.bin",O_WRONLY|O_CREAT|O_TRUNC,0644);
    for(int i=0;i<MAXPAGES;i++){
        if(g_page_written[i]) continue;
        if(g_page_dst[i] && su==g_page_dst[i]){
            pwrite(g_outfd, src, 4096, (off_t)i*4096);
            g_page_written[i]=1;
            logmsg("PAGE %d\n", i);
            break;
        }
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
