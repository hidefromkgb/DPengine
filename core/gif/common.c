#include "common.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/// extension header
#define GIF_EHDR 0x21
/// frame header
#define GIF_FHDR 0x2C
/// end-of-file mark
#define GIF_EOFH 0x3B
/// graphics control extension
#define EXT_GCTL 0xF9

/// Transparency flag
#define FLG_CTRN 0x01

/// Mode not set
#define FLG_CUDN 0x00
/// Leave result as is
#define FLG_CUDL 0x04
/// Restore background
#define FLG_CUDB 0x08
/// Restore previous
#define FLG_CUDP 0x0C
/// Mask to extract the states above
#define FLG_CUDM 0x1C

/// Default delay for frames having no graphics headers, just in case
/// Given in GIF time units; 1 unit = 10 ms
#define DEF_SKIP 0



#pragma pack(push, 1)
typedef struct _FGRH {    /// ============ FRAME GRAPHICS HEADER ============
    uint8_t size;         /// block size; must be 4
    uint8_t flgs;         /** FLAGS:
                              [reserved]   bit 7-5   [undefined]
                              BlendMode    bit 4-2   000: not set; static GIF
                                                     001: leave result as is
                                                     010: restore background
                                                     011: restore previous
                                                     1--: [undefined]
                              UserInput    bit 1     1: show frame till input
                                                     0: default; ~99% of GIFs
                              TransColor   bit 0     1: got transparent color
                                                     0: frame is fully opaque
                          **/
    uint16_t time;        /// delay in GIF time units; 1 unit = 10 ms
    uint8_t tran;         /// transparent color index
    uint8_t term;         /// terminator; must be 0x00
} FGRH;

typedef struct _CTBE {
    int16_t prev, size;
    BGRA pixl;
} CTBE;
#pragma pack(pop)



inline void ReadChunk(uint8_t **buff) {
    long skip;

    (*buff)++;
    while ((skip = *(*buff)++))
        *buff += skip;
}



long ReadFrameHeader(uint8_t **buff, GHDR *ghdr, FHDR **fhdr, RGBX **rpal) {
    long rclr = 0;

    *fhdr = (FHDR*)*buff;
    *buff += sizeof(**fhdr);
    if ((*fhdr)->flgs & GIF_FPAL) {
        /// local palette always has a priority over global
        *rpal = (RGBX*)*buff;
        *buff += (rclr = 2 << ((*fhdr)->flgs & 7)) * sizeof(**rpal);
    }
    else if (ghdr->flgs & GIF_FPAL) {
        /// no local palette, using global
        rclr = 2 << (ghdr->flgs & 7);
        *rpal = (RGBX*)((uint8_t*)ghdr + sizeof(*ghdr));
    }
    return rclr;
}



/// CCLR < 0 means that every pixel would contain its palette index in alpha.
/// Return values:
///  -5: no palette found
///  -4: the data stream is empty
///  -3: minimum LZW size is out of its nominal [2; 8] bounds
///  -2: initial code is not equal to minimum LZW size
///  -1: no end-of-stream mark after end-of-data code
///   0: no end-of-data code before end-of-stream mark -> [RECOVERABLE ERROR]
///   1: decoding successful
long DecodeFrame(uint8_t **buff, FGRH *fgrh,
                 RGBX *cpal, long cclr, BGRA *bptr) {
    long iter, /// loop iterator for expanding codes into BGRA strings
         bseq, /// block sequence loop counter
         size, /// bit size counter
         ctbl, /// code table counter, points to the last element
         curr, /// current code from the code stream
         prev, /// previous code from the code stream
         ctsz, /// minimum LZW code table size, in bits
         ccsz; /// current code table size, in bits
    uint16_t read, mask;

    #define DEF_CLEN (1 << 12)
    CTBE code[DEF_CLEN];

    /// preparing initial values
    ctsz = *(*buff)++;
    if (!cclr)
        return -5;
    if (!(bseq = *(*buff)++))
        return -4;
    if ((ctsz < 2) || (ctsz > 8))
        return -3;

    mask =  (1 << (ccsz = ctsz + 1)) - 1;
    curr = *(typeof(read)*)*buff & mask;
    size =  -ccsz;
    prev =   0;

    if (curr != (ctbl = (1 << ctsz)))
        return -2;

    /// filling persistent part of the code table
    for (iter = ((cclr > 0)? cclr : -cclr) - 1; iter >= 0; iter--) {
        code[iter].size = 0;
        code[iter].pixl.B = cpal[iter].B;
        code[iter].pixl.G = cpal[iter].G;
        code[iter].pixl.R = cpal[iter].R;
        code[iter].pixl.A = (cclr > 0)? 0xFF : iter;
    }
    for (iter = ((cclr > 0)? cclr : -cclr); iter < ctbl; iter++) {
        code[iter].size = 0;
        code[iter].pixl.BGRA = 0x00000000;
        if (cclr <= 0)
            code[iter].pixl.A = iter;
    }
    if ((cclr > 0) && fgrh && (fgrh->flgs & FLG_CTRN))
        code[fgrh->tran].pixl.BGRA = 0x00000000;

    /// splitting data stream into codes
    do {
        for (; bseq > 0; bseq -= sizeof(read), *buff += sizeof(read)) {
            read = *(typeof(read)*)*buff;

            if (bseq < sizeof(read))
                read &= (1 << (8 * bseq)) - 1;

            curr |= read << (ccsz + size);
            read >>= -size;
            size += 8 * ((bseq < sizeof(read))? bseq : sizeof(read));

            while (size >= 0) {
                curr &= mask;
                if ((curr & -2) == (1 << ctsz)) {
                    /// 1 + (1 << ctsz): "end of data" code
                    if (curr & 1) {
                        *buff += bseq;
                        return (!*(*buff)++)? 1 : -1;
                    }
                    /// 0 + (1 << ctsz): "drop table" code
                    ctbl = (1 << ctsz);
                    mask = (1 << (ccsz = ctsz + 1)) - 1;
                }
                else {
                    /// single-pixel (CODE[ITER].SIZE == 0) or multi-pixel code
                    if (ctbl < DEF_CLEN) {
                        ctbl++;
                        code[ctbl].prev = prev;
                        code[ctbl].size = code[prev].size + 1;
                    }
                    /// appending pixel string to the frame
                    iter = (curr >= ctbl)? prev : curr;
                    bptr += (prev = code[iter].size);
                    while (!0) {
                        *bptr-- = code[iter].pixl;
                        if (!code[iter].size)
                            break;
                        iter = code[iter].prev;
                    }
                    bptr += prev + 2;
                    if (curr >= ctbl)
                        *bptr++ = code[iter].pixl;

                    /// adding new code to the code table
                    if (ctbl < DEF_CLEN)
                        code[ctbl].pixl = code[iter].pixl;
                }
                /// does code table exceed bit limit?
                if ((ctbl == mask) && (ctbl < DEF_CLEN - 1)) {
                    /// yes; extending
                    mask += mask + 1;
                    ccsz++;
                }
                prev = curr;
                curr = read;
                read >>= ccsz;
                size -= ccsz;
            }
        }
        *buff += bseq;
    } while ((bseq = *(*buff)++));
    return 0;
    #undef DEF_CLEN
}



long MakeAnim(void *inpt, long flgs, void *anim,
              GGET gget, GINI gini, GWFR gwfr, GPUT gput) {
    long desc, iter, prev, fram;
    uint8_t *buff, *btmp;
    BGRA *init, *bptr;
    GHDR *ghdr = NULL;
    FGRH *fgrh = NULL;
    FHDR *fhdr;
    RGBX *cpal;

    if (!(flgs & MAF_GGET) &&
       ((desc = open((char*)inpt, O_RDONLY | O_BINARY)) > 0)) {
        /// FLGS only has MAF_FILE, reading from a file
        buff = malloc(iter = lseek(desc, (off_t)0, SEEK_END));
        lseek(desc, (off_t)0, SEEK_SET);
        read(desc, buff, iter);
        close(desc);
        ghdr = (GHDR*)buff;
    }
    else if ((flgs & MAF_GGET) && gget) {
        /// FLGS has MAF_GGET, reading from some place supported by GGET()
        ghdr = gget(inpt);
    }
    iter = 0;
    if (ghdr) {
        /// skipping global header, skipping global palette (if there is any)
        btmp = buff = (uint8_t*)ghdr + sizeof(*ghdr) +
        ((ghdr->flgs & GIF_FPAL)? (2 << (ghdr->flgs & 7)) * sizeof(*cpal) : 0);

        /// counting frames
        fram = 0;
        while ((desc = *btmp++) != GIF_EOFH) {
            /// found a frame, incrementing the frame counter
            if (desc == GIF_FHDR) {
                ReadFrameHeader(&btmp, ghdr, &fhdr, &cpal);
                fram++;
            }
            ReadChunk(&btmp);
        }
        /// initializing the main structure, beginning frame extraction
        if (fram && (gini(ghdr, anim, fram--) > 0)) {
            init = malloc(ghdr->xdim * ghdr->ydim * sizeof(*init) + 16);
            bptr = (BGRA*)(((uintptr_t)init & -16) + 16);
            iter = prev = 0;

            while ((desc = *buff++) != GIF_EOFH) {
                /// found an extension
                if (desc == GIF_EHDR) {
                    if (*buff == EXT_GCTL) {
                        fgrh = (FGRH*)++buff;
                        buff += sizeof(*fgrh);
                    }
                    else
                        ReadChunk(&buff);
                }
                /// found a frame
                else if (desc == GIF_FHDR) {
                    cpal = NULL;
                    desc = ReadFrameHeader(&buff, ghdr, &fhdr, &cpal);
                    /// return code 0 means the error is recoverable; accepting
                    if (DecodeFrame(&buff, fgrh, cpal, (flgs & MAF_AIND)?
                                    -desc : desc, bptr) >= 0) {
                        /// computing blend mode
                        desc = -2;
                        if (fgrh && (iter < fram))
                            switch (fgrh->flgs & FLG_CUDM) {
                                case FLG_CUDP: desc = prev; break;
                                case FLG_CUDL: desc = iter; break;
                                case FLG_CUDN:
                                case FLG_CUDB: desc = -1;   prev = iter;
                            }
                        /// writing extracted frame to its persistent location
                        gwfr(ghdr, fhdr, anim, bptr,
                            (fgrh && (fgrh->flgs & FLG_CTRN))? fgrh->tran : -1,
                            (fgrh)? fgrh->time : DEF_SKIP, iter++, desc + 1);
                    }
                    else {
                        /// failed to extract ITER-th frame, exiting
                        fram = GIF_EOFH;
                        buff = (uint8_t*)&fram;
                        iter = -(iter + 1);
                    }
                }
            }
            free(init);
        }
    }
    if (!(flgs & MAF_GGET))
        free(ghdr);
    else if (gput) {
        /// if no error (ITER > 0), overwriting frame count with GPUT() output
        desc = gput((void*)ghdr, anim);
        iter = (iter > 0)? desc : iter;
    }
    return iter;
}
