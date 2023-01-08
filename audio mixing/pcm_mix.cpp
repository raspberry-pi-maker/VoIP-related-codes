#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <list>
#include <string>

using namespace std;
void usage();
void do_process();

string input1(""), input2(""),output("");


int main(int argc, char * argv[])
{
    int opt;
    string codec("");
    while ((opt = getopt(argc, argv, "hi:j:o:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage();
                exit(1);
            case 'i':
                input1 = (string)optarg;
                break;
            case 'j':
                input2 = (string)optarg;
                break;
            case 'o':
                output = (string)optarg;
                break;
        }
    }
    if (input1.length() == 0 || input2.length() == 0 || output.length() == 0){
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
    fprintf(stderr, "-i: mono pcm file name \n");
    fprintf(stderr, "-j: mono pcm file name \n");
    fprintf(stderr, "-o: mixed pcm file name  \n");
    fprintf(stderr, "./mix_pcm -i input1.pcm -j input2.pcm -o output.pcm \n");
}

int16_t mix(int a, int b){
    int m;
    // Make both samples unsigned (0..65535)
    a += 32768;
    b += 32768;

    // Pick the equation
    if ((a < 32768) || (b < 32768)) {
        // Viktor's first equation when both sources are "quiet"
        // (i.e. less than middle of the dynamic range)
        m = a * b / 32768;
    } else {
        // Viktor's second equation when one or both sources are loud
        m = 2 * (a + b) - (a * b) / 32768 - 65536;
    }

    // Output is unsigned (0..65536) so convert back to signed (-32768..32767)
    if (m == 65536) m = 65535;
    m -= 32768;
    return (int16_t)m;
}

void do_process()
{
    FILE *fh1 = fopen(input1.c_str(), "r");
    if(!fh1){
        fprintf(stderr, "cannot open file[%s]\n",input1.c_str());
        exit(0);        
    }

    FILE *fh2 = fopen(input2.c_str(), "r");
    if(!fh2){
        fprintf(stderr, "cannot open file[%s]\n",input2.c_str());
        fclose(fh1);
        exit(0);        
    }
    
    
    FILE *fh_out = fopen(output.c_str(), "w+");
    if(!fh_out){
        fprintf(stderr, "cannot open file[%s]\n",output.c_str());
        fclose(fh1);
        fclose(fh2);
        exit(0);        
    }

    int16_t r1, r2, r3;
    long fsize1, fsize2, lSize;
    char buf[12];

    fseek (fh1 , 0 , SEEK_END);
    fsize1 = ftell (fh1) / sizeof(int16_t);
    rewind (fh1);
    fseek (fh2 , 0 , SEEK_END);
    fsize2 = ftell (fh2) / sizeof(int16_t);
    rewind (fh2);
    lSize = min(fsize1, fsize2);

    for(long x = 0; x < lSize; x++){
        fread(&r1, sizeof(int16_t), 1, fh1);
        fread(&r2, sizeof(int16_t), 1, fh2);
        r3 = mix(r1, r2);
        fwrite(&r3, sizeof(int16_t), 1, fh_out);
    }
    FILE *fht = (fsize1 > fsize2)? fh1:fh2;
    r2 = 0;
    for(long x = lSize; x < max(fsize1, fsize2); x++){
        fread(&r1, sizeof(int16_t), 1, fht);
        r3 = mix(r1, r2);
        fwrite(&r3, sizeof(int16_t), 1, fh_out);
    }
    
    
    fclose(fh1);
    fclose(fh2);
    fclose(fh_out);
}
