// game/AA_viBuf.c -- Clock Tower 3: the movie's video stream ring (SCE's ezmpegstr vibuf.c, AA_-prefixed).
// Written 2026-11-12, all 13 functions.
//
// AA_videoCallback copies each video packet's payload into the ring (AA_videoDecBeginPut / EndPut) and
// queues its time stamp (AA_viBufPutTs); libmpeg's Nodata callback (AA_mpegNodata) hands the IPU's input
// DMA the whole blocks written since and retires those it has read (AA_viBufAddDMA). The DMA runs a chain
// of REF tags, one per 2 KB block, over the blocks it holds.
//
// PORT: there is no IPU on the host. MpegStream feeds its own decoder the payload of every packet the
// callback accepts, so the IPU's read position is taken from what that decoder has parsed (ViBufHostIpu);
// the ring then paces the demultiplexer as on the console (no room: the packet waits). DMA addresses live
// in the ring's host DMA space (AA_viBuf.h VIBUF_DMA_DATA / VIBUF_DMA_TAG).

#include "game/AA_viBuf.h"

#include "game/AA_scTag2.h"
#include "game/movie.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/libmpeg_video.h"

#include <stddef.h>

static const char s_dmaAddNotActive[] = "DMA ADD not active\n"; /* 0x01935700 */

// The ring's bytes and its tag list as the DMA addresses them (the ROM's f->data and f->tag & 0x0fffffff).
static u_int ViBufDmaData(const ViBuf *f)
{
    (void)f;
    return VIBUF_DMA_DATA;
}

static u_int ViBufDmaTag(const ViBuf *f)
{
    (void)f;
    return VIBUF_DMA_TAG;
}

static void *ViBufTagAt(ViBuf *f, int index)
{
    return (u_char *)f->tag + index * VIBUF_TAG_SIZE;
}

// PORT: the host's stand-in for the IPU reading the chain. The bytes it has taken are the ring's unretired
// bytes less those the decoder has not parsed yet (AlyssaPanMpegVideoPending), never more than the blocks
// the DMA holds; D4 MADR goes there, as the console's would after the transfer ran that far.
static void ViBufHostIpu(ViBuf *f)
{
    int held = f->dmaN * VIBUF_ELM_SIZE;
    int read = held + f->readBytes - AlyssaPanMpegVideoPending();

    if (read < 0)
        read = 0;
    if (read > held)
        read = held;
    REG_DMAC_4_IPU_TO_MADR = ViBufDmaData(f) + (f->dmaStart * VIBUF_ELM_SIZE + read) % f->buffSize;
}

// CT3 0x018c0440: libmpeg's time stamp for the picture the IPU is reading: the queued stamp whose bytes hold
// the IPU's position (D4 MADR less the quadwords still in its FIFO, plus the saved bit pointer's bytes); it
// leaves the queue. None found: pts and dts -1.
int AA_viBufGetTs(ViBuf *f, TimeStamp *ts)
{
    u_int madr, bp, ipubp;
    int size, count, wt, i;
    int found = 0;

    ViBufHostIpu(f); /* PORT */
    madr = REG_DMAC_4_IPU_TO_MADR;
    bp = REG_IPU_BP;
    WaitSema(f->sema);
    ipubp = f->env.ipubp;
    size = f->n * VIBUF_ELM_SIZE;
    ts->pts = -1;
    ts->dts = -1;
    count = f->count_ts;
    wt = f->wt_ts;
    for (i = 0; i < count && !found; i++) {
        TimeStamp *t = &f->ts[(i + f->n_ts + (wt - count)) % f->n_ts];
        int pos = (int)(size + madr - IPU_BP_FIFO_QWC(bp) * 16 + (IPU_BP_BP(ipubp) >> 3) - ViBufDmaData(f))
                  % size;

        if (AA_IsPtsInRegion(pos, t->pos, t->len, size)) {
            ts->pts = t->pts;
            ts->dts = t->dts;
            t->pts = -1;
            t->dts = -1;
            found = 1;
            f->count_ts -= f->count_ts > 0 ? 1 : f->count_ts;
        }
    }
    SignalSema(f->sema);
    return 1;
}

// CT3 0x018c0620: queues a packet's stamp (after trimming the ones its bytes overlap); a stamp with neither
// pts nor dts is dropped but answers 1. A full queue answers 0.
int AA_viBufPutTs(ViBuf *f, TimeStamp *ts)
{
    int ret = 0;

    WaitSema(f->sema);
    if (f->count_ts < f->n_ts) {
        AA_viBufModifyPts(f, ts);
        if (ts->pts >= 0 || ts->dts >= 0) {
            TimeStamp *t = &f->ts[f->wt_ts];

            t->pts = ts->pts;
            t->dts = ts->dts;
            t->pos = ts->pos;
            t->len = ts->len;
            f->count_ts++;
            f->wt_ts = (f->wt_ts + 1) % f->n_ts;
        }
        ret = 1;
    }
    SignalSema(f->sema);
    return ret;
}

// CT3 0x018c0750: from the oldest queued stamp on, while the new stamp's bytes hold each one's start, the
// overlap is cut off that one's front; a stamp left with no bytes is cleared and leaves the count. Stops at
// the first stamp it does not overlap, or at a stamp (old or new) with no bytes. Always answers 0.
int AA_viBufModifyPts(ViBuf *f, TimeStamp *ts)
{
    int more = 1;
    int rd = (f->n_ts + (f->wt_ts - f->count_ts)) % f->n_ts;
    int size = f->n << 11;

    if (f->count_ts <= 0)
        return 0;
    do {
        TimeStamp *t = &f->ts[rd];

        if (t->len == 0)
            return 0;
        if (ts->len == 0)
            return 0;
        if (AA_IsPtsInRegion(t->pos, ts->pos, ts->len, size)) {
            int cut = ts->pos + ts->len - t->pos;

            if (t->len < cut)
                cut = t->len;
            t->pos = (t->pos + cut) % size;
            t->len -= cut;
            if (t->len == 0) {
                if (t->pts >= 0) {
                    t->pts = -1;
                    t->dts = -1;
                    t->pos = 0;
                    t->len = 0;
                }
                f->count_ts = f->count_ts - 1 < 0 ? 0 : f->count_ts - 1;
            }
        } else {
            more = 0;
        }
        rd = (rd + 1) % f->n_ts;
    } while (more);
    return 0;
}

// CT3 0x018c08f0
void AA_viBufFlush(ViBuf *f)
{
    WaitSema(f->sema);
    f->readBytes = (f->readBytes + VIBUF_ELM_SIZE - 1) / VIBUF_ELM_SIZE * VIBUF_ELM_SIZE;
    SignalSema(f->sema);
}

// CT3 0x018c0940
int AA_viBufCount(ViBuf *f)
{
    int dmaN, readBytes;

    WaitSema(f->sema);
    dmaN = f->dmaN;
    readBytes = f->readBytes;
    SignalSema(f->sema);
    return readBytes + dmaN * VIBUF_ELM_SIZE;
}

// CT3 0x018c0990: stops the DMA and clears its registers.
int AA_viBufDelete(ViBuf *f)
{
    AA_setD4_CHCR(VIBUF_CHCR_STOP);
    REG_DMAC_4_IPU_TO_QWC = 0;
    REG_DMAC_4_IPU_TO_MADR = 0;
    REG_DMAC_4_IPU_TO_TADR = 0;
    DeleteSema(f->sema);
    return 1;
}

// CT3 0x018c09e0: resumes the DMA StopDMA saved, backed up by the quadwords the IPU's FIFO still held (the
// BCLR below drops them). Backed up past the ring's start, or into the block before MADR's, the chain
// restarts from that block's tag; a block the DMA had already retired is taken back into the held run.
int AA_viBufRestartDMA(ViBuf *f)
{
    u_int bp, madr0, base, tadr, madr, qwc, chcr;
    int fifo;

    WaitSema(f->sema);
    bp = f->env.ipubp;
    madr0 = f->env.d4madr;
    base = ViBufDmaData(f);
    tadr = f->env.d4tadr;
    fifo = IPU_BP_FIFO_QWC(bp);
    qwc = f->env.d4qwc + fifo;
    madr = madr0 - fifo * 16;
    chcr = f->env.d4chcr | VIBUF_CHCR_STR;
    if (madr < base) {
        int n = f->n;
        int id, k;

        qwc = (base - madr) >> 4;
        tadr = ViBufDmaTag(f);
        madr += n * VIBUF_ELM_SIZE;
        id = (madr0 == base || madr0 == base + n * VIBUF_ELM_SIZE) ? VIBUF_TAG_REFE : VIBUF_TAG_REF;
        chcr = (f->env.d4chcr & VIBUF_CHCR_KEEP) | (u_int)id << VIBUF_CHCR_TAG_ID_SHIFT | VIBUF_CHCR_STR;
        k = (n - f->dmaStart) % n;
        if (k < 0 || f->dmaN <= k) {
            f->dmaStart = n - 1;
            f->dmaN++;
        }
    } else {
        int index0 = AA_getFIFOindex(f, madr0);
        int index1 = AA_getFIFOindex(f, madr);

        if (index0 != index1) {
            int n = f->n;
            int id = VIBUF_TAG_REFE;
            int k;

            qwc = (base + index0 * VIBUF_ELM_SIZE - madr) >> 4;
            tadr = ViBufDmaTag(f) + index0 * VIBUF_TAG_SIZE;
            k = (index1 + n - f->dmaStart) % n;
            if (base + (int)(f->env.d4madr - base) % (n << 11)
                != base + ((f->dmaStart + f->dmaN) % n) * VIBUF_ELM_SIZE)
                id = VIBUF_TAG_REF;
            chcr = (f->env.d4chcr & VIBUF_CHCR_KEEP) | (u_int)id << VIBUF_CHCR_TAG_ID_SHIFT | VIBUF_CHCR_STR;
            if (k < 0 || f->dmaN <= k) {
                f->dmaStart = index1;
                f->dmaN++;
            }
        }
    }
    if (f->env.d3madr != 0 && f->env.d3qwc != 0) {
        REG_DMAC_3_IPU_FROM_MADR = f->env.d3madr;
        REG_DMAC_3_IPU_FROM_QWC = f->env.d3qwc;
        AA_setD3_CHCR(f->env.d3chcr | VIBUF_CHCR_STR);
    }
    if (f->dmaN != 0) {
        while (REG_IPU_CTRL & IPU_CTRL_BUSY)
            ;
        REG_IPU_CMD = IPU_BP_BP(bp); /* BCLR at the saved bit pointer */
        while (REG_IPU_CTRL & IPU_CTRL_BUSY)
            ;
    }
    REG_DMAC_4_IPU_TO_MADR = madr;
    REG_DMAC_4_IPU_TO_TADR = tadr;
    REG_DMAC_4_IPU_TO_QWC = qwc;
    if (f->dmaN != 0)
        AA_setD4_CHCR(chcr);
    REG_IPU_CTRL = f->env.ipuctrl;
    f->isActive = 1;
    SignalSema(f->sema);
    return 1;
}

// CT3 0x018c0ce0: stops both IPU channels (D4 at once, D3 once the IPU's input FIFO is empty) and saves
// their registers with the IPU's BP and CTRL; AddDMA refuses until RestartDMA.
int AA_viBufStopDMA(ViBuf *f)
{
    WaitSema(f->sema);
    f->isActive = 0;
    AA_setD4_CHCR(VIBUF_CHCR_STOP);
    f->env.d4madr = REG_DMAC_4_IPU_TO_MADR;
    f->env.d4tadr = REG_DMAC_4_IPU_TO_TADR;
    f->env.d4qwc = REG_DMAC_4_IPU_TO_QWC;
    f->env.d4chcr = REG_DMAC_4_IPU_TO_CHCR;
    while (REG_IPU_CTRL & IPU_CTRL_IFC_PENDING)
        ;
    AA_setD3_CHCR(0);
    f->env.d3madr = REG_DMAC_3_IPU_FROM_MADR;
    f->env.d3qwc = REG_DMAC_3_IPU_FROM_QWC;
    f->env.d3chcr = REG_DMAC_3_IPU_FROM_CHCR;
    f->env.ipubp = REG_IPU_BP;
    f->env.ipuctrl = REG_IPU_CTRL;
    SignalSema(f->sema);
    return 1;
}

// CT3 0x018c0dc0: stops D4, retires the blocks before the one MADR points into, then chains the whole
// blocks written since: the held run's old last tag becomes a REF, the new blocks follow with a REFE on the
// last, and the channel restarts (resuming as after a REF when blocks were added).
int AA_viBufAddDMA(ViBuf *f)
{
    u_int chcr;
    int done, at, add, i;

    WaitSema(f->sema);
    if (f->isActive == 0) {
        ErrMessage(s_dmaAddNotActive);
        SignalSema(f->sema);
        return 0;
    }
    AA_setD4_CHCR(VIBUF_CHCR_STOP);
    ViBufHostIpu(f); /* PORT: where the IPU has read to */
    chcr = REG_DMAC_4_IPU_TO_CHCR;
    done = (AA_getFIFOindex(f, REG_DMAC_4_IPU_TO_MADR) + f->n - f->dmaStart) % f->n;
    f->dmaStart = (f->dmaStart + done) % f->n;
    f->dmaN -= done;

    at = (f->dmaStart + f->dmaN) % f->n;
    add = f->readBytes / VIBUF_ELM_SIZE;
    f->readBytes %= VIBUF_ELM_SIZE;
    if (add > 0) {
        int last = (f->n + f->dmaStart + f->dmaN - 1) % f->n;

        AA_scTag2(ViBufTagAt(f, last), ViBufDmaData(f) + last * VIBUF_ELM_SIZE, VIBUF_TAG_REF, VIBUF_ELM_QWC);
    }
    for (i = 0; i < add; i++) {
        AA_scTag2(ViBufTagAt(f, at), ViBufDmaData(f) + at * VIBUF_ELM_SIZE,
                  i == add - 1 ? VIBUF_TAG_REFE : VIBUF_TAG_REF, VIBUF_ELM_QWC);
        at = (at + 1) % f->n;
    }
    f->dmaN += add;
    if (f->dmaN != 0) {
        u_int restart = chcr | VIBUF_CHCR_STR;

        if (add > 0)
            restart = (chcr & VIBUF_CHCR_KEEP) | (u_int)VIBUF_TAG_REF << VIBUF_CHCR_TAG_ID_SHIFT | VIBUF_CHCR_STR;
        AA_setD4_CHCR(restart);
    }
    SignalSema(f->sema);
    return 1;
}

// CT3 0x018c1000
void AA_viBufEndPut(ViBuf *f, int size)
{
    WaitSema(f->sema);
    f->readBytes += size;
    f->totalBytes += size;
    SignalSema(f->sema);
}

// CT3 0x018c1060
void AA_viBufBeginPut(ViBuf *f, u_char **ptr0, int *len0, u_char **ptr1, int *len1)
{
    int at, room;

    WaitSema(f->sema);
    at = ((f->dmaStart + f->dmaN) * VIBUF_ELM_SIZE + f->readBytes) % f->buffSize;
    room = (f->n - 2 - f->dmaN) * VIBUF_ELM_SIZE - f->readBytes;
    if (f->buffSize - at < room) {
        *ptr0 = f->data + at;
        *len0 = f->buffSize - at;
        *ptr1 = f->data;
        *len1 = room - (f->buffSize - at);
    } else {
        *ptr0 = f->data + at;
        *len0 = room;
        *ptr1 = NULL;
        *len1 = 0;
    }
    SignalSema(f->sema);
}

// CT3 0x018c1160: empties the ring and the stamp queue, chains every block's tag (REF) with a NEXT back to
// the first after them, and points D4 at the ring, stopped.
int AA_viBufReset(ViBuf *f)
{
    int i;

    f->dmaStart = 0;
    f->dmaN = 0;
    f->readBytes = 0;
    f->isActive = 1;
    f->count_ts = 0;
    f->wt_ts = 0;
    for (i = 0; i < f->n_ts; i++) {
        f->ts[i].pts = -1;
        f->ts[i].dts = -1;
        f->ts[i].pos = 0;
        f->ts[i].len = 0;
    }
    for (i = 0; i < f->n; i++)
        AA_scTag2(ViBufTagAt(f, i), ViBufDmaData(f) + i * VIBUF_ELM_SIZE, VIBUF_TAG_REF, VIBUF_ELM_QWC);
    AA_scTag2(ViBufTagAt(f, f->n), ViBufDmaTag(f), VIBUF_TAG_NEXT, 0);
    REG_DMAC_4_IPU_TO_QWC = 0;
    REG_DMAC_4_IPU_TO_MADR = ViBufDmaData(f);
    REG_DMAC_4_IPU_TO_TADR = ViBufDmaTag(f);
    AA_setD4_CHCR(VIBUF_CHCR_STOP);
    return 1;
}

// CT3 0x018c12c0: the ring over n blocks at data, its tag list (n + 1 tags), the stamp queue, a binary
// semaphore; then Reset.
int AA_viBufCreate(ViBuf *f, u_char *data, void *tag, int n, TimeStamp *ts, int n_ts)
{
    SemaParam param = {};

    f->data = data;
    f->tag = tag; /* PORT: the ROM keeps the uncached alias, (tag & 0x0fffffff) | 0x20000000 */
    f->n = n;
    f->buffSize = n << 11;
    f->ts = ts;
    f->n_ts = n_ts;
    param.initCount = 1;
    param.maxCount = 1;
    f->sema = CreateSema(&param);
    AA_viBufReset(f);
    f->totalBytes = 0;
    return 1;
}
