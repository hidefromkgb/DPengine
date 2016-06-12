#include "common.h"



static inline long SkipChunk(uint8_t **buff, long *size) {
    long skip;

    ++(*buff);
    if (--(*size) <= 0)
        return 0;
    do {
        *buff += (skip = 1 + **buff);
        if ((*size -= skip) <= 0)
            return 0;
    } while (skip > 1);
    return 1;
}



long ReadFrameHeader(uint8_t **buff, long *size,
                     GHDR *ghdr, FHDR **fhdr, RGBX **rpal) {
    long rclr = 0;

    if ((*size -= sizeof(**fhdr)) <= 0)
        return -2;

    *fhdr = (FHDR*)*buff;
    *buff += sizeof(**fhdr);
    if ((*fhdr)->flgs & GIF_FPAL) {
        /// local palette always has a priority over global
        *rpal = (RGBX*)*buff;
        *buff += (rclr = 2 << ((*fhdr)->flgs & 7)) * sizeof(**rpal);
        if ((*size -= rclr * sizeof(**rpal)) <= 0)
            return -1;
    }
    else if (ghdr->flgs & GIF_FPAL) {
        /// no local palette, using global
        rclr = 2 << (ghdr->flgs & 7);
        *rpal = (RGBX*)(ghdr + 1);
    }
    return rclr;
}



/// return values:
///  -5: unexpected end of the data stream
///  -4: the data stream is empty
///  -3: minimum LZW size is out of its nominal [2; 8] bounds
///  -2: initial code is not equal to minimum LZW size
///  -1: no end-of-stream mark after end-of-data code
///   0: no end-of-data code before end-of-stream mark => [RECOVERABLE ERROR]
///   1: decoding successful
long DecodeFrame(uint8_t **buff, long *size, uint8_t *bptr) {
    #define DEF_CLEN (1 << 12)
    long iter, /// loop iterator for expanding codes into index strings
         bseq, /// block sequence loop counter
         bszc, /// bit size counter
         ctbl, /// code table counter, points to the last element
         curr, /// current code from the code stream
         prev, /// previous code from the code stream
         ctsz, /// minimum LZW code table size, in bits
         ccsz; /// current code table size, in bits
    uint16_t read, mask;
    uint32_t code[DEF_CLEN];

    /// manual stack checking (must not be reordered, hence the barriers);
    /// assumes the page size to be (1 << 12) = 4096, correct this if not so
    code[3 * DEF_CLEN / 4] = 0; asm volatile("" ::: "memory");
    code[2 * DEF_CLEN / 4] = 0; asm volatile("" ::: "memory");
    code[1 * DEF_CLEN / 4] = 0; asm volatile("" ::: "memory");
    code[0 * DEF_CLEN / 4] = 0; asm volatile("" ::: "memory");

    /// does the size suffice our needs?
    if (--(*size) <= sizeof(read))
        return -5;

    /// preparing initial values
    ctsz = *(*buff)++;
    if (!(bseq = *(*buff)++))
        return -4;
    if ((ctsz < 2) || (ctsz > 8))
        return -3;

    mask =  (1 << (ccsz = ctsz + 1)) - 1;
    curr = *(typeof(read)*)*buff & mask;
    bszc =  -ccsz;
    prev =   0;

    if (curr != (ctbl = (1 << ctsz)))
        return -2;

    /// filling persistent part of the code table
    for (iter = 0; iter < ctbl; iter++)
        code[iter] = iter << 24;

    /// splitting data stream into codes
    do {
        if ((*size -= bseq + 1) <= 0)
            return -5;
        for (; bseq > 0; bseq -= sizeof(read), *buff += sizeof(read)) {
            read = *(typeof(read)*)*buff;

            if (bseq < sizeof(read))
                read &= (1 << (8 * bseq)) - 1;

            curr |= read << (ccsz + bszc);
            read >>= -bszc;
            bszc += 8 * ((bseq < sizeof(read))? bseq : sizeof(read));

            while (bszc >= 0) {
                curr &= mask;
                if ((curr & -2) == (1 << ctsz)) {
                    /// 1 + (1 << ctsz): "end of data" code (ED)
                    if (curr & 1) {
                        (*size)--;
                        *buff += bseq;
                        return (!*(*buff)++)? 1 : -1;
                    }
                    /// 0 + (1 << ctsz): "drop table" code (DT); ED = DT + 1
                    ctbl = (1 << ctsz);
                    mask = (1 << (ccsz = ctsz + 1)) - 1;
                }
                else {
                    /// single-pixel code (SP) or multi-pixel code (MP)
                    if (ctbl < DEF_CLEN - 1) /// prev = DT? => curr < ctbl = DT
                        code[++ctbl] = prev + (code[prev] & 0xFFF000) + 0x1000;
                    /// appending pixel string to the frame
                    iter = (curr >= ctbl)? prev : curr;
                    bptr += (prev = (code[iter] >> 12) & 0xFFF);
                    while (!0) {
                        *bptr-- = code[iter] >> 24;
                        if (!(code[iter] & 0xFFF000))
                            break;
                        iter = code[iter] & 0xFFF;
                    }
                    bptr += prev + 2;
                    if (curr >= ctbl)
                        *bptr++ = code[iter] >> 24;

                    /// adding new code to the code table
                    if (ctbl < DEF_CLEN)
                        code[ctbl] |= code[iter] & 0xFF000000;
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
                bszc -= ccsz;
            }
        }
        *buff += bseq;
    } while ((bseq = *(*buff)++));
    (*size)--;
    return 0;
}



long MakeAnim(void *data, long size, long skip, GWFR gwfr, void *anim) {
    /// magic header constant
    #define GIF_HEAD ('G' + 'I' * 0x100 + 'F' * 0x10000 + '8' * 0x1000000)
    /// magic type constants
    #define GIF_TYP7 ('7' + 'a' * 0x100)
    #define GIF_TYP9 ('9' + 'a' * 0x100)

    /// extension header
    #define GIF_EHDR 0x21
    /// main frame header
    #define GIF_FHDR 0x2C
    /// end-of-file mark
    #define GIF_EOFH 0x3B
    /// extension: frame graphics control
    #define EXT_FGRC 0xF9

    /// default delay for frames having no graphics headers, just in case
    /// (given in GIF time units; 1 unit = 10 ms)
    #define DEF_SKIP 0

    #pragma pack(push, 1)
    struct {              /// ====== EXTENSION: FRAME GRAPHICS CONTROL ======
        uint8_t flgs;     /** FLAGS:
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
        uint16_t time;    /// delay in GIF time units; 1 unit = 10 ms
        uint8_t tran;     /// transparent color index
    } *efgc = 0;
    #pragma pack(pop)

    /// mode not set
    #define FLG_CUDN 0x00
    /// leave result as is
    #define FLG_CUDL 0x04
    /// restore background
    #define FLG_CUDB 0x08
    /// restore previous
    #define FLG_CUDP 0x0C
    /// mask to extract the states above
    #define FLG_CUDM 0x1C

    /// transparency flag
    #define FLG_CTRN 0x01

    GHDR *ghdr = (GHDR*)data;
    FHDR *fhdr, *back = 0;
    RGBX *cpal;

    long desc, ifrm, nfrm = 0;
    uint8_t *buff, *bptr;

    /// checking if the stream is not empty and has a valid signature,
    /// the data has sufficient size and frameskip value is non-negative
    if (!ghdr || (size <= sizeof(*ghdr)) || (ghdr->head != GIF_HEAD)
    || ((ghdr->type != GIF_TYP7) && (ghdr->type != GIF_TYP9)) || (skip < 0))
        return 0;

    /// skipping global header
    buff = (typeof(buff))(ghdr + 1);
    /// skipping global palette (if there is any)
    if (ghdr->flgs & GIF_FPAL)
        buff += (2 << (ghdr->flgs & 7)) * sizeof(*cpal);
    if ((size -= buff - (typeof(buff))ghdr) <= 0)
        return 0;

    /// counting frames
    ifrm = size;
    bptr = buff;
    while ((desc = *bptr++) != GIF_EOFH) {
        ifrm--;
        if (desc == GIF_FHDR) {
            if (ReadFrameHeader(&bptr, &ifrm, ghdr, &fhdr, &cpal) <= 0)
                break;
            nfrm++;
        }
        if (!SkipChunk(&bptr, &ifrm))
            break;
    }
    if (desc != GIF_EOFH)
        nfrm = -nfrm;

    /// extracting frames
    ifrm = 0;
    bptr = malloc(ghdr->xdim * ghdr->ydim * sizeof(*bptr));
    while (skip < labs(nfrm)) {
        size--;
        desc = *buff++;
        /// found a frame
        if (desc == GIF_FHDR) {
            desc = ReadFrameHeader(&buff, &size, ghdr, &fhdr, &cpal);
            /// DESC != GIF_EOFH because GIF_EOFH & (GIF_EOFH - 1) != 0
            if (++ifrm > skip) {
                if ((desc > 0) && (DecodeFrame(&buff, &size, bptr) >= 0)) {
                    /// writing extracted frame to its persistent location
                    gwfr(ghdr, fhdr, back, cpal, desc, bptr, anim, nfrm,
                        (efgc && (efgc->flgs & FLG_CTRN))? efgc->tran : -1,
                        (efgc)? efgc->time : DEF_SKIP, ifrm - 1);
                    /// computing blend mode for the next frame
                    switch ((efgc)? efgc->flgs & FLG_CUDM : FLG_CUDN) {
                        case FLG_CUDB: back = fhdr;            break;
                        case FLG_CUDP: back = (typeof(back))2; break;
                        case FLG_CUDL: back = (typeof(back))1; break;
                        default:       back = (typeof(back))0; break;
                    }
                    if (size > 0)
                        continue;
                }
                else {
                    /// failed to extract ITER-th frame; exiting
                    desc = GIF_EOFH;
                    ifrm--;
                }
            }
        }
        /// found an extension
        else if (desc == GIF_EHDR)
            if (*buff == EXT_FGRC) /// FGRC _|   |_ chunk size (must be 4)
                efgc = (typeof(efgc))(buff + 1 + 1);
        /// found a valid GIF ending mark (or there`s no more data left)
        if ((desc == GIF_EOFH) || !SkipChunk(&buff, &size))
            break;
    }
    free(bptr);
    return (nfrm < 0)? -ifrm + skip : ifrm;
}
