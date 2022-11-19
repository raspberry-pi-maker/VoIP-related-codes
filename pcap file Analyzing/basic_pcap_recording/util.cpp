#include "global.h"

const char *ip_to_str(const u_char *ip, char *pstr)
{
  sprintf(pstr, "%d.%d.%d.%d", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
  return(const char *)pstr;
}

// this function converts pcmu (8bit) to pcm(16bit)
int16_t ulaw2linear(u_char u_val)
{
  int16_t t;
   /* Complement to obtain normal u-law value. */
   u_val = ~u_val;
   
   /*
    * Extract and bias the quantization bits. Then
    * shift up by the segment number and subtract out the bias.
    */
   t = ((u_val & QUANT_MASK) << 3) + BIAS;
   t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
   
   return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));  
}

// this function converts pcma (8bit) to pcm(16bit)
int16_t alaw2linear(u_char	a_val)
{
   int16_t t;
   int16_t seg;
   
   a_val ^= 0x55;
   
   t = (a_val & QUANT_MASK) << 4;
   seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
   switch (seg) {
   case 0:
      t += 8;
      break;
   case 1:
      t += 0x108;
      break;
   default:
      t += 0x108;
      t <<= seg - 1;
   }
   return ((a_val & SIGN_BIT) ? t : -t);
}

void get_wav_header(int raw_sz, wav_header_t *wh)
{
    // RIFF chunk
    memcpy(wh->chunk_id, "RIFF", 4);
    wh->chunk_size = 36 + raw_sz;

    // fmt sub-chunk (to be optimized)
    memcpy(wh->sub_chunk1_id, "WAVEfmt ", strlen("WAVEfmt "));
    wh->sub_chunk1_size = 16;
    wh->audio_format = 1;
    wh->num_channels = 1;
    wh->sample_rate = 8000;
    wh->bits_per_sample = 16;
    wh->block_align = wh->num_channels * wh->bits_per_sample / 8;
    wh->byte_rate = wh->sample_rate * wh->num_channels * wh->bits_per_sample / 8;

    // data sub-chunk
    memcpy(wh->sub_chunk2_id, "data", strlen("data"));
    wh->sub_chunk2_size = raw_sz;
}

char* dummy_get_raw_pcm (const char *p, int *bytes_read)
{
    long lSize;
    char *pcm_buf;
    size_t result;
    FILE *fp_pcm;

    fp_pcm = fopen (p, "rb");
    if (fp_pcm == NULL) {
        fprintf(stderr, "pcm File[%s] open error", p);
        return NULL;
    }

    // obtain file size:
    fseek (fp_pcm , 0 , SEEK_END);
    lSize = ftell (fp_pcm);
    rewind (fp_pcm);

    // allocate memory to contain the whole file:
    pcm_buf = (char*) malloc (sizeof(char) * lSize);
    if (pcm_buf == NULL) {
        fprintf(stderr, "pcm File[%s] memory allocation error", p);
        fclose(fp_pcm);
        return NULL;
    }

    // copy the file into the pcm_buf:
    result = fread (pcm_buf, 1, lSize, fp_pcm);
    if (result != lSize) {
        fprintf(stderr, "pcm File[%s] reading error", p);
        fclose(fp_pcm);
        free(pcm_buf);
        return NULL;
    }

    *bytes_read = (int) lSize;
    return pcm_buf;
}

CALL *add_call(const u_char *myip, const u_char *otherip, uint32_t ssrc)
{
    CALL *call = new CALL();
    memcpy(call->my_ip, myip, 4);
    memcpy(call->other_ip, otherip, 4);
    call->ssrc = ssrc;
    char filename[128];
    char from_ip[64], to_ip[64];

    ip_to_str((const u_char*)myip, from_ip);
    ip_to_str((const u_char*)otherip, to_ip);

    sprintf(filename, "%s%s-%s-%llu.pcm", g_outputdir, from_ip, to_ip, ssrc);
    call->filename = (string) filename;
    call->fh = fopen(filename, "w+");   
    g_calls.push_back(call);
    return call;

}

CALL *findcall(const u_char *myip, const u_char *otherip, uint32_t ssrc)
{
    list<CALL *>::iterator it;
    for(it = g_calls.begin();it != g_calls.end();it++){ 
        CALL *call = (CALL *) *it;
        if(0 == memcmp(myip, call->my_ip, 4) && 0 == memcmp(otherip, call->other_ip, 4) && ssrc == call->ssrc){
            return call;        
        }
    }
    return NULL;
}

//convert G.711 to PCM and save to file
int add_rtp(CALL *call, const u_char *rtp, int size, int payload)
{
    u_char r;
    int16_t w;    
    int16_t *pcm = (int16_t *)malloc(sizeof(int16_t) * size);
    for(int x = 0; x < size; x++){
        r = rtp[x];
        if(0 == payload) w = ulaw2linear(r);
        else w = alaw2linear(r);
        pcm[x] = w;
    }
    fwrite(pcm, size , sizeof(int16_t), call->fh);
    free(pcm);  
    return 0;    
}

/*
close file descriptor and convert pcm to wav
*/
void finalize_call(CALL *call)
{
    fclose(call->fh);
    int raw_sz = 0;
    FILE *fwav;
    wav_header_t wheader;
    memset (&wheader, '\0', sizeof (wav_header_t));
    char *pcm_buf = dummy_get_raw_pcm (call->filename.c_str(), &raw_sz);
    if(NULL == pcm_buf){
        return;
    }
    string output = call->filename;
    output.replace(output.find("pcm"), 3, "wav");
    get_wav_header (raw_sz, &wheader);
    //dump_wav_header (&wheader);

    // write out the .wav file
    fwav = fopen(output.c_str(), "wb");
    fwrite(&wheader, 1, sizeof(wheader), fwav);
    fwrite(pcm_buf, 1, raw_sz, fwav);
    fclose(fwav);
    free (pcm_buf);
    return ;

    
}

void finalize_calls()
{
    list<CALL *>::iterator it;
    for(it = g_calls.begin();it != g_calls.end();it++){ 
        CALL *call = (CALL *) *it;
        finalize_call(call);
    }
}
