/* MINIX-only exploratory resource adapter, not a general Linux compatibility layer. */
#include <minix/drivers.h>
#include <minix/mthread.h>
#include <minix/sef.h>
#include <minix/endpoint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include "drift_linux_pcspkr.h"
/* RS services have their own descriptor table. Write records to an explicitly
 * opened serial device, not inherited stdout or libsys's diagnostic printf. */
static int serial_fd=-1;
static void serial_printf(const char *format,...){char b[512];va_list ap;va_start(ap,format);int n=vsnprintf(b,sizeof(b),format,ap);va_end(ap);if(n<=0||n>=(int)sizeof(b))return;for(int i=0;i<n;){ssize_t r=write(serial_fd,b+i,n-i);if(r<=0)return;i+=(int)r;}}
struct host { mthread_mutex_t mutex; unsigned held,reads,writes,waits; uint32_t trace; unsigned events,bad,actual_kind,actual_port,actual_value; };
static void trace(struct host*h,unsigned char kind,uint16_t port,uint32_t value){
 static const char kinds[]="LWDWDWRDWDU";static const unsigned ports[]={0,0x43,0,0x42,0,0x42,0x61,0,0x61,0,0};static const unsigned vals[]={0,0xb6,1,75,1,5,3,1,3,1,0};unsigned index=h->events++%11;if(h->trace&&(!h->bad)&&(kind!=kinds[index]||port!=ports[index]||value!=((index==6&&h->events<=11)?0:vals[index]))){h->bad=h->events;h->actual_kind=kind;h->actual_port=port;h->actual_value=value;}
unsigned char b[7]={kind,port&255,port>>8,value&255,(value>>8)&255,(value>>16)&255,value>>24};for(unsigned i=0;i<7;i++){h->trace^=b[i];h->trace*=16777619u;}}
static int neg(int r){return r>0?-r:r;}
static int32_t rd(void *p,uint16_t port){struct host*h=p;u32_t v;int r;if(!h->held||port!=0x61)return -22;r=sys_inb(port,&v);if(r)return neg(r);++h->reads;trace(h,'R',port,v&3);return v<=255?(int)v:-5;}
static int32_t wr(void *p,uint16_t port,uint8_t v){struct host*h=p;int r;if(!h->held||(port!=0x42&&port!=0x43&&port!=0x61))return -22;r=sys_outb(port,v);if(r)return neg(r);++h->writes;trace(h,'W',port,port==0x61?(v&3):v);return 0;}
/* No IRQ handler is registered in this userspace service. The kernel may
 * service PIT0 interrupts; only PIT2 and speaker bits are assigned here.
 * This actual MINIX mutex protects service calls; flags is a lock cookie,
 * not fabricated CPU IRQ state. nobeep=1 prevents TTY from owning PIT2. */
static int32_t lock_host(void*p,unsigned long*f){struct host*h=p;int r=mthread_mutex_lock(&h->mutex);if(r)return neg(r);h->held=1;trace(h,'L',0,0);*f=0x4d585049;return 0;}
static int32_t unlock_host(void*p,unsigned long f){struct host*h=p;int r;if(!h->held||f!=0x4d585049)return -22;r=mthread_mutex_unlock(&h->mutex);if(r)return neg(r);h->held=0;trace(h,'U',0,0);return 0;}
static int32_t delay_host(void*p,uint32_t us){struct host*h=p;if(!h->held||us!=1)return -22;++h->waits;trace(h,'D',0,us);return neg(micro_delay(us));}
static int gate_bits(unsigned expected){u32_t v;int r=sys_inb(0x61,&v);return r?neg(r):((v&3)==expected?0:-5);}
static int check_mode(void){u32_t v;int r=sys_outb(0x43,0xe8);if(r)return neg(r);r=sys_inb(0x42,&v);return r?neg(r):((v&0x3f)==0x36?0:-5);}
static int reference_tone(struct host *h){unsigned long flags;int r,v;r=lock_host(h,&flags);if(r)return r;
 #define REF(x) do{r=(x);if(r)goto unlocked;}while(0)
 REF(wr(h,0x43,0xb6));REF(delay_host(h,1));REF(wr(h,0x42,(1193182/880)&255));REF(delay_host(h,1));REF(wr(h,0x42,(1193182/880)>>8));
 v=rd(h,0x61);if(v<0){r=v;goto unlocked;}REF(delay_host(h,1));REF(wr(h,0x61,(uint8_t)(v|3)));REF(delay_host(h,1));
 unlocked:;int u=unlock_host(h,flags);return r?r:u;
}
static int run(void){
 static struct host h;static drift_linux_pcspkr state;drift_linux_pcspkr_host ops={&h,rd,wr,lock_host,unlock_host,delay_host};int r;struct timeval begin,end;
 r=mthread_mutex_init(&h.mutex,NULL);if(r)return neg(r);u32_t forbidden=0;r=sys_inb(0x80,&forbidden);if(!r)return -5;serial_printf("DRIFT_MINIX_PIO_DENIAL port=0x80 code=%d\n",r);
 #define TRY(x) do{r=(x);if(r){serial_printf("DRIFT_MINIX_PIO_FAIL line=%d code=%d\n",__LINE__,r);goto done;}}while(0)
 TRY(drift_linux_pcspkr_open(&state,&ops));
 TRY(drift_linux_pcspkr_event(&state,DRIFT_PCSPKR_EV_SND,DRIFT_PCSPKR_TONE,440));TRY(gate_bits(3));TRY(check_mode());
 TRY(drift_linux_pcspkr_suspend(&state));TRY(gate_bits(0));
 for(unsigned round=0;round<4;round++) for(unsigned slot=0;slot<2;slot++){
 unsigned use_reference=(round%2)?1-slot:slot,reads=h.reads,writes=h.writes,waits=h.waits;
 TRY(drift_linux_pcspkr_suspend(&state));TRY(gate_bits(0));reads=h.reads;writes=h.writes;waits=h.waits;h.trace=2166136261u;h.events=0;h.bad=0; TRY(gettimeofday(&begin,NULL));
 for(unsigned i=0;i<10000;i++) {if(use_reference)TRY(reference_tone(&h));else TRY(drift_linux_pcspkr_event(&state,DRIFT_PCSPKR_EV_SND,DRIFT_PCSPKR_TONE,880));}
 TRY(gettimeofday(&end,NULL));TRY(gate_bits(3));TRY(check_mode());
 if(h.bad)serial_printf("DRIFT_MINIX_TRACE_BAD index=%u kind=%u port=%u value=%u\n",h.bad,h.actual_kind,h.actual_port,h.actual_value);
 unsigned elapsed=(unsigned)((long long)(end.tv_sec-begin.tv_sec)*1000000+(end.tv_usec-begin.tv_usec));
 serial_printf("DRIFT_MINIX_PIO_BENCH kind=%s round=%u events=10000 elapsed_us=%u reads=%u writes=%u waits=%u trace=%u\n",use_reference?"reference":"drift",round+1,elapsed,h.reads-reads,h.writes-writes,h.waits-waits,h.trace);
 }
 TRY(drift_linux_pcspkr_shutdown(&state));TRY(gate_bits(0));TRY(drift_linux_pcspkr_close(&state));TRY(gate_bits(0));
 done:
 if(state.phase){int cleanup=drift_linux_pcspkr_close(&state);if(!r)r=cleanup;}
 if(!h.held){int destroyed=mthread_mutex_destroy(&h.mutex);if(!r)r=neg(destroyed);}
 if(!r) serial_printf("DRIFT_MINIX_PIO_PASS: actual sys_inb/sys_outb and PIT2 mode/gate readback; TTY nobeep=1; no IRQ handler\n");
 return r;
}
int main(int argc,char**argv){int r,fd;message m;env_setargs(argc,argv);sef_startup();serial_fd=open("/dev/tty00",O_WRONLY);r=serial_fd<0?-5:run();fd=open("/tmp/drift-pio.status",O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd>=0){const char*s=r?"FAIL\n":"PASS\n";write(fd,s,5);close(fd);}for(;;)sef_receive(ANY,&m);}
