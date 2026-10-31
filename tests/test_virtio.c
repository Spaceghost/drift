/* Independent noncoherent Virtio MMIO/PCI device model. SPDX-License-Identifier: MIT */
#include "drift_virtio.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"virtio: %s:%d: %s\n",__FILE__,__LINE__,#c); abort(); } } while(0)
static unsigned checks;
typedef struct allocation { uint8_t *cpu,*device; uint64_t address; unsigned size,live; } allocation;
typedef struct queue { uint64_t desc,avail,used; uint16_t seen,produced; unsigned enabled; } queue;
typedef struct model {
    allocation allocation[8]; queue queue[2];
    uint32_t transport,id,status,feature_select,driver_select,queue_select;
    uint32_t offered,accepted[2],version,maximum;
    unsigned allocations,releases,drop,bad_id,bad_length,bad_index,io_error,reset_stuck,fail_alloc;
    unsigned high_features,reject_features,unstable_generation,generation,head_id;
    unsigned notifications,zero_length,external_mask,external_release_allowed;
    unsigned payload_acquires,payload_publishes,last_payload_acquire_status;
    uint64_t time;
    uint8_t disk[256*512],sent[2048]; unsigned sent_size;
} model;
static uint64_t get(const uint8_t *p,unsigned bytes)
{
    uint64_t v=0; for(unsigned i=0;i<bytes;i++) v|=(uint64_t)p[i]<<(8*i); return v;
}
static void put(uint8_t *p,uint64_t value,unsigned bytes)
{
    for(unsigned i=0;i<bytes;i++) p[i]=(uint8_t)(value>>(8*i));
}
static uint8_t *device(model *m,uint64_t address,unsigned size)
{
    for(unsigned i=0;i<8;i++) {
        allocation *a=&m->allocation[i];
        if(a->live && address>=a->address && address-a->address<=a->size && size<=a->size-(address-a->address))
            return a->device+(size_t)(address-a->address);
    }
    CHECK(!"device DMA address outside a live allocation"); return NULL;
}
static void used(model *m,unsigned index,uint32_t length)
{
    queue *q=&m->queue[index];
    uint8_t *ring=device(m,q->used,68);
    put(ring+4+(q->produced&7)*8,m->bad_id?8:m->head_id,4);
    put(ring+8+(q->produced&7)*8,m->bad_length?UINT32_MAX:m->zero_length?0:length,4);
    q->produced+=(uint16_t)(m->bad_index?2:1);
    put(ring+2,q->produced,2);
}
static uint8_t *descriptor(model *m,unsigned index,unsigned entry,unsigned expected_flags,unsigned *size)
{
    uint8_t *d=device(m,m->queue[index].desc+entry*16,16);
    CHECK(get(d+12,2)==expected_flags);
    *size=(unsigned)get(d+8,4);
    return device(m,get(d,8),*size);
}
static void service(model *m,unsigned index)
{
    queue *q=&m->queue[index];
    unsigned size,n,status_length;
    uint8_t *a=device(m,q->avail,20),*p,*header,*status;
    CHECK(q->enabled && m->status==15);
    if(m->drop || (m->id!=2 && index==0 && m->id!=4)) return;
    CHECK((uint16_t)(get(a+2,2)-q->seen)==1);
    CHECK(get(a+4+(q->seen&7)*2,2)==0);
    ++q->seen;
    if(m->id==2) {
        header=descriptor(m,0,0,1,&size); CHECK(size==16 && get(header+4,4)==0);
        unsigned command=(unsigned)get(header,4); uint64_t sector=get(header+8,8);
        if(command==4) {
            status=descriptor(m,0,1,2,&status_length); CHECK(status_length==1);
            *status=m->io_error?1:0; used(m,0,1); return;
        }
        CHECK(command<=1);
        p=descriptor(m,0,1,command?1:3,&n);
        CHECK(n && !(n%512) && sector<256 && n/512<=256-sector);
        status=descriptor(m,0,2,2,&status_length); CHECK(status_length==1);
        if(!m->io_error) {
            if(command) memcpy(m->disk+sector*512,p,n); else memcpy(p,m->disk+sector*512,n);
        }
        *status=m->io_error?1:0;
        used(m,0,command||m->io_error?1:n+1);
    } else if(m->id==1 && index==1) {
        p=descriptor(m,1,0,0,&n); CHECK(n>=26 && n<=2048);
        for(unsigned i=0;i<12;i++) CHECK(!p[i]);
        memcpy(m->sent,p+12,n-12); m->sent_size=n-12; used(m,1,0);
    } else if(m->id==3 && index==1) {
        p=descriptor(m,1,0,0,&n); CHECK(n<=sizeof(m->sent));
        memcpy(m->sent,p,n); m->sent_size=n; used(m,1,0);
    } else if(m->id==4) {
        p=descriptor(m,0,0,2,&n); CHECK(n>0 && n<=4096);
        for(unsigned i=0;i<n;i++) p[i]=(uint8_t)(i*17+9);
        used(m,0,n);
    } else CHECK(!"unexpected device queue");
}
static uint64_t config(model *m,unsigned off,unsigned width)
{
    if(m->id==2 && (off==0||off==4)) return off?0:256;
    if(m->id==1 && off<6 && width==1) return off+2;
    CHECK(!"unexpected configuration read"); return 0;
}
static uint64_t read_reg(void *context,uint32_t region,uint32_t offset,uint32_t width)
{
    model *m=context; queue *q=&m->queue[m->queue_select];
    if(!m->transport) {
        CHECK(region==0);
        if(offset>=0x100) return config(m,offset-0x100,width);
        CHECK(width==4);
        switch(offset) {
        case 0:return 0x74726976; case 4:return m->version; case 8:return m->id;
        case 0x10:return m->feature_select?m->high_features:m->offered;
        case 0x34:return m->maximum; case 0x44:return q->enabled;
        case 0x70:return m->status; case 0xfc:return m->unstable_generation?m->generation++:0;
        default:CHECK(!"unknown MMIO read"); return 0;
        }
    }
    if(region==2) return config(m,offset-0x300,width);
    CHECK(region==0 && offset>=0x100); offset-=0x100;
    switch(offset) {
    case 4:return m->feature_select?m->high_features:m->offered;
    case 20:CHECK(width==1);return m->status;
    case 21:CHECK(width==1);return m->unstable_generation?m->generation++:0;
    case 24:CHECK(width==2);return m->maximum;
    case 28:CHECK(width==2);return q->enabled;
    case 30:CHECK(width==2);return m->queue_select*3;
    default:CHECK(!"unknown PCI common read");return 0;
    }
}
static void status_write(model *m,uint64_t value)
{
    if(!value && m->reset_stuck && m->status==15) return;
    m->status=(uint32_t)(m->reject_features && value==11 ? 3 : value);
    if(!value) memset(m->queue,0,sizeof(m->queue));
}
static void write_reg(void *context,uint32_t region,uint32_t offset,uint32_t width,uint64_t value)
{
    model *m=context;
    if(!m->transport) {
        CHECK(region==0 && width==4);
        if(offset==0x14){m->feature_select=(uint32_t)value;return;}
        if(offset==0x24){m->driver_select=(uint32_t)value;return;}
        if(offset==0x20){CHECK(m->driver_select<2);m->accepted[m->driver_select]=(uint32_t)value;return;}
        if(offset==0x30){CHECK(value<2);m->queue_select=(uint32_t)value;return;}
        if(offset==0x70){status_write(m,value);return;}
        if(offset==0x50){CHECK(value<2);++m->notifications;service(m,(unsigned)value);return;}
        queue *q=&m->queue[m->queue_select];
        if(offset==0x38){CHECK(value==8);return;}
        if(offset==0x44){q->enabled=(unsigned)value;return;}
        uint64_t *address=NULL;
        if(offset==0x80||offset==0x84) address=&q->desc;
        if(offset==0x90||offset==0x94) address=&q->avail;
        if(offset==0xa0||offset==0xa4) address=&q->used;
        CHECK(address);
        if(offset&4)*address=(*address&UINT32_MAX)|(value<<32); else *address=(*address&~(uint64_t)UINT32_MAX)|value;
        return;
    }
    if(region==1) {
        CHECK(width==2 && value<2 && offset==0x200+value*3*4);
        ++m->notifications;service(m,(unsigned)value); return;
    }
    CHECK(region==0 && offset>=0x100); offset-=0x100;
    if(offset==0){m->feature_select=(uint32_t)value;return;}
    if(offset==8){m->driver_select=(uint32_t)value;return;}
    if(offset==12){CHECK(m->driver_select<2);m->accepted[m->driver_select]=(uint32_t)value;return;}
    if(offset==20){CHECK(width==1);status_write(m,value);return;}
    if(offset==22){CHECK(width==2 && value<2);m->queue_select=(uint32_t)value;return;}
    queue *q=&m->queue[m->queue_select];
    if(offset==24){CHECK(width==2 && value==8);return;}
    if(offset==26){CHECK(width==2 && value==0xffff);return;}
    if(offset==28){CHECK(width==2);q->enabled=(unsigned)value;return;}
    uint64_t *address=NULL;
    if(offset==32||offset==36) address=&q->desc;
    if(offset==40||offset==44) address=&q->avail;
    if(offset==48||offset==52) address=&q->used;
    CHECK(address && width==4);
    if(offset&4)*address=(*address&UINT32_MAX)|(value<<32); else *address=(*address&~(uint64_t)UINT32_MAX)|value;
}
static int32_t allocate(void *context,uint32_t size,uint32_t alignment,drift_dma *out)
{
    model *m=context;
    CHECK(alignment==4096);
    if(m->fail_alloc && m->allocations+1==m->fail_alloc) return -1;
    for(unsigned i=0;i<8;i++) if(!m->allocation[i].live) {
        allocation *a=&m->allocation[i];
        a->cpu=calloc(1,size);a->device=calloc(1,size);CHECK(a->cpu&&a->device);
        a->size=size;a->address=UINT64_C(0x100000000)+(uint64_t)i*0x100000;a->live=1;
        *out=(drift_dma){a->cpu,a->address,size,i};++m->allocations;return 0;
    }
    return -1;
}
static void release(void *context,drift_dma *buffer)
{
    model *m=context;CHECK(buffer->token<8);allocation *a=&m->allocation[buffer->token];
    /* External mappings are caller-owned even after their transfer completes.
     * Only the test's explicit, post-reset release may relinquish them. */
    if(m->external_mask&(1u<<buffer->token))CHECK(m->external_release_allowed&(1u<<buffer->token));
    CHECK(a->live && buffer->data==a->cpu && m->status==0);
    free(a->cpu);free(a->device);a->live=0;++m->releases;
    buffer->data=NULL;
}
static void sync_dma(void *context,drift_dma *buffer,uint32_t offset,uint32_t length,uint32_t direction)
{
    model *m=context;allocation *a=&m->allocation[buffer->token];
    CHECK(a->live && offset<=a->size && length<=a->size-offset);
    CHECK(buffer->data==a->cpu && buffer->address==a->address && buffer->size==a->size);
    CHECK(direction>=1&&direction<=3);
    if(m->external_mask&(1u<<buffer->token)) {
        if(direction==DRIFT_DMA_FROM_DEVICE){++m->payload_acquires;m->last_payload_acquire_status=m->status;}
        else ++m->payload_publishes;
    }
    if(direction==2) memcpy(a->cpu+offset,a->device+offset,length);
    else memcpy(a->device+offset,a->cpu+offset,length);
}
static uint64_t time_us(void *context){return ((model *)context)->time;}
static void wait_us(void *context,uint32_t time){((model *)context)->time+=time;}
static void init(model *m,drift_hw *h,unsigned transport,unsigned id)
{
    memset(m,0,sizeof(*m));m->transport=transport;m->id=id;m->version=2;m->maximum=8;m->offered=id==2?0x200:0x20;m->high_features=1;
    for(unsigned i=0;i<sizeof(m->disk);i++) m->disk[i]=(uint8_t)(i^(i>>8));
    *h=(drift_hw){m,read_reg,write_reg,allocate,release,sync_dma,time_us,wait_us};
}
static int open_device(drift_virtio *d,drift_hw *h,model *m)
{
    return m->transport?drift_virtio_open_pci(d,h,0,0x100,1,0x200,4,2,0x300,m->id):drift_virtio_open_mmio(d,h,0,m->id);
}
static void inject(model *m,const uint8_t *bytes,unsigned size)
{
    queue *q=&m->queue[0];unsigned capacity;
    uint8_t *available=device(m,q->avail,20);
    unsigned outstanding=(uint16_t)(get(available+2,2)-q->seen);
    CHECK(outstanding>0 && outstanding<=8);
    m->head_id=(unsigned)get(available+4+(q->seen&7)*2,2);
    uint8_t *p=descriptor(m,0,m->head_id,2,&capacity);
    CHECK(size<=capacity);
    ++q->seen;memcpy(p,bytes,size);used(m,0,size);
}
typedef struct pci_fixture { uint32_t word[64]; } pci_fixture;
static uint32_t pci_read_test(void *context,uint32_t bdf,uint32_t offset)
{
    pci_fixture *p=context;CHECK(bdf==0x1234 && !(offset&3) && offset<256);return p->word[offset/4];
}
static void pci_write_test(void *context,uint32_t bdf,uint32_t offset,uint32_t value)
{
    (void)context;(void)bdf;(void)offset;(void)value;CHECK(!"attach never writes PCI config");
}
static void pci_setup(pci_fixture *p,drift_pci *pci,drift_virtio_bar *bindings,unsigned id)
{
    memset(p,0,sizeof(*p));p->word[0]=((0x1040+id)<<16)|0x1af4;p->word[1]=0x100006;
    p->word[4]=0x80000000;p->word[5]=0x81000000;p->word[6]=0x82000000;p->word[0x34/4]=0x40;
    p->word[0x40/4]=(1u<<24)|(16u<<16)|(0x50u<<8)|9u;
    p->word[0x44/4]=0;p->word[0x48/4]=0x100;p->word[0x4c/4]=56;
    p->word[0x50/4]=(2u<<24)|(20u<<16)|(0x64u<<8)|9u;
    p->word[0x54/4]=1;p->word[0x58/4]=0x200;p->word[0x5c/4]=32;p->word[0x60/4]=4;
    p->word[0x64/4]=(4u<<24)|(16u<<16)|9u;
    p->word[0x68/4]=2;p->word[0x6c/4]=0x300;p->word[0x70/4]=8;
    for(unsigned i=0;i<6;i++)bindings[i]=(drift_virtio_bar){i<3?i:UINT32_MAX,i<3?4096:0};
    *pci=(drift_pci){p,pci_read_test,pci_write_test};
}
static int raw_open(drift_virtio *d,drift_hw *h,model *m,unsigned queues)
{
    return m->transport?drift_virtio_raw_open_pci(d,h,0,0x100,1,0x200,4,2,0x300,m->id,queues):
        drift_virtio_raw_open_mmio(d,h,0,m->id,queues);
}
static void raw_allocate(model *m,drift_dma *mapping)
{
    CHECK(allocate(m,256,4096,mapping)==0);
    CHECK(mapping->address>=UINT64_C(0x100000000));
    CHECK(mapping->data!=device(m,mapping->address,mapping->size));
    m->external_mask|=1u<<mapping->token;
}
static void raw_release(model *m,drift_dma *mapping)
{
    CHECK(m->status==0);
    m->external_release_allowed|=1u<<mapping->token;
    release(m,mapping);
    m->external_mask&=~(1u<<mapping->token);
}
static void raw_incoming(model *m,unsigned transport)
{
    drift_hw h;drift_virtio d={0},foreign={0};drift_dma payload;
    drift_virtio_raw_transfer transfer={0},other={0};
    unsigned marker=0;void *cookie=(void *)1;uint32_t length=99;
    init(m,&h,transport,4);
    CHECK(raw_open(&d,&h,m,1)==0);
    CHECK(d.state==4 && d.capacity==1 && m->status==11);
    CHECK(m->accepted[0]==0 && m->accepted[1]==1 && !m->notifications);
    CHECK(d.queue[0].ring.data && !d.queue[0].buffer.data && !d.queue[1].ring.data);
    unsigned internal=m->allocations;
    raw_allocate(m,&payload);memset(payload.data,0xcc,payload.size);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,64,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(!transfer.active && !m->notifications && !m->queue[0].seen && !m->queue[0].produced);
    CHECK(drift_virtio_entropy(&d,payload.data,64)<0);
    CHECK(drift_virtio_raw_device_ready(&d)==0);
    CHECK(d.state==5 && m->status==15 && !m->notifications);
    CHECK(drift_virtio_raw_device_ready(&d)<0);
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==0);
    CHECK(!cookie && !length);
    CHECK(drift_virtio_raw_kick(&d,0)<0);
    CHECK(drift_virtio_raw_add(NULL,0,&transfer,&payload,0,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,1,&transfer,&payload,0,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,NULL,&payload,0,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,NULL,0,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,64,DRIFT_DMA_FROM_DEVICE,NULL)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,64,DRIFT_DMA_BIDIRECTIONAL,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,0,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,257,1,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,255,2,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    drift_dma invalid=payload;invalid.data=NULL;
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&invalid,0,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    invalid=payload;invalid.address=UINT64_MAX-31;
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&invalid,32,1,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&invalid,0,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(!transfer.active && !d.queue[0].pending && !m->notifications && !m->payload_publishes);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,64,64,DRIFT_DMA_FROM_DEVICE,&marker)==0);
    CHECK(transfer.active && transfer.owner==&d && transfer.queue==0 && transfer.cookie==&marker);
    CHECK(transfer.mapping.data==payload.data && transfer.mapping.address==payload.address &&
        transfer.mapping.size==payload.size && transfer.mapping.token==payload.token);
    CHECK(transfer.offset==64 && transfer.length==64 && transfer.direction==DRIFT_DMA_FROM_DEVICE);
    CHECK(m->payload_publishes==1 && !m->payload_acquires && !m->notifications);
    CHECK(get(device(m,m->queue[0].desc,16),8)==payload.address+64);
    CHECK(get(device(m,m->queue[0].desc+8,8),4)==64);
    CHECK(get(device(m,m->queue[0].desc+12,4),2)==2);
    CHECK(get(device(m,m->queue[0].avail+2,2),2)==1 && !m->queue[0].seen);
    CHECK(drift_virtio_raw_add(&d,0,&other,&payload,0,32,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,32,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(drift_virtio_raw_get(&d,0,&other,&cookie,&length)<0);
    CHECK(drift_virtio_raw_get(&d,0,&transfer,NULL,&length)<0);
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,NULL)<0);
    CHECK(drift_virtio_raw_get(&d,1,&transfer,&cookie,&length)<0);
    void *owner=transfer.owner;transfer.owner=&foreign;
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)<0);transfer.owner=owner;
    unsigned queue_index=transfer.queue;transfer.queue=1;
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)<0);transfer.queue=queue_index;
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==0);
    CHECK(!cookie && !length && transfer.active && !m->payload_acquires);
    CHECK(drift_virtio_raw_kick(&d,0)==0 && m->notifications==1);
    CHECK(m->queue[0].seen==1 && m->queue[0].produced==1);
    for(unsigned i=0;i<payload.size;i++)CHECK(payload.data[i]==0xcc);
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==1);
    CHECK(cookie==&marker && length==64 && !transfer.active && !transfer.owner);
    CHECK(m->payload_acquires==1 && m->last_payload_acquire_status==15);
    for(unsigned i=0;i<payload.size;i++)CHECK(payload.data[i]==(i>=64&&i<128?(uint8_t)((i-64)*17+9):0xcc));
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==0 && !cookie && !length);
    CHECK(drift_virtio_raw_kick(&d,0)<0);
    CHECK(drift_virtio_entropy(&d,payload.data,64)<0);
    /* Reuse the returned context, and preserve legal generic zero-used results. */
    m->zero_length=1;
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,32,DRIFT_DMA_FROM_DEVICE,&marker)==0);
    CHECK(drift_virtio_raw_kick(&d,0)==0);
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==1 && cookie==&marker && length==0);
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==0 && !cookie && !length);
    /* Raw transfers preserve the split-ring's 16-bit index wrap too. */
    m->zero_length=0;
    d.queue[0].available=d.queue[0].used=65535;
    m->queue[0].seen=m->queue[0].produced=65535;
    put(device(m,m->queue[0].used,68)+2,65535,2);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,16,DRIFT_DMA_FROM_DEVICE,&marker)==0);
    CHECK(drift_virtio_raw_kick(&d,0)==0);
    CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==1 && cookie==&marker && length==16);
    CHECK(!d.queue[0].available && !d.queue[0].used && !m->queue[0].seen && !m->queue[0].produced);
    CHECK(drift_virtio_close(&d)==0 && !m->status && d.state==0);
    CHECK(m->releases==internal && m->allocation[payload.token].live);
    raw_release(m,&payload);CHECK(m->allocations==m->releases);
}
static void raw_duplex(model *m,unsigned transport)
{
    drift_hw h;drift_virtio d={0};drift_dma incoming,outgoing;
    drift_virtio_raw_transfer rx={0},tx={0};
    unsigned rx_marker=1,tx_marker=2;void *cookie=NULL;uint32_t length=0;
    init(m,&h,transport,3);CHECK(raw_open(&d,&h,m,2)==0);
    CHECK(d.state==4 && m->status==11 && d.queue[0].ring.data && d.queue[1].ring.data);
    CHECK(!d.queue[0].buffer.data && !d.queue[1].buffer.data);
    unsigned internal=m->allocations;
    raw_allocate(m,&incoming);raw_allocate(m,&outgoing);
    memset(incoming.data,0xcd,incoming.size);memcpy(outgoing.data+16,"hello",5);
    CHECK(drift_virtio_raw_device_ready(&d)==0);
    CHECK(drift_virtio_raw_add(&d,0,&rx,&incoming,32,64,DRIFT_DMA_FROM_DEVICE,&rx_marker)==0);
    CHECK(drift_virtio_raw_add(&d,1,&tx,&outgoing,16,5,DRIFT_DMA_TO_DEVICE,&tx_marker)==0);
    CHECK(rx.active && tx.active && !m->notifications);
    CHECK(get(device(m,m->queue[1].desc,16),8)==outgoing.address+16);
    CHECK(get(device(m,m->queue[1].desc+8,8),4)==5);
    CHECK(get(device(m,m->queue[1].desc+12,4),2)==0);
    CHECK(!memcmp(device(m,outgoing.address+16,5),"hello",5));
    CHECK(drift_virtio_raw_kick(&d,0)==0 && m->notifications==1);
    CHECK(drift_virtio_raw_kick(&d,1)==0 && m->notifications==2);
    CHECK(m->sent_size==5 && !memcmp(m->sent,"hello",5));
    CHECK(drift_virtio_raw_get(&d,1,&tx,&cookie,&length)==1 && cookie==&tx_marker && length==0);
    CHECK(rx.active && !tx.active && !m->payload_acquires);
    CHECK(drift_virtio_raw_get(&d,0,&rx,&cookie,&length)==0 && !cookie && !length);
    inject(m,(const uint8_t *)"asynchronous",12);
    for(unsigned i=0;i<incoming.size;i++)CHECK(incoming.data[i]==0xcd);
    CHECK(drift_virtio_raw_get(&d,0,&rx,&cookie,&length)==1 && cookie==&rx_marker && length==12);
    CHECK(!memcmp(incoming.data+32,"asynchronous",12) && m->payload_acquires==1);
    CHECK(drift_virtio_close(&d)==0 && m->releases==internal);
    CHECK(m->allocation[incoming.token].live && m->allocation[outgoing.token].live);
    raw_release(m,&incoming);raw_release(m,&outgoing);CHECK(m->allocations==m->releases);
}
static void raw_failures(model *m,unsigned transport)
{
    for(unsigned failure=0;failure<4;failure++)for(unsigned stuck=0;stuck<2;stuck++) {
        drift_hw h;drift_virtio d={0};drift_dma payload;drift_virtio_raw_transfer transfer={0};
        unsigned marker=0;void *cookie=(void *)1;uint32_t length=99;
        init(m,&h,transport,4);CHECK(raw_open(&d,&h,m,1)==0);
        unsigned internal=m->allocations;
        raw_allocate(m,&payload);memset(payload.data,0xce,payload.size);
        CHECK(drift_virtio_raw_device_ready(&d)==0);d.timeout_us=30;
        m->bad_id=failure==0;m->bad_length=failure==1;m->bad_index=failure==2;m->drop=failure==3;
        m->reset_stuck=stuck;
        CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,64,DRIFT_DMA_FROM_DEVICE,&marker)==0);
        CHECK(drift_virtio_raw_kick(&d,0)==0);
        if(failure==3){CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==0);wait_us(m,31);}
        CHECK(drift_virtio_raw_get(&d,0,&transfer,&cookie,&length)==(failure==3?-2:-5));
        CHECK(!cookie && !length && !m->payload_acquires);
        CHECK(m->allocation[payload.token].live);
        for(unsigned i=0;i<payload.size;i++)CHECK(payload.data[i]==0xce);
        if(stuck) {
            CHECK(d.state==3 && m->status==15 && transfer.active && transfer.owner==&d && !m->releases);
            CHECK(drift_virtio_close(&d)==-2 && transfer.active && !m->releases);
            m->reset_stuck=0;CHECK(drift_virtio_close(&d)==0);
        }
        CHECK(d.state==0 && m->status==0 && !transfer.active && !transfer.owner && m->releases==internal);
        CHECK(m->allocation[payload.token].live);
        raw_release(m,&payload);CHECK(m->allocations==m->releases);
    }
}
static void raw_api_checks(model *m)
{
    for(unsigned transport=0;transport<2;transport++) {
        raw_incoming(m,transport);raw_duplex(m,transport);raw_failures(m,transport);
    }
    drift_hw h;drift_virtio d={0};drift_dma payload;drift_virtio_raw_transfer transfer={0};unsigned marker=0;
    init(m,&h,0,4);
    CHECK(drift_virtio_raw_open_mmio(NULL,&h,0,4,1)<0);
    CHECK(drift_virtio_raw_open_mmio(&d,&h,0,4,0)<0);
    CHECK(drift_virtio_raw_open_mmio(&d,&h,0,4,3)<0);
    CHECK(drift_virtio_raw_device_ready(&d)<0 && !m->allocations);
    CHECK(raw_open(&d,&h,m,1)==0 && d.state==4 && m->status==11);
    CHECK(drift_virtio_close(&d)==0 && m->status==0 && m->allocations==m->releases);
    init(m,&h,0,4);
    CHECK(open_device(&d,&h,m)==0);raw_allocate(m,&payload);
    CHECK(drift_virtio_raw_device_ready(&d)<0);
    CHECK(drift_virtio_raw_add(&d,0,&transfer,&payload,0,64,DRIFT_DMA_FROM_DEVICE,&marker)<0);
    CHECK(!transfer.active && d.state==2);
    CHECK(drift_virtio_close(&d)==0);raw_release(m,&payload);CHECK(m->allocations==m->releases);
    pci_fixture fixture;drift_pci pci;drift_virtio_bar bindings[6];
    init(m,&h,1,4);pci_setup(&fixture,&pci,bindings,4);
    CHECK(drift_virtio_raw_open_function(&d,&h,&pci,0x1234,bindings,3,1)<0);
    CHECK(!d.state && !m->allocations);
    CHECK(drift_virtio_raw_open_function(&d,&h,&pci,0x1234,bindings,4,1)==0);
    CHECK(d.state==4 && m->status==11);CHECK(drift_virtio_raw_device_ready(&d)==0);
    CHECK(drift_virtio_close(&d)==0 && m->allocations==m->releases);
    /* Entropy has no device-specific configuration. A real modern RNG may
     * expose only common and notification capabilities. */
    init(m,&h,1,4);pci_setup(&fixture,&pci,bindings,4);
    fixture.word[0x50/4]&=~0xff00u;bindings[2]=(drift_virtio_bar){UINT32_MAX,0};
    CHECK(drift_virtio_raw_open_function(&d,&h,&pci,0x1234,bindings,4,1)==0);
    CHECK(d.state==4 && m->status==11);CHECK(drift_virtio_raw_device_ready(&d)==0);
    CHECK(drift_virtio_close(&d)==0 && m->allocations==m->releases);
}
int main(void)
{
    model *m=malloc(sizeof(*m));CHECK(m);
    drift_hw h;drift_virtio d={0};uint8_t data[65536],wire[128];
    for(unsigned transport=0;transport<2;transport++) {
        init(m,&h,transport,2);CHECK(open_device(&d,&h,m)==0);CHECK(d.capacity==256 && m->accepted[1]==1);
        CHECK(drift_virtio_block(&d,1,2,data,0)==0);CHECK(!memcmp(data,m->disk+512,1024));
        memset(data,0xa5,1024);CHECK(drift_virtio_block(&d,3,2,data,1)==0);CHECK(!memcmp(data,m->disk+1536,1024));
        CHECK(drift_virtio_block(&d,128,128,data,0)==0);CHECK(!memcmp(data,m->disk+65536,65536));
        CHECK(drift_virtio_block(&d,256,1,data,0)==-3);CHECK(drift_virtio_block(&d,255,2,data,0)==-3);
        CHECK(drift_virtio_block(&d,UINT64_MAX,1,data,0)==-3);CHECK(drift_virtio_block(&d,0,0,data,0)==-3);
        CHECK(drift_virtio_flush(&d)==0);
        /* Exercise 16-bit queue index wrap without 65,536 transfers. */
        d.queue[0].available=d.queue[0].used=65535;m->queue[0].seen=m->queue[0].produced=65535;
        put(device(m,m->queue[0].used,68)+2,65535,2);
        CHECK(drift_virtio_block(&d,0,1,data,0)==0);CHECK(!d.queue[0].available&&!d.queue[0].used);
        CHECK(drift_virtio_close(&d)==0);CHECK(m->allocations==m->releases);
        init(m,&h,transport,1);CHECK(open_device(&d,&h,m)==0);CHECK(d.mac[0]==2&&d.mac[5]==7);
        memset(data,0x3c,64);CHECK(drift_virtio_send(&d,data,64)==0);CHECK(m->sent_size==64&&!memcmp(m->sent,data,64));
        CHECK(drift_virtio_receive(&d,data,sizeof(data))==0);
        memset(wire,0,12);for(unsigned i=0;i<64;i++)wire[12+i]=(uint8_t)i;
        inject(m,wire,76);CHECK(drift_virtio_receive(&d,data,sizeof(data))==64);CHECK(!memcmp(data,wire+12,64));
        inject(m,wire,76);CHECK(drift_virtio_receive(&d,data,10)==-3);
        CHECK(drift_virtio_receive(&d,data,sizeof(data))==0);
        CHECK(drift_virtio_close(&d)==0);CHECK(m->allocations==m->releases);
        init(m,&h,transport,3);CHECK(open_device(&d,&h,m)==0);
        CHECK(drift_virtio_console_write(&d,(uint8_t *)"hello",5)==5);CHECK(m->sent_size==5&&!memcmp(m->sent,"hello",5));
        inject(m,(uint8_t *)"world",5);CHECK(drift_virtio_console_read(&d,data,sizeof(data))==5);CHECK(!memcmp(data,"world",5));
        CHECK(drift_virtio_close(&d)==0);
        init(m,&h,transport,4);CHECK(open_device(&d,&h,m)==0);
        CHECK(drift_virtio_entropy(&d,data,64)==64);for(unsigned i=0;i<64;i++)CHECK(data[i]==(uint8_t)(i*17+9));
        CHECK(drift_virtio_close(&d)==0);
        init(m,&h,transport,18);CHECK(open_device(&d,&h,m)==0);
        drift_virtio_input_event event;put(wire,1,2);put(wire+2,30,2);put(wire+4,1,4);
        inject(m,wire,8);put(wire,0,2);put(wire+2,0,2);put(wire+4,0,4);inject(m,wire,8);
        CHECK(drift_virtio_input(&d,&event)==1);CHECK(event.type==1&&event.code==30&&event.value==1);
        CHECK(drift_virtio_input(&d,&event)==1);CHECK(event.type==0&&event.code==0&&event.value==0);
        CHECK(drift_virtio_input(&d,&event)==0);CHECK(drift_virtio_close(&d)==0);
    }
    init(m,&h,0,2);m->offered|=32;CHECK(open_device(&d,&h,m)==0);
    CHECK(drift_virtio_block(&d,0,1,data,1)==-4);CHECK(drift_virtio_close(&d)==0);
    for(unsigned failure=0;failure<5;failure++) {
        init(m,&h,0,2);CHECK(open_device(&d,&h,m)==0);d.timeout_us=30;
        m->bad_id=failure==0;m->bad_length=failure==1;m->bad_index=failure==2;m->drop=failure>=3;m->reset_stuck=failure==4;
        CHECK(drift_virtio_block(&d,0,1,data,0)==(failure>=3?-2:-5));
        if(failure==4){CHECK(d.state==3 && m->releases==0);m->reset_stuck=0;CHECK(drift_virtio_close(&d)==0);}
        else CHECK(d.state==0);
        CHECK(m->allocations==m->releases);
    }
    for(unsigned failure=1;failure<=4;failure++) {
        init(m,&h,0,1);m->fail_alloc=failure;
        CHECK(open_device(&d,&h,m)==-6);CHECK(m->allocations==m->releases);
    }
    init(m,&h,0,2);m->maximum=4;CHECK(open_device(&d,&h,m)==-6);CHECK(!m->allocations);
    CHECK(drift_virtio_open_mmio(NULL,&h,0,2)==-1);CHECK(drift_virtio_close(NULL)==-1);
    init(m,&h,0,2);m->high_features=0;CHECK(open_device(&d,&h,m)==-4);CHECK(!m->allocations);
    init(m,&h,1,2);m->reject_features=1;CHECK(open_device(&d,&h,m)==-4);CHECK(!m->allocations);
    init(m,&h,1,2);m->unstable_generation=1;CHECK(open_device(&d,&h,m)==-5);CHECK(m->allocations==m->releases);
    init(m,&h,0,2);m->version=1;CHECK(open_device(&d,&h,m)==-1);CHECK(!m->allocations);
    init(m,&h,0,99);CHECK(open_device(&d,&h,m)==-4);CHECK(!m->allocations);
    init(m,&h,1,2);CHECK(open_device(&d,&h,m)==0);m->io_error=1;
    CHECK(drift_virtio_block(&d,0,1,data,0)==-5);CHECK(d.state==2);
    m->io_error=0;CHECK(drift_virtio_block(&d,0,1,data,0)==0);CHECK(drift_virtio_close(&d)==0);
    init(m,&h,0,1);CHECK(open_device(&d,&h,m)==0);
    memset(wire,0,76);wire[0]=1;inject(m,wire,76);
    CHECK(drift_virtio_receive(&d,data,sizeof(data))==-5);CHECK(d.state==0);
    init(m,&h,1,18);CHECK(open_device(&d,&h,m)==0);inject(m,wire,7);
    CHECK(drift_virtio_input(&d,(drift_virtio_input_event *)wire)==-5);CHECK(d.state==0);
    init(m,&h,0,1);m->offered=0;CHECK(open_device(&d,&h,m)==0);
    for(unsigned i=0;i<6;i++) CHECK(d.mac[i]==0);
    CHECK(drift_virtio_close(&d)==0);
    init(m,&h,0,18);CHECK(open_device(&d,&h,m)==0);memset(wire,0,8);
    inject(m,wire,8);inject(m,wire,8);
    put(device(m,m->queue[0].used,68)+12,0,4); /* duplicate ID in already-published batch */
    drift_virtio_input_event bad_event;
    CHECK(drift_virtio_input(&d,&bad_event)==-5);CHECK(d.state==0 && m->allocations==m->releases);
    pci_fixture pc;drift_pci pci;drift_virtio_bar bindings[6];
    init(m,&h,1,2);pci_setup(&pc,&pci,bindings,2);
    CHECK(drift_virtio_open_function(&d,&h,&pci,0x1234,bindings)==0);
    CHECK(d.capacity==256);CHECK(drift_virtio_block(&d,0,1,data,0)==0);CHECK(drift_virtio_close(&d)==0);
    for(unsigned fault=0;fault<26;fault++) {
        init(m,&h,1,2);pci_setup(&pc,&pci,bindings,2);
        switch(fault) {
        case 0:pc.word[0x64/4]|=0x40u<<8;break; /* cycle */
        case 1:pc.word[0x40/4]=(pc.word[0x40/4]&~0xff00u)|(0x51u<<8);break;
        case 2:pc.word[0x40/4]=(pc.word[0x40/4]&~0xff00u)|(0xf4u<<8);pc.word[0xf4/4]=(2u<<24)|(20u<<16)|9;break;
        case 3:pc.word[0x64/4]=(1u<<24)|(16u<<16)|9;break; /* duplicate common */
        case 4:pc.word[0x54/4]=6;break;
        case 5:pc.word[0x5c/4]=1;break;
        case 6:pc.word[0x4c/4]=55;break;
        case 7:pc.word[0x70/4]=7;break;
        case 8:pc.word[0x5c/4]=UINT32_MAX;break;
        case 9:bindings[1].region=UINT32_MAX;break;
        case 10:bindings[0].size=32;break;
        case 11:pc.word[5]|=4;break; /* device-config BAR points into a 64-bit upper half */
        case 12:pc.word[0x50/4]=(pc.word[0x50/4]&~0xff00u)|(0x60u<<8);break;
        case 13:pc.word[0]^=1;break;
        case 14:pc.word[0]=(0x1001u<<16)|0x1af4;break;
        case 15:pc.word[1]&=~4u;break;
        case 16:pc.word[1]&=~0x100000u;break;
        case 17:pc.word[3]=1u<<16;break;
        case 18:pc.word[0x64/4]=(3u<<24)|(16u<<16)|9;break; /* missing device config */
        case 19:bindings[1].size=UINT64_MAX;pc.word[0x58/4]=0xfffffff0;break; /* 32-bit offset overflow */
        case 20:pc.word[5]|=1;break; /* IO BAR */
        case 21:pc.word[0x40/4]=(1u<<24)|(12u<<16)|(0x50u<<8)|9;break;
        case 22:pc.word[0]=(0x107fu<<16)|0x1af4;break; /* unsupported device type */
        case 23:pc.word[0x48/4]++;break; /* unaligned common register window */
        case 24:pc.word[0x58/4]++;break; /* unaligned notify window */
        case 25:pc.word[0x6c/4]++;break; /* unaligned block capacity register */
        }
        CHECK(drift_virtio_open_function(&d,&h,&pci,0x1234,bindings)<0);
        CHECK(d.state==0 && m->allocations==0);
    }
    init(m,&h,1,1);pci_setup(&pc,&pci,bindings,1);pc.word[0x5c/4]=8;
    CHECK(drift_virtio_open_function(&d,&h,&pci,0x1234,bindings)==-6); /* queue1 notify offset outside capability */
    CHECK(d.state==0 && m->allocations==m->releases);
    init(m,&h,1,2);pci_setup(&pc,&pci,bindings,2);
    pc.word[0x64/4]|=0x74u<<8;pc.word[0x74/4]=(5u<<24)|(20u<<16)|9; /* unused PCI_CFG capability */
    CHECK(drift_virtio_open_function(&d,&h,&pci,0x1234,bindings)==0);CHECK(drift_virtio_close(&d)==0);
    CHECK(drift_virtio_open_function(&d,&h,&pci,0x1234,NULL)==-1);
    raw_api_checks(m);
    free(m);printf("virtio: %u checks passed (MMIO/PCI; block, net, console, entropy, input; raw external DMA)\n",checks);return 0;
}
