#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <string>

using namespace std;


// WAVE file header format
typedef struct {
    unsigned char   chunk_id[4];        // RIFF string
    unsigned int    chunk_size;         // overall size of file in bytes (36 + data_size)
    unsigned char   sub_chunk1_id[8];   // WAVEfmt string with trailing null char
    unsigned int    sub_chunk1_size;    // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
    unsigned short  audio_format;       // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    unsigned short  num_channels;       // Mono = 1, Stereo = 2
    unsigned int    sample_rate;        // 8000, 16000, 44100, etc. (blocks per second)
    unsigned int    byte_rate;          // SampleRate * NumChannels * BitsPerSample/8
    unsigned short  block_align;        // NumChannels * BitsPerSample/8
    unsigned short  bits_per_sample;    // bits per sample, 8- 8bits, 16- 16 bits etc
    unsigned char   sub_chunk2_id[4];   // Contains the letters "data"
    unsigned int    sub_chunk2_size;    // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} wav_header_t;

void usage();
void do_process();
char* dummy_get_raw_pcm (const char *p, int *bytes_read);
void get_wav_header(int raw_sz, wav_header_t *wh);

string input(""), output("");



int main(int argc, char *argv[])
{
    int opt;
    string codec("");
    while ((opt = getopt(argc, argv, "hi:o:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage();
                exit(1);
            case 'i':
                input = (string)optarg;
                break;
            case 'o':
                output = (string)optarg;
                break;
        }
    }
    if (input.length() == 0 || output.length() == 0){
        cout <<"no filename" << endl;
        usage();
        exit(1);
    }
    do_process();
    return 0;
}


void usage()
{
    fprintf(stderr, "===== Usage =====\n");
    fprintf(stderr, "-i: pcm file  name  \n");
    fprintf(stderr, "-o: pcm output name \n");
    fprintf(stderr, "./pcm_2_wav -i input.pcm -o output.wav  \n");
}



void do_process()
{

        int raw_sz = 0;
        FILE *fwav;
        wav_header_t wheader;
    memset (&wheader, '\0', sizeof (wav_header_t));
    char *pcm_buf = dummy_get_raw_pcm (input.c_str(), &raw_sz);
    if(NULL == pcm_buf){
        return;
    }
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
