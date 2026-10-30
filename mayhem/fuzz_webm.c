#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nestegg/nestegg.h"

struct membuf {
    const uint8_t *data;
    size_t size;
    size_t pos;
};

static int mem_read(void *buffer, size_t length, void *userdata)
{
    struct membuf *m = userdata;
    if(m->pos >= m->size) {
        return 0;
    }
    if(length > m->size - m->pos) {
        return -1;
    }
    memcpy(buffer, m->data + m->pos, length);
    m->pos += length;
    return 1;
}

static int mem_seek(int64_t offset, int whence, void *userdata)
{
    struct membuf *m = userdata;
    int64_t base;

    switch(whence) {
    case NESTEGG_SEEK_SET:
        base = 0;
        break;
    case NESTEGG_SEEK_CUR:
        base = (int64_t)m->pos;
        break;
    case NESTEGG_SEEK_END:
        base = (int64_t)m->size;
        break;
    default:
        return -1;
    }
    if(offset < -base || offset > (int64_t)m->size - base) {
        return -1;
    }
    m->pos = (size_t)(base + offset);
    return 0;
}

static int64_t mem_tell(void *userdata)
{
    struct membuf *m = userdata;
    return (int64_t)m->pos;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct membuf m = { data, size, 0 };
    nestegg_io io = { mem_read, mem_seek, mem_tell, &m };
    nestegg *ctx = NULL;
    nestegg_packet *pkt;
    unsigned int tracks, t, i, count, track;
    uint64_t u64;
    int packets = 0;

    if(nestegg_init(&ctx, io, NULL, -1) != 0) {
        return -1;
    }

    nestegg_duration(ctx, &u64);
    nestegg_tstamp_scale(ctx, &u64);
    nestegg_has_cues(ctx);

    if(nestegg_track_count(ctx, &tracks) == 0) {
        for(t = 0; t < tracks && t < 16; t++) {
            nestegg_video_params vp;
            nestegg_audio_params ap;
            const unsigned char *cd;
            size_t cdsize;
            unsigned int items;

            nestegg_track_codec_id(ctx, t);
            nestegg_track_default_duration(ctx, t, &u64);
            if(nestegg_track_type(ctx, t) == NESTEGG_TRACK_VIDEO) {
                nestegg_track_video_params(ctx, t, &vp);
            } else {
                nestegg_track_audio_params(ctx, t, &ap);
            }
            if(nestegg_track_codec_data_count(ctx, t, &items) == 0) {
                for(i = 0; i < items && i < 4; i++) {
                    nestegg_track_codec_data(ctx, t, i, &cd, &cdsize);
                }
            }
        }
    }

    while(packets < 256 && nestegg_read_packet(ctx, &pkt) > 0) {
        const unsigned char *chunk;
        size_t chunk_size;

        packets++;
        nestegg_packet_track(pkt, &track);
        nestegg_packet_tstamp(pkt, &u64);
        nestegg_packet_duration(pkt, &u64);
        if(nestegg_packet_count(pkt, &count) == 0) {
            for(i = 0; i < count; i++) {
                if(nestegg_packet_data(pkt, i, &chunk, &chunk_size) == 0 && chunk_size > 0) {
                    volatile unsigned char sink = chunk[0] ^ chunk[chunk_size - 1];
                    (void)sink;
                }
            }
        }
        nestegg_free_packet(pkt);
    }

    nestegg_destroy(ctx);
    return 0;
}
