#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>

static int logfd = -1;
static void open_log(void){ if(logfd<0) logfd=open("/tmp/ojd_dump.log",O_WRONLY|O_CREAT|O_TRUNC,0644); }
static void logmsg(const char*fmt,...){
    char b[256]; va_list a; va_start(a,fmt); int n=vsnprintf(b,sizeof b,fmt,a); va_end(a);
    if(logfd>=0) write(logfd,b,n);
}
static unsigned char g_ct[] = {0xed,0xce,0x8a,0x22,0x70,0x78,0xea,0x1d};
static volatile void *g_decbuf = NULL;   // buffer that gets decrypted in place
static volatile int g_gotplain = 0;

typedef void*(*memcpy_t)(void*,const void*,size_t);
void *memcpy(void *dst, const void *src, size_t n){
    static memcpy_t real=NULL; if(!real) real=(memcpy_t)dlsym(RTLD_NEXT,"memcpy");
    const unsigned char *s=(const unsigned char*)src;
    // trigger: a copy whose source is the known ciphertext prefix
    if(n>=16 && memcmp(s,g_ct,8)==0){
        logmsg("CTCOPY n=%zu dst=%p src=%p\n", n, dst, src);
        g_decbuf = dst;   // this buffer will be decrypted in place
        // also save the raw ciphertext for reference
        int fd=open("/tmp/ojd_ct_ref.bin",O_WRONLY|O_CREAT|O_TRUNC,0644);
        if(fd>=0){ write(fd,src,n>1400000?1400000:n); close(fd); }
    }
    // capture plaintext: a copy reading FROM the decrypt buffer (post-decrypt)
    if(!g_gotplain && g_decbuf && src==g_decbuf && n>=256){
        int fd=open("/tmp/ojd_plain.bin",O_WRONLY|O_CREAT|O_TRUNC,0644);
        if(fd>=0){ write(fd,src,n>1400000?1400000:n); close(fd); }
        logmsg("PLAIN n=%zu src=%p\n", n, src);
        g_gotplain=1;
    }
    return real(dst,src,n);
}
void __attribute__((constructor)) init(void){ open_log(); }
